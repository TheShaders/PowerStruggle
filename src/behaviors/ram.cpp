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

constexpr float ram_stop_time_constant = 0.2f;
#define ARCHETYPE_RAM_HITBOX (ARCHETYPE_RECTANGLE_HITBOX)

RamDefinition ram_definitions[] = {
    { // Till-R
        { // base
            "models/ram_Till-R", // model_name
            nullptr,      // model
            "Till-R",     // enemy_name
            100,          // max_health
            0,           // controllable_health
            7.0f,         // move_speed
            EnemyType::Ram, // enemy_type
            87, // head_y_offset
            53, // head_z_offset
        },
        { // params
            1536.0f, // sight_radius
            700.0f, // follow_distance
            16.0f, // ram_speed
            800.0f, // ram_range
            50, // hitbox_forward_offset
            100, // hitbox_width
            30, // hitbox_length
            100, // hitbox_height
            30, // cooldown_length
        }
    }
};

Model* ram_weapon_model = nullptr;

int update_ram_hitbox(const Vec3& rammer_pos, Vec3s& rammer_rot, Vec3& rammer_vel, ColliderParams& rammer_collider, RamParams* params, RamState* state)
{
    Entity* ram_entity = state->ram_hitbox;
    void* ram_components[1 + NUM_COMPONENTS(ARCHETYPE_RAM_HITBOX)];
    getEntityComponents(ram_entity, ram_components);

    Vec3& ram_pos = *get_component<Bit_Position, Vec3>(ram_components, ARCHETYPE_RAM_HITBOX);
    Vec3s& ram_rot = *get_component<Bit_Rotation, Vec3s>(ram_components, ARCHETYPE_RAM_HITBOX);
    Hitbox& ram_hitbox = *get_component<Bit_Hitbox, Hitbox>(ram_components, ARCHETYPE_RAM_HITBOX);

    float sin_rot = sinsf(state->ram_angle);
    float cos_rot = cossf(state->ram_angle);

    // If the rammer hit a wall, end the ram
    if (rammer_collider.hit_wall)
    {
        rammer_vel[0] = -32.0f * sin_rot;
        rammer_vel[2] = -32.0f * cos_rot;
        return true;
    }
    // If the rammer hit an entity, end the ram
    if (ram_hitbox.hits != nullptr)
    {
        apply_recoil(rammer_pos, rammer_vel, ram_hitbox.hits->hit, 32.0f);
        return true;
    }
    
    // Translate and rotate the ram hitbox accordingly
    ram_pos[0] = rammer_pos[0] + (float)(int)(params->hitbox_forward_offset) * sin_rot;
    ram_pos[2] = rammer_pos[2] + (float)(int)(params->hitbox_forward_offset) * cos_rot;
    ram_pos[1] = rammer_pos[1];
    ram_rot[1] = state->ram_angle;
    rammer_rot[1] = state->ram_angle;
    rammer_vel[0] = params->ram_speed * sin_rot;
    rammer_vel[2] = params->ram_speed * cos_rot;

    return false;
}

void setup_ram_hitbox(const Vec3& rammer_pos, Vec3s& rammer_rot, Vec3& rammer_vel, ColliderParams& rammer_collider, RamState* state, void** hitbox_components, unsigned int hitbox_mask)
{
    Entity* hitbox_entity = get_entity(hitbox_components);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(hitbox_components, ARCHETYPE_RAM_HITBOX);
    Model** model = get_component<Bit_Model, Model*>(hitbox_components, ARCHETYPE_RAM_HITBOX);
    Hitbox& hitbox = *get_component<Bit_Hitbox, Hitbox>(hitbox_components, ARCHETYPE_RAM_HITBOX);

    RamDefinition* definition = static_cast<RamDefinition*>(state->definition);
    RamParams* params = &definition->params;

    if (ram_weapon_model == nullptr)
    {
        ram_weapon_model = load_model("models/Weapon");
    }
    *model = ram_weapon_model;

    hitbox.mask = hitbox_mask;
    hitbox.radius = params->hitbox_width;
    hitbox.size_y = params->hitbox_height;
    hitbox.size_z = params->hitbox_length;
    hitbox.hits = nullptr;

    state->ram_hitbox = hitbox_entity;
    rot[0] = 0;
    rot[2] = 0;

    update_ram_hitbox(rammer_pos, rammer_rot, rammer_vel, rammer_collider, params, state);
}

