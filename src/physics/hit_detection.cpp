#include <array>
#include <bit>

#include <ecs.h>
#include <grid.h>
#include <collision.h>
#include <block_vector.h>
#include <n64_mathutils.h>

extern "C" {
#include <debug.h>
}

consteval unsigned int nearest_pow_2(unsigned int x)
{
    // If x is already a power of 2, return it
    if (std::has_single_bit(x))
    {
        return x;
    }
    // Get the number of leading zeroes
    unsigned int num_zeroes = std::countl_zero(x);
    // Subtract that from the total bit count to get the bit width of the number
    unsigned int width = sizeof(x) * 8 - num_zeroes;
    // Shift 1 by the bit width to get the next power of 2
    return 1 << width;
}

// Maximum number of chunks that can be loaded simultaneously in the x direction
constexpr int max_chunks_x = round_away_divide(visible_inner_range_x * 2, static_cast<int>(tile_size * chunk_size)) + 1;
// Maximum number of chunks that can be loaded simultaneously in the z direction
constexpr int max_chunks_z = round_away_divide(visible_outer_range_pos_z + visible_outer_range_neg_z, static_cast<int>(tile_size * chunk_size)) + 1;

// Maximum number of tiles that can be loaded simultaneously in the x direction
constexpr int max_tiles_x = max_chunks_x * chunk_size;
// Maximum number of tiles that can be loaded simultaneously in the z direction
constexpr int max_tiles_z = max_chunks_z * chunk_size;

// Dimensions of the collision grid in tiles
// Keeping it a power of 2 ensures quick access (a singular left shift and add for indexing a 2D array)
constexpr unsigned int collision_grid_size_x = nearest_pow_2(max_tiles_x);
constexpr unsigned int collision_grid_size_z = nearest_pow_2(max_tiles_z);

struct HitboxNode
{
    HitboxNode *next;
    Hitbox *hitbox;
    Entity *entity;
    Vec3 *pos;
};

// Array of the list of hitboxes for each tile
std::array<std::array<HitboxNode*, max_tiles_x>, max_tiles_z> tile_hitboxes;
block_vector<HitboxNode> node_pool;

struct GatherHitboxesParams
{
    int start_tile_x;
    int start_tile_z;
};

void gather_hitboxes(size_t count, void *arg, void **componentArrays)
{
    // Read the parameters
    GatherHitboxesParams* params = static_cast<GatherHitboxesParams*>(arg);
    int start_tile_x = params->start_tile_x;
    int start_tile_z = params->start_tile_z;
    // Read the components
    Entity **cur_entity = static_cast<Entity**>(componentArrays[0]);
    Vec3 *cur_pos = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_HITBOX)]);
    Hitbox *cur_hitbox = static_cast<Hitbox*>(componentArrays[COMPONENT_INDEX(Hitbox, ARCHETYPE_HITBOX)]);
    // Iterate over every entity
    while (count)
    {
        // debug_printf("Binning hitbox entity %08X at {%4.0f %4.0f %4.0f}\n", (*cur_pos)[0], (*cur_pos)[1], (*cur_pos)[2]);
        // Get the extents of the hitbox
        int min_x = lfloor((*cur_pos)[0] - cur_hitbox->radius);
        int min_z = lfloor((*cur_pos)[2] - cur_hitbox->radius);
        int max_x = lceil((*cur_pos)[0] + cur_hitbox->radius);
        int max_z = lceil((*cur_pos)[2] + cur_hitbox->radius);
        
        // debug_printf("min_x %d min_z %d max_x %d max_z %d\n", min_x, min_z, max_x, max_z);
        // Convert the extents to tile coordinates, offset by the array's start tile
        int min_array_x = round_down_divide<tile_size>(min_x) - start_tile_x;
        int min_array_z = round_down_divide<tile_size>(min_z) - start_tile_z;
        int max_array_x = round_up_divide<tile_size>(max_x)   - start_tile_x;
        int max_array_z = round_up_divide<tile_size>(max_z)   - start_tile_z;

        // debug_printf("min_arr_x %d min_arr_z %d max_arr_x %d max_arr_z %d\n", min_array_x, min_array_z, max_array_x, max_array_z);
        // Check if the entity is within the bounds of the currently loaded chunks
        if (
            min_array_x < max_tiles_x &&
            min_array_z < max_tiles_z &&
            max_array_x >= 0 &&
            max_array_z >= 0)
        {
            // Constrain the entity bounds to the loaded chunks
            min_array_x = std::max(0, min_array_x);
            min_array_z = std::max(0, min_array_z);
            max_array_x = std::min(max_tiles_x - 1, max_array_x);
            max_array_z = std::min(max_tiles_z - 1, max_array_z);
            // Iterate over the tile extents and insert a new node into each tile pointing to the current hitbox
            for (int x = min_array_x; x <= max_array_x; x++)
            {
                for (int z = min_array_z; z <= max_array_z; z++)
                {
                    // Get the current start node for this tile
                    auto& cur_node = tile_hitboxes[x][z];
                    // Allocate a new node in the node pool pointing to the start node that was retrieved
                    auto it = node_pool.emplace_back(cur_node, cur_hitbox, *cur_entity, cur_pos);
                    // Replace the start node with the newly allocated one
                    tile_hitboxes[x][z] = &(*it);
                }
            }
        }
        else
        {
            // debug_printf("Hitbox entity %08X not in loaded chunks\n", cur_entity);
        }
        count--;
        cur_entity++;
        cur_pos++;
        cur_hitbox++;
    }
}

