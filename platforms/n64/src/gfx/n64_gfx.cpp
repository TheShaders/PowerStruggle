#include <memory>

#include <ultra64.h>
#include <PR/sched.h>
#include <PR/gs2dex.h>

#include <n64_gfx.h>
#include <n64_mem.h>
#include <mathutils.h>
#include <n64_mathutils.h>
#include <gfx.h>
#include <mem.h>
#include <model.h>
#include <collision.h>
#include <camera.h>
#include <audio.h>
#include <n64_task_sched.h>
#include <ecs.h>
#include <interaction.h>

alignas(64) std::array<std::array<u16, screen_width * screen_height>, num_frame_buffers> g_frameBuffers;
alignas(64) std::array<u16, screen_width * screen_height> g_depthBuffer;

struct GfxContext g_gfxContexts[num_frame_buffers];

std::array<OSScTask, num_frame_buffers> gfxTasks;

std::array<u64, SP_DRAM_STACK_SIZE64> taskStack;
std::array<u64, output_buff_len> taskOutputBuffer;
std::array<u64, OS_YIELD_DATA_SIZE / sizeof(u64)> taskYieldBuffer;

u8* introSegAddr;
u8 *curGfxPoolPtr;
u8 *curGfxPoolEnd;

Gfx *g_dlist_head;
Gfx *g_gui_dlist_head;

MtxF *g_curMatFPtr;
// The index of the context for the task being constructed
u32 g_curGfxContext;
// The index of the framebuffer being displayed next
u32 g_curFramebuffer;
u16 g_perspNorm;

// C++ brace elision does not play nicely with gbi structs, so double braces are needed
static std::array<std::array<Gfx, 2>, gfx::draw_layers> drawLayerRenderModes1Cycle = {{
    {{ gsDPSetRenderMode(G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2), gsSPEndDisplayList() }}, // background
    {{ gsDPSetRenderMode(G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2), gsSPEndDisplayList() }}, // opa_surf
    {{ gsDPSetRenderMode(G_RM_AA_ZB_TEX_EDGE, G_RM_AA_ZB_TEX_EDGE2), gsSPEndDisplayList() }}, // tex_edge
    {{ gsDPSetRenderMode(G_RM_ZB_OPA_DECAL, G_RM_ZB_OPA_DECAL2), gsSPEndDisplayList() }}, // opa_decal
    {{ gsDPSetRenderMode(G_RM_ZB_XLU_DECAL, G_RM_ZB_XLU_DECAL2), gsSPEndDisplayList() }}, // xlu_decal
    {{ gsDPSetRenderMode(G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF), gsSPEndDisplayList() }}, // xlu_surf
}};

static std::array<Gfx*, gfx::draw_layers> drawLayerStarts;
static std::array<Gfx*, gfx::draw_layers> drawLayerHeads;
static std::array<u32, gfx::draw_layers> drawLayerSlotsLeft;

void initGfx(void)
{
    unsigned int i;

    // Set up the graphics tasks
    for (i = 0; i < num_frame_buffers; i++)
    {
        // Set up OSScTask fields

        // Set up fifo task, configure it to automatically swap buffers after completion
        gfxTasks[i].flags = OS_SC_NEEDS_RSP | OS_SC_NEEDS_RDP | OS_SC_SWAPBUFFER | OS_SC_LAST_TASK;

        gfxTasks[i].framebuffer = g_frameBuffers[i].data();
        gfxTasks[i].msgQ = &g_gfxContexts[i].taskDoneQueue;
        osCreateMesgQueue(&g_gfxContexts[i].taskDoneQueue, &g_gfxContexts[i].taskDoneMesg, 1);

        // Set up OSTask fields

        // Make this a graphics task
        gfxTasks[i].list.t.type = M_GFXTASK;
        gfxTasks[i].list.t.flags = OS_TASK_DP_WAIT;

        // Set up the gfx task boot microcode pointer and size
        gfxTasks[i].list.t.ucode_boot = (u64*) rspbootTextStart;
        gfxTasks[i].list.t.ucode_boot_size = (u32)rspbootTextEnd - (u32)rspbootTextStart;

        // // Set up the gfx task gfx microcode text pointer and size
        gfxTasks[i].list.t.ucode = (u64*) gspF3DEX2_fifoTextStart;
        gfxTasks[i].list.t.ucode_size = (u32)gspF3DEX2_fifoTextEnd - (u32)gspF3DEX2_fifoTextStart;

        // // Set up the gfx task gfx microcode data pointer and size
        gfxTasks[i].list.t.ucode_data = (u64*) gspF3DEX2_fifoDataStart;
        gfxTasks[i].list.t.ucode_data_size = (u32)gspF3DEX2_fifoDataEnd - (u32)gspF3DEX2_fifoDataStart;

        gfxTasks[i].list.t.dram_stack = &taskStack[0];
        gfxTasks[i].list.t.dram_stack_size = SP_DRAM_STACK_SIZE8;

        gfxTasks[i].list.t.output_buff = &taskOutputBuffer[0];
        gfxTasks[i].list.t.output_buff_size = &taskOutputBuffer[output_buff_len];

        gfxTasks[i].list.t.data_ptr = (u64*)&g_gfxContexts[i].dlist_buffer[0];

        gfxTasks[i].list.t.yield_data_ptr = &taskYieldBuffer[0];
        gfxTasks[i].list.t.yield_data_size = OS_YIELD_DATA_SIZE;
    }

    // Send a dummy complete message to the last task, so the first one can run
    osSendMesg(gfxTasks[num_frame_buffers - 1].msgQ, gfxTasks[num_frame_buffers - 1].msg, OS_MESG_BLOCK);

    // Set the gfx context index to 0
    g_curGfxContext = 0;
}

