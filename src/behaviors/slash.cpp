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

#define ARCHETYPE_SLASH_HITBOX (ARCHETYPE_RECTANGLE_HITBOX | Bit_Model)

SlasherDefinition slasher_definitions[] = {
    { // 0
        { // base
            "models/Box", // model_name
            nullptr,      // model
            100,          // max_health
            25,           // controllable_health
            7.0f,         // move_speed
            EnemyType::Slash, // enemy_type
        },
        { // params
            1536.0f, // sight_radius
            150.0f, // follow_distance
            200, // slash_length
            40, // slash_width
            40, // slash_height
            30, // slash_y_offset
            degrees_to_angle(120.0f), // slash_angle
            degrees_to_angle(120.0f / 15) // slash_angular_rate
        }
    }
};

Model* slash_weapon_model = nullptr;

int update_slash_hitbox(const Vec3& slasher_pos, const Vec3s& slasher_rot, SlasherParams* params, SlasherState* state, int first)
{
    Entity* slash_hitbox = state->slash_hitbox;
    void* slash_components[1 + NUM_COMPONENTS(ARCHETYPE_SLASH_HITBOX)];
    getEntityComponents(slash_hitbox, slash_components);

    Vec3& slash_pos = *get_component<Bit_Position, Vec3>(slash_components, ARCHETYPE_SLASH_HITBOX);
    Vec3s& slash_rot = *get_component<Bit_Rotation, Vec3s>(slash_components, ARCHETYPE_SLASH_HITBOX);

    // If this is not the first frame of the slash, advance its angle by the amount in the params
    if (!first)
    {
        state->cur_slash_angle += params->slash_angular_rate;
    }
    
    // Translate and rotate the slash hitbox accordingly
    int slash_world_angle = ((int)state->cur_slash_angle - (int)(params->slash_angle / 2)) + slasher_rot[1] - 0x4000;
    slash_pos[0] = slasher_pos[0] + (float)(int)(params->slash_length / 2) * cossf(slash_world_angle);
    slash_pos[2] = slasher_pos[2] - (float)(int)(params->slash_length / 2) * sinsf(slash_world_angle);
    slash_pos[1] = slasher_pos[1] + params->slash_y_offset;
    slash_rot[1] = slash_world_angle;

    // Check if the slash is done and return true if so
    if (state->cur_slash_angle >= params->slash_angle)
    {
        return true;
    }
    return false;
}

void setup_slash_hitbox(const Vec3& slasher_pos, const Vec3s& slasher_rot, SlasherState* state, void** hitbox_components, unsigned int hitbox_mask)
{
    Entity* hitbox_entity = get_entity(hitbox_components);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(hitbox_components, ARCHETYPE_SLASH_HITBOX);
    Model** model = get_component<Bit_Model, Model*>(hitbox_components, ARCHETYPE_SLASH_HITBOX);
    Hitbox& hitbox = *get_component<Bit_Hitbox, Hitbox>(hitbox_components, ARCHETYPE_SLASH_HITBOX);

    SlasherDefinition* definition = static_cast<SlasherDefinition*>(state->definition);
    SlasherParams* params = &definition->params;

    if (slash_weapon_model == nullptr)
    {
        slash_weapon_model = load_model("models/Weapon");
    }
    *model = slash_weapon_model;

    hitbox.mask = hitbox_mask;
    hitbox.radius = params->slash_length;
    hitbox.size_y = params->slash_height;
    hitbox.size_z = params->slash_width;

    state->cur_slash_angle = 0;
    state->slash_hitbox = hitbox_entity;
    rot[0] = 0;
    rot[2] = 0;

    update_slash_hitbox(slasher_pos, slasher_rot, params, state, true);
}

void create_slasher_hitbox_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* slasher_entity = (Entity*)arg;
    void* slasher_components[1 + NUM_COMPONENTS(ARCHETYPE_SLASH)];
    getEntityComponents(slasher_entity, slasher_components);
    Vec3& slasher_pos = *get_component<Bit_Position, Vec3>(slasher_components, ARCHETYPE_SLASH);
    Vec3s& slasher_rot = *get_component<Bit_Rotation, Vec3s>(slasher_components, ARCHETYPE_SLASH);
    BehaviorState& slasher_bhv = *get_component<Bit_Behavior, BehaviorState>(slasher_components, ARCHETYPE_SLASH);

    SlasherState* state = reinterpret_cast<SlasherState*>(slasher_bhv.data.data());

    setup_slash_hitbox(slasher_pos, slasher_rot, state, componentArrays, player_hitbox_mask);
}

