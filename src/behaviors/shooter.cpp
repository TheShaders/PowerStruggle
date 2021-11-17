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

#define ARCHETYPE_SHOOTER_HITBOX (ARCHETYPE_CYLINDER_HITBOX | Bit_Model | Bit_Velocity)

ShooterDefinition shooter_definitions[] = {
    { // 0
        { // base
            "models/Box", // model_name
            nullptr,      // model
            100,          // max_health
            5.0f,         // move_speed
        },
        { // params
            "models/Sphere",
            nullptr,
            1536.0f, // sight_radius
            512.0f,  // follow_distance
            15.0f,   // shot_speed
            25,      // shot_radius
            50,      // shot_height
            25.0f,   // shot_y_offset
            800.0f,  // fire_radius
            30,      // shot_rate
        }
    }
};

void setup_shot_hitbox(const Vec3& shooter_pos, const Vec3s& shooter_rot, ShooterDefinition* definition, void** shot_components, unsigned int shot_mask)
{
    ShooterParams* params = &definition->params;

    // Entity* hitbox_entity = get_entity(shot_components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(shot_components, ARCHETYPE_SHOOTER_HITBOX);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(shot_components, ARCHETYPE_SHOOTER_HITBOX);
    Model** model = get_component<Bit_Model, Model*>(shot_components, ARCHETYPE_SHOOTER_HITBOX);
    Hitbox& hitbox = *get_component<Bit_Hitbox, Hitbox>(shot_components, ARCHETYPE_SHOOTER_HITBOX);

    *model = params->shot_model;

    hitbox.mask = shot_mask;
    hitbox.radius = params->shot_radius;
    hitbox.size_y = params->shot_height;
    hitbox.size_z = 0;

    vel[0] = sinsf(shooter_rot[1]) * params->shot_speed;
    vel[2] = cossf(shooter_rot[1]) * params->shot_speed;

    pos[0] = shooter_pos[0] + vel[0];
    pos[1] = shooter_pos[1] + params->shot_y_offset;
    pos[2] = shooter_pos[2] + vel[2];
}

void create_shooter_hitbox_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* shooter_entity = (Entity*)arg;
    void* shooter_components[1 + NUM_COMPONENTS(ARCHETYPE_SHOOTER)];
    getEntityComponents(shooter_entity, shooter_components);
    Vec3& shooter_pos = *get_component<Bit_Position, Vec3>(shooter_components, ARCHETYPE_SHOOTER);
    Vec3s& shooter_rot = *get_component<Bit_Rotation, Vec3s>(shooter_components, ARCHETYPE_SHOOTER);
    BehaviorState& shooter_bhv = *get_component<Bit_Behavior, BehaviorState>(shooter_components, ARCHETYPE_SHOOTER);
    ShooterState* state = reinterpret_cast<ShooterState*>(shooter_bhv.data.data());
    ShooterDefinition* definition = static_cast<ShooterDefinition*>(state->definition);
    setup_shot_hitbox(shooter_pos, shooter_rot, definition, componentArrays, player_hitbox_mask);
}

void shooter_callback(void **components, void *data)
{
    Entity* shooter = get_entity(components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_SHOOTER);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_SHOOTER);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_SHOOTER);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_SHOOTER);
    HealthState& health = *get_component<Bit_Health, HealthState>(components, ARCHETYPE_SHOOTER);

    ShooterState* state = reinterpret_cast<ShooterState*>(data);
    ShooterDefinition* definition = static_cast<ShooterDefinition*>(state->definition);
    ShooterParams* params = &definition->params;

    Entity* player = g_PlayerEntity;
    void *player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);

    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);

    float player_dist = approach_target(params->sight_radius, params->follow_distance, definition->base.move_speed, pos, vel, rot, player_pos);

    // If in cooldown, decrement the cooldown timer
    if (state->shot_timer > 0)
    {
        state->shot_timer--;
    }
    // Otherwise if the player is close enough to be shot at, shoot
    else if (player_dist < params->fire_radius)
    {
        queue_entity_creation(ARCHETYPE_SHOOTER_HITBOX, shooter, 1, create_shooter_hitbox_callback);
        state->shot_timer = params->shot_rate;
    }
    handle_enemy_hits(shooter, collider, health);
}