static LookAt *lookAt;
static Lights1 *light;

void setupDrawLayers(void)
{
    unsigned int i;
    for (i = 0; i < gfx::draw_layers; i++)
    {
        // Allocate the room for the draw layer's slots plus a gSPBranchList to the next buffer
        drawLayerHeads[i] = drawLayerStarts[i] = (Gfx*)allocGfx((draw_layer_buffer_len + 1) * sizeof(Gfx));
        drawLayerSlotsLeft[i] = draw_layer_buffer_len;
    }
}

void removeDrawLayerSlot(DrawLayer drawLayer)
{
    unsigned int drawLayerIndex = static_cast<unsigned int>(drawLayer);
    // Remove a slot from the draw layer's current buffer
    // If there are no slots left, allocate a new buffer for this layer
    if (--drawLayerSlotsLeft[drawLayerIndex] == 0)
    {
        // Allocate the draw layer's new buffer
        Gfx* newBuffer = (Gfx*)allocGfx((draw_layer_buffer_len + 1) * sizeof(Gfx));
        // Branch to the new buffer from the old one
        gSPBranchList(drawLayerHeads[drawLayerIndex]++, newBuffer);
        // Update the draw layer's buffer pointer and remaining slot count
        drawLayerHeads[drawLayerIndex] = newBuffer;
        drawLayerSlotsLeft[drawLayerIndex] = draw_layer_buffer_len;
    }
}

void addGfxToDrawLayer(DrawLayer drawLayer, Gfx* toAdd)
{
    // Add the displaylist to the current draw layer's buffer
    gSPDisplayList(drawLayerHeads[static_cast<int>(drawLayer)]++, toAdd);

    // Remove a slot from the current draw layer
    removeDrawLayerSlot(drawLayer);
}

void addMtxToDrawLayer(DrawLayer drawLayer, Mtx* mtx)
{
    // Add the matrix to the current draw layer's buffer
    gSPMatrix(drawLayerHeads[static_cast<int>(drawLayer)]++, mtx, 
	       G_MTX_MODELVIEW|G_MTX_LOAD|G_MTX_NOPUSH);

    // Remove a slot from the current draw layer
    removeDrawLayerSlot(drawLayer);
}

void resetGfxFrame(void)
{
    // Set up the master displaylist head
    g_dlist_head = &g_gfxContexts[g_curGfxContext].dlist_buffer[0];
    g_gui_dlist_head = &g_gfxContexts[g_curGfxContext].gui_dlist_buffer[0];
    curGfxPoolPtr = (u8*)&g_gfxContexts[g_curGfxContext].pool[0];
    curGfxPoolEnd = (u8*)&g_gfxContexts[g_curGfxContext].pool[gfx_pool_size64];

    // Reset the matrix stack index
    g_curMatFPtr = &g_gfxContexts[g_curGfxContext].mtxFStack[0];

    // Allocate the lookAt and light
    lookAt = (LookAt*) allocGfx(sizeof(LookAt));
    light = (Lights1*) allocGfx(sizeof(Lights1));

    // Clear the modelview matrix
    gfx::load_identity();
}

void sendGfxTask(void)
{
    gfxTasks[g_curGfxContext].list.t.data_size = (u32)g_dlist_head - (u32)&g_gfxContexts[g_curGfxContext].dlist_buffer[0];

    // Writeback cache for graphics task data
    osWritebackDCacheAll();

    // Wait for the previous RSP task to complete
    osRecvMesg(gfxTasks[(g_curGfxContext + (num_frame_buffers - 1)) % num_frame_buffers].msgQ, nullptr, OS_MESG_BLOCK);

    // This may be required, but isn't preset in the demo, so if problems arise later on this may solve them
    // if (gfxTasks[(g_curGfxContext + (num_frame_buffers - 1)) % num_frame_buffers].state & OS_SC_NEEDS_RDP)
    // {
    //     // Wait for the task's RDP portion to complete as well
    //     osRecvMesg(gfxTasks[(g_curGfxContext + (num_frame_buffers - 1)) % num_frame_buffers].msgQ, nullptr, OS_MESG_BLOCK);
    // }
    
    // Start the RSP task
    scheduleGfxTask(&gfxTasks[g_curGfxContext]);

    // while (1);
    
    // Switch to the next context
    g_curGfxContext = (g_curGfxContext + 1) % num_frame_buffers;
}

const Gfx rdpInitDL[] = {
    gsDPSetOtherMode(
        G_PM_NPRIMITIVE | G_CYC_1CYCLE | G_TP_PERSP | G_TD_CLAMP | G_TL_TILE | G_TF_BILERP |
            G_TC_FILT | G_CK_NONE | G_CD_DISABLE | G_AD_DISABLE,
        G_AC_NONE | G_ZS_PIXEL | G_RM_OPA_SURF | G_RM_OPA_SURF2),
#ifndef INTERLACED
    gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, screen_width, screen_height),
#endif
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsDPSetPrimColor(0, 0, 0x00, 0xFF, 0x00, 0xFF),
    gsSPEndDisplayList(),
};