void slasher_callback(void **components, void *data)
{
    // Entity* entity = get_entity(components);
    Entity* slasher = get_entity(components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_SLASH);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_SLASH);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_SLASH);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_SLASH);
    HealthState& health = *get_component<Bit_Health, HealthState>(components, ARCHETYPE_SLASH);

    SlasherState* state = reinterpret_cast<SlasherState*>(data);
    SlasherDefinition* definition = static_cast<SlasherDefinition*>(state->definition);
    SlasherParams* params = &definition->params;

    Entity* player = g_PlayerEntity;
    void *player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);

    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);

    float player_dist = approach_target(params->sight_radius, params->follow_distance, definition->base.move_speed, pos, vel, rot, player_pos);

    // Check if the slasher died
    if (handle_enemy_hits(slasher, collider, health))
    {
        // If it did, delete the hitbox if it exists
        if (state->slash_hitbox != nullptr)
        {
            queue_entity_deletion(state->slash_hitbox);
            state->slash_hitbox = nullptr;
        }
    }
    // If a slash is currently happening, continue it
    else if (state->slash_hitbox != nullptr)
    {
        // If the slash is over, queue the hitbox's deletion
        if (update_slash_hitbox(pos, rot, params, state))
        {
            queue_entity_deletion(state->slash_hitbox);
            state->slash_hitbox = nullptr;
        }
    }
    // Otherwise if the player is close enough to be hit, start a slash
    else if (player_dist < (float)(int)params->slash_length + PLAYER_RADIUS)
    {
        queue_entity_creation(ARCHETYPE_SLASH_HITBOX, slasher, 1, create_slasher_hitbox_callback);
    }
}

Entity* create_slasher(float x, float y, float z, int subtype)
{
    Entity* slasher = createEntity(ARCHETYPE_SLASH);
    SlasherDefinition& definition = slasher_definitions[subtype];

    void* components[1 + NUM_COMPONENTS(ARCHETYPE_SLASH)];
    getEntityComponents(slasher, components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_SLASH);
    BehaviorState* bhv_params = get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_SLASH);
    Model** model = get_component<Bit_Model, Model*>(components, ARCHETYPE_SLASH);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_SLASH);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_SLASH);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_SLASH);
    HealthState *health = get_component<Bit_Health, HealthState>(components, ARCHETYPE_SLASH);
    ControlParams *control_params = get_component<Bit_Control, ControlParams>(components, ARCHETYPE_SLASH);

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
    bhv_params->callback = slasher_callback;
    SlasherState* state = reinterpret_cast<SlasherState*>(bhv_params->data.data());
    state->definition = &definition;

    init_enemy_common(&definition.base, model, health);
    health->health = health->max_health;

    return slasher;
}

void create_player_slash_hitbox_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* player = (Entity*)arg;
    void* player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);
    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3s& player_rot = *get_component<Bit_Rotation, Vec3s>(player_components, ARCHETYPE_PLAYER);
    BehaviorState& player_bhv = *get_component<Bit_Behavior, BehaviorState>(player_components, ARCHETYPE_PLAYER);

    PlayerState* state = reinterpret_cast<PlayerState*>(player_bhv.data.data());
    SlasherState* slasher_state = static_cast<SlasherState*>(state->controlled_state);

    setup_slash_hitbox(player_pos, player_rot, slasher_state, componentArrays, enemy_hitbox_mask);
}

void on_slasher_enter(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    SlasherDefinition* definition = static_cast<SlasherDefinition*>(base_state->definition);
    SlasherState* state = static_cast<SlasherState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;

    if (slash_weapon_model == nullptr)
    {
        slash_weapon_model = load_model("models/Weapon");
    }
}

void on_slasher_update(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    SlasherDefinition* definition = static_cast<SlasherDefinition*>(base_state->definition);
    SlasherState* state = static_cast<SlasherState*>(base_state);
    SlasherParams* params = &definition->params;
    Entity* player = get_entity(player_components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(player_components, ARCHETYPE_PLAYER);
    
    // If a slash is currently happening, continue it
    if (state->slash_hitbox != nullptr)
    {
        // If the slash is over, queue the hitbox's deletion
        if (update_slash_hitbox(pos, rot, params, state))
        {
            queue_entity_deletion(state->slash_hitbox);
            state->slash_hitbox = nullptr;
        }
    }
    // Otherwise if the player is close enough to be hit, start a slash
    else if (input->buttonsPressed & Z_TRIG)
    {
        queue_entity_creation(ARCHETYPE_SLASH_HITBOX, player, 1, create_player_slash_hitbox_callback);
    }
}

void on_slasher_leave(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    SlasherDefinition* definition = static_cast<SlasherDefinition*>(base_state->definition);
    SlasherState* state = static_cast<SlasherState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;
}

ControlHandler slash_control_handler {
    on_slasher_enter,
    on_slasher_update,
    on_slasher_leave
};