void create_ram_hitbox_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* rammer_entity = (Entity*)arg;
    void* rammer_components[1 + NUM_COMPONENTS(ARCHETYPE_RAM)];
    getEntityComponents(rammer_entity, rammer_components);
    Vec3& rammer_pos = *get_component<Bit_Position, Vec3>(rammer_components, ARCHETYPE_RAM);
    Vec3& rammer_vel = *get_component<Bit_Velocity, Vec3>(rammer_components, ARCHETYPE_RAM);
    Vec3s& rammer_rot = *get_component<Bit_Rotation, Vec3s>(rammer_components, ARCHETYPE_RAM);
    ColliderParams& rammer_collider = *get_component<Bit_Collider, ColliderParams>(rammer_components, ARCHETYPE_RAM);
    BehaviorState& rammer_bhv = *get_component<Bit_Behavior, BehaviorState>(rammer_components, ARCHETYPE_RAM);

    RamState* state = reinterpret_cast<RamState*>(rammer_bhv.data.data());

    setup_ram_hitbox(rammer_pos, rammer_rot, rammer_vel, rammer_collider, state, componentArrays, player_hitbox_mask);
}

void ram_callback(void **components, void *data)
{
    // Entity* entity = get_entity(components);
    Entity* ram = get_entity(components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_RAM);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_RAM);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_RAM);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_RAM);
    HealthState& health = *get_component<Bit_Health, HealthState>(components, ARCHETYPE_RAM);

    RamState* state = reinterpret_cast<RamState*>(data);
    RamDefinition* definition = static_cast<RamDefinition*>(state->definition);
    RamParams* params = &definition->params;

    if (pos[1] < min_height)
    {
        if (state->ram_hitbox != nullptr)
        {
            queue_entity_deletion(state->ram_hitbox);
            state->ram_hitbox = nullptr;
        }
        queue_entity_deletion(ram);
        return;
    }

    Entity* player = g_PlayerEntity;
    void *player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);

    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);

    if (state->cooldown_timer > 0)
    {
        state->cooldown_timer--;
        vel[0] = glm::lerp(vel[0], 0.0f, ram_stop_time_constant);
        vel[2] = glm::lerp(vel[2], 0.0f, ram_stop_time_constant);
        rot[1] = state->ram_angle;
    }
    else
    {
        float player_dist = approach_target(params->sight_radius, params->follow_distance, definition->base.move_speed, pos, vel, rot, player_pos);

        // Check if the ram died
        if (handle_enemy_hits(ram, collider, health))
        {
            // If it did, delete the hitbox if it exists
            if (state->ram_hitbox != nullptr)
            {
                queue_entity_deletion(state->ram_hitbox);
                state->ram_hitbox = nullptr;
            }
            state->is_ramming = false;
            return;
        }
        // Otherwise if the player is close enough to be hit, start a ram
        if (!state->is_ramming && player_dist < params->ram_range)
        {
            queue_entity_creation(ARCHETYPE_RAM_HITBOX, ram, 1, create_ram_hitbox_callback);
            state->ram_angle = rot[1];
            state->is_ramming = true;
        }
        // If a ram is currently happening, continue it
        if (state->ram_hitbox != nullptr)
        {
            // If the ram is over, queue the hitbox's deletion
            if (update_ram_hitbox(pos, rot, vel, collider, params, state))
            {
                queue_entity_deletion(state->ram_hitbox);
                state->ram_hitbox = nullptr;
                state->is_ramming = false;
                state->cooldown_timer = params->cooldown_length;
            }
        }
    }
}

