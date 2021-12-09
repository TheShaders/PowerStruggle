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

#define ARCHETYPE_BOMB (ARCHETYPE_MODEL | Bit_Behavior  | Bit_Deactivatable)

BombDefinition bomber_definitions[] = {
    { // Herb-E
        { // base
            "models/bomb_Herb-E", // model_name
            nullptr,      // model
            "Herb-E",     // enemy_name
            100,          // max_health
            25,           // controllable_health
            12.0f,        // move_speed
            EnemyType::Bomb, // enemy_type
            81, // head_y_offset
        },
        { // params
            "models/Sphere", // bomb_model_name
            nullptr, // bomb_model
            1536.0f, // sight_radius
            50.0f,  // follow_distance
            256,  // bomb_explosion_radius
            256,  // bomb_explosion_height
            150.0f,  // bomb_place_radius
            60,      // bomb_time
            90,      // bomb_rate
        }
    }
};

struct BombObjectState {
    BombDefinition* definition;
    uint16_t timer;
    uint8_t hitbox_mask;
};

void bomb_behavior_callback(void **components, void *data)
{
    Entity* bomb_entity = get_entity(components);
    Vec3& bomb_position = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_BOMB);
    BombObjectState* bomb_state = reinterpret_cast<BombObjectState*>(data);
    if (--bomb_state->timer == 0)
    {
        create_explosion(bomb_position, bomb_state->definition->params.bomb_explosion_radius, 10, bomb_state->hitbox_mask);
        queue_entity_deletion(bomb_entity);
    }
}

// someone set us up the bomb
void setup_bomb(const Vec3& bomber_pos, BombDefinition* definition, void** bomb_components, unsigned int bomb_hitbox_mask)
{
    BombParams* params = &definition->params;

    Vec3& pos = *get_component<Bit_Position, Vec3>(bomb_components, ARCHETYPE_BOMB);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(bomb_components, ARCHETYPE_BOMB);
    Model** model = get_component<Bit_Model, Model*>(bomb_components, ARCHETYPE_BOMB);
    BehaviorState& behavior = *get_component<Bit_Behavior, BehaviorState>(bomb_components, ARCHETYPE_BOMB);
    ActiveState& active_state = *get_component<Bit_Deactivatable, ActiveState>(bomb_components, ARCHETYPE_BOMB);

    *model = params->bomb_model;

    pos[0] = bomber_pos[0];
    pos[1] = bomber_pos[1];
    pos[2] = bomber_pos[2];
    
    rot[0] = 0;
    rot[1] = 0;
    rot[2] = 0;

    behavior.callback = bomb_behavior_callback;
    BombObjectState* bomb_state = reinterpret_cast<BombObjectState*>(behavior.data.data());
    bomb_state->definition = definition;
    bomb_state->hitbox_mask = bomb_hitbox_mask;
    bomb_state->timer = params->bomb_time;

    active_state.delete_on_deactivate = 1;
}

void create_bomb_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* bomber_entity = (Entity*)arg;
    void* bomber_components[1 + NUM_COMPONENTS(ARCHETYPE_BOMBER)];
    getEntityComponents(bomber_entity, bomber_components);
    Vec3& bomber_pos = *get_component<Bit_Position, Vec3>(bomber_components, ARCHETYPE_BOMBER);
    BehaviorState& bomber_bhv = *get_component<Bit_Behavior, BehaviorState>(bomber_components, ARCHETYPE_BOMBER);
    BombState* state = reinterpret_cast<BombState*>(bomber_bhv.data.data());
    BombDefinition* definition = static_cast<BombDefinition*>(state->definition);
    setup_bomb(bomber_pos, definition, componentArrays, player_hitbox_mask | enemy_hitbox_mask);
}

