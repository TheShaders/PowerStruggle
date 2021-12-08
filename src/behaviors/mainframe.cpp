#include <ecs.h>
#include <behaviors.h>
#include <grid.h>
#include <interaction.h>
#include <misc_scenes.h>
#include <control.h>
#include <player.h>
#include <physics.h>

#define ARCHETYPE_MAINFRAME (ARCHETYPE_CONTROLLABLE)

struct MainframeDefinition : public BaseEnemyDefinition {};
struct MainframeState : public BaseEnemyState {
    int timer = 0;
};

MainframeDefinition mainframe_definition {
    {
        nullptr, // model_name
        nullptr, // model
        "Mainframe", // enemy_name
        100,  // max_health
        25,   // controllable_health
        0.0f, // move_speed
        EnemyType::Mainframe, // enemy_type
        147 // head_y_offset
    }
};

void mainframe_callback(void**, void*) {}

void create_mainframe_callback(UNUSED size_t count, void *arg, void **components)
{
    Vec3& pos_in = *((Vec3*)arg);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_MAINFRAME);
    HealthState& health = *get_component<Bit_Health, HealthState>(components, ARCHETYPE_MAINFRAME);
    ControlParams& control = *get_component<Bit_Control, ControlParams>(components, ARCHETYPE_MAINFRAME);
    BehaviorState& bhv_params = *get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_MAINFRAME);

    pos[0] = pos_in[0];
    pos[1] = pos_in[1];
    pos[2] = pos_in[2];

    health.max_health = 100;
    health.health = 0;

    control.controllable_health = 25;

    // Set up the entity's behavior
    memset(bhv_params.data.data(), 0, sizeof(BehaviorState::data));
    bhv_params.callback = mainframe_callback;
    MainframeState* state = reinterpret_cast<MainframeState*>(bhv_params.data.data());
    state->definition = &mainframe_definition;
    state->timer = 0;
}

void create_mainframe(int tile_x, int tile_y, int tile_z)
{
    Vec3 pos;
    pos[0] = tile_x * tile_size + tile_size / 2;
    pos[1] = tile_y * tile_size;
    pos[2] = tile_z * tile_size + tile_size / 2;

    createEntitiesCallback(ARCHETYPE_MAINFRAME, &pos, 1, create_mainframe_callback);
}

void on_mainframe_enter(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    MainframeDefinition* definition = static_cast<MainframeDefinition*>(base_state->definition);
    MainframeState* state = static_cast<MainframeState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    
    Vec3s& player_rot = *get_component<Bit_Rotation, Vec3s>(player_components, ARCHETYPE_PLAYER);
    Vec3& player_vel = *get_component<Bit_Velocity, Vec3>(player_components, ARCHETYPE_PLAYER);
    GravityParams& player_gravity = *get_component<Bit_Gravity, GravityParams>(player_components, ARCHETYPE_PLAYER);
    
    player_rot[1] = 0;
    state->timer = 0;

    player_vel[0] = player_vel[1] = player_vel[2] = 0;

    player_gravity.accel = 0;
    player_gravity.terminalVelocity = 0;
}

void on_mainframe_update(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    MainframeDefinition* definition = static_cast<MainframeDefinition*>(base_state->definition);
    MainframeState* state = static_cast<MainframeState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;

    if (state->timer > 60)
    {
        start_scene_load(std::make_unique<EndingScene>());
    }
    
    Vec3s& player_rot = *get_component<Bit_Rotation, Vec3s>(player_components, ARCHETYPE_PLAYER);
    player_rot[1] = 0;
    state->timer++;
}

void on_mainframe_leave(BaseEnemyState* base_state, InputData* input, void** player_components)
{
    MainframeDefinition* definition = static_cast<MainframeDefinition*>(base_state->definition);
    MainframeState* state = static_cast<MainframeState*>(base_state);
    (void)definition;
    (void)state;
    (void)input;
    (void)player_components;
}

ControlHandler mainframe_control_handler {
    on_mainframe_enter,
    on_mainframe_update,
    on_mainframe_leave
};
