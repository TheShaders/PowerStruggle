#ifndef __INTERACTION_H__
#define __INTERACTION_H__

#include <types.h>

struct HealthState
{
    uint16_t maxHealth;
    uint16_t health;
};

// The width of a health bar is the entity's max health divided by this number, times four (since fillrects need to be a multiple of 4)
constexpr size_t health_per_pixel = 4;
constexpr size_t health_bar_height = 4;

Entity *findClosestEntity(Vec3 pos, archetype_t archetype, float maxDist, float *foundDist, Vec3 foundPos);

#endif