const Gfx clearScreenDL[] = {
    gsDPFillRectangle(0, 0, screen_width - 1, screen_height - 1),
    gsDPPipeSync(),

    // gsDPSetFillColor(GPACK_RGBA5551(0x3F, 0x3F, 0x3F, 1) << 16 | GPACK_RGBA5551(0x3F, 0x3F, 0x3F, 1)),
    // gsDPFillRectangle(10, 10, screen_width - 10 - 1, screen_height - 10 - 1),
    // gsDPPipeSync(),
    gsSPEndDisplayList(),
};

const Gfx clearDepthBuffer[] = {
	gsDPSetCycleType(G_CYC_FILL),
    gsDPSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, screen_width, g_depthBuffer.data()),

    gsDPSetFillColor(GPACK_ZDZ(G_MAXFBZ, 0) << 16 | GPACK_ZDZ(G_MAXFBZ, 0)),
    gsDPFillRectangle(0, 0, screen_width - 1, screen_height - 1),
    gsDPPipeSync(),
    gsDPSetDepthImage(g_depthBuffer.data()),
    gsSPEndDisplayList(),
};

void rspUcodeLoadInit(void)
{
    gSPLoadGeometryMode(g_dlist_head++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK | G_LIGHTING);
    gSPTexture(g_dlist_head++, 0, 0, 0, 0, G_OFF);
    
    gSPSetLights1(g_dlist_head++, (*light));
    gSPLookAt(g_dlist_head++, lookAt);
}

u32 fillColor = GPACK_RGBA5551(0, 61, 8, 1) << 16 | GPACK_RGBA5551(0, 61, 8, 1);

void startFrame(void)
{
    int segIndex;
    resetGfxFrame();

    gSPSegment(g_dlist_head++, 0x00, 0x00000000);
    gSPSegment(g_dlist_head++, BUFFER_SEGMENT, g_frameBuffers[g_curGfxContext].data());

    for (segIndex = 2; segIndex < NUM_SEGMENTS; segIndex++)
    {
        uintptr_t segOffset = (uintptr_t) getSegment(segIndex);
        if (segOffset != 0)
        {
            gSPSegment(g_dlist_head++, segIndex, segOffset);
        }
    }

#ifdef INTERLACED
    if (osViGetCurrentField())
    {
        gDPSetScissor(g_dlistHead++, G_SC_EVEN_INTERLACE, 0, 0, screen_width, screen_height);
    }
    else
    {
        gDPSetScissor(g_dlistHead++, G_SC_ODD_INTERLACE, 0, 0, screen_width, screen_height);
    }
#endif
    gSPDisplayList(g_dlist_head++, rdpInitDL);
    gSPDisplayList(g_dlist_head++, clearDepthBuffer);
    
    gDPSetCycleType(g_dlist_head++, G_CYC_FILL);
    gDPSetColorImage(g_dlist_head++, G_IM_FMT_RGBA, G_IM_SIZ_16b, screen_width, BUFFER_SEGMENT << 24);
    gDPSetFillColor(g_dlist_head++, fillColor);
    gSPDisplayList(g_dlist_head++, clearScreenDL);
    
    gDPSetCycleType(g_dlist_head++, G_CYC_1CYCLE);
    
    rspUcodeLoadInit();

    setupDrawLayers();
}

// Draws a model with 1 joint and no posing
// Does not inherit from or affect the matrix stack
void drawTileModel(Model *toDraw, Mtx* curMtx)
{
    int cur_material = -1;

    if (toDraw == nullptr) return;

    // Draw the model's singular joint
    const Joint& joint_to_draw = toDraw->joints[0];

    // Draw the joint's layers
    for (size_t cur_layer = 0; cur_layer < gfx::draw_layers; cur_layer++)
    {
        const JointMeshLayer *curJointLayer = &joint_to_draw.layers[cur_layer];

        // Don't bother adding a matrix load if there's nothing to draw on this layer for this joint
        if (curJointLayer->num_draws == 0)
        {
            continue;
        }

        addMtxToDrawLayer(static_cast<DrawLayer>(cur_layer), curMtx);
        
        // Draw the layer
        for (size_t draw_idx = 0; draw_idx < curJointLayer->num_draws; draw_idx++)
        {
            auto& cur_draw = curJointLayer->draws[draw_idx];
            // Check if we've changed materials; if so load the new material
            if (cur_draw.material_index != cur_material)
            {
                cur_material = cur_draw.material_index;
                addGfxToDrawLayer(static_cast<DrawLayer>(cur_layer), toDraw->materials[cur_material]->gfx);
            }
            // Check if this draw has any groups and skip it if it doesn't
            if (cur_draw.num_groups != 0)
            {
                addGfxToDrawLayer(static_cast<DrawLayer>(cur_layer), cur_draw.gfx);
            }
        }
    }
}