Entity* create_ram_enemy(float x, float y, float z, int subtype)
{
    Entity* ram = createEntity(ARCHETYPE_RAM);
    RamDefinition& definition = ram_definitions[subtype];

    void* components[1 + NUM_COMPONENTS(ARCHETYPE_RAM)];
    getEntityComponents(ram, components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_RAM);
    BehaviorState* bhv_params = get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_RAM);
    Model** model = get_component<Bit_Model, Model*>(components, ARCHETYPE_RAM);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_RAM);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_RAM);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_RAM);
    HealthState *health = get_component<Bit_Health, HealthState>(components, ARCHETYPE_RAM);
    ControlParams *control_params = get_component<Bit_Control, ControlParams>(components, ARCHETYPE_RAM);

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
    bhv_params->callback = ram_callback;
    RamState* state = reinterpret_cast<RamState*>(bhv_params->data.data());
    state->definition = &definition;

    init_enemy_common(&definition.base, model, health);
    health->health = health->max_health;

    return ram;
}

void delete_ram_enemy(Entity *ram_enemy)
{
    void* components[1 + NUM_COMPONENTS(ARCHETYPE_RAM)];
    getEntityComponents(ram_enemy, components);

    BehaviorState* bhv_params = get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_RAM);
    RamState* state = reinterpret_cast<RamState*>(bhv_params->data.data());

    if (state->ram_hitbox != nullptr)
    {
        queue_entity_deletion(state->ram_hitbox);
    }
}

void create_player_ram_hitbox_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* player = (Entity*)arg;
    void* player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);
    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3& player_vel = *get_component<Bit_Velocity, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3s& player_rot = *get_component<Bit_Rotation, Vec3s>(player_components, ARCHETYPE_PLAYER);
    ColliderParams& player_collider = *get_component<Bit_Collider, ColliderParams>(player_components, ARCHETYPE_PLAYER);
    BehaviorState& player_bhv = *get_component<Bit_Behavior, BehaviorState>(player_components, ARCHETYPE_PLAYER);

    PlayerState* state = reinterpret_cast<PlayerState*>(player_bhv.data.data());
    RamState* ram_state = static_cast<RamState*>(state->controlled_state);

    setup_ram_hitbox(player_pos, player_rot, player_vel, player_collider, ram_state, componentArrays, enemy_hitbox_mask);
}

void on_ram_enter(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    RamDefinition* definition = static_cast<RamDefinition*>(base_state->definition);
    RamState* state = static_cast<RamState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;

    if (ram_weapon_model == nullptr)
    {
        ram_weapon_model = load_model("models/Weapon");
    }
}

void on_ram_update(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    RamDefinition* definition = static_cast<RamDefinition*>(base_state->definition);
    RamState* state = static_cast<RamState*>(base_state);
    RamParams* params = &definition->params;
    Entity* player = get_entity(player_components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(player_components, ARCHETYPE_PLAYER);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(player_components, ARCHETYPE_PLAYER);
    
    if (state->cooldown_timer > 0)
    {
        state->cooldown_timer--;
        vel[0] = glm::lerp(vel[0], 0.0f, ram_stop_time_constant);
        vel[2] = glm::lerp(vel[2], 0.0f, ram_stop_time_constant);
        rot[1] = state->ram_angle;
    }
    else
    {
        // If a ram is currently happening, continue it
        if (state->ram_hitbox != nullptr)
        {
            // If the ram is over, queue the hitbox's deletion
            if (update_ram_hitbox(pos, rot, vel, collider, params, state))
            {
                queue_entity_deletion(state->ram_hitbox);
                state->ram_hitbox = nullptr;
                state->is_ramming = false;
                state->cooldown_timer = params->cooldown_length;
            }
        }
        // Otherwise if the player is close enough to be hit, start a ram
        else if (input->buttonsPressed & Z_TRIG)
        {
            state->is_ramming = true;
            state->ram_angle = rot[1];
            queue_entity_creation(ARCHETYPE_RAM_HITBOX, player, 1, create_player_ram_hitbox_callback);
        }
    }
}

void on_ram_leave(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    RamDefinition* definition = static_cast<RamDefinition*>(base_state->definition);
    RamState* state = static_cast<RamState*>(base_state);
    (void)definition;
    (void)input;
    (void)player_components;

    // If a ram is currently happening, end it
    if (state->ram_hitbox != nullptr)
    {
        queue_entity_deletion(state->ram_hitbox);
        state->ram_hitbox = nullptr;
    }
}

ControlHandler ram_control_handler {
    on_ram_enter,
    on_ram_update,
    on_ram_leave
};
