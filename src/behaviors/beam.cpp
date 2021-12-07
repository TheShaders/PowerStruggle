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

#include <text.h>

#define ARCHETYPE_BEAM_HITBOX (ARCHETYPE_RECTANGLE_HITBOX | Bit_Model)

BeamDefinition beam_definitions[] = {
    { // Zap-E
        { // base
            "models/beam_Beam-E", // model_name
            nullptr,      // model
            "Zap-E",     // enemy_name
            100,          // max_health
            25,           // controllable_health
            7.0f,         // move_speed
            EnemyType::Beam, // enemy_type
            144 // head_y_offset
        },
        { // params
            1536.0f, // sight_radius
            500.0f, // follow_distance
            550.0f, // beam_fire_radius
            1270, // beam_length
            29, // beam_width
            60, // beam_height
            40, // beam_y_offset
            30, // beam_start_lag
            30, // beam_duration
        }
    }
};

Model* beam_weapon_model = nullptr;

int update_beam_hitbox(const Vec3& beam_pos, const Vec3s& beam_rot, UNUSED Vec3& beam_vel, BeamParams* params, BeamState* state, int first = false)
{
    Entity* beam_entity = state->beam_hitbox;
    void* hitbox_components[1 + NUM_COMPONENTS(ARCHETYPE_BEAM_HITBOX)];
    getEntityComponents(beam_entity, hitbox_components);

    Vec3& beam_hitbox_pos = *get_component<Bit_Position, Vec3>(hitbox_components, ARCHETYPE_BEAM_HITBOX);
    Vec3s& beam_hitbox_rot = *get_component<Bit_Rotation, Vec3s>(hitbox_components, ARCHETYPE_BEAM_HITBOX);
    Hitbox& beam_hitbox = *get_component<Bit_Hitbox, Hitbox>(hitbox_components, ARCHETYPE_BEAM_HITBOX);

    if (beam_hitbox.hits != nullptr)
    {
        playSound(Sfx::zap);
        // apply_recoil(beam_pos, beam_vel, beam_hitbox.hits->hit, 16.0f);
    }

    // If this is not the first frame of the beam, advance its timer
    if (!first)
    {
        state->beam_timer--;
    }
    
    // Translate and rotate the beam hitbox accordingly
    beam_hitbox_pos[0] = beam_pos[0] + params->beam_length / 2 * sinsf(beam_rot[1]);
    beam_hitbox_pos[2] = beam_pos[2] + params->beam_length / 2 * cossf(beam_rot[1]);
    beam_hitbox_pos[1] = beam_pos[1] + params->beam_y_offset;
    beam_hitbox_rot[1] = beam_rot[1];

    // Check if the beam is done and return true if so
    if (state->beam_timer == 0)
    {
        return true;
    }
    return false;
}

void setup_beam_hitbox(const Vec3& beam_pos, const Vec3s& beam_rot, Vec3& beam_vel, BeamState* state, void** hitbox_components, unsigned int hitbox_mask)
{
    Entity* hitbox_entity = get_entity(hitbox_components);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(hitbox_components, ARCHETYPE_BEAM_HITBOX);
    Model** model = get_component<Bit_Model, Model*>(hitbox_components, ARCHETYPE_BEAM_HITBOX);
    Hitbox& hitbox = *get_component<Bit_Hitbox, Hitbox>(hitbox_components, ARCHETYPE_BEAM_HITBOX);

    BeamDefinition* definition = static_cast<BeamDefinition*>(state->definition);
    BeamParams* params = &definition->params;

    if (beam_weapon_model == nullptr)
    {
        beam_weapon_model = load_model("models/BeamLaser");
    }
    *model = beam_weapon_model;

    hitbox.mask = hitbox_mask;
    hitbox.radius = params->beam_width;
    hitbox.size_y = params->beam_height;
    hitbox.size_z = params->beam_length;
    hitbox.hits = nullptr;

    state->beam_timer = params->beam_duration;
    state->beam_hitbox = hitbox_entity;
    rot[0] = 0;
    rot[2] = 0;

    update_beam_hitbox(beam_pos, beam_rot, beam_vel, params, state, true);
}

void beam_callback(void **components, void *data)
{
    // Entity* entity = get_entity(components);
    Entity* beam = get_entity(components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_BEAM);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_BEAM);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_BEAM);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_BEAM);
    HealthState& health = *get_component<Bit_Health, HealthState>(components, ARCHETYPE_BEAM);

    BeamState* state = reinterpret_cast<BeamState*>(data);
    BeamDefinition* definition = static_cast<BeamDefinition*>(state->definition);
    BeamParams* params = &definition->params;

    Entity* player = g_PlayerEntity;
    void *player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);

    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);

    float move_speed = definition->base.move_speed;

    // If the timer is nonzero, then we're either attacking or in start lag
    if (state->beam_timer > 0)
    {
        // Cut the movement speed in half
        move_speed *= 0.5f;
    }

    float player_dist = approach_target(params->sight_radius, params->follow_distance, move_speed, pos, vel, rot, player_pos);

    // If the timer is nonzero, then we're either attacking or in start lag
    if (state->beam_timer > 0)
    {
        // Lock rotation
        rot[1] = state->locked_rotation;
    }

    // Check if the beam died
    if (handle_enemy_hits(beam, collider, health))
    {
        // If it did, delete the hitbox if it exists
        if (state->beam_hitbox != nullptr)
        {
            queue_entity_deletion(state->beam_hitbox);
            state->beam_hitbox = nullptr;
        }
    }
    // If a beam is currently happening, continue it
    else if (state->beam_hitbox != nullptr)
    {
        // If the beam is over, queue the hitbox's deletion
        if (update_beam_hitbox(pos, rot, vel, params, state))
        {
            queue_entity_deletion(state->beam_hitbox);
            state->beam_hitbox = nullptr;
        }
    }
    // If there's no hitbox but the beam timer is not at zero, then we're in start lag
    else if (state->beam_timer > 0)
    {
        state->beam_timer--;
        if (state->beam_timer == 0)
        {
            // play fire sound
            Entity* beam_hitbox = createEntity(ARCHETYPE_BEAM_HITBOX);
            void* hitbox_components[NUM_COMPONENTS(ARCHETYPE_BEAM_HITBOX) + 1];
            getEntityComponents(beam_hitbox, hitbox_components);

            setup_beam_hitbox(pos, rot, vel, state, hitbox_components, player_hitbox_mask);
        }
    }
    // Otherwise if the player is close enough to be hit, start a beam
    else if (player_dist < params->beam_fire_radius + PLAYER_RADIUS)
    {
        // play charge sound
        state->beam_timer = params->beam_start_lag;
        state->locked_rotation = rot[1];
    }
}