// Draws a model (TODO add posing)
void drawModel(Model *toDraw, Animation *anim, u32 frame)
{
    int jointIndex;
    Joint *joints, *curJoint;
    JointTable *curJointTable = nullptr;
    // Gfx *callbackReturn;
    std::unique_ptr<MtxF[]> jointMatrices;
    u32 numFrames = 0;
    int cur_material = -1;

    if (toDraw == nullptr) return;

    // Allocate space for this model's joint matrices
    // Use new instead of make_unique to avoid unnecessary initialization
    jointMatrices = std::unique_ptr<MtxF[]>(new MtxF[toDraw->num_joints]);

    // Draw the model's joints
    curJoint = joints = toDraw->joints;
    if (anim != nullptr)
    {
        numFrames = anim->frameCount;
        curJointTable = anim->jointTables;
    }
    for (jointIndex = 0; jointIndex < toDraw->num_joints; jointIndex++)
    {
        // If the joint has a parent, load the parent's matrix before transforming
        if (curJoint->parent != 0xFF)
        {
            gfx::push_load_mat(&jointMatrices[curJoint->parent]);
        }
        // Otherwise, use the current matrix on the stack
        else
        {
            gfx::push_mat();
        }

        gfx::apply_translation_affine(curJoint->posX, curJoint->posY, curJoint->posZ);

        if (anim != nullptr)
        {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            u32 hasCurrentTransformComponent = 0;
            s16 *curJointChannel = curJointTable->channels;
            s16 rx = 0; s16 ry = 0; s16 rz = 0;
            curJointChannel += frame;

            // TODO fix this, horrible for cache performance.
            // Data should not be split up into channels, but instead be one contiguous stream of data.
            // Each frame's data should be one span in the total data.
            // That would be much better for locality.
            if (curJointTable->flags & CHANNEL_POS_X)
            {
                x = *curJointChannel;
                curJointChannel += numFrames; 
                hasCurrentTransformComponent = 1;
            }
            if (curJointTable->flags & CHANNEL_POS_Y)
            {
                y = *curJointChannel;
                curJointChannel += numFrames;
                hasCurrentTransformComponent = 1;
            }
            if (curJointTable->flags & CHANNEL_POS_Z)
            {
                z = *curJointChannel;
                curJointChannel += numFrames;
                hasCurrentTransformComponent = 1;
            }
            if (hasCurrentTransformComponent)
            {
                gfx::apply_translation_affine(x, y, z);
            }
            
            hasCurrentTransformComponent = 0;

            if (curJointTable->flags & CHANNEL_ROT_X)
            {
                rx = *curJointChannel;
                curJointChannel += numFrames;
                hasCurrentTransformComponent = 1;
            }
            if (curJointTable->flags & CHANNEL_ROT_Y)
            {
                ry = *curJointChannel;
                curJointChannel += numFrames;
                hasCurrentTransformComponent = 1;
            }
            if (curJointTable->flags & CHANNEL_ROT_Z)
            {
                rz = *curJointChannel;
                curJointChannel += numFrames;
                hasCurrentTransformComponent = 1;
            }

            if (hasCurrentTransformComponent)
            {
                gfx::rotate_euler_xyz(rx, ry, rz);
            }

            hasCurrentTransformComponent = 0;

            x = y = z = 1.0f;

            if (curJointTable->flags & CHANNEL_SCALE_X)
            {
                x = *(u16*)curJointChannel / 256.0f;
                curJointChannel += numFrames;
                hasCurrentTransformComponent = 1;
            }
            if (curJointTable->flags & CHANNEL_SCALE_Y)
            {
                y = *(u16*)curJointChannel / 256.0f;
                curJointChannel += numFrames;
                hasCurrentTransformComponent = 1;
            }
            if (curJointTable->flags & CHANNEL_SCALE_Z)
            {
                z = *(u16*)curJointChannel / 256.0f;
                curJointChannel += numFrames;
                hasCurrentTransformComponent = 1;
            }
            if (hasCurrentTransformComponent)
            {
                gfx::apply_scale_affine(x, y, z);
            }
            
        }
        
        Mtx* curMtx = (Mtx*)allocGfx(sizeof(Mtx));
        guMtxF2L(*g_curMatFPtr, curMtx);

        // Draw the joint's layers
        for (size_t cur_layer = 0; cur_layer < gfx::draw_layers; cur_layer++)
        {
            JointMeshLayer *curJointLayer = &curJoint->layers[cur_layer];

            // Don't bother adding a matrix load if there's nothing to draw on this layer for this joint
            if (curJointLayer->num_draws == 0)
            {
                continue;
            }

            addMtxToDrawLayer(static_cast<DrawLayer>(cur_layer), curMtx);
            
            // Check if this joint has a before drawn callback, and if so call it
            // if (curJoint->beforeCb)
            // {
            //     callbackReturn = curJoint->beforeCb(curJoint, curJointLayer);
            //     // If the callback returned a displaylist, draw it
            //     if (callbackReturn)
            //     {
            //         gSPDisplayList(g_dlist_head++, callbackReturn);
            //     }
            // }
            
            // Draw the layer
            for (size_t draw_idx = 0; draw_idx < curJointLayer->num_draws; draw_idx++)
            {
                auto& cur_draw = curJointLayer->draws[draw_idx];
                // Check if we've changed materials; if so load the new material
                if (cur_draw.material_index != cur_material)
                {
                    cur_material = cur_draw.material_index;
                    addGfxToDrawLayer(static_cast<DrawLayer>(cur_layer), toDraw->materials[cur_material]->gfx);
                }
                // Check if this draw has any groups and skip it if it doesn't
                if (cur_draw.num_groups != 0)
                {
                    addGfxToDrawLayer(static_cast<DrawLayer>(cur_layer), cur_draw.gfx);
                }
            }

            // Check if this joint has an after drawn callback, and if so call it
            // if (curJoint->afterCb)
            // {
            //     callbackReturn = curJoint->afterCb(curJoint, curJointLayer);
            //     // If the callback returned a displaylist, draw it
            //     if (callbackReturn)
            //     {
            //         gSPDisplayList(g_dlist_head++, callbackReturn);
            //     }
            // }
        }

        // Save this joint's matrix in case other joints are children of this one
        gfx::save_mat(&jointMatrices[jointIndex]);

        // Pop this joint's matrix off the stack
        gfx::pop_mat();

        curJoint++;
        if (anim)
            curJointTable++;
    }
}

