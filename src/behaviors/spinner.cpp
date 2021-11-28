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

#define ARCHETYPE_SPINNER_HITBOX (ARCHETYPE_RECTANGLE_HITBOX | Bit_Model)

SpinnerDefinition spinner_definitions[] = {
    { // Harv-E
        { // base
            "models/spinner_Harv-E", // model_name
            nullptr,      // model
            "Harv-E",     // enemy_name
            100,          // max_health
            25,           // controllable_health
            7.0f,         // move_speed
            EnemyType::Spinner, // enemy_type
        },
        { // params
            "models/blade_Harv-E", // blade_model_name
            nullptr, // blade_model
            1536.0f, // sight_radius
            150.0f, // follow_distance
            30, // blade_y_offset
            0x400, // blade_speed
            200, // blade_length
            40, // blade_height
            40, // blade_width
        }
    }
};

void update_blade_hitbox(const Vec3& spinner_pos, SpinnerParams* params, SpinnerState* state)
{
    Entity* blade = state->blade_entity;
    void* blade_components[1 + NUM_COMPONENTS(ARCHETYPE_SPINNER_HITBOX)];
    getEntityComponents(blade, blade_components);

    Vec3& blade_pos = *get_component<Bit_Position, Vec3>(blade_components, ARCHETYPE_SPINNER_HITBOX);
    Vec3s& blade_rot = *get_component<Bit_Rotation, Vec3s>(blade_components, ARCHETYPE_SPINNER_HITBOX);
    
    // Translate and rotate the slash hitbox accordingly
    blade_rot[1] += params->blade_speed;
    blade_pos[0] = spinner_pos[0] + (float)(int)(params->blade_length / 2) * cossf(blade_rot[1]);
    blade_pos[2] = spinner_pos[2] - (float)(int)(params->blade_length / 2) * sinsf(blade_rot[1]);
    blade_pos[1] = spinner_pos[1] + params->blade_y_offset;
}

void setup_blade_hitbox(const Vec3& spinner_pos, SpinnerState* state, void** hitbox_components, unsigned int hitbox_mask)
{
    Entity* blade = get_entity(hitbox_components);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(hitbox_components, ARCHETYPE_SPINNER_HITBOX);
    Model** model = get_component<Bit_Model, Model*>(hitbox_components, ARCHETYPE_SPINNER_HITBOX);
    Hitbox& hitbox = *get_component<Bit_Hitbox, Hitbox>(hitbox_components, ARCHETYPE_SPINNER_HITBOX);

    SpinnerDefinition* definition = static_cast<SpinnerDefinition*>(state->definition);
    SpinnerParams* params = &definition->params;

    if (params->blade_model == nullptr)
    {
        params->blade_model = load_model(params->blade_model_name);
    }
    *model = params->blade_model;

    hitbox.mask = hitbox_mask;
    hitbox.radius = params->blade_length;
    hitbox.size_y = params->blade_height;
    hitbox.size_z = params->blade_width;

    rot[0] = 0;
    rot[1] = 0;
    rot[2] = 0;

    state->blade_entity = blade;
    update_blade_hitbox(spinner_pos, params, state);

}

void create_spinner_hitbox_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* spinner_entity = (Entity*)arg;
    void* spinner_components[1 + NUM_COMPONENTS(ARCHETYPE_SPINNER)];
    getEntityComponents(spinner_entity, spinner_components);
    Vec3& spinner_pos = *get_component<Bit_Position, Vec3>(spinner_components, ARCHETYPE_SPINNER);
    BehaviorState& spinner_bhv = *get_component<Bit_Behavior, BehaviorState>(spinner_components, ARCHETYPE_SPINNER);

    SpinnerState* state = reinterpret_cast<SpinnerState*>(spinner_bhv.data.data());

    setup_blade_hitbox(spinner_pos, state, componentArrays, player_hitbox_mask);
}