FORCEINLINE void test_collider(
    Entity* entity, ColliderParams* collider, Vec3 pos,
    int min_array_x, int min_array_z, int max_array_x, int max_array_z,
    skipfield<Entity*, 16>& checked_entities)
{
    // debug_printf("Testing collider entity %08X at {%4.0f %4.0f %4.0f}\n", pos[0], pos[1], pos[2]);
    // Iterate over the tile extents and insert a new node into each tile pointing to the current hitbox
    for (int x = min_array_x; x <= max_array_x; x++)
    {
        for (int z = min_array_z; z <= max_array_z; z++)
        {
            // Get the hitbox list for this tile
            HitboxNode* cur_node = tile_hitboxes[x][z];
            // Ensure that the tile has an associated hitbox list
            if (cur_node != nullptr)
            {
                Hitbox* cur_hitbox = cur_node->hitbox;
                Entity* hitbox_entity = cur_node->entity;
                Vec3& hitbox_pos = *cur_node->pos;
                // If this entity hasn't been checked yet, check it
                if (std::find(checked_entities.begin(), checked_entities.end(), hitbox_entity) == checked_entities.end())
                {
                    // debug_printf("checking %d %d\n", x, z);
                    // Add the hitbox entity to the checked entities
                    checked_entities.emplace(hitbox_entity);

                    // Check if the collider and hitbox intersect
                    // Start by checking if their y ranges overlap
                    if (pos[1] < hitbox_pos[1] + cur_hitbox->height &&
                        hitbox_pos[1] < pos[1] + collider->height)
                    {
                        // Now check if the circles of the collider and hitbox intersect
                        float x_dist_sqr = std::pow(pos[0] - hitbox_pos[0], 2.0f);
                        float z_dist_sqr = std::pow(pos[2] - hitbox_pos[2], 2.0f);
                        float radii_sum_sqr = std::pow(collider->radius + cur_hitbox->radius, 2.0f);
                        if (radii_sum_sqr > x_dist_sqr + z_dist_sqr)
                        {
                            // The collider intersects with the hitbox
                            // debug_printf("Collider entity %08X intersects with hitbox entity %08X\n", entity, hitbox_entity);
                        }
                    }

                    // If we've checked the maximum number of entities, return
                    if (checked_entities.full())
                    {
                        return;
                    }
                }
                else
                {
                    // debug_printf("already checked %d %d\n", x, z);
                }
            }
        }
    }
}

void test_hitboxes(size_t count, void *arg, void **componentArrays)
{
    // Read the parameters
    GatherHitboxesParams* params = static_cast<GatherHitboxesParams*>(arg);
    int start_tile_x = params->start_tile_x;
    int start_tile_z = params->start_tile_z;
    // Read the components
    Entity **cur_entity = static_cast<Entity**>(componentArrays[0]);
    Vec3 *cur_pos = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_COLLIDER)]);
    ColliderParams *cur_collider = static_cast<ColliderParams*>(componentArrays[COMPONENT_INDEX(Collider, ARCHETYPE_COLLIDER)]);
    // Skipfield for holding the entities that have been checked
    // TODO replace this with a custom fixed-capacity stack-allocated vector type instead
    skipfield<Entity*, 16> checked_entities{};
    // Iterate over every entity
    while (count)
    {
        // Clear the list of checked entities
        checked_entities.clear();
        // Get the extents of the collider
        int min_x = lfloor((*cur_pos)[0] - cur_collider->radius);
        int min_z = lfloor((*cur_pos)[2] - cur_collider->radius);
        int max_x = lceil((*cur_pos)[0] + cur_collider->radius);
        int max_z = lceil((*cur_pos)[2] + cur_collider->radius);
        // Convert the extents to tile coordinates, offset by the array's start tile
        int min_array_x = round_down_divide<tile_size>(min_x) - start_tile_x;
        int min_array_z = round_down_divide<tile_size>(min_z) - start_tile_z;
        int max_array_x = round_up_divide<tile_size>(max_x)   - start_tile_x;
        int max_array_z = round_up_divide<tile_size>(max_z)   - start_tile_z;
        // Test the collider
        test_collider(*cur_entity, cur_collider, *cur_pos, min_array_x, min_array_z, max_array_x, max_array_z, checked_entities);
        count--;
        cur_entity++;
        cur_pos++;
        cur_collider++;
    }
}

void find_collisions(Grid& grid)
{
    // Clear the tile hitbox list
    std::fill_n(&tile_hitboxes[0][0], max_tiles_x * max_tiles_z, nullptr);

    // Get the minimum tile index, which will be used to offset all entity positions
    chunk_pos min_chunk = grid.get_minimum_loaded_chunk();
    int start_tile_x = min_chunk.first  * chunk_size;
    int start_tile_z = min_chunk.second * chunk_size;
    // debug_printf("start_x %d start_z %d\n", start_tile_x, start_tile_z);

    // Initialize the node pool
    // This will free the previous frame's node pool's memory via move assignment
    node_pool = {};
    GatherHitboxesParams params {
        start_tile_x,
        start_tile_z
    };

    // Assign node lists to each tile containing the hitboxes that extend into the respective tile
    iterateOverEntities(gather_hitboxes, &params, ARCHETYPE_HITBOX, 0);

    // Iterate over every collider and check for intersection with any hitboxes using the hitbox list array
    iterateOverEntities(test_hitboxes, &params, ARCHETYPE_COLLIDER, 0);
}
