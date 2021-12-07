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

#define ARCHETYPE_MULTISHOT_HITBOX (ARCHETYPE_CYLINDER_HITBOX | Bit_Model | Bit_Velocity | Bit_Deactivatable)

MultishotDefinition multishoter_definitions[] = {
    { // Gas-E
        { // base
            "models/multishot_Fume-R", // model_name
            nullptr,      // model
            "Gas-E",      // enemy_name
            100,          // max_health
            25,           // controllable_health
            5.0f,         // move_speed
            EnemyType::Multishot, // enemy_type
            110 // head_y_offset
        },
        { // params
            "models/Sphere", // shot_model_name
            nullptr, // shot_model
            1536.0f, // sight_radius
            512.0f,  // follow_distance
            8.0f,   // shot_speed
            25,      // shot_radius
            50,      // shot_height
            60.0f,   // shot_y_offset
            800.0f,  // fire_radius
            120,      // shot_rate
        }
    }
};

// HACK
int shot_idx = 0;

void setup_multishot_hitboxes(int count, const Vec3& multishoter_pos, MultishotDefinition* definition, void** shot_components, unsigned int shot_mask)
{
    MultishotParams* params = &definition->params;

    // Entity* hitbox_entity = get_entity(shot_components);
    Vec3* pos = get_component<Bit_Position, Vec3>(shot_components, ARCHETYPE_MULTISHOT_HITBOX);
    Vec3* vel = get_component<Bit_Velocity, Vec3>(shot_components, ARCHETYPE_MULTISHOT_HITBOX);
    Model** model = get_component<Bit_Model, Model*>(shot_components, ARCHETYPE_MULTISHOT_HITBOX);
    Hitbox* hitbox = get_component<Bit_Hitbox, Hitbox>(shot_components, ARCHETYPE_MULTISHOT_HITBOX);
    ActiveState* active_state = get_component<Bit_Deactivatable, ActiveState>(shot_components, ARCHETYPE_MULTISHOT_HITBOX);

    for (int i = 0; i < count; i++)
    {
        *model = params->shot_model;

        hitbox->mask = shot_mask;
        hitbox->radius = params->shot_radius;
        hitbox->size_y = params->shot_height;
        hitbox->size_z = 0;

        if ((shot_idx % 2) == 0)
        {
            if (shot_idx >= 2)
            {
                (*vel)[0] = params->shot_speed;
            }
            else
            {
                (*vel)[0] = -params->shot_speed;
            }
            (*vel)[2] = 0;
        }
        else
        {
            if (shot_idx >= 2)
            {
                (*vel)[2] = params->shot_speed;
            }
            else
            {
                (*vel)[2] = -params->shot_speed;
            }
            (*vel)[0] = 0;
        }

        (*pos)[0] = multishoter_pos[0] + 8 * (*vel)[0];
        (*pos)[1] = multishoter_pos[1] + params->shot_y_offset;
        (*pos)[2] = multishoter_pos[2] + 8 * (*vel)[2];

        active_state->delete_on_deactivate = 1;

        pos++;
        vel++;
        model++;
        hitbox++;
        active_state++;
        shot_idx++;
    }
}

void create_multishot_hitbox_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* multishoter_entity = (Entity*)arg;
    void* multishoter_components[1 + NUM_COMPONENTS(ARCHETYPE_MULTISHOT)];
    getEntityComponents(multishoter_entity, multishoter_components);
    Vec3& multishoter_pos = *get_component<Bit_Position, Vec3>(multishoter_components, ARCHETYPE_MULTISHOT);
    BehaviorState& multishoter_bhv = *get_component<Bit_Behavior, BehaviorState>(multishoter_components, ARCHETYPE_MULTISHOT);
    MultishotState* state = reinterpret_cast<MultishotState*>(multishoter_bhv.data.data());
    MultishotDefinition* definition = static_cast<MultishotDefinition*>(state->definition);
    setup_multishot_hitboxes(count, multishoter_pos, definition, componentArrays, player_hitbox_mask);
}

