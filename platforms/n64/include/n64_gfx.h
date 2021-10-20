#ifndef __N64_GFX_H__
#define __N64_GFX_H__

#include <config.h>
#include <gfx.h>
#include <PR/gbi.h>
#include <PR/os_thread.h>
#include <PR/os_message.h>

// Fix for gSPLoadUcodeL on gcc
#ifdef gSPLoadUcodeL
#undef gSPLoadUcodeL
#endif

#define	gSPLoadUcodeL(pkt, ucode)					\
        gSPLoadUcode((pkt), OS_K0_TO_PHYSICAL(&ucode##TextStart),	\
		            OS_K0_TO_PHYSICAL(&ucode##DataStart))

constexpr unsigned int output_buff_len = 1024;

constexpr unsigned int display_list_len = 1024;
constexpr unsigned int gfx_pool_size = 65536 * 8;
constexpr unsigned int gfx_pool_size64 = gfx_pool_size / 8;

constexpr unsigned int num_frame_buffers = 2;

#ifdef HIGH_RES
constexpr unsigned int screen_width = 640;
constexpr unsigned int screen_height = 480;
#else
constexpr unsigned int screen_width = 320;
constexpr unsigned int screen_height = 240;
#endif

#define BUFFER_SEGMENT 0x01

#ifdef OS_K0_TO_PHYSICAL
 #undef OS_K0_TO_PHYSICAL
#endif

// Gets rid of warnings with -Warray-bounds
#define OS_K0_TO_PHYSICAL(x) ((u32)(x)-0x80000000)

struct GfxContext {
    // Master displaylist
    Gfx dlistBuffer[display_list_len];
    // Floating point modelview matrix stack
    MtxF mtxFStack[matf_stack_len];
    // Floating point projection matrix
    MtxF projMtxF;
    // Graphics tasks done message
    OSMesg taskDoneMesg;
    // Graphics tasks done message queue
    OSMesgQueue taskDoneQueue;
    // Graphics pool
    u64 pool[gfx_pool_size64];
};

extern struct GfxContext g_gfxContexts[num_frame_buffers];

extern Mtx *g_curMatPtr;
extern u32 g_curGfxContext;
extern u16 g_perspNorm;
extern Gfx *g_dlistHead;

#include <array>
extern std::array<std::array<u16, screen_width * screen_height>, num_frame_buffers> g_frameBuffers;

void addGfxToDrawLayer(DrawLayer drawLayer, Gfx* toAdd);
void addMtxToDrawLayer(DrawLayer drawLayer, Mtx* mtx);

void drawGfx(DrawLayer layer, Gfx *toDraw);

u8* allocGfx(s32 size);

namespace gfx
{
    inline Vp viewport = {{											
        { screen_width << 1, screen_height << 1, G_MAXZ / 2, 0},
        { screen_width << 1, screen_height << 1, G_MAXZ / 2, 0},
    }};
}

#endif