// Gfx* gfxSetEnvColor(Joint* joint, UNUSED JointMeshLayer *layer)
// {
//     if (joint->index == 0)
//     {
//         gDPSetEnvColor(g_dlist_head++, 255, 0, 0, 0);
//     }
//     else if (joint->index == 1)
//     {
//         gDPSetEnvColor(g_dlist_head++, 0, 255, 0, 0);
//     }
//     return nullptr;
// }

void drawGfx(DrawLayer layer, Gfx* toDraw)
{
    Mtx* curMtx = (Mtx*)allocGfx(sizeof(Mtx));
    guMtxF2L(*g_curMatFPtr, curMtx);

    addMtxToDrawLayer(layer, curMtx);
    addGfxToDrawLayer(layer, toDraw);
}

#define USE_TRIS_FOR_AABB

void drawAABB(DrawLayer layer, AABB *toDraw, u32 color)
{
    int i;
    Vtx *verts = (Vtx*)allocGfx(sizeof(Vtx) * 8);
    Mtx *curMtx = (Mtx*)allocGfx(sizeof(Mtx));
    
#ifdef USE_TRIS_FOR_AABB
    Gfx *dlist = (Gfx*)allocGfx(sizeof(Gfx) * 11);
#else
    Gfx *dlist = (Gfx*)allocGfx(sizeof(Gfx) * 20);
#endif

    addGfxToDrawLayer(layer, dlist);

    for (i = 0; i < 8; i++)
    {
        *(u32*)(&verts[i].v.cn[0]) = color;
    }
    verts[0].v.ob[0] = static_cast<s16>(toDraw->min[0]);
    verts[0].v.ob[1] = static_cast<s16>(toDraw->min[1]);
    verts[0].v.ob[2] = static_cast<s16>(toDraw->min[2]);
    
    verts[1].v.ob[0] = static_cast<s16>(toDraw->min[0]);
    verts[1].v.ob[1] = static_cast<s16>(toDraw->min[1]);
    verts[1].v.ob[2] = static_cast<s16>(toDraw->max[2]);
    
    verts[2].v.ob[0] = static_cast<s16>(toDraw->min[0]);
    verts[2].v.ob[1] = static_cast<s16>(toDraw->max[1]);
    verts[2].v.ob[2] = static_cast<s16>(toDraw->min[2]);
    
    verts[3].v.ob[0] = static_cast<s16>(toDraw->min[0]);
    verts[3].v.ob[1] = static_cast<s16>(toDraw->max[1]);
    verts[3].v.ob[2] = static_cast<s16>(toDraw->max[2]);
    
    verts[4].v.ob[0] = static_cast<s16>(toDraw->max[0]);
    verts[4].v.ob[1] = static_cast<s16>(toDraw->min[1]);
    verts[4].v.ob[2] = static_cast<s16>(toDraw->min[2]);
    
    verts[5].v.ob[0] = static_cast<s16>(toDraw->max[0]);
    verts[5].v.ob[1] = static_cast<s16>(toDraw->min[1]);
    verts[5].v.ob[2] = static_cast<s16>(toDraw->max[2]);
    
    verts[6].v.ob[0] = static_cast<s16>(toDraw->max[0]);
    verts[6].v.ob[1] = static_cast<s16>(toDraw->max[1]);
    verts[6].v.ob[2] = static_cast<s16>(toDraw->min[2]);
    
    verts[7].v.ob[0] = static_cast<s16>(toDraw->max[0]);
    verts[7].v.ob[1] = static_cast<s16>(toDraw->max[1]);
    verts[7].v.ob[2] = static_cast<s16>(toDraw->max[2]);

    gDPPipeSync(dlist++);
    gDPSetCombineMode(dlist++, G_CC_SHADE, G_CC_SHADE);

    guMtxF2L(*g_curMatFPtr, curMtx);
    gSPMatrix(dlist++, curMtx,
	       G_MTX_MODELVIEW|G_MTX_LOAD|G_MTX_NOPUSH);
    gSPTexture(dlist++, 0xFFFF, 0xFFFF, 0, 0, G_OFF);
    gSPClearGeometryMode(dlist++, G_LIGHTING);
    gSPVertex(dlist++, verts, 8, 0);
#ifdef USE_TRIS_FOR_AABB
    // Top and left
    gSP2Triangles(dlist++, 2, 3, 6, 0x00, 0, 1, 2, 0x00);
    // Front and right
    gSP2Triangles(dlist++, 1, 7, 3, 0x00, 5, 6, 7, 0x00);
    // Back and bottom
    gSP2Triangles(dlist++, 0, 6, 4, 0x00, 1, 4, 5, 0x00);
#else
    // Top
    gSPLine3D(dlist++, 2, 3, 0x00);
    gSPLine3D(dlist++, 2, 6, 0x00);
    gSPLine3D(dlist++, 3, 7, 0x00);
    gSPLine3D(dlist++, 6, 7, 0x00);
    // Bottom
    gSPLine3D(dlist++, 0, 1, 0x00);
    gSPLine3D(dlist++, 0, 4, 0x00);
    gSPLine3D(dlist++, 1, 5, 0x00);
    gSPLine3D(dlist++, 4, 5, 0x00);
    // Edges
    gSPLine3D(dlist++, 0, 2, 0x00);
    gSPLine3D(dlist++, 1, 3, 0x00);
    gSPLine3D(dlist++, 4, 6, 0x00);
    gSPLine3D(dlist++, 5, 7, 0x00);
#endif
    gSPSetGeometryMode(dlist++, G_LIGHTING);
    gSPEndDisplayList(dlist++);
}

