#include <cstring>
extern "C" { 
#include <debug.h>
}
#include <mathutils.h>
#include <main.h>
#include <gfx.h>
#include <mem.h>
#include <ecs.h>
#include <model.h>
#include <player.h>
#include <collision.h>
#include <physics.h>
#include <input.h>
#include <camera.h>
#include <audio.h>
#include <level.h>
#include <profiling.h>
#include <platform.h>

#include <platform_gfx.h>

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

PlayerState playerState;

uint32_t g_gameTimer = 0;
uint32_t g_graphicsTimer = 0;
extern uint32_t fillColor;

#define CREDITS_LOAD_FADE_TIME 60
#define CREDITS_LOAD_TIME 90

uint32_t creditsTimer = 0;

extern int32_t firstFrame; // HACK

int main(UNUSED int argc, UNUSED char **arg)
{
    int frame = 0;
    
    debug_printf("Main\n");

    platformInit();

    initInput();
    initGfx();

    // Create the player entity
    createPlayer(&playerState);

    // Create the level collision entity
    // // debug_printf("Creating collision entity\n");
    // createEntitiesCallback(Bit_Collision, segmentedToVirtual(&test_collision_collision_tree), 1, setCollision);

    // {
    //     Entity *toDelete;
    //     Entity *lastEntity;
    //     // debug_printf("Creating 5000 position only entities\n");
    //     createEntities(Bit_Position, 5000);

    //     // debug_printf("Creating 1 position only entity to delete\n");
    //     toDelete = createEntity(Bit_Position);
    //     // debug_printf("To delete entity archetype array index: %d\n", toDelete->archetypeArrayIndex);

    //     // debug_printf("Creating 5000 more position only entities\n");
    //     createEntities(Bit_Position, 5000);

    //     // debug_printf("Creating 1 position only entity to test\n");
    //     lastEntity = createEntity(Bit_Position);
    //     // debug_printf("Test entity archetype array index: %d\n", lastEntity->archetypeArrayIndex);
        
    //     // debug_printf("Deleting entity at array position %d\n", toDelete->archetypeArrayIndex);
    //     deleteEntity(toDelete);

    //     // debug_printf("Last entity was moved to %d\n", lastEntity->archetypeArrayIndex);
    // }
    // debug_printf("Processing level header\n");
    // processLevelHeader(segmentedToVirtual(&mainHeader));

    while (1)
    {
        Vec3 lightDir = { 100.0f * sinf((M_PIf / 180.0f) * 45.0f), 100.0f * cosf((M_PIf / 180.0f) * 45.0f), 0.0f};
        
        // Vec3 lightDir = { 100.0f * sinf((M_PI / 180.0f) * angle), 100.0f * cosf((M_PI / 180.0f) * angle), 0.0f};
#ifdef DEBUG_MODE
        profileStartMainLoop();
#endif
        debug_printf("before input polling\n");
        beginInputPolling();
        debug_printf("before start frame\n");
        startFrame();
        debug_printf("before read input\n");
        readInput();

        if (frame < 10)
        {
            memset(&g_PlayerInput, 0, sizeof(g_PlayerInput));
        }

        // Increment the physics state
        // debug_printf("before physics tick\n");
        physicsTick();
        // Process all entities that have a behavior
        // debug_printf("before behaviors\n");
        iterateOverEntitiesAllComponents(processBehaviorEntities, nullptr, Bit_Behavior, 0);
        g_gameTimer++;
#ifdef FPS30
        beginInputPolling();
        readInput();

        if (frame < 10)
        {
            memset(&g_PlayerInput, 0, sizeof(g_PlayerInput));
        }
        // Just run everything twice per frame to match 60 fps gameplay speed lol
        // Increment the physics state
        // debug_printf("before physics tick\n");
        physicsTick();
        // Process all entities that have a behavior
        // debug_printf("before behaviors\n");
        iterateOverEntitiesAllComponents(processBehaviorEntities, nullptr, Bit_Behavior, 0);
        g_gameTimer++;
#endif
        
        // Set up the camera
        // debug_printf("before camera\n");
        setupCameraMatrices(&g_Camera);
        // debug_printf("before light dir\n");
        setLightDirection(lightDir);

        // debug_printf("before drawing\n");
        drawAllEntities();
        
#ifdef DEBUG_MODE
        profileBeforeGfxMainLoop();
#endif

        g_graphicsTimer++;

        endFrame();
        // debug_printf("After end frame\n");

#ifdef DEBUG_MODE
        profileEndMainLoop();
#endif

        frame++;
    }
}