void bomber_callback(void **components, void *data)
{
    Entity* bomber = get_entity(components);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_BOMBER);
    Vec3& vel = *get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_BOMBER);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_BOMBER);
    ColliderParams& collider = *get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_BOMBER);
    HealthState& health = *get_component<Bit_Health, HealthState>(components, ARCHETYPE_BOMBER);

    BombState* state = reinterpret_cast<BombState*>(data);
    BombDefinition* definition = static_cast<BombDefinition*>(state->definition);
    BombParams* params = &definition->params;

    if (pos[1] < min_height)
    {
        queue_entity_deletion(bomber);
        return;
    }

    Entity* player = g_PlayerEntity;
    void *player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);

    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);


    int make_bomb = false;
    // If in cooldown, decrement the cooldown timer
    if (state->bomb_timer > 0)
    {
        state->bomb_timer--;
        vel[0] = approachFloatLinear(definition->base.move_speed * sinsf(state->run_angle), 0.0f, 1.0f);
        vel[2] = approachFloatLinear(definition->base.move_speed * cossf(state->run_angle), 0.0f, 1.0f);
    }
    else
    {
        float player_dist = approach_target(params->sight_radius, params->follow_distance, definition->base.move_speed, pos, vel, rot, player_pos);

        // Otherwise if the player is close enough place a bomb, then do so
        if (player_dist < params->bomb_place_radius)
        {
            make_bomb = true;
        }
    }
    handle_enemy_hits(bomber, collider, health, definition->base.controllable_health);
    if (make_bomb && health.health > 0)
    {
        createEntitiesCallback(ARCHETYPE_BOMB, bomber, 1, create_bomb_callback);
        state->bomb_timer = params->bomb_rate;
        state->bomb_x = pos[0];
        state->bomb_z = pos[2];
        state->run_angle = atan2s(pos[2] - player_pos[2], pos[0] - player_pos[0]);
    }
}

Entity* create_bomb_enemy(float x, float y, float z, int subtype)
{
    Entity* bomber = createEntity(ARCHETYPE_BOMBER);
    BombDefinition& definition = bomber_definitions[subtype];

    void* components[1 + NUM_COMPONENTS(ARCHETYPE_BOMBER)];
    getEntityComponents(bomber, components);

    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_BOMBER);
    BehaviorState* bhv_params = get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_BOMBER);
    Model** model = get_component<Bit_Model, Model*>(components, ARCHETYPE_BOMBER);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_BOMBER);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_BOMBER);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_BOMBER);
    HealthState *health = get_component<Bit_Health, HealthState>(components, ARCHETYPE_BOMBER);
    ControlParams *control_params = get_component<Bit_Control, ControlParams>(components, ARCHETYPE_BOMBER);

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
    bhv_params->callback = bomber_callback;
    BombState* state = reinterpret_cast<BombState*>(bhv_params->data.data());
    state->definition = &definition;

    init_enemy_common(&definition.base, model, health);
    health->health = health->max_health;

    // Load the projectile model if it isn't already loaded
    if (definition.params.bomb_model == nullptr)
    {
        definition.params.bomb_model = load_model(definition.params.bomb_model_name);
    }

    return bomber;
}

void on_bomber_enter(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    BombDefinition* definition = static_cast<BombDefinition*>(base_state->definition);
    BombState* state = static_cast<BombState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;

    // Load the projectile model if it isn't already loaded
    if (definition->params.bomb_model == nullptr)
    {
        definition->params.bomb_model = load_model(definition->params.bomb_model_name);
    }
}

void create_player_bomb_callback(UNUSED size_t count, void *arg, void **componentArrays)
{
    Entity* player = (Entity*)arg;
    void* player_components[1 + NUM_COMPONENTS(ARCHETYPE_PLAYER)];
    getEntityComponents(player, player_components);
    Vec3& player_pos = *get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    BehaviorState& player_bhv = *get_component<Bit_Behavior, BehaviorState>(player_components, ARCHETYPE_PLAYER);

    PlayerState* state = reinterpret_cast<PlayerState*>(player_bhv.data.data());
    BombDefinition* definition = static_cast<BombDefinition*>(state->controlled_state->definition);

    setup_bomb(player_pos, definition, componentArrays, player_hitbox_mask | enemy_hitbox_mask);
}

void on_bomber_update(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    BombDefinition* definition = static_cast<BombDefinition*>(base_state->definition);
    BombState* state = static_cast<BombState*>(base_state);
    BombParams* params = &definition->params;
    Entity* player = get_entity(player_components);

    // If in cooldown, decrement the cooldown timer
    if (state->bomb_timer > 0)
    {
        state->bomb_timer--;
    }
    // Otherwise if the player is pressing the fire button, fire
    else if (input->buttonsPressed & Z_TRIG)
    {
        createEntitiesCallback(ARCHETYPE_BOMB, player, 1, create_player_bomb_callback);
        state->bomb_timer = params->bomb_rate;
    }
}

void on_bomber_leave(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    BombDefinition* definition = static_cast<BombDefinition*>(base_state->definition);
    BombState* state = static_cast<BombState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;
}

ControlHandler bomb_control_handler {
    on_bomber_enter,
    on_bomber_update,
    on_bomber_leave
};
