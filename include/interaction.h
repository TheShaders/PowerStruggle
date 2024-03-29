#ifndef __INTERACTION_H__
#define __INTERACTION_H__

#include <types.h>

struct HealthState
{
    uint16_t max_health;
    uint16_t health;
    uint32_t last_hit_time;
};

// The width of a health bar is the entity's max health divided by this number, times four (since fillrects need to be a multiple of 4)
constexpr int health_per_pixel = 4;
constexpr int health_bar_height = 4;

Entity *findClosestEntity(Vec3 pos, archetype_t archetype, float maxDist, float *foundDist, Vec3 foundPos);
int take_damage(Entity* entity, HealthState& health_state, int damage);
void create_explosion(Vec3 pos, int radius, int time, int mask);

constexpr float min_height = -256 * 10;

#endif
