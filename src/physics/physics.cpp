#include <physics.h>
#include <config.h>
#include <mathutils.h>
#include <ecs.h>
#include <collision.h>

#include <memory>

extern "C" {
#include <debug.h>
}

#define ARCHETYPE_POSVEL       (Bit_Position | Bit_Velocity)
#define ARCHETYPE_GRAVITY      (Bit_Velocity | Bit_Gravity)
#define ARCHETYPE_COLLIDER     (Bit_Position | Bit_Velocity | Bit_Collider)

void applyGravityCallback(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Velocity, Gravity
    Vec3 *curVel = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Velocity, ARCHETYPE_GRAVITY)]);
    GravityParams *gravity = static_cast<GravityParams*>(componentArrays[COMPONENT_INDEX(Gravity, ARCHETYPE_GRAVITY)]);

    while (count)
    {
        (*curVel)[1] += gravity->accel;

        if ((*curVel)[1] < gravity->terminalVelocity)
        {
            (*curVel)[1] = gravity->terminalVelocity;
        }
        
        curVel++;
        gravity++;
        count--;
    }
}

void applyVelocityCallback(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Velocity
    Vec3 *curPos = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_POSVEL)]);
    Vec3 *curVel = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Velocity, ARCHETYPE_POSVEL)]);

    while (count)
    {
        VEC3_ADD(*curPos, *curPos, *curVel);

        curPos++;
        curVel++;
        count--;
    }
}

void resolveGridCollisionsCallback(size_t count, void *arg, void **componentArrays)
{
    Grid* grid = static_cast<Grid*>(arg);
    // Components: Position, Velocity, Collider
    Vec3 *curPos = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_COLLIDER)]);
    Vec3 *curVel = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Velocity, ARCHETYPE_COLLIDER)]);
    ColliderParams *curCollider = static_cast<ColliderParams*>(componentArrays[COMPONENT_INDEX(Collider, ARCHETYPE_COLLIDER)]);
    Entity **cur_entity = static_cast<Entity**>(componentArrays[0]);

    while (count)
    {
        // Handle wall collision (since it's universal across states currently)
        handleWalls(grid, curCollider, *curPos, *curVel);
        if (curCollider->floor_surface_type != surface_none)
        {
            // debug_printf("Entity %08X is on the ground\n", *cur_entity);
            curCollider->floor_surface_type = handleFloorOnGround(grid, curCollider, *curPos, *curVel, MAX_STEP_UP, MAX_STEP_DOWN);
        }
        else
        {
            // debug_printf("Entity %08X is in the air\n", *cur_entity);
            curCollider->floor_surface_type = handleFloorInAir(grid, curCollider, *curPos, *curVel);
        }
        if (curCollider->floor_surface_type != surface_none)
        {
            (*curVel)[0] *= curCollider->friction_damping;
            (*curVel)[2] *= curCollider->friction_damping;
        }

        curPos++;
        curVel++;
        curCollider++;
        cur_entity++;
        count--;
    }
}

void physicsTick(Grid& grid)
{
    // Apply gravity to all objects that are affected by it
    iterateOverEntities(applyGravityCallback, nullptr, ARCHETYPE_GRAVITY, 0);
    // Apply every object's velocity to their position
    iterateOverEntities(applyVelocityCallback, nullptr, ARCHETYPE_POSVEL, 0);

    // Resolve collisions with the grid
    iterateOverEntities(resolveGridCollisionsCallback, &grid, ARCHETYPE_COLLIDER, 0);
}