void spinner_callback(void **components, void *data)
{
    // Entity* entity = get_entity(components);
    Entity* spinner = get_entity(components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_SPINNER);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_SPINNER);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_SPINNER);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_SPINNER);
    HealthState& health = *get_component<Bit_Health, HealthState>(components, ARCHETYPE_SPINNER);

    SpinnerState* state = reinterpret_cast<SpinnerState*>(data);
    SpinnerDefinition* definition = static_cast<SpinnerDefinition*>(state->definition);
    SpinnerParams* params = &definition->params;

    Entity* player = g_PlayerEntity;
    void *player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);

    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);

    approach_target(params->sight_radius, params->follow_distance, definition->base.move_speed, pos, vel, rot, player_pos);

    // Check if the spinner died
    if (handle_enemy_hits(spinner, collider, health))
    {
        // If it did, delete the hitbox if it exists
        if (state->blade_entity != nullptr)
        {
            queue_entity_deletion(state->blade_entity);
            state->blade_entity = nullptr;
        }
    }
    else if (state->blade_entity != nullptr)
    {
        update_blade_hitbox(pos, params, state);
    }
}

Entity* create_spinner_enemy(float x, float y, float z, int subtype)
{
    Entity* spinner = createEntity(ARCHETYPE_SPINNER);
    SpinnerDefinition& definition = spinner_definitions[subtype];

    void* components[1 + NUM_COMPONENTS(ARCHETYPE_SPINNER)];
    getEntityComponents(spinner, components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_SPINNER);
    BehaviorState* bhv_params = get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_SPINNER);
    Model** model = get_component<Bit_Model, Model*>(components, ARCHETYPE_SPINNER);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_SPINNER);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_SPINNER);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_SPINNER);
    HealthState *health = get_component<Bit_Health, HealthState>(components, ARCHETYPE_SPINNER);
    ControlParams *control_params = get_component<Bit_Control, ControlParams>(components, ARCHETYPE_SPINNER);

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
    bhv_params->callback = spinner_callback;
    SpinnerState* state = reinterpret_cast<SpinnerState*>(bhv_params->data.data());
    state->definition = &definition;

    init_enemy_common(&definition.base, model, health);
    health->health = health->max_health;
    
    state->blade_entity = createEntity(ARCHETYPE_SPINNER_HITBOX);
    createEntitiesCallback(ARCHETYPE_SPINNER_HITBOX, spinner, 1, create_spinner_hitbox_callback);
    // void* blade_components[NUM_COMPONENTS(ARCHETYPE_SPINNER_HITBOX) + 1];
    // getEntityComponents(state->blade_entity, blade_components);
    // create_spinner_hitbox_callback(1, spinner, blade_components);

    return spinner;
}

void create_player_spinner_blade_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* player = (Entity*)arg;
    void* player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);
    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    BehaviorState& player_bhv = *get_component<Bit_Behavior, BehaviorState>(player_components, ARCHETYPE_PLAYER);

    PlayerState* state = reinterpret_cast<PlayerState*>(player_bhv.data.data());
    SpinnerState* spinner_state = static_cast<SpinnerState*>(state->controlled_state);

    setup_blade_hitbox(player_pos, spinner_state, componentArrays, enemy_hitbox_mask);
}

void on_spinner_enter(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    SpinnerDefinition* definition = static_cast<SpinnerDefinition*>(base_state->definition);
    SpinnerState* state = static_cast<SpinnerState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;

    if (definition->params.blade_model == nullptr)
    {
        definition->params.blade_model = load_model(definition->params.blade_model_name);
    }
    
    queue_entity_creation(ARCHETYPE_SPINNER_HITBOX, get_entity(player_components), 1, create_player_spinner_blade_callback);
}

void on_spinner_update(BaseEnemyState* base_state, UNUSED InputData* input, void** player_components)
{
    SpinnerDefinition* definition = static_cast<SpinnerDefinition*>(base_state->definition);
    SpinnerState* state = static_cast<SpinnerState*>(base_state);
    SpinnerParams* params = &definition->params;

    Vec3& pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    
    if (state->blade_entity != nullptr)
    {
        update_blade_hitbox(pos, params, state);
    }
}

void on_spinner_leave(BaseEnemyState* base_state, UNUSED InputData* input, void** player_components)
{
    SpinnerDefinition* definition = static_cast<SpinnerDefinition*>(base_state->definition);
    SpinnerState* state = static_cast<SpinnerState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;
}

ControlHandler spinner_control_handler {
    on_spinner_enter,
    on_spinner_update,
    on_spinner_leave
};

