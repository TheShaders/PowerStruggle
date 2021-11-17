#include <array>
#include <cmath>

#include <files.h>
#include <mathutils.h>
#include <interaction.h>
#include <behaviors.h>
#include <control.h>
#include <collision.h>
#include <main.h>

float approach_target(float sight_radius, float follow_distance, float move_speed, Vec3 pos, Vec3 vel, Vec3s rot, Vec3 target_pos)
{
    // Get the displacement vector from the player to this enemy
    float dx = pos[0] - target_pos[0];
    float dz = pos[2] - target_pos[2];
    float dist_sq = dx * dx + dz * dz;
    if (dist_sq < EPSILON) return 0.0f;
    float dist = sqrtf(dist_sq);
    // Check if the player can be seen
    if (dist < sight_radius)
    {
        // If so, determine where to move towards

        // Get the normalized vector from the player to this enemy
        float dx_norm = dx * (1.0f / dist);
        float dz_norm = dz * (1.0f / dist);

        // Calculate the target position
        float target_x = target_pos[0] + dx_norm * follow_distance;
        float target_z = target_pos[2] + dz_norm * follow_distance;

        // Get the displacement vector from the enemy to the target position
        float target_dx = target_x - pos[0];
        float target_dz = target_z - pos[2];
        float target_dist_sq = target_dx * target_dx + target_dz * target_dz;
        float target_dist = sqrtf(target_dist_sq);

        // Calculate the normal vector from the enemy to the target position
        float target_dx_norm = target_dx * (1.0f / target_dist);
        float target_dz_norm = target_dz * (1.0f / target_dist);

        // Calculate the target speed
        float target_speed = std::min(move_speed, target_dist * 0.25f);
        float target_vel_x = target_speed * target_dx_norm;
        float target_vel_z = target_speed * target_dz_norm;

        vel[0] = approachFloatLinear(vel[0], target_vel_x, 1.0f);
        vel[2] = approachFloatLinear(vel[2], target_vel_z, 1.0f);

        rot[1] = atan2s(-dz, -dx);
    }
    else
    {
        vel[0] = approachFloatLinear(vel[0], 0.0f, 1.0f);
        vel[2] = approachFloatLinear(vel[2], 0.0f, 1.0f);
    }
    return dist;
}

std::array create_enemy_funcs {
    create_shooter,
    create_slasher
};

Entity* create_enemy(float x, float y, float z, EnemyType type, int subtype)
{
    return create_enemy_funcs[static_cast<int>(type)](x, y, z, subtype);
}

void init_enemy_common(BaseEnemyInfo* base_info, Model** model_out, HealthState* health_out)
{
    // Set up the enemy's model
    // Load the model if it isn't already loaded
    if (base_info->model == nullptr)
    {
        base_info->model = load_model(base_info->model_name);
    }
    *model_out = base_info->model;

    // Set up the enemy's health
    health_out->max_health = base_info->max_health;
}

extern ControlHandler shooter_control_handler;
extern ControlHandler slasher_control_handler;

ControlHandler* control_handlers[] = {
    &shooter_control_handler,
    &slasher_control_handler
};

void take_damage(Entity* hit_entity, HealthState& health_state, int damage)
{
    if (damage >= health_state.health)
    {
        queue_entity_deletion(hit_entity);
    }
    health_state.health -= damage;
}

void handle_enemy_hits(Entity* enemy, ColliderParams& collider, HealthState& health_state)
{
    Hit* cur_hit = collider.hits;
    while (cur_hit != nullptr)
    {
        if (g_gameTimer - health_state.last_hit_time < 10)
        {
            break;
        }
        take_damage(enemy, health_state, 10);
        health_state.last_hit_time = g_gameTimer;
        // queue_entity_deletion(cur_hit->entity);
        cur_hit = cur_hit->next;
    }
}
