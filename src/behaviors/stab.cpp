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
#include <audio.h>

#define ARCHETYPE_STAB_HITBOX (ARCHETYPE_RECTANGLE_HITBOX | Bit_Model)

StabDefinition stab_definitions[] = {
    { // 0
        { // base
            "models/stab_Drill-R", // model_name
            nullptr,      // model
            "Drill-R",     // enemy_name
            100,          // max_health
            25,           // controllable_health
            7.0f,         // move_speed
            EnemyType::Stab, // enemy_type
            144 // head_y_offset
        },
        { // params
            1536.0f, // sight_radius
            150.0f, // follow_distance
            150, // stab_length
            60, // stab_width
            60, // stab_height
            40, // stab_y_offset
            20, // stab_start_pos
            10, // stab_forward_speed
            10, // stab_duration
        }
    }
};

Model* stab_weapon_model = nullptr;

int update_stab_hitbox(const Vec3& stab_pos, const Vec3s& stab_rot, Vec3& stab_vel, StabParams* params, StabState* state, int first = false)
{
    Entity* stab_entity = state->stab_hitbox;
    void* hitbox_components[1 + NUM_COMPONENTS(ARCHETYPE_STAB_HITBOX)];
    getEntityComponents(stab_entity, hitbox_components);

    Vec3& stab_hitbox_pos = *get_component<Bit_Position, Vec3>(hitbox_components, ARCHETYPE_STAB_HITBOX);
    Vec3s& stab_hitbox_rot = *get_component<Bit_Rotation, Vec3s>(hitbox_components, ARCHETYPE_STAB_HITBOX);
    Hitbox& stab_hitbox = *get_component<Bit_Hitbox, Hitbox>(hitbox_components, ARCHETYPE_STAB_HITBOX);

    if (state->recoil_timer == 0)
    {
        if (stab_hitbox.hits != nullptr)
        {
            playSound(Sfx::zap);
            apply_recoil(stab_pos, stab_vel, stab_hitbox.hits->hit, 16.0f);
            state->recoil_timer = 15;
        }
    }
    else
    {
        state->recoil_timer--;
    }

    // If this is not the first frame of the stab, advance its angle by the amount in the params
    if (!first)
    {
        state->stab_timer++;
        if (state->stab_timer > params->stab_duration)
        {
            state->stab_offset += params->stab_forward_speed;
        }
        else
        {
            state->stab_offset -= params->stab_forward_speed;
        }
    }
    else
    {
        state->stab_offset = params->stab_start_pos;
    }
    
    // Translate and rotate the stab hitbox accordingly
    stab_hitbox_pos[0] = stab_pos[0] - state->stab_offset * sinsf(stab_rot[1]);
    stab_hitbox_pos[2] = stab_pos[2] - state->stab_offset * cossf(stab_rot[1]);
    stab_hitbox_pos[1] = stab_pos[1] + params->stab_y_offset;
    stab_hitbox_rot[1] = stab_rot[1];

    // Check if the stab is done and return true if so
    if (state->stab_timer >= 2 * params->stab_duration)
    {
        return true;
    }
    return false;
}

void setup_stab_hitbox(const Vec3& stab_pos, const Vec3s& stab_rot, Vec3& stab_vel, StabState* state, void** hitbox_components, unsigned int hitbox_mask)
{
    Entity* hitbox_entity = get_entity(hitbox_components);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(hitbox_components, ARCHETYPE_STAB_HITBOX);
    Model** model = get_component<Bit_Model, Model*>(hitbox_components, ARCHETYPE_STAB_HITBOX);
    Hitbox& hitbox = *get_component<Bit_Hitbox, Hitbox>(hitbox_components, ARCHETYPE_STAB_HITBOX);

    StabDefinition* definition = static_cast<StabDefinition*>(state->definition);
    StabParams* params = &definition->params;

    if (stab_weapon_model == nullptr)
    {
        stab_weapon_model = load_model("models/drill_Drill-R");
    }
    *model = stab_weapon_model;

    hitbox.mask = hitbox_mask;
    hitbox.radius = params->stab_width;
    hitbox.size_y = params->stab_height;
    hitbox.size_z = params->stab_length;
    hitbox.hits = nullptr;

    state->stab_timer = 0;
    state->stab_hitbox = hitbox_entity;
    rot[0] = 0;
    rot[2] = 0;

    update_stab_hitbox(stab_pos, stab_rot, stab_vel, params, state, true);
}