void drawLine(DrawLayer layer, Vec3 start, Vec3 end, u32 color)
{
    Vtx *verts = (Vtx*)allocGfx(sizeof(Vtx) * 2);
    Mtx *curMtx = (Mtx*)allocGfx(sizeof(Mtx));
    Gfx *dlist = (Gfx*)allocGfx(sizeof(Gfx) * 9);

    addGfxToDrawLayer(layer, dlist);
    
    verts[0].v.ob[0] = static_cast<s16>(start[0]);
    verts[0].v.ob[1] = static_cast<s16>(start[1]);
    verts[0].v.ob[2] = static_cast<s16>(start[2]);
    *(u32*)(&verts[0].v.cn[0]) = color;
    
    verts[1].v.ob[0] = static_cast<s16>(end[0]);
    verts[1].v.ob[1] = static_cast<s16>(end[1]);
    verts[1].v.ob[2] = static_cast<s16>(end[2]);
    *(u32*)(&verts[1].v.cn[0]) = color;

    gDPPipeSync(dlist++);
    gDPSetCombineMode(dlist++, G_CC_SHADE, G_CC_SHADE);

    guMtxF2L(*g_curMatFPtr, curMtx);
    gSPMatrix(dlist++, curMtx,
	       G_MTX_MODELVIEW|G_MTX_LOAD|G_MTX_NOPUSH);
    gSPTexture(dlist++, 0xFFFF, 0xFFFF, 0, 0, G_OFF);
    gSPClearGeometryMode(dlist++, G_LIGHTING);

    gSPVertex(dlist++, verts, 2, 0);
    gSPLine3D(dlist++, 0, 1, 0x00);    
    
    gSPSetGeometryMode(dlist++, G_LIGHTING);
    gSPEndDisplayList(dlist++);
}

u8* allocGfx(s32 size)
{
    u8* retVal = curGfxPoolPtr;
    curGfxPoolPtr += ROUND_UP(size, 8);
    if (curGfxPoolPtr >= curGfxPoolEnd)
        return nullptr;
    return retVal;
}

void setLightDirection(Vec3 lightDir)
{
    light->a.l.col[0] = light->a.l.colc[0] = 0x3F;
    light->a.l.col[1] = light->a.l.colc[1] = 0x3F;
    light->a.l.col[2] = light->a.l.colc[2] = 0x3F;

    light->l->l.col[0] = light->l->l.colc[0] = 0x7F;
    light->l->l.col[1] = light->l->l.colc[1] = 0x7F;
    light->l->l.col[2] = light->l->l.colc[2] = 0x7F;

    light->l->l.dir[0] = (s8)(lightDir[0] * (*g_curMatFPtr)[0][0] + lightDir[1] * (*g_curMatFPtr)[1][0] + lightDir[2] * (*g_curMatFPtr)[2][0]);
    light->l->l.dir[1] = (s8)(lightDir[0] * (*g_curMatFPtr)[0][1] + lightDir[1] * (*g_curMatFPtr)[1][1] + lightDir[2] * (*g_curMatFPtr)[2][1]);
    light->l->l.dir[2] = (s8)(lightDir[0] * (*g_curMatFPtr)[0][2] + lightDir[1] * (*g_curMatFPtr)[1][2] + lightDir[2] * (*g_curMatFPtr)[2][2]);

    lookAt->l[0].l.dir[0] = -light->l->l.dir[0];
    lookAt->l[0].l.dir[1] = -light->l->l.dir[1];
    lookAt->l[0].l.dir[2] = -light->l->l.dir[2];

    lookAt->l[1].l.dir[0] = 0;
    lookAt->l[1].l.dir[1] = 127;
    lookAt->l[1].l.dir[2] = 0;
}

float get_aspect_ratio()
{
    return static_cast<float>(screen_width) / static_cast<float>(screen_height);
}