Entity* create_beam_enemy(float x, float y, float z, int subtype)
{
    Entity* beam = createEntity(ARCHETYPE_BEAM);
    BeamDefinition& definition = beam_definitions[subtype];

    void* components[1 + NUM_COMPONENTS(ARCHETYPE_BEAM)];
    getEntityComponents(beam, components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_BEAM);
    BehaviorState* bhv_params = get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_BEAM);
    Model** model = get_component<Bit_Model, Model*>(components, ARCHETYPE_BEAM);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_BEAM);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_BEAM);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_BEAM);
    HealthState *health = get_component<Bit_Health, HealthState>(components, ARCHETYPE_BEAM);
    ControlParams *control_params = get_component<Bit_Control, ControlParams>(components, ARCHETYPE_BEAM);

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
    bhv_params->callback = beam_callback;
    BeamState* state = reinterpret_cast<BeamState*>(bhv_params->data.data());
    state->definition = &definition;

    init_enemy_common(&definition.base, model, health);
    health->health = health->max_health;

    return beam;
}

void delete_beam_enemy(Entity *beam_enemy)
{

}

void on_beam_enter(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    BeamDefinition* definition = static_cast<BeamDefinition*>(base_state->definition);
    BeamState* state = static_cast<BeamState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;

    if (beam_weapon_model == nullptr)
    {
        beam_weapon_model = load_model("models/BeamLaser");
    }
}

// TODO move this into player state
extern float player_speed_mul;

void on_beam_update(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    BeamDefinition* definition = static_cast<BeamDefinition*>(base_state->definition);
    BeamState* state = static_cast<BeamState*>(base_state);
    BeamParams* params = &definition->params;
    // Entity* player = get_entity(player_components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(player_components, ARCHETYPE_PLAYER);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(player_components, ARCHETYPE_PLAYER);

    // extern uint32_t g_gameTimer;
    // if (g_gameTimer & 1)
    // {
    //     char text[16];
    //     sprintf(text, "%hu", state->beam_timer);
    //     print_text(10, 50, text);
    //     sprintf(text, "%08X", (uintptr_t)&state->beam_timer);
    //     print_text(10, 60, text);
    // }

    // If the timer is nonzero, then we're either attacking or in start lag
    if (state->beam_timer > 0)
    {
        // Lock rotation
        rot[1] = state->locked_rotation;
    }

    // If a beam is currently happening, continue it
    if (state->beam_hitbox != nullptr)
    {
        // If the beam is over, queue the hitbox's deletion
        if (update_beam_hitbox(pos, rot, vel, params, state))
        {
            queue_entity_deletion(state->beam_hitbox);
            state->beam_hitbox = nullptr;
            player_speed_mul = 1.0f;
        }
    }
    // If there's no hitbox but the beam timer is not at zero, then we're in start lag
    else if (state->beam_timer > 0)
    {
        state->beam_timer--;
        if (state->beam_timer == 0)
        {
            // play fire sound
            Entity* beam_hitbox = createEntity(ARCHETYPE_BEAM_HITBOX);
            void* hitbox_components[NUM_COMPONENTS(ARCHETYPE_BEAM_HITBOX) + 1];
            getEntityComponents(beam_hitbox, hitbox_components);

            setup_beam_hitbox(pos, rot, vel, state, hitbox_components, enemy_hitbox_mask);
        }
    }
    // Otherwise if the player is close enough to be hit, start a beam
    else if (input->buttonsPressed & Z_TRIG)
    {
        // play charge sound
        state->beam_timer = params->beam_start_lag;
        state->locked_rotation = rot[1];
        player_speed_mul = 0.5f;
    }
}

void on_beam_leave(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    BeamDefinition* definition = static_cast<BeamDefinition*>(base_state->definition);
    BeamState* state = static_cast<BeamState*>(base_state);
    (void)definition;
    (void)input;
    (void)player_components;

    // If a beam is currently happening, end it
    if (state->beam_hitbox != nullptr)
    {
        queue_entity_deletion(state->beam_hitbox);
        state->beam_hitbox = nullptr;
    }
}

ControlHandler beam_control_handler {
    on_beam_enter,
    on_beam_update,
    on_beam_leave
};