void stab_callback(void **components, void *data)
{
    // Entity* entity = get_entity(components);
    Entity* stab = get_entity(components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_STAB);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_STAB);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_STAB);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_STAB);
    HealthState& health = *get_component<Bit_Health, HealthState>(components, ARCHETYPE_STAB);

    StabState* state = reinterpret_cast<StabState*>(data);
    StabDefinition* definition = static_cast<StabDefinition*>(state->definition);
    StabParams* params = &definition->params;

    Entity* player = g_PlayerEntity;
    void *player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);

    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);

    float player_dist = approach_target(params->sight_radius, params->follow_distance, definition->base.move_speed, pos, vel, rot, player_pos);

    // Check if the stab died
    if (handle_enemy_hits(stab, collider, health))
    {
        // If it did, delete the hitbox if it exists
        if (state->stab_hitbox != nullptr)
        {
            queue_entity_deletion(state->stab_hitbox);
            state->stab_hitbox = nullptr;
        }
    }
    // If a stab is currently happening, continue it
    else if (state->stab_hitbox != nullptr)
    {
        // If the stab is over, queue the hitbox's deletion
        if (update_stab_hitbox(pos, rot, vel, params, state))
        {
            queue_entity_deletion(state->stab_hitbox);
            state->stab_hitbox = nullptr;
        }
    }
    // Otherwise if the player is close enough to be hit, start a stab
    else if (player_dist < (float)(int)params->stab_length + PLAYER_RADIUS)
    {
        Entity* stab_hitbox = createEntity(ARCHETYPE_STAB_HITBOX);
        void* hitbox_components[NUM_COMPONENTS(ARCHETYPE_STAB_HITBOX) + 1];
        getEntityComponents(stab_hitbox, hitbox_components);

        setup_stab_hitbox(pos, rot, vel, state, hitbox_components, player_hitbox_mask);
    }
}

Entity* create_stab_enemy(float x, float y, float z, int subtype)
{
    Entity* stab = createEntity(ARCHETYPE_STAB);
    StabDefinition& definition = stab_definitions[subtype];

    void* components[1 + NUM_COMPONENTS(ARCHETYPE_STAB)];
    getEntityComponents(stab, components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_STAB);
    BehaviorState* bhv_params = get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_STAB);
    Model** model = get_component<Bit_Model, Model*>(components, ARCHETYPE_STAB);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_STAB);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_STAB);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_STAB);
    HealthState *health = get_component<Bit_Health, HealthState>(components, ARCHETYPE_STAB);
    ControlParams *control_params = get_component<Bit_Control, ControlParams>(components, ARCHETYPE_STAB);

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
    bhv_params->callback = stab_callback;
    StabState* state = reinterpret_cast<StabState*>(bhv_params->data.data());
    state->definition = &definition;

    init_enemy_common(&definition.base, model, health);
    health->health = health->max_health;

    return stab;
}

void delete_stab_enemy(Entity *stab_enemy)
{
    void* components[1 + NUM_COMPONENTS(ARCHETYPE_RAM)];
    getEntityComponents(stab_enemy, components);

    BehaviorState* bhv_params = get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_RAM);
    StabState* state = reinterpret_cast<StabState*>(bhv_params->data.data());

    if (state->stab_hitbox != nullptr)
    {
        queue_entity_deletion(state->stab_hitbox);
    }
}

void on_stab_enter(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    StabDefinition* definition = static_cast<StabDefinition*>(base_state->definition);
    StabState* state = static_cast<StabState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;

    if (stab_weapon_model == nullptr)
    {
        stab_weapon_model = load_model("models/drill_Drill-R");
    }
}

void on_stab_update(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    StabDefinition* definition = static_cast<StabDefinition*>(base_state->definition);
    StabState* state = static_cast<StabState*>(base_state);
    StabParams* params = &definition->params;
    // Entity* player = get_entity(player_components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(player_components, ARCHETYPE_PLAYER);
    
    // If a stab is currently happening, continue it
    if (state->stab_hitbox != nullptr)
    {
        // If the stab is over, queue the hitbox's deletion
        if (update_stab_hitbox(pos, rot, vel, params, state))
        {
            queue_entity_deletion(state->stab_hitbox);
            state->stab_hitbox = nullptr;
        }
    }
    // Otherwise if the player is close enough to be hit, start a stab
    else if (input->buttonsPressed & Z_TRIG)
    {
        Entity* stab_hitbox = createEntity(ARCHETYPE_STAB_HITBOX);
        void* hitbox_components[NUM_COMPONENTS(ARCHETYPE_STAB_HITBOX) + 1];
        getEntityComponents(stab_hitbox, hitbox_components);

        setup_stab_hitbox(pos, rot, vel, state, hitbox_components, enemy_hitbox_mask);
    }
}

void on_stab_leave(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    StabDefinition* definition = static_cast<StabDefinition*>(base_state->definition);
    StabState* state = static_cast<StabState*>(base_state);
    (void)definition;
    (void)input;
    (void)player_components;

    // If a stab is currently happening, end it
    if (state->stab_hitbox != nullptr)
    {
        queue_entity_deletion(state->stab_hitbox);
        state->stab_hitbox = nullptr;
    }
}

ControlHandler stab_control_handler {
    on_stab_enter,
    on_stab_update,
    on_stab_leave
};
