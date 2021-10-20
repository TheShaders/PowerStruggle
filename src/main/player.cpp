extern "C" {
#include <debug.h>
}
#include <gfx.h>
#include <mathutils.h>
#include <main.h>
#include <mem.h>
#include <ecs.h>
#include <model.h>
#include <surface_types.h>
#include <interaction.h>
#include <player.h>
#include <collision.h>
#include <physics.h>
#include <input.h>
#include <camera.h>
#include <audio.h>
#include <platform.h>

#include <memory>

void setAnim(AnimState *animState, Animation *newAnim)
{
    newAnim = segmentedToVirtual(newAnim);
    if (animState->anim != newAnim)
    {
        animState->anim = newAnim;
        animState->counter = 0;
        animState->speed = 1 << ANIM_COUNTER_SHIFT;
    }
}

void updateGround(PlayerState *state, InputData *input, UNUSED Vec3 pos, UNUSED Vec3 vel, UNUSED ColliderParams *collider, UNUSED Vec3s rot, UNUSED GravityParams *gravity, UNUSED AnimState *animState)
{
    (void)state;
    (void)input;
}

void processGround(PlayerState *state, InputData *input, UNUSED Vec3 pos, UNUSED Vec3 vel, UNUSED ColliderParams *collider, UNUSED Vec3s rot, UNUSED GravityParams *gravity, UNUSED AnimState *animState)
{
    (void)state;
    (void)input;
}

void updateAir(PlayerState *state, InputData *input, UNUSED Vec3 pos, UNUSED Vec3 vel, UNUSED ColliderParams *collider, UNUSED Vec3s rot, UNUSED GravityParams *gravity, UNUSED AnimState *animState)
{
    (void)state;
    (void)input;
}

void processAir(PlayerState *state, InputData *input, UNUSED Vec3 pos, UNUSED Vec3 vel, UNUSED ColliderParams *collider, UNUSED Vec3s rot, UNUSED GravityParams *gravity, UNUSED AnimState *animState)
{
    (void)state;
    (void)input;
}

// These functions handle state transitions
void (*stateUpdateCallbacks[])(PlayerState *state, InputData *input, Vec3 pos, Vec3 vel, ColliderParams *collider, Vec3s rot, GravityParams *gravity, AnimState *anim) = {
    updateGround, // Ground
    updateAir, // Air
};

// These functions handle the actual state behavioral code
void (*stateProcessCallbacks[])(PlayerState *state, InputData *input, Vec3 pos, Vec3 vel, ColliderParams *collider, Vec3s rot, GravityParams *gravity, AnimState *anim) = {
    processGround, // Ground
    processAir, // Air
};

void createPlayer(PlayerState *state)
{
    // debug_printf("Creating player entity\n");
    createEntitiesCallback(ARCHETYPE_PLAYER, state, 1, createPlayerCallback);
}

void createPlayerCallback(UNUSED size_t count, void *arg, void **componentArrays)
{
    // Components: Position, Velocity, Rotation, BehaviorParams, Model, AnimState, Gravity
    Vec3 *pos = get_component<Bit_Position, Vec3>(componentArrays, ARCHETYPE_PLAYER);
    UNUSED Vec3s *rot = get_component<Bit_Rotation, Vec3s>(componentArrays, ARCHETYPE_PLAYER);
    UNUSED ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(componentArrays, ARCHETYPE_PLAYER);
    UNUSED BehaviorParams *bhvParams = get_component<Bit_Behavior, BehaviorParams>(componentArrays, ARCHETYPE_PLAYER);
    UNUSED Model **model = get_component<Bit_Model, Model*>(componentArrays, ARCHETYPE_PLAYER);
    UNUSED GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(componentArrays, ARCHETYPE_PLAYER);
    PlayerState *state = (PlayerState *)arg;
    // *model = &character_model;
    
    // Set up gravity
    gravity->accel = -PLAYER_GRAVITY;
    gravity->terminalVelocity = -PLAYER_TERMINAL_VELOCITY;

    // Set up behavior code
    bhvParams->callback = playerCallback;
    bhvParams->data = arg;
    state->playerEntity = findEntityFromComponent(ARCHETYPE_PLAYER, Component_Position, pos);
    state->state = PSTATE_AIR;
    state->subState = PASUBSTATE_FALLING;
    state->stateArg = 0;

    // Set up collider
    collider->radius = PLAYER_RADIUS;
    collider->numHeights = PLAYER_WALL_RAYCAST_HEIGHT_COUNT;
    collider->startOffset = PLAYER_WALL_RAYCAST_OFFSET;
    collider->ySpacing = PLAYER_WALL_RAYCAST_SPACING;
    collider->frictionDamping = 1.0f;
    collider->floor = nullptr;
}

void playerCallback(UNUSED void **components, void *data)
{
    // Components: Position, Velocity, Rotation, BehaviorParams, Model, AnimState, Gravity
    Vec3 *pos = get_component<Bit_Position, Vec3>(components, ARCHETYPE_PLAYER);
    Vec3 *vel = get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_PLAYER);
    Vec3s *rot = get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_PLAYER);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_PLAYER);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_PLAYER);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_PLAYER);
    PlayerState *state = (PlayerState *)data;
    
    // Transition between states if applicable
    stateUpdateCallbacks[state->state](state, &g_PlayerInput, *pos, *vel, collider, *rot, gravity, animState);
    // Process the current state
    stateProcessCallbacks[state->state](state, &g_PlayerInput, *pos, *vel, collider, *rot, gravity, animState);

    // debug_printf("Player position: %5.2f %5.2f %5.2f\n", (*pos)[0], (*pos)[1], (*pos)[2]);
}