void gfx::load_view_proj(Vec3 eye_pos, Camera *camera, float aspect, float near, float far, float scale)
{
    Mtx* vp_fixed;
    Mtx* v_fixed;
    // MtxF vp;

    // Set up projection matrix
    guPerspectiveF(g_gfxContexts[g_curGfxContext].projMtxF, &g_perspNorm, camera->fov, aspect, near, far, scale);
    gSPViewport(g_dlist_head++, &viewport);
    gSPPerspNormalize(g_dlist_head++, g_perspNorm);
        
    // Ortho
    // guOrthoF(g_gfxContexts[g_curGfxContext].projMtxF, -screen_width / 2, screen_width / 2, -screen_height / 2, screen_height / 2, 100.0f, 20000.0f, 1.0f);
    // g_perspNorm = 0xFFFF;

    // Set up view matrix
    guLookAtF(g_gfxContexts[g_curGfxContext].viewMtxF,
        eye_pos[0] - camera->model_offset[0],
        eye_pos[1] - camera->model_offset[1],
        eye_pos[2] - camera->model_offset[2], // Eye pos
        camera->target[0] - camera->model_offset[0],
        camera->target[1] - camera->model_offset[1] + camera->yOffset,
        camera->target[2] - camera->model_offset[2], // Look pos
        0.0f, 1.0f, 0.0f); // Up vector

    // mtxfMul(*g_curMatFPtr, *g_curMatFPtr, g_gfxContexts[g_curGfxContext].viewMtxF);


    // Load VP matrix
    vp_fixed = (Mtx*)allocGfx(sizeof(Mtx));
    
    v_fixed = (Mtx*)allocGfx(sizeof(Mtx));
    guMtxF2L(g_gfxContexts[g_curGfxContext].projMtxF, vp_fixed);
    guMtxF2L(g_gfxContexts[g_curGfxContext].viewMtxF, v_fixed);
    
    // Calculate vp matrix
    // guMtxCatF(g_gfxContexts[g_curGfxContext].projMtxF, g_gfxContexts[g_curGfxContext].viewMtxF, vp);
    mtxfMul(g_gfxContexts[g_curGfxContext].viewProjMtxF, g_gfxContexts[g_curGfxContext].projMtxF, g_gfxContexts[g_curGfxContext].viewMtxF);
    // guMtxF2L(vp, vp_fixed);

    gSPMatrix(g_dlist_head++, vp_fixed,
        G_MTX_PROJECTION|G_MTX_LOAD|G_MTX_NOPUSH);
    gSPMatrix(g_dlist_head++, v_fixed,
        G_MTX_PROJECTION|G_MTX_MUL|G_MTX_NOPUSH);
}

void endFrame()
{
    unsigned int i;

    // Finalize the draw layer displaylists and link them to the main one
    for (i = 0; i < gfx::draw_layers; i++)
    {
        // Pipe sync before switching draw layers
        gDPPipeSync(g_dlist_head++);

        // No sprite or line microcode anymore
        // switch (static_cast<DrawLayer>(i))
        // {
        //     // Switch to l3dex2 for line layers
        //     case DrawLayer::opa_line:
        //     case DrawLayer::xlu_line:
        //         gSPLoadUcodeL(g_dlist_head++, gspL3DEX2_fifo);
        //         rspUcodeLoadInit();
        //         break;
        //     // Switch back to f3dex2 for sprite layers
        //     case (static_cast<DrawLayer>(static_cast<int>(DrawLayer::opa_line) + 1)):
        //     case (static_cast<DrawLayer>(static_cast<int>(DrawLayer::xlu_line) + 1)):
        //         gSPLoadUcodeL(g_dlist_head++, gspF3DEX2_fifo);
        //         rspUcodeLoadInit();
        //         break;
        //     // Switch to s2dex2 for the two sprite layers (only one switch needed because they are the last two layers)
        //     case DrawLayer::opa_sprite:
        //         gSPLoadUcodeL(g_dlist_head++, gspS2DEX2_fifo);
        //         break;
        //     default:
        //         break;
        // }

        // Set up the render mode for this draw layer
        gSPDisplayList(g_dlist_head++, drawLayerRenderModes1Cycle[i].data());

        // Link this layer's displaylist to the main displaylist
        gSPDisplayList(g_dlist_head++, drawLayerStarts[i]);

        // Terminate this draw layer's displaylist
        gSPEndDisplayList(drawLayerHeads[i]);
    }
    
    // Set up ortho projection matrix and identity view matrix
    Mtx *ortho = (Mtx *)allocGfx(sizeof(Mtx));
    Mtx *ident = (Mtx *)allocGfx(sizeof(Mtx));
    Gfx *fadeDL = (Gfx *)allocGfx(sizeof(Gfx) * 11);
    Gfx *fadeDLHead = fadeDL;

    guOrtho(ortho, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f);
    guMtxIdent(ident);

    gSPMatrix(fadeDLHead++, ident, G_MTX_MODELVIEW|G_MTX_LOAD|G_MTX_NOPUSH);
    gSPMatrix(fadeDLHead++, ortho, G_MTX_PROJECTION|G_MTX_LOAD|G_MTX_NOPUSH);

    // Disable perspective correction
    gSPPerspNormalize(g_dlist_head++, 0xFFFF);
    // Turn off z buffering
    gSPClearGeometryMode(fadeDLHead++, G_ZBUFFER);
    gDPPipeSync(fadeDLHead++);

    // Terminate and append the gui displaylist
    gSPEndDisplayList(g_gui_dlist_head++);
    gSPDisplayList(g_dlist_head++, &g_gfxContexts[g_curGfxContext].gui_dlist_buffer[0]);

    gDPFullSync(g_dlist_head++);
    gSPEndDisplayList(g_dlist_head++);

    sendGfxTask();
}

#define NUM_TEXTURE_SCROLLS (sizeof(textureScrolls) / sizeof(textureScrolls[0]))

void scrollTextures()
{
}

void animateTextures()
{
}

Vtx fullscreenVerts[] = {
    {{{-1, -1, 0}, 0, {0, 0},{0x0, 0x0, 0x0, 0x0}}},
    {{{-1,  1, 0}, 0, {0, 0},{0x0, 0x0, 0x0, 0x0}}},
    {{{ 1, -1, 0}, 0, {0, 0},{0x0, 0x0, 0x0, 0x0}}},
    {{{ 1,  1, 0}, 0, {0, 0},{0x0, 0x0, 0x0, 0x0}}},
};