Entity* create_shooter(float x, float y, float z, int subtype)
{
    Entity* shooter = createEntity(ARCHETYPE_SHOOTER);
    ShooterDefinition& definition = shooter_definitions[subtype];

    void* components[1 + NUM_COMPONENTS(ARCHETYPE_SHOOTER)];
    getEntityComponents(shooter, components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_SHOOTER);
    BehaviorState* bhv_params = get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_SHOOTER);
    Model** model = get_component<Bit_Model, Model*>(components, ARCHETYPE_SHOOTER);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_SHOOTER);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_SHOOTER);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_SHOOTER);
    HealthState *health = get_component<Bit_Health, HealthState>(components, ARCHETYPE_SHOOTER);

    // Set up gravity
    gravity->accel = -PLAYER_GRAVITY;
    gravity->terminalVelocity = -PLAYER_TERMINAL_VELOCITY;

    // Set up collider
    collider->radius = PLAYER_RADIUS;
    collider->height = PLAYER_HEIGHT;
    collider->friction_damping = 1.0f;
    collider->floor_surface_type = surface_none;
    collider->mask = enemy_hitbox_mask;
    
    animState->anim = nullptr;

    // Set the entity's position
    pos[0] = x; pos[1] = y; pos[2] = z;

    // Set up the entity's behavior
    memset(bhv_params->data.data(), 0, sizeof(BehaviorState::data));
    bhv_params->callback = shooter_callback;
    ShooterState* state = reinterpret_cast<ShooterState*>(bhv_params->data.data());
    state->definition = &definition;

    init_enemy_common(&definition.base, model, health);
    health->health = health->max_health;

    // Load the projectile model if it isn't already loaded
    if (definition.params.shot_model == nullptr)
    {
        definition.params.shot_model = load_model(definition.params.shot_model_name);
    }

    return shooter;
}

void on_shooter_enter(BaseEnemyDefinition* base_definition, BaseEnemyState* base_state, InputData* input, void** player_components)
{
    ShooterDefinition* definition = static_cast<ShooterDefinition*>(base_definition);
    ShooterState* state = static_cast<ShooterState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;

    // Load the projectile model if it isn't already loaded
    if (definition->params.shot_model == nullptr)
    {
        definition->params.shot_model = load_model(definition->params.shot_model_name);
    }
}

void create_player_shot_hitbox_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* player = (Entity*)arg;
    void* shooter_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, shooter_components);
    Vec3& shooter_pos = *get_component<Bit_Position, Vec3>(shooter_components, ARCHETYPE_PLAYER);
    Vec3s& shooter_rot = *get_component<Bit_Rotation, Vec3s>(shooter_components, ARCHETYPE_PLAYER);
    BehaviorState& shooter_bhv = *get_component<Bit_Behavior, BehaviorState>(shooter_components, ARCHETYPE_PLAYER);

    PlayerState* state = reinterpret_cast<PlayerState*>(shooter_bhv.data.data());
    ShooterDefinition* definition = static_cast<ShooterDefinition*>(state->controlled_definition);

    setup_shot_hitbox(shooter_pos, shooter_rot, definition, componentArrays, enemy_hitbox_mask);
}

void on_shooter_update(BaseEnemyDefinition* base_definition, BaseEnemyState* base_state, InputData* input, void** player_components)
{
    ShooterDefinition* definition = static_cast<ShooterDefinition*>(base_definition);
    ShooterState* state = static_cast<ShooterState*>(base_state);
    ShooterParams* params = &definition->params;
    Entity* player = get_entity(player_components);

    // If in cooldown, decrement the cooldown timer
    if (state->shot_timer > 0)
    {
        state->shot_timer--;
    }
    // Otherwise if the player is pressing the fire button, fire
    else if (input->buttonsPressed & Z_TRIG)
    {
        queue_entity_creation(ARCHETYPE_SHOOTER_HITBOX, player, 1, create_player_shot_hitbox_callback);
        state->shot_timer = params->shot_rate;
    }
}

void on_shooter_leave(BaseEnemyDefinition* base_definition, BaseEnemyState* base_state, InputData* input, void** player_components)
{
    ShooterDefinition* definition = static_cast<ShooterDefinition*>(base_definition);
    ShooterState* state = static_cast<ShooterState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;
}

ControlHandler shooter_control_handler {
    on_shooter_enter,
    on_shooter_update,
    on_shooter_leave
};
