#include <ecs.h>
#include <player.h>
#include <behaviors.h>
#include <files.h>
#include <mathutils.h>
#include <physics.h>
#include <collision.h>
#include <interaction.h>
#include <control.h>
#include <input.h>

#include <glm/gtx/compatibility.hpp>

#define ARCHETYPE_SHELL (ARCHETYPE_MODEL | Bit_Behavior | Bit_Collider | Bit_Gravity | Bit_Velocity | Bit_Deactivatable)

MortarDefinition mortar_definitions[] = {
    { // Blast-E
        { // base
            "models/mortar_Boom-R", // model_name
            nullptr,      // model
            "Boom-R",    // enemy_name
            100,          // max_health
            25,           // controllable_health
            5.0f,        // move_speed
            EnemyType::Mortar, // enemy_type
        },
        { // params
            "models/Sphere", // mortar_model_name
            nullptr, // mortar_model
            1536.0f, // sight_radius
            512.0f,  // follow_distance
            256,  // mortar_explosion_radius
            256,  // mortar_explosion_height
            512.0f+128.0f,  // mortar_fire_radius
            15,      // mortar_time
            90,      // mortar_rate
        }
    }
};

struct ShellState {
    MortarDefinition* definition;
    uint16_t timer;
    uint8_t hitbox_mask;
};


constexpr float shell_damping = 0.2f;

void shell_behavior_callback(void **components, void *data)
{
    Entity* shell = get_entity(components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_SHELL);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_SHELL);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_SHELL);
    ShellState* mortar_state = reinterpret_cast<ShellState*>(data);

    if (pos[1] < min_height)
    {
        queue_entity_deletion(shell);
        return;
    }

    if (collider.floor_surface_type != surface_none)
    {
        vel[0] = glm::lerp(vel[0], 0.0f, shell_damping);
        vel[2] = glm::lerp(vel[2], 0.0f, shell_damping);
        if (--mortar_state->timer == 0)
        {
            create_explosion(pos, mortar_state->definition->params.mortar_explosion_radius, 10, mortar_state->hitbox_mask);
            queue_entity_deletion(shell);
        }
    }
}

constexpr float shell_forward_vel = 10;
constexpr float shell_y_vel = 25;

void setup_shell(const Vec3& mortar_pos, const Vec3s& mortar_rot, const Vec3& mortar_vel, MortarDefinition* definition, void** shell_components, unsigned int explosion_hitbox_mask)
{
    MortarParams* params = &definition->params;

    Vec3& pos = *get_component<Bit_Position, Vec3>(shell_components, ARCHETYPE_SHELL);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(shell_components, ARCHETYPE_SHELL);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(shell_components, ARCHETYPE_SHELL);
    Model** model = get_component<Bit_Model, Model*>(shell_components, ARCHETYPE_SHELL);
    BehaviorState& behavior = *get_component<Bit_Behavior, BehaviorState>(shell_components, ARCHETYPE_SHELL);
    ActiveState& active_state = *get_component<Bit_Deactivatable, ActiveState>(shell_components, ARCHETYPE_SHELL);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(shell_components, ARCHETYPE_SHELL);
    GravityParams& gravity = *get_component<Bit_Gravity, GravityParams>(shell_components, ARCHETYPE_SHELL);

    *model = params->mortar_model;

    float sin_rot = sinsf(mortar_rot[1]);
    float cos_rot = cossf(mortar_rot[1]);

    pos[0] = mortar_pos[0] - 45 * sin_rot;
    pos[1] = mortar_pos[1] + 100;
    pos[2] = mortar_pos[2] - 45 * cos_rot;

    vel[0] = shell_forward_vel * sin_rot + mortar_vel[0];
    vel[1] = shell_y_vel;
    vel[2] = shell_forward_vel * cos_rot + mortar_vel[2];
    
    rot[0] = 0;
    rot[1] = 0;
    rot[2] = 0;

    behavior.callback = shell_behavior_callback;
    ShellState* shell_state = reinterpret_cast<ShellState*>(behavior.data.data());
    shell_state->definition = definition;
    shell_state->hitbox_mask = explosion_hitbox_mask;
    shell_state->timer = params->mortar_time;

    collider.radius = 32;
    collider.floor_surface_type = surface_none;
    collider.friction_damping = 0.0f;
    collider.height = 64;
    collider.mask = 0;
    collider.hits = nullptr;

    gravity.accel = -PLAYER_GRAVITY;
    gravity.terminalVelocity = -PLAYER_TERMINAL_VELOCITY;

    active_state.delete_on_deactivate = 1;
}

void create_shell_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* mortar_entity = (Entity*)arg;
    void* mortar_components[1 + NUM_COMPONENTS(ARCHETYPE_MORTAR)];
    getEntityComponents(mortar_entity, mortar_components);
    Vec3& mortar_pos = *get_component<Bit_Position, Vec3>(mortar_components, ARCHETYPE_MORTAR);
    Vec3s& mortar_rot = *get_component<Bit_Rotation, Vec3s>(mortar_components, ARCHETYPE_MORTAR);
    Vec3& mortar_vel = *get_component<Bit_Velocity, Vec3>(mortar_components, ARCHETYPE_MORTAR);
    BehaviorState& mortar_bhv = *get_component<Bit_Behavior, BehaviorState>(mortar_components, ARCHETYPE_MORTAR);
    MortarState* state = reinterpret_cast<MortarState*>(mortar_bhv.data.data());
    MortarDefinition* definition = static_cast<MortarDefinition*>(state->definition);
    setup_shell(mortar_pos, mortar_rot, mortar_vel, definition, componentArrays, player_hitbox_mask | enemy_hitbox_mask);
}