void shadeScreen(float alphaPercent)
{
    Mtx *ortho = (Mtx *)allocGfx(sizeof(Mtx));
    Mtx *ident = (Mtx *)allocGfx(sizeof(Mtx));
    Gfx *fadeDL = (Gfx *)allocGfx(sizeof(Gfx) * 11);
    Gfx *fadeDLHead = fadeDL;

    guOrtho(ortho, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f);
    guMtxIdent(ident);

    gSPMatrix(fadeDLHead++, ident, G_MTX_MODELVIEW|G_MTX_LOAD|G_MTX_NOPUSH);
    gSPMatrix(fadeDLHead++, ortho, G_MTX_PROJECTION|G_MTX_LOAD|G_MTX_NOPUSH);
    gSPPerspNormalize(fadeDLHead++, 0xFFFF);
    gSPVertex(fadeDLHead++, fullscreenVerts, 4, 0);
    gSPClearGeometryMode(fadeDLHead++, G_ZBUFFER);
    gDPPipeSync(fadeDLHead++);
    gDPSetEnvColor(fadeDLHead++, 0, 0, 0, (u8)(alphaPercent * 255.0f));
    gDPSetCombineLERP(fadeDLHead++, 0, 0, 0, 0, 0, 0, 0, ENVIRONMENT, 0, 0, 0, 0, 0, 0, 0, ENVIRONMENT);
    gDPSetRenderMode(fadeDLHead++, G_RM_XLU_SURF, G_RM_XLU_SURF);
    gSP2Triangles(fadeDLHead++, 0, 2, 1, 0x00, 2, 3, 1, 0x00);
    gSPEndDisplayList(fadeDLHead++);

    addGfxToDrawLayer(DrawLayer::xlu_surf, fadeDL);
}

extern "C" {
#include <debug.h>
}

glm::vec2 calculate_normalized_device_coords(const Vec3& pos, const MtxF* mat)
{
    // Multiple by MVP matrix
    glm::vec4 clip_pos = *reinterpret_cast<const glm::mat4*>(mat) * glm::vec4{reinterpret_cast<const glm::vec3&>(pos), 1.0f};
    // Perspective divide to convert from clip space to NDC
    return glm::vec2{clip_pos[0] / clip_pos[3], clip_pos[1] / clip_pos[3]};
}

void drawHealthBars(size_t count, void *, void **componentArrays)
{
    HealthState *cur_health_state = static_cast<HealthState*>(componentArrays[COMPONENT_INDEX(Health, ARCHETYPE_HEALTHBAR)]);
    Vec3 *cur_position = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_HEALTHBAR)]);
    gDPPipeSync(g_gui_dlist_head++);
    gDPSetCycleType(g_gui_dlist_head++, G_CYC_1CYCLE);
    gDPSetTexturePersp(g_gui_dlist_head++, G_TP_NONE);
    gDPSetCombineLERP(g_gui_dlist_head++, 0, 0, 0, ENVIRONMENT, 0, 0, 0, 1, 0, 0, 0, ENVIRONMENT, 0, 0, 0, 1);
    while (count)
    {
        if (cur_health_state->maxHealth > 0)
        {
            size_t cur_health_bar_size = cur_health_state->maxHealth / health_per_pixel;
            size_t filled_size = cur_health_state->health / health_per_pixel;
            Vec3 cur_pos_corrected;
            cur_pos_corrected[0] = (*cur_position)[0] - g_Camera.model_offset[0];
            cur_pos_corrected[1] = (*cur_position)[1] - g_Camera.model_offset[1] + 256.0f;
            cur_pos_corrected[2] = (*cur_position)[2] - g_Camera.model_offset[2];
            glm::vec2 entity_ndc = calculate_normalized_device_coords(cur_pos_corrected, &g_gfxContexts[g_curGfxContext].viewProjMtxF);

            unsigned int screen_space_x = lround(entity_ndc[0] *  ((int)screen_width  / 2)) + screen_width  / 2;
            unsigned int screen_space_y = lround(entity_ndc[1] * -((int)screen_height / 2)) + screen_height / 2;
            
            int cur_health_bar_pos_x = (screen_space_x - cur_health_bar_size / 2);
            int cur_health_bar_pos_y = (screen_space_y - health_bar_height / 2);

            if (cur_health_bar_pos_x < 16)
            {
                cur_health_bar_pos_x = 16;
            }
            if (cur_health_bar_pos_x > (int)(screen_width - 16 - 16))
            {
                cur_health_bar_pos_x = screen_width - 16 - 16;
            }
            
            if (cur_health_bar_pos_y < 16)
            {
                cur_health_bar_pos_y = 16;
            }
            if (cur_health_bar_pos_y > static_cast<int>(screen_height - 16 - health_bar_height))
            {
                cur_health_bar_pos_y = static_cast<int>(screen_height - 16 - health_bar_height);
            }

            gDPPipeSync(g_gui_dlist_head++);
            gDPSetEnvColor(g_gui_dlist_head++, 255, 153, 0, 255);
            gDPFillRectangle(g_gui_dlist_head++, cur_health_bar_pos_x, cur_health_bar_pos_y, cur_health_bar_pos_x + filled_size, cur_health_bar_pos_y + health_bar_height);
            gDPPipeSync(g_gui_dlist_head++);
            gDPSetEnvColor(g_gui_dlist_head++, 60, 60, 60, 255);
            gDPFillRectangle(g_gui_dlist_head++, cur_health_bar_pos_x + filled_size, cur_health_bar_pos_y, cur_health_bar_pos_x + cur_health_bar_size, cur_health_bar_pos_y + health_bar_height);
        }

        cur_health_state++;
        cur_position++;
        count--;
    }
}
