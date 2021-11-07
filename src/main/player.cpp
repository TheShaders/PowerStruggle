extern "C" {
#include <debug.h>
}
#include <gfx.h>
#include <mathutils.h>
#include <main.h>
#include <mem.h>
#include <ecs.h>
#include <model.h>
#include <surface_types.h>
#include <interaction.h>
#include <player.h>
#include <collision.h>
#include <physics.h>
#include <input.h>
#include <camera.h>
#include <audio.h>
#include <platform.h>
#include <platform_gfx.h>
#include <files.h>

#include <memory>

void setAnim(AnimState *animState, Animation *newAnim)
{
    newAnim = segmentedToVirtual(newAnim);
    if (animState->anim != newAnim)
    {
        animState->anim = newAnim;
        animState->counter = 0;
        animState->speed = 1 << ANIM_COUNTER_SHIFT;
    }
}

void updateGround(PlayerState *state, InputData *input, UNUSED Vec3 pos, UNUSED Vec3 vel, UNUSED ColliderParams *collider, UNUSED Vec3s rot, UNUSED GravityParams *gravity, UNUSED AnimState *animState)
{
    (void)state;
    (void)input;
}

void processGround(UNUSED PlayerState *state, InputData *input, UNUSED Vec3 pos, UNUSED Vec3 vel, UNUSED ColliderParams *collider, UNUSED Vec3s rot, UNUSED GravityParams *gravity, UNUSED AnimState *animState)
{
    float targetSpeed = MAX_PLAYER_SPEED * input->magnitude;
    vel[0] = vel[0] * (1.0f - PLAYER_GROUND_ACCEL_TIME_CONST) + targetSpeed * (PLAYER_GROUND_ACCEL_TIME_CONST) * cossf(input->angle + g_Camera.yaw);
    vel[2] = vel[2] * (1.0f - PLAYER_GROUND_ACCEL_TIME_CONST) - targetSpeed * (PLAYER_GROUND_ACCEL_TIME_CONST) * sinsf(input->angle + g_Camera.yaw);
}

void updateAir(PlayerState *state, InputData *input, UNUSED Vec3 pos, UNUSED Vec3 vel, UNUSED ColliderParams *collider, UNUSED Vec3s rot, UNUSED GravityParams *gravity, UNUSED AnimState *animState)
{
    (void)state;
    (void)input;
}

void processAir(PlayerState *state, InputData *input, UNUSED Vec3 pos, UNUSED Vec3 vel, UNUSED ColliderParams *collider, UNUSED Vec3s rot, UNUSED GravityParams *gravity, UNUSED AnimState *animState)
{
    (void)state;
    (void)input;
}

// These functions handle state transitions
void (*stateUpdateCallbacks[])(PlayerState *state, InputData *input, Vec3 pos, Vec3 vel, ColliderParams *collider, Vec3s rot, GravityParams *gravity, AnimState *anim) = {
    updateGround, // Ground
    updateAir, // Air
};

// These functions handle the actual state behavioral code
void (*stateProcessCallbacks[])(PlayerState *state, InputData *input, Vec3 pos, Vec3 vel, ColliderParams *collider, Vec3s rot, GravityParams *gravity, AnimState *anim) = {
    processGround, // Ground
    processAir, // Air
};

void createPlayer(PlayerState *state)
{
    // debug_printf("Creating player entity\n");
    createEntitiesCallback(ARCHETYPE_PLAYER, state, 1, createPlayerCallback);
}

extern Model *get_cube_model();

// extern u8 _testmodelSegmentRomStart[];
// extern u8 _testmodelSegmentSize[];

// Model *testmodel;
// extern Animation character_anim;

#include <n64_mem.h>

