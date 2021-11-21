#include <physics.h>
#include <config.h>
#include <mathutils.h>
#include <ecs.h>
#include <collision.h>

#include <n64_mathutils.h>

#include <memory>

extern "C" {
#include <debug.h>
}

#define ARCHETYPE_POSVEL        (Bit_Position | Bit_Velocity)
#define ARCHETYPE_GRAVITY       (Bit_Velocity | Bit_Gravity)
#define ARCHETYPE_VEL_COLLIDER  (Bit_Position | Bit_Velocity | Bit_Collider)
#define ARCHETYPE_DEACTIVATABLE (Bit_Position | Bit_Deactivatable)

void applyGravityImpl(size_t count, Vec3* cur_vel, GravityParams* gravity, ActiveState* active_state)
{
    while (count)
    {
        if (active_state == nullptr || !active_state->deactivated)
        {
            (*cur_vel)[1] += gravity->accel;

            if ((*cur_vel)[1] < gravity->terminalVelocity)
            {
                (*cur_vel)[1] = gravity->terminalVelocity;
            }
            
        }

        if (active_state != nullptr)
        {
            active_state++;
        }
        
        cur_vel++;
        gravity++;
        count--;
    }
}

void applyGravityCallback(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Velocity, Gravity
    Vec3 *cur_vel = get_component<Bit_Velocity, Vec3>(componentArrays, ARCHETYPE_GRAVITY);
    GravityParams *gravity =  get_component<Bit_Gravity, GravityParams>(componentArrays, ARCHETYPE_GRAVITY);
    applyGravityImpl(count, cur_vel, gravity, nullptr);
}

void applyGravityDeactivatableCallback(size_t count, UNUSED void *arg, void **componentArrays)
{
    Vec3 *cur_vel = get_component<Bit_Velocity, Vec3>(componentArrays, ARCHETYPE_GRAVITY | Bit_Deactivatable);
    GravityParams *gravity =  get_component<Bit_Gravity, GravityParams>(componentArrays, ARCHETYPE_GRAVITY | Bit_Deactivatable);
    ActiveState *active_state = get_component<Bit_Deactivatable, ActiveState>(componentArrays, ARCHETYPE_GRAVITY | Bit_Deactivatable);
    applyGravityImpl(count, cur_vel, gravity, active_state);
}

void applyVelocityImpl(size_t count, Vec3* cur_pos, Vec3* cur_vel, ActiveState* active_state)
{
    while (count)
    {
        if (active_state == nullptr || !active_state->deactivated)
        {
            VEC3_ADD(*cur_pos, *cur_pos, *cur_vel);
        }

        if (active_state)
        {
            active_state++;
        }

        cur_pos++;
        cur_vel++;
        count--;
    }
}

void applyVelocityCallback(size_t count, UNUSED void *arg, void **componentArrays)
{
    Vec3 *cur_pos = get_component<Bit_Position, Vec3>(componentArrays, ARCHETYPE_POSVEL);
    Vec3 *cur_vel = get_component<Bit_Velocity, Vec3>(componentArrays, ARCHETYPE_POSVEL);
    applyVelocityImpl(count, cur_pos, cur_vel, nullptr);
}

void applyVelocityDeactivatableCallback(size_t count, UNUSED void *arg, void **componentArrays)
{
    Vec3 *cur_pos = get_component<Bit_Position, Vec3>(componentArrays, ARCHETYPE_POSVEL | Bit_Deactivatable);
    Vec3 *cur_vel = get_component<Bit_Velocity, Vec3>(componentArrays, ARCHETYPE_POSVEL | Bit_Deactivatable);
    ActiveState *active_state = get_component<Bit_Deactivatable, ActiveState>(componentArrays, ARCHETYPE_POSVEL | Bit_Deactivatable);
    applyVelocityImpl(count, cur_pos, cur_vel, active_state);
}

void resolveGridCollisionsCallback(size_t count, void *arg, void **componentArrays)
{
    Grid* grid = static_cast<Grid*>(arg);
    // Components: Position, Velocity, Collider
    Vec3 *curPos = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_VEL_COLLIDER)]);
    Vec3 *curVel = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Velocity, ARCHETYPE_VEL_COLLIDER)]);
    ColliderParams *curCollider = static_cast<ColliderParams*>(componentArrays[COMPONENT_INDEX(Collider, ARCHETYPE_VEL_COLLIDER)]);
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

void update_active_states_callback(size_t count, void *arg, void **componentArrays)
{
    Grid* grid = static_cast<Grid*>(arg);
    Entity** cur_entity = static_cast<Entity**>(componentArrays[0]);
    Vec3 *cur_pos = get_component<Bit_Position, Vec3>(componentArrays, ARCHETYPE_DEACTIVATABLE);
    ActiveState *cur_active_state = get_component<Bit_Deactivatable, ActiveState>(componentArrays, ARCHETYPE_DEACTIVATABLE);

    while (count)
    {
        int chunk_x = round_down_divide<tile_size * chunk_size>(lround((*cur_pos)[0]));
        int chunk_z = round_down_divide<tile_size * chunk_size>(lround((*cur_pos)[2]));

        if (!grid->is_loaded({chunk_x, chunk_z}))
        {
            if (cur_active_state->delete_on_deactivate)
            {
                queue_entity_deletion(*cur_entity);
            }
            else
            {
                cur_active_state->deactivated = 1;
            }
        }
        else
        {
            cur_active_state->deactivated = 0;
        }

        cur_entity++;
        cur_pos++;
        cur_active_state++;
        count--;
    }
}

void physicsTick(Grid& grid)
{
    // Unload any entities outside of loaded chunks of the grid
    iterateOverEntities(update_active_states_callback, &grid, ARCHETYPE_DEACTIVATABLE, 0);
    // Apply gravity to all objects that cannot be deactivated and are affected by it
    iterateOverEntities(applyGravityCallback, nullptr, ARCHETYPE_GRAVITY, Bit_Deactivatable);
    // Apply gravity to all objects that can be deactivated and are affected by it
    iterateOverEntities(applyGravityDeactivatableCallback, nullptr, ARCHETYPE_GRAVITY | Bit_Deactivatable, 0);
    // Apply every non-deactivatable object's velocity to their position
    iterateOverEntities(applyVelocityCallback, nullptr, ARCHETYPE_POSVEL, Bit_Deactivatable);
    // Apply every deactivatable object's velocity to their position
    iterateOverEntities(applyVelocityDeactivatableCallback, nullptr, ARCHETYPE_POSVEL | Bit_Deactivatable, 0);

    // Resolve collisions with the grid
    iterateOverEntities(resolveGridCollisionsCallback, &grid, ARCHETYPE_VEL_COLLIDER, 0);
}
