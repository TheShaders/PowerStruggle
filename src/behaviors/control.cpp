#include <cmath>

#include <mathutils.h>
#include <ecs.h>
#include <control.h>
#include <interaction.h>
#include <player.h>
#include <input.h>

struct FindControllableData
{
    float* nearPos;
    float max_dist_sq;
    float closest_dist_sq;
    Vec3* closest_pos;
    Entity* entity;
    BehaviorState* behavior;
    int closest_health;
};

void findClosestControllableCallback(size_t count, void *arg, void **componentArrays)
{
    FindControllableData *find_data = (FindControllableData *)arg;

    Entity** cur_entity = static_cast<Entity**>(componentArrays[0]);
    Vec3* cur_pos = get_component<Bit_Position, Vec3>(componentArrays, ARCHETYPE_CONTROLLABLE);
    HealthState* cur_health = get_component<Bit_Health, HealthState>(componentArrays, ARCHETYPE_CONTROLLABLE);
    ControlParams* cur_control_param = get_component<Bit_Control, ControlParams>(componentArrays, ARCHETYPE_CONTROLLABLE);
    BehaviorState* cur_behavior = get_component<Bit_Behavior, BehaviorState>(componentArrays, ARCHETYPE_CONTROLLABLE);
    
    float max_dist_sq = find_data->max_dist_sq;
    float closest_dist_sq = find_data->closest_dist_sq;
    Entity* closest_entity = find_data->entity;
    Vec3* closest_pos = find_data->closest_pos;
    BehaviorState* closest_behavior = find_data->behavior;
    int closest_health = find_data->closest_health;

    Vec3 nearPos = { find_data->nearPos[0], find_data->nearPos[1], find_data->nearPos[2] };
    while (count)
    {
        if (cur_health->health <= cur_control_param->controllable_health)
        {
            Vec3 posDiff;
            float curDistSq;
            VEC3_DIFF(posDiff, *cur_pos, nearPos);
            curDistSq = VEC3_DOT(posDiff, posDiff);

            if (curDistSq < max_dist_sq && curDistSq < closest_dist_sq)
            {
                closest_dist_sq = curDistSq;
                closest_pos = cur_pos;
                closest_entity = *cur_entity;
                closest_behavior = cur_behavior;
                closest_health = cur_health->health;
            }
        }

        count--;
        cur_pos++;
        cur_entity++;
        cur_health++;
        cur_control_param++;
        cur_behavior++;
    }

    find_data->closest_dist_sq = closest_dist_sq;
    find_data->closest_pos = closest_pos;
    find_data->entity = closest_entity;
    find_data->behavior = closest_behavior;
    find_data->closest_health = closest_health;
}

Entity* get_controllable_entity_at_position(Vec3 pos, float radius, Vec3 found_pos, float& found_dist, BehaviorState*& found_behavior, int& found_health)
{
    FindControllableData find_data = 
    {
        .nearPos = pos,
        .max_dist_sq = std::pow<float, float>(radius, 2),
        .closest_dist_sq = std::numeric_limits<float>::max(),
        .closest_pos = nullptr,
        .entity = nullptr,
        .behavior = nullptr,
        .closest_health = 0,
    };
    iterateOverEntities(findClosestControllableCallback, &find_data, ARCHETYPE_CONTROLLABLE, 0);

    if (find_data.entity != nullptr)
    {
        found_dist = std::sqrt(find_data.closest_dist_sq);
        VEC3_COPY(found_pos, *find_data.closest_pos);
        found_behavior = find_data.behavior;
        found_health = find_data.closest_health;
        return find_data.entity;
    }

    return nullptr;
}

Vec3 control_search_pos;
Vec3 control_pos;
float control_dist;
Entity* to_control = nullptr;
BehaviorState* to_control_behavior = nullptr;
int control_health = 0;

void control_update()
{
    Entity* player = g_PlayerEntity;
    void *player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);

    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    // Vec3s& player_rot = *get_component<Bit_Rotation, Vec3s>(player_components, ARCHETYPE_PLAYER);

    control_search_pos[0] = g_PlayerInput.x * 150.0f + player_pos[0];
    control_search_pos[1] = player_pos[1];
    control_search_pos[2] = -g_PlayerInput.y * 150.0f + player_pos[2];

    to_control = get_controllable_entity_at_position(control_search_pos, 150.0f, control_pos, control_dist, to_control_behavior, control_health);
}
