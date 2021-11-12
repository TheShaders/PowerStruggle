#include <ecs.h>
#include <player.h>
#include <behaviors.h>
#include <files.h>
#include <mathutils.h>
#include <physics.h>
#include <collision.h>
#include <interaction.h>

ShooterDefinition shooter_definitions[] = {
    {                // 0
        "models/Box", // model_name
        nullptr,       // model
        {              // params
            1536.0f,   // sight_radius
            512.0f,   // follow_distance
            5.0f,      // move_speed
            32.0f,     // shot_speed
            15,        // shot_rate
        }
    }
};

void shooter_callback(void **components, void *data)
{
    // Entity* entity = get_entity(components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_SHOOTER);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_SHOOTER);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_SHOOTER);

    ShooterState* state = reinterpret_cast<ShooterState*>(data);
    ShooterParams* params = state->params;

    Entity* player = g_PlayerEntity;
    void *player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);

    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);

    approach_target(params->sight_radius, params->follow_distance, params->move_speed, pos, vel, player_pos);
}

Entity* create_shooter(int subtype, float x, float y, float z)
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
    *model = load_model("models/Box");

    health->health = health->maxHealth = 200;

    // Set the entity's position
    pos[0] = x; pos[1] = y; pos[2] = z;

    // Set up the entity's behavior
    memset(bhv_params->data.data(), 0, sizeof(BehaviorState::data));
    bhv_params->callback = shooter_callback;
    ShooterState* state = reinterpret_cast<ShooterState*>(bhv_params->data.data());
    state->params = &definition.params;

    // Set up the entity's model
    // Load the model if it isn't already loaded
    if (definition.model == nullptr)
    {
        definition.model = load_model(definition.model_name);
    }
    *model = definition.model;

    return shooter;
}
