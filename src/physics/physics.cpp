#include <physics.h>
#include <config.h>
#include <mathutils.h>
#include <ecs.h>
#include <collision.h>

#include <memory>

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

extern void debug_printf(const char* message, ...);

void resolveCollisionsCallback(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Velocity, Collider
    Vec3 *curPos = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_COLLIDER)]);
    Vec3 *curVel = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Velocity, ARCHETYPE_COLLIDER)]);
    ColliderParams *curCollider = static_cast<ColliderParams*>(componentArrays[COMPONENT_INDEX(Collider, ARCHETYPE_COLLIDER)]);

    while (count)
    {
        // Handle wall collision (since it's universal across states currently)
        handleWalls(*curPos, *curVel, curCollider);
        if (curCollider->floor != nullptr)
        {
            curCollider->floor = handleFloorOnGround(*curPos, *curVel, MAX_STEP_UP, MAX_STEP_DOWN, &curCollider->floorSurfaceType);
        }
        else
        {
            curCollider->floor = handleFloorInAir(*curPos, *curVel, &curCollider->floorSurfaceType);
        }
        if (curCollider->floor != nullptr)
        {
            (*curVel)[0] *= curCollider->frictionDamping;
            (*curVel)[2] *= curCollider->frictionDamping;
        }

        curPos++;
        curVel++;
        curCollider++;
        count--;
    }
}

void physicsTick(void)
{
    // Apply gravity to all objects that are affected by it
    iterateOverEntities(applyGravityCallback, nullptr, ARCHETYPE_GRAVITY, 0);
    // Apply every non-holdable object's velocity to their position
    iterateOverEntities(applyVelocityCallback, nullptr, ARCHETYPE_POSVEL, 0);

    // Resolve intersections
    iterateOverEntities(resolveCollisionsCallback, nullptr, ARCHETYPE_COLLIDER, 0);
}