void multishoter_callback(void **components, void *data)
{
    Entity* enemy = get_entity(components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_MULTISHOT);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_MULTISHOT);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_MULTISHOT);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_MULTISHOT);
    HealthState& health = *get_component<Bit_Health, HealthState>(components, ARCHETYPE_MULTISHOT);

    MultishotState* state = reinterpret_cast<MultishotState*>(data);
    MultishotDefinition* definition = static_cast<MultishotDefinition*>(state->definition);
    MultishotParams* params = &definition->params;

    Entity* player = g_PlayerEntity;
    void *player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);

    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);

    float player_dist = approach_target(params->sight_radius, params->follow_distance, definition->base.move_speed, pos, vel, rot, player_pos);
    rot[1] = 0;

    // If in cooldown, decrement the cooldown timer
    if (state->shot_timer > 0)
    {
        state->shot_timer--;
    }
    // Otherwise if the player is close enough to be shot at, shoot
    else if (player_dist < params->fire_radius)
    {
        shot_idx = 0;
        createEntitiesCallback(ARCHETYPE_MULTISHOT_HITBOX, enemy, 4, create_multishot_hitbox_callback);
        state->shot_timer = params->shot_rate;
    }
    handle_enemy_hits(enemy, collider, health);
}

Entity* create_multishot_enemy(float x, float y, float z, int subtype)
{
    Entity* enemy = createEntity(ARCHETYPE_MULTISHOT);
    MultishotDefinition& definition = multishoter_definitions[subtype];

    void* components[1 + NUM_COMPONENTS(ARCHETYPE_MULTISHOT)];
    getEntityComponents(enemy, components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_MULTISHOT);
    BehaviorState* bhv_params = get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_MULTISHOT);
    Model** model = get_component<Bit_Model, Model*>(components, ARCHETYPE_MULTISHOT);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_MULTISHOT);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_MULTISHOT);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_MULTISHOT);
    HealthState *health = get_component<Bit_Health, HealthState>(components, ARCHETYPE_MULTISHOT);
    ControlParams *control_params = get_component<Bit_Control, ControlParams>(components, ARCHETYPE_MULTISHOT);

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
    bhv_params->callback = multishoter_callback;
    MultishotState* state = reinterpret_cast<MultishotState*>(bhv_params->data.data());
    state->definition = &definition;

    init_enemy_common(&definition.base, model, health);
    health->health = health->max_health;

    // Load the projectile model if it isn't already loaded
    if (definition.params.shot_model == nullptr)
    {
        definition.params.shot_model = load_model(definition.params.shot_model_name);
    }

    return enemy;
}

void on_multishoter_enter(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    MultishotDefinition* definition = static_cast<MultishotDefinition*>(base_state->definition);
    MultishotState* state = static_cast<MultishotState*>(base_state);
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

void create_player_multishot_hitbox_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* player = (Entity*)arg;
    void* player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);
    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    BehaviorState& player_bhv = *get_component<Bit_Behavior, BehaviorState>(player_components, ARCHETYPE_PLAYER);

    PlayerState* state = reinterpret_cast<PlayerState*>(player_bhv.data.data());
    MultishotDefinition* definition = static_cast<MultishotDefinition*>(state->controlled_state->definition);

    setup_multishot_hitboxes(count, player_pos, definition, componentArrays, enemy_hitbox_mask);
}

void on_multishoter_update(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    MultishotDefinition* definition = static_cast<MultishotDefinition*>(base_state->definition);
    MultishotState* state = static_cast<MultishotState*>(base_state);
    MultishotParams* params = &definition->params;
    Entity* player = get_entity(player_components);
    Vec3s& player_rot = *get_component<Bit_Rotation, Vec3s>(player_components, ARCHETYPE_PLAYER);

    player_rot[1] = 0;
    // If in cooldown, decrement the cooldown timer
    if (state->shot_timer > 0)
    {
        state->shot_timer--;
    }
    // Otherwise if the player is pressing the fire button, fire
    else if (input->buttonsPressed & Z_TRIG)
    {
        shot_idx = 0;
        createEntitiesCallback(ARCHETYPE_MULTISHOT_HITBOX, player, 4, create_player_multishot_hitbox_callback);
        state->shot_timer = params->shot_rate;
    }
}

void on_multishoter_leave(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    MultishotDefinition* definition = static_cast<MultishotDefinition*>(base_state->definition);
    MultishotState* state = static_cast<MultishotState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;
}

ControlHandler multishot_control_handler {
    on_multishoter_enter,
    on_multishoter_update,
    on_multishoter_leave
};