void mortar_callback(void **components, void *data)
{
    Entity* mortar = get_entity(components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_MORTAR);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_MORTAR);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_MORTAR);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_MORTAR);
    HealthState& health = *get_component<Bit_Health, HealthState>(components, ARCHETYPE_MORTAR);

    MortarState* state = reinterpret_cast<MortarState*>(data);
    MortarDefinition* definition = static_cast<MortarDefinition*>(state->definition);
    MortarParams* params = &definition->params;

    if (pos[1] < min_height)
    {
        queue_entity_deletion(mortar);
        return;
    }

    Entity* player = g_PlayerEntity;
    void *player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);

    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);


    int fire_mortar = false;
    // If in cooldown, decrement the cooldown timer
    if (state->mortar_timer > 0)
    {
        state->mortar_timer--;
    }
    float player_dist = approach_target(params->sight_radius, params->follow_distance, definition->base.move_speed, pos, vel, rot, player_pos);

    // If the player is close enough place a mortar, then do so
    if (player_dist < params->mortar_fire_radius && state->mortar_timer == 0)
    {
        fire_mortar = true;
    }
    handle_enemy_hits(mortar, collider, health);
    if (fire_mortar && health.health > 0)
    {
        queue_entity_creation(ARCHETYPE_SHELL, mortar, 1, create_shell_callback);
        state->mortar_timer = params->mortar_rate;
        state->mortar_x = pos[0];
        state->mortar_z = pos[2];
        state->run_angle = atan2s(pos[2] - player_pos[2], pos[0] - player_pos[0]);
    }
}

Entity* create_mortar_enemy(float x, float y, float z, int subtype)
{
    Entity* mortar = createEntity(ARCHETYPE_MORTAR);
    MortarDefinition& definition = mortar_definitions[subtype];

    void* components[1 + NUM_COMPONENTS(ARCHETYPE_MORTAR)];
    getEntityComponents(mortar, components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_MORTAR);
    BehaviorState* bhv_params = get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_MORTAR);
    Model** model = get_component<Bit_Model, Model*>(components, ARCHETYPE_MORTAR);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_MORTAR);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_MORTAR);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_MORTAR);
    HealthState *health = get_component<Bit_Health, HealthState>(components, ARCHETYPE_MORTAR);
    ControlParams *control_params = get_component<Bit_Control, ControlParams>(components, ARCHETYPE_MORTAR);

    // Set up gravity
    gravity->accel = -PLAYER_GRAVITY;
    gravity->terminalVelocity = -PLAYER_TERMINAL_VELOCITY;

    // Set up collider
    collider->radius = PLAYER_RADIUS;
    collider->height = PLAYER_HEIGHT;
    collider->friction_damping = 1.0f;
    collider->floor_surface_type = surface_none;
    collider->mask = enemy_hitbox_mask;

    control_params->controllable_health = definition.base.controllable_health;
    
    animState->anim = nullptr;

    // Set the entity's position
    pos[0] = x; pos[1] = y; pos[2] = z;

    // Set up the entity's behavior
    memset(bhv_params->data.data(), 0, sizeof(BehaviorState::data));
    bhv_params->callback = mortar_callback;
    MortarState* state = reinterpret_cast<MortarState*>(bhv_params->data.data());
    state->definition = &definition;

    init_enemy_common(&definition.base, model, health);
    health->health = health->max_health;

    // Load the projectile model if it isn't already loaded
    if (definition.params.mortar_model == nullptr)
    {
        definition.params.mortar_model = load_model(definition.params.mortar_model_name);
    }

    return mortar;
}

void on_mortar_enter(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    MortarDefinition* definition = static_cast<MortarDefinition*>(base_state->definition);
    MortarState* state = static_cast<MortarState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;

    // Load the projectile model if it isn't already loaded
    if (definition->params.mortar_model == nullptr)
    {
        definition->params.mortar_model = load_model(definition->params.mortar_model_name);
    }
}

void create_player_mortar_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* player = (Entity*)arg;
    void* player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);
    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3& player_vel = *get_component<Bit_Velocity, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3s& player_rot = *get_component<Bit_Rotation, Vec3s>(player_components, ARCHETYPE_PLAYER);
    BehaviorState& player_bhv = *get_component<Bit_Behavior, BehaviorState>(player_components, ARCHETYPE_PLAYER);

    PlayerState* state = reinterpret_cast<PlayerState*>(player_bhv.data.data());
    MortarDefinition* definition = static_cast<MortarDefinition*>(state->controlled_state->definition);

    setup_shell(player_pos, player_rot, player_vel, definition, componentArrays, player_hitbox_mask | enemy_hitbox_mask);
}

void on_mortar_update(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    MortarDefinition* definition = static_cast<MortarDefinition*>(base_state->definition);
    MortarState* state = static_cast<MortarState*>(base_state);
    MortarParams* params = &definition->params;
    Entity* player = get_entity(player_components);

    // If in cooldown, decrement the cooldown timer
    if (state->mortar_timer > 0)
    {
        state->mortar_timer--;
    }
    // Otherwise if the player is pressing the fire button, fire
    else if (input->buttonsPressed & Z_TRIG)
    {
        queue_entity_creation(ARCHETYPE_SHELL, player, 1, create_player_mortar_callback);
        state->mortar_timer = params->mortar_rate;
    }
}

void on_mortar_leave(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    MortarDefinition* definition = static_cast<MortarDefinition*>(base_state->definition);
    MortarState* state = static_cast<MortarState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;
}

ControlHandler mortar_control_handler {
    on_mortar_enter,
    on_mortar_update,
    on_mortar_leave
};
