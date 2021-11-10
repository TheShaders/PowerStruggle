#include <cstdint>
#include <cstring>

#include <multiarraylist.h>
#include <mem.h>
#include <ecs.h>
#include <model.h>
#include <collision.h>
#include <physics.h>
#include <interaction.h>
#include <block_vector.h>

#include <memory>

extern "C" {
#include <debug.h>
}

#define COMPONENT(Name, Type) sizeof(Type),

const size_t g_componentSizes[] = {
#include "components.inc.h"
};

#undef COMPONENT

int archetypeEntityCounts[MAX_ARCHETYPES];
archetype_t currentArchetypes[MAX_ARCHETYPES];
MultiArrayList archetypeArrays[MAX_ARCHETYPES];

int numArchetypes = 0;

Entity allEntities[MAX_ENTITIES];
// End of the populated entities in the array
int entitiesEnd = 0;
// Actual number of entities (accounts for gaps in the array)
int numEntities = 0;
int numGaps = 0;
int firstGap = INT32_MAX;

// Underlying implementation for popcount
// https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
extern "C" int numberOfSetBits(uint32_t i)
{
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

block_vector<Entity*> queued_deletions;


void queue_entity_deletion(Entity *e)
{
    debug_printf("Entity %08X queued for deletion\n", e);
    queued_deletions.emplace_back(e);
}

void process_deletion_queue()
{
    for (Entity* to_delete : queued_deletions)
    {
        debug_printf("Deleting entity %08X\n", to_delete);
        deleteEntity(to_delete);
    }
}

void iterateOverEntities(EntityArrayCallback callback, void *arg, archetype_t componentMask, archetype_t rejectMask)
{
    int componentIndex, curArchetypeIndex;
    int numComponents = NUM_COMPONENTS(componentMask);
    int numComponentsFound = 0;
    auto components = std::unique_ptr<size_t[]>(new size_t[numComponents]);
    archetype_t componentBits = componentMask;

    // Clear the deletion queue
    queued_deletions = {};

    componentIndex = 0;
    while (componentBits)
    {
        if (componentBits & 1)
        {
            components[numComponentsFound++] = componentIndex;
        }
        componentBits >>= 1;
        componentIndex++;
    }

    for (curArchetypeIndex = 0; curArchetypeIndex < numArchetypes; curArchetypeIndex++)
    {
        archetype_t curArchetype = currentArchetypes[curArchetypeIndex];
        if (((curArchetype & componentMask) == componentMask) && !(curArchetype & rejectMask))
        {
            MultiArrayList *arr = &archetypeArrays[curArchetypeIndex];
            MultiArrayListBlock *curBlock = arr->start;
            auto curOffsets = std::unique_ptr<size_t[]>(new size_t[numComponents]);
            // Array for each component pointer, plus the pointer to the entity itself
            auto curAddresses = std::unique_ptr<void*[]>(new void*[numComponents + 1]);
            int i;
            
            // Find the offsets for each component
            // TODO this can be optimized by not using multiarraylist_get_component_offset
            for (i = 0; i < numComponents; i++)
            {
                curOffsets[i] = multiarraylist_get_component_offset(arr, components[i]);
            }

            // Iterate over every block in this multiarray
            while (curBlock)
            {
                // Address for the entity pointer
                curAddresses[0] = multiarraylist_get_block_entity_pointers(curBlock);
                // Get the addresses for each sub-array in the block
                for (i = 0; i < numComponents; i++)
                {
                    curAddresses[i + 1] = (void*)(curOffsets[i] + (uintptr_t)curBlock);
                }
                // Call the provided callback
                callback(curBlock->numElements, arg, curAddresses.get());
                // Advance to the next block
                curBlock = curBlock->next;
            }
        }
    }

    process_deletion_queue();
}

void iterateOverEntitiesAllComponents(EntityArrayCallbackAll callback, void *arg, archetype_t componentMask, archetype_t rejectMask)
{
    int curArchetypeIndex;

    // Clear the deletion queue
    queued_deletions = {};

    for (curArchetypeIndex = 0; curArchetypeIndex < numArchetypes; curArchetypeIndex++)
    {
        archetype_t curArchetype = currentArchetypes[curArchetypeIndex];
        if (((curArchetype & componentMask) == componentMask) && !(curArchetype & rejectMask))
        {
            int curComponentIndex;
            int curNumComponents = NUM_COMPONENTS(curArchetype);
            int curNumComponentsFound = 0;
            archetype_t componentBits = curArchetype;
            MultiArrayList *arr = &archetypeArrays[curArchetypeIndex];
            MultiArrayListBlock *curBlock = arr->start;
            auto curComponentSizes = std::unique_ptr<size_t[]>(new size_t[curNumComponents]);
            auto curOffsets = std::unique_ptr<size_t[]>(new size_t[curNumComponents]);
            auto curAddresses = std::unique_ptr<void*[]>(new void*[curNumComponents + 1]);
            size_t curOffset = sizeof(MultiArrayListBlock) + arr->elementCount * sizeof(Entity*);
            
            // Find all components in the current archetype and determine their size and offset in the multi array block
            curComponentIndex = 0;
            while (componentBits)
            {
                if (componentBits & 1)
                {
                    curOffsets[curNumComponentsFound] = curOffset;
                    curComponentSizes[curNumComponentsFound] = g_componentSizes[curComponentIndex];
                    curOffset += curComponentSizes[curNumComponentsFound] * arr->elementCount;
                    curNumComponentsFound++;
                }
                componentBits >>= 1;
                curComponentIndex++;
            }

            // Iterate over every block in this multiarray
            while (curBlock)
            {
                int i;
                curAddresses[0] = (void*)((uintptr_t)curBlock + sizeof(MultiArrayListBlock));
                // Get the addresses for each sub-array in the block
                for (i = 0; i < curNumComponents; i++)
                {
                    curAddresses[i + 1] = (void*)(curOffsets[i] + (uintptr_t)curBlock);
                }
                // Call the provided callback
                callback(curBlock->numElements, arg, curNumComponents, curArchetype, curAddresses.get(), curComponentSizes.get());
                // Advance to the next block
                curBlock = curBlock->next;
            }
        }
    }

    process_deletion_queue();
}

// TODO sort upon add, use binary search to check if already exists
void registerArchetype(archetype_t archetype)
{
    int i;
    // Don't add an archetype that is already registered
    for (i = 0; i < numArchetypes; i++)
    {
        if (currentArchetypes[i] == archetype)
            return;
    }
    currentArchetypes[numArchetypes] = archetype;
    multiarraylist_init(&archetypeArrays[numArchetypes], archetype);
    archetypeEntityCounts[numArchetypes] = 0;
    numArchetypes++;
}

// TODO use binary search after adding sorted archetype array
int getArchetypeIndex(archetype_t archetype)
{
    int i;
    for (i = 0; i < numArchetypes; i++)
    {
        if (currentArchetypes[i] == archetype)
            return i;
    }
    // If the loop completed then the archetype isn't registered, so register it
    registerArchetype(archetype);
    // Return the last archetype, which will be the newly registered one
    return numArchetypes - 1;
}

int findNextGap(int prevGap)
{
    int retVal;
    for (retVal = prevGap + 1; retVal < entitiesEnd; retVal++)
    {
        if (allEntities[retVal].archetype == 0)
        {
            return retVal;
        }
    }
    return -1;
}

void allocEntities(archetype_t archetype, int count, Entity** output)
{
    // The index of this archetype
    int archetypeIndex = getArchetypeIndex(archetype);
    int archetypeEntityCount = archetypeEntityCounts[archetypeIndex];
    Entity *curEntity;
    int curNumEntities = numEntities;
    int curGap = firstGap;

    // Fill in any gaps in the entity array
    while (numGaps)
    {
        allEntities[curGap].archetype = archetype;
        allEntities[curGap].archetypeArrayIndex = archetypeEntityCount++;
        *output = &allEntities[curGap];
        output++;
        curGap = findNextGap(curGap);
        curNumEntities++;
        numGaps--;
        count--;
    }
    // Update the first gap index
    firstGap = curGap;

    // Append new entities to the end of the array
    for (curEntity = &allEntities[entitiesEnd]; curEntity < &allEntities[entitiesEnd + count]; curEntity++)
    {
        curEntity->archetype = archetype;
        curEntity->archetypeArrayIndex = archetypeEntityCount++;
        *output = curEntity;
        output++;
        curNumEntities++;
    }

    // Update the count of entities of this archetype
    archetypeEntityCounts[archetypeIndex] = archetypeEntityCount;
    // Update the total entity count
    numEntities = curNumEntities;

    // If the entity list has run past the previous end index, update the end
    if (numEntities > entitiesEnd)
    {
        entitiesEnd = numEntities;
    }
}

Entity *createEntity(archetype_t archetype)
{
    // The index of this archetype
    int archetypeIndex = getArchetypeIndex(archetype);
    Entity *curEntity;
    // The arraylist for this archetype
    MultiArrayList *archetypeList = &archetypeArrays[archetypeIndex];
    
    // Allocate the components for the new entity
    multiarraylist_alloccount(archetypeList, 1);

    if (numGaps)
    {
        curEntity = &allEntities[firstGap];
        firstGap = findNextGap(firstGap);
    }
    else
    {
        curEntity = &allEntities[entitiesEnd];
        entitiesEnd++;
    }

    curEntity->archetype = archetype;
    curEntity->archetypeArrayIndex = archetypeEntityCounts[archetypeIndex];

    archetypeEntityCounts[archetypeIndex]++;
    numEntities++;

    return curEntity;
}

void deleteEntity(Entity *e)
{
    // The index of this archetype
    int archetypeIndex = getArchetypeIndex(e->archetype);
    size_t newLength = --archetypeEntityCounts[archetypeIndex];
    Entity *curEntity;

    multiarraylist_delete(&archetypeArrays[archetypeIndex], e->archetypeArrayIndex);

    // Ugly linear search over all entities to update the archetype array index of the entity that was moved in the delete
    // e->archetypeArrayIndex = 
    // TODO keep the entity list sorted?
    for (curEntity = &allEntities[0]; curEntity != &allEntities[entitiesEnd]; curEntity++)
    {
        if (curEntity->archetypeArrayIndex == newLength)
        {
            curEntity->archetypeArrayIndex = e->archetypeArrayIndex;
            break;
        }
    }
    
    // Find the deleted entity in the list and clear its data
    int entityIndex = e - &allEntities[0];
    e->archetype = 0;
    e->archetypeArrayIndex = 0;
    numEntities--;
    // If this is the last entity, update the end index
    if (entityIndex == entitiesEnd - 1)
    {
        entitiesEnd--;
    }
    // Otherwise, increase the number of gaps
    else
    {
        numGaps++;
        // If this new gap is less than the previous first gap, update the first gap to this one
        if (entityIndex < firstGap)
        {
            firstGap = entityIndex;
        }
    }
}

void deleteEntityIndex(int index)
{
    // The archetype and array list index for this entity
    archetype_t archetype = allEntities[index].archetype;
    size_t archetypeArrayIndex = allEntities[index].archetypeArrayIndex;
    // The index of this archetype
    int archetypeIndex = getArchetypeIndex(archetype);
    size_t newLength = --archetypeEntityCounts[archetypeIndex];
    Entity *curEntity;

    // Delete this entity's component info
    multiarraylist_delete(&archetypeArrays[archetypeIndex], archetypeArrayIndex);

    // Ugly linear search over all entities to update the archetype array index of the entity that was moved in the delete
    // TODO keep the entity list sorted?
    for (curEntity = &allEntities[0]; curEntity != &allEntities[entitiesEnd]; curEntity++)
    {
        if (curEntity->archetypeArrayIndex == newLength)
        {
            curEntity->archetypeArrayIndex = archetypeArrayIndex;
            break;
        }
    }
    
    // Clear the deleted entity's data
    allEntities[index].archetype = 0;
    allEntities[index].archetypeArrayIndex = 0;
    numEntities--;

    // If this is the last entity, update the end index
    if (index == entitiesEnd - 1)
    {
        entitiesEnd--;
    }
    // Otherwise, increase the number of gaps
    else
    {
        numGaps++;
        // If this new gap is less than the previous first gap, update the first gap to this one
        if (index < firstGap)
        {
            firstGap = index;
        }
    }
}

void createEntities(archetype_t archetype, int count)
{
    // // The index of this archetype
    // int archetypeIndex = getArchetypeIndex(archetype);
    // // The arraylist for this archetype
    // MultiArrayList *archetypeList = &archetypeArrays[archetypeIndex];
    // // Array of pointers to each entity created
    // auto entity_pointers = std::unique_ptr<Entity*[]>(new Entity*[count]);
    
    // // Allocate the components for the given number of the given archetype
    // multiarraylist_alloccount(archetypeList, count);
    // // Allocate the number of entities given of the archetype given
    // allocEntities(archetype, count, entity_pointers.get());
    createEntitiesCallback(archetype, nullptr, count, nullptr);
}

void createEntitiesCallback(archetype_t archetype, void *arg, int count, EntityArrayCallback callback)
{
    // The index of this archetype
    int archetypeIndex = getArchetypeIndex(archetype);
    // The arraylist for this archetype
    MultiArrayList *archetypeList = &archetypeArrays[archetypeIndex];
    // Number of components in the given archetype
    int numComponents = NUM_COMPONENTS(archetype);

    // Iteration values
    // Offsets for each component in the arraylist blocks
    auto componentOffsets = std::unique_ptr<size_t[]>(new size_t[numComponents]);
    // Sizes of each component in the arraylist blocks
    auto componentSizes = std::unique_ptr<size_t[]>(new size_t[numComponents]);
    // Array of pointers to each entity created
    auto entity_pointers = std::unique_ptr<Entity*[]>(new Entity*[count]);
    // The block being iterated through
    MultiArrayListBlock *curBlock = archetypeList->end;
    // Number of elements in the current block before allocating more
    uint32_t startingElementCount = curBlock->numElements;
    
    // Allocate the number of entities given of the archetype given
    allocEntities(archetype, count, entity_pointers.get());
    // Increase the entity count for the given archetype by the given amount
    archetypeEntityCounts[archetypeIndex] += count;
    // Allocate the requested number of entities for the given archetype
    multiarraylist_alloccount(archetypeList, count);

    // Find all the components in this archetype and get their offsets and sizes for iteration
    // TODO this can be optimized by not using multiarraylist_get_component_offset
    {
        int numComponentsFound = 0;
        int componentIndex = 0;
        archetype_t componentBits = archetype;
        while (componentBits)
        {
            if (componentBits & 1)
            {
                componentOffsets[numComponentsFound] = multiarraylist_get_component_offset(archetypeList, componentIndex);
                componentSizes[numComponentsFound++] = g_componentSizes[componentIndex];
            }
            componentBits >>= 1;
            componentIndex++;
        }
    }

    // Call the provided callback for modified or new block in the list
    {
        int i;
        auto componentArrays = std::unique_ptr<void*[]>(new void*[numComponents + 1]);
        Entity **cur_entity = entity_pointers.get();

        componentArrays[0] = multiarraylist_get_block_entity_pointers(curBlock);
        // Call the callback for the original block, which was modified
        for (i = 0; i < numComponents; i++)
        {
            componentArrays[i + 1] = (void*)((uintptr_t)curBlock + componentOffsets[i] + componentSizes[i] * startingElementCount);
        }

        // Copy the entity pointers into the 0th component array
        std::copy_n(cur_entity, curBlock->numElements - startingElementCount, (Entity**)componentArrays[0]);
        cur_entity += curBlock->numElements - startingElementCount;

        if (callback)
        {
            callback(curBlock->numElements - startingElementCount, arg, componentArrays.get());
        }
        curBlock = curBlock->next;

        // Call the callback for any following blocks, which were allocated
        while (curBlock)
        {
            componentArrays[0] = multiarraylist_get_block_entity_pointers(curBlock);
            cur_entity++;
            for (i = 0; i < numComponents; i++)
            {
                componentArrays[i] = (void*)((uintptr_t)curBlock + componentOffsets[i]);
            }
            
            // Copy the entity pointers into the 0th component array
            std::copy_n(cur_entity, curBlock->numElements, (Entity**)componentArrays[0]);
            cur_entity += curBlock->numElements;

            if (callback)
            {
                callback(curBlock->numElements, arg, componentArrays.get());
            }
            curBlock = curBlock->next;
        }
    }
}

void getEntityComponents(Entity *entity, void **componentArrayOut)
{
    archetype_t archetype = entity->archetype;
    int archetypeIndex = getArchetypeIndex(archetype);
    MultiArrayList *archetypeArray = &archetypeArrays[archetypeIndex];
    size_t blockElementCount = archetypeArray->elementCount;
    size_t arrayIndex = entity->archetypeArrayIndex;
    MultiArrayListBlock *curBlock = archetypeArray->start;
    int componentIndex = 0; // Index of the component in all components
    int componentArrayIndex = 0; // Index of the component in those in the archetype

    while (arrayIndex >= blockElementCount)
    {
        curBlock = curBlock->next;
        arrayIndex -= blockElementCount;
    }

    while (archetype)
    {
        if (archetype & 0x01)
        {
            componentArrayOut[componentArrayIndex] = (void *)(
                (uintptr_t)curBlock + // skip the block header
                multiarraylist_get_component_offset(archetypeArray, componentIndex) + // go to the start of the component array for this component
                g_componentSizes[componentIndex] * arrayIndex), // index the component array
            componentArrayIndex++;
        }
        componentIndex++;
        archetype >>= 1;
    }
}

Entity *findEntity(archetype_t archetype, size_t archetypeArrayIndex)
{
    Entity* curEntity;

    for (curEntity = &allEntities[0]; curEntity != &allEntities[entitiesEnd]; curEntity++)
    {
        if (curEntity->archetype == archetype && curEntity->archetypeArrayIndex == archetypeArrayIndex)
            return curEntity;
    }
    return nullptr;
}

void deleteAllEntities(void)
{
    int archetypeIndex;
    for (archetypeIndex = 0; archetypeIndex < numArchetypes; archetypeIndex++)
    {
        MultiArrayList *curArr = &archetypeArrays[archetypeIndex];
        MultiArrayListBlock *curBlock = curArr->start;

        memset(curArr, 0, sizeof(MultiArrayList));
        archetypeEntityCounts[archetypeIndex] = 0;
        currentArchetypes[archetypeIndex] = 0;
        while (curBlock)
        {
            MultiArrayListBlock *nextBlock = curBlock->next;

            freeAlloc(curBlock);

            curBlock = nextBlock;
        }
    }
    numArchetypes = 0;
    memset(allEntities, 0, sizeof(allEntities));
    numEntities = 0;
    entitiesEnd = 0;
    numGaps = 0;
    firstGap = 0;
}

void processBehaviorEntities(size_t count, UNUSED void *arg, int numComponents, archetype_t archetype, void **componentArrays, size_t *componentSizes)
{
    int i = 0;
    // Get the index of the BehaviorParams component in the component array and iterate over it
    BehaviorParams *curBhvParams = static_cast<BehaviorParams*>(componentArrays[COMPONENT_INDEX(Behavior, archetype)]);
    // Iterate over every entity in the given array
    while (count)
    {
        // Call the entity's callback with the component pointers and it's data pointer
        curBhvParams->callback(componentArrays, curBhvParams->data);

        // Increment the component pointers so they are valid for the next entity
        for (i = 0; i < numComponents; i++)
        {
            componentArrays[i] = static_cast<uint8_t*>(componentArrays[i]) + componentSizes[i];
        }
        // Increment to the next entity's behavior params
        curBhvParams++;
        // Decrement the remaining entity count
        count--;
    }
}

void iterateBehaviorEntities()
{
    iterateOverEntitiesAllComponents(processBehaviorEntities, nullptr, Bit_Behavior, 0);
}
