#ifndef __PLATFORM_GRID_H__
#define __PLATFORM_GRID_H__

#include <ultra64.h>
#include <block_vector.h>
#include <n64_gfx.h>

struct ChunkGfx {
    std::array<block_vector<Mtx>, num_frame_buffers> matrices;
};

#endif
