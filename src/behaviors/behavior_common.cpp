#include <cmath>
#include <mathutils.h>

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

        rot[1] = atan2s(dz, dx);
    }
    else
    {
        vel[0] = approachFloatLinear(vel[0], 0.0f, 1.0f);
        vel[2] = approachFloatLinear(vel[2], 0.0f, 1.0f);
    }
    return dist;
}