void createPlayerCallback(UNUSED size_t count, void *arg, void **componentArrays)
{
    // debug_printf("Creating player entity\n");

    // Components: Position, Velocity, Rotation, BehaviorParams, Model, AnimState, Gravity
    Vec3 *pos = get_component<Bit_Position, Vec3>(componentArrays, ARCHETYPE_PLAYER);
    UNUSED Vec3s *rot = get_component<Bit_Rotation, Vec3s>(componentArrays, ARCHETYPE_PLAYER);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(componentArrays, ARCHETYPE_PLAYER);
    BehaviorParams *bhvParams = get_component<Bit_Behavior, BehaviorParams>(componentArrays, ARCHETYPE_PLAYER);
    Model **model = get_component<Bit_Model, Model*>(componentArrays, ARCHETYPE_PLAYER);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(componentArrays, ARCHETYPE_PLAYER);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(componentArrays, ARCHETYPE_PLAYER);
    PlayerState *state = (PlayerState *)arg;
    // *model = &character_model;
    // debug_printf("Player components\n");
    // debug_printf(" pos %08X\n", pos);
    // debug_printf(" rot %08X\n", rot);
    // debug_printf(" collider %08X\n", collider);
    // debug_printf(" bhvParams %08X\n", bhvParams);
    // debug_printf(" model %08X\n", model);
    
    // Set up gravity
    gravity->accel = -PLAYER_GRAVITY;
    gravity->terminalVelocity = -PLAYER_TERMINAL_VELOCITY;

    // Set up behavior code
    bhvParams->callback = playerCallback;
    bhvParams->data = arg;
    state->playerEntity = get_entity(componentArrays);
    state->state = PSTATE_GROUND;
    state->subState = PGSUBSTATE_WALKING;
    state->stateArg = 0;

    // Set up collider
    collider->radius = PLAYER_RADIUS;
    collider->height = PLAYER_HEIGHT;
    // collider->numHeights = PLAYER_WALL_RAYCAST_HEIGHT_COUNT;
    // collider->startOffset = PLAYER_WALL_RAYCAST_OFFSET;
    // collider->ySpacing = PLAYER_WALL_RAYCAST_SPACING;
    collider->friction_damping = 1.0f;
    // collider->floor = nullptr;
    collider->floor_surface_type = surface_none;
    
    setAnim(animState, nullptr);
    *model = load_model("models/Box");

    (*pos)[0] = 2229.0f;
    (*pos)[1] = 512.0f;
    (*pos)[2] = 26620.0f;

    // debug_printf("Set up player entity: 0x%08X\n", state->playerEntity);

    // // Set up animation
    // setAnim(animState, &character_anim);

    // // *model = get_cube_model();
    // testmodel = (Model*)allocRegion((u32)_testmodelSegmentSize, ALLOC_GFX);
    // {
    //     OSMesgQueue queue;
    //     OSMesg msg;
    //     OSIoMesg io_msg;
    //     // Set up the intro segment DMA
    //     io_msg.hdr.pri = OS_MESG_PRI_NORMAL;
    //     io_msg.hdr.retQueue = &queue;
    //     io_msg.dramAddr = testmodel;
    //     io_msg.devAddr = (u32)_testmodelSegmentRomStart;
    //     io_msg.size = (u32)_testmodelSegmentSize;
    //     osCreateMesgQueue(&queue, &msg, 1);
    //     osEPiStartDma(g_romHandle, &io_msg, OS_READ);
    //     osRecvMesg(&queue, nullptr, OS_MESG_BLOCK);
    // }
    // testmodel->adjust_offsets();
    // testmodel->setup_gfx();
    // *model = testmodel;
}

void playerCallback(UNUSED void **components, void *data)
{
    // Components: Position, Velocity, Rotation, BehaviorParams, Model, AnimState, Gravity
    Vec3 *pos = get_component<Bit_Position, Vec3>(components, ARCHETYPE_PLAYER);
    Vec3 *vel = get_component<Bit_Velocity, Vec3>(components, ARCHETYPE_PLAYER);
    Vec3s *rot = get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_PLAYER);
    AnimState *animState = get_component<Bit_AnimState, AnimState>(components, ARCHETYPE_PLAYER);
    ColliderParams *collider = get_component<Bit_Collider, ColliderParams>(components, ARCHETYPE_PLAYER);
    GravityParams *gravity = get_component<Bit_Gravity, GravityParams>(components, ARCHETYPE_PLAYER);
    PlayerState *state = (PlayerState *)data;
    
    // Transition between states if applicable
    stateUpdateCallbacks[state->state](state, &g_PlayerInput, *pos, *vel, collider, *rot, gravity, animState);
    // Process the current state
    stateProcessCallbacks[state->state](state, &g_PlayerInput, *pos, *vel, collider, *rot, gravity, animState);

    VEC3_COPY(g_Camera.target, *pos);

    if (g_PlayerInput.buttonsHeld & U_CBUTTONS)
    {
        g_Camera.distance -= 50.0f;
    }

    if (g_PlayerInput.buttonsHeld & D_CBUTTONS)
    {
        g_Camera.distance += 50.0f;
    }

    if (g_PlayerInput.buttonsPressed & Z_TRIG)
    {
        (*pos)[0] = 2229.0f;
        (*pos)[1] = 512.0f;
        (*pos)[2] = 26620.0f;
    }

    // debug_printf("Player position: %5.2f %5.2f %5.2f\n", (*pos)[0], (*pos)[1], (*pos)[2]);
}
