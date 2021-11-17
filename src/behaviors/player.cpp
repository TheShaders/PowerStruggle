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
#include <platform_gfx.h>
#include <files.h>
#include <behaviors.h>
#include <control.h>

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

void processGround(PlayerState *state, InputData *input, UNUSED Vec3 pos, UNUSED Vec3 vel, UNUSED ColliderParams *collider, Vec3s rot, UNUSED GravityParams *gravity, UNUSED AnimState *animState)
{
    float targetSpeed = player_speed_buff * state->controlled_definition->base.move_speed * input->magnitude;
    vel[0] = vel[0] * (1.0f - PLAYER_GROUND_ACCEL_TIME_CONST) + targetSpeed * (PLAYER_GROUND_ACCEL_TIME_CONST) * cossf(input->angle + g_Camera.yaw);
    vel[2] = vel[2] * (1.0f - PLAYER_GROUND_ACCEL_TIME_CONST) - targetSpeed * (PLAYER_GROUND_ACCEL_TIME_CONST) * sinsf(input->angle + g_Camera.yaw);
    rot[1] = atan2s(vel[2], vel[0]);
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

void createPlayer()
{
    // debug_printf("Creating player entity\n");
    createEntitiesCallback(ARCHETYPE_PLAYER, nullptr, 1, createPlayerCallback);
}

extern Model *get_cube_model();

#include <n64_mem.h>

Entity* g_PlayerEntity;

// Extra space for storing the state related to the player's current body
std::array<uint8_t, 16> player_control_state;

void createPlayerCallback(UNUSED size_t count, UNUSED void *arg, void **componentArrays)
{
    // debug_printf("Creating player entity\n");

    // Components: Position, Velocity, Rotation, BehaviorState, Model, AnimState, Gravity
    Entity* entity = get_entity(componentArrays);
    Vec3 *pos = get_component<Bit_Position, Vec3>(componentArrays, ARCHETYPE_PLAYER);
    UNUSED Vec3s *rot = get_component<Bit_Rotation, Vec3s>(componentArrays, ARCHETYPE_PLAYER);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(componentArrays, ARCHETYPE_PLAYER);
    BehaviorState *bhv = get_component<Bit_Behavior, BehaviorState>(componentArrays, ARCHETYPE_PLAYER);
    Model **model = get_component<Bit_Model, Model*>(componentArrays, ARCHETYPE_PLAYER);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(componentArrays, ARCHETYPE_PLAYER);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(componentArrays, ARCHETYPE_PLAYER);
    HealthState *health = get_component<Bit_Health, HealthState>(componentArrays, ARCHETYPE_PLAYER);
    PlayerState *state = reinterpret_cast<PlayerState*>(bhv->data.data());
    g_PlayerEntity = entity;
    // *model = &character_model;
    // debug_printf("Player components\n");
    // debug_printf(" pos %08X\n", pos);
    // debug_printf(" rot %08X\n", rot);
    // debug_printf(" collider %08X\n", collider);
    // debug_printf(" bhvParams %08X\n", bhvParams);
    // debug_printf(" model %08X\n", model);
    
    // Set up gravity
    gravity->accel = -PLAYER_GRAVITY;
    gravity->terminalVelocity = -PLAYER_TERMINAL_VELOCITY;

    // Set up behavior code
    bhv->callback = playerCallback;
    state->playerEntity = get_entity(componentArrays);
    state->state = PSTATE_GROUND;
    state->subState = PGSUBSTATE_WALKING;
    state->stateArg = 0;

    // Set up collider
    collider->radius = PLAYER_RADIUS;
    collider->height = PLAYER_HEIGHT;
    collider->friction_damping = 1.0f;
    collider->floor_surface_type = surface_none;
    collider->mask = player_hitbox_mask;
    
    setAnim(animState, nullptr);

    state->controlled_definition = &shooter_definitions[0];
    state->controlled_handler = control_handlers[(int)EnemyType::Shooter];
    player_control_state.fill(0);
    state->controlled_handler->on_enter(
        state->controlled_definition,
        reinterpret_cast<BaseEnemyState*>(player_control_state.data()),
        &g_PlayerInput,
        componentArrays);

    // Set the player's body to the default (shooter 0)
    init_enemy_common(&state->controlled_definition->base, model, health);
    health->max_health = static_cast<int>(player_health_buff * health->max_health);
    health->health = health->max_health;

    (*pos)[0] = 2229.0f;
    (*pos)[1] = 512.0f;
    (*pos)[2] = 26620.0f;
}

uint32_t last_player_hit_time = 0;
constexpr uint32_t player_iframes = 30;

void take_player_damage(HealthState* health_state, int damage)
{
    if (damage >= health_state->health)
    {
        health_state->health = health_state->max_health;
    }
    else
    {
        health_state->health -= damage;
    }
}

void handle_player_hits(ColliderParams* collider, HealthState* health_state)
{
    Hit* cur_hit = collider->hits;
    while (cur_hit != nullptr)
    {
        if (g_gameTimer - health_state->last_hit_time < player_iframes)
        {
            break;
        }
        take_player_damage(health_state, 10);
        health_state->last_hit_time = g_gameTimer;
        // queue_entity_deletion(cur_hit->entity);
        cur_hit = cur_hit->next;
    }
}

void playerCallback(void **components, void *data)
{
    // Components: Position, Velocity, Rotation, BehaviorState, Model, AnimState, Gravity
    Vec3 *pos = get_component<Bit_Position, Vec3>(components, ARCHETYPE_PLAYER);
    Vec3 *vel = get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_PLAYER);
    Vec3s *rot = get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_PLAYER);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_PLAYER);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_PLAYER);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_PLAYER);
    HealthState *health_state = get_component<Bit_Health, HealthState>(components, ARCHETYPE_PLAYER);
    PlayerState *state = (PlayerState *)data;
    
    // Transition between states if applicable
    stateUpdateCallbacks[state->state](state, &g_PlayerInput, *pos, *vel, collider, *rot, gravity, animState);
    // Process the current state
    stateProcessCallbacks[state->state](state, &g_PlayerInput, *pos, *vel, collider, *rot, gravity, animState);
    handle_player_hits(collider, health_state);

    VEC3_COPY(g_Camera.target, *pos);

    if (g_PlayerInput.buttonsHeld & U_CBUTTONS)
    {
        g_Camera.distance -= 50.0f;
        if (g_Camera.distance <= 50.0f)
        {
            g_Camera.distance = 50.0f;
        }
    }

    if (g_PlayerInput.buttonsHeld & D_CBUTTONS)
    {
        g_Camera.distance += 50.0f;
    }

    if (g_PlayerInput.buttonsPressed & Z_TRIG)
    {
        (*pos)[0] = 2229.0f;
        (*pos)[1] = 512.0f;
        (*pos)[2] = 26620.0f;
    }

    if (state->controlled_handler != nullptr)
    {
        state->controlled_handler->on_update(
            state->controlled_definition,
            reinterpret_cast<BaseEnemyState*>(player_control_state.data()),
            &g_PlayerInput,
            components);
    }

    // debug_printf("Player position: %5.2f %5.2f %5.2f\n", (*pos)[0], (*pos)[1], (*pos)[2]);
}
