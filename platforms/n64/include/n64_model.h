#ifndef __N64_MODEL_H__
#define __N64_MODEL_H__

#include <array>
#include <cstdint>
#include <memory>

#include <ultra64.h>
#include <glm/glm.hpp>

#include <gfx.h>
#include <material_flags.h>

template <typename T>
T add_offset(T input, void *base_addr)
{
    return (T)((uintptr_t)input + (uintptr_t)base_addr);
}

// Header before material data
// Contains info about how to interpret the data that follows into a material DL
struct MaterialHeader {
    MaterialFlags flags;
    uint8_t gfx_length;
    // Automatic alignment padding
    Gfx *gfx;

    Gfx *setup_gfx(Gfx *gfx_pos);
};

// A sequence of vertices to load from a model's vertex list
struct VertexLoad {
    uint16_t start;
    uint8_t count;
    uint8_t buffer_offset;
};

using TriangleIndices = std::array<uint8_t, 3>;

// A vertex load and the corresponding triangles to draw after the load
struct TriangleGroup {
    VertexLoad load;
    uint32_t num_tris;
    TriangleIndices *triangles;

    void adjust_offsets(void *base_addr);
};

// A collection of successive triangle groups that share a material
struct MaterialDraw {
    uint16_t num_groups;
    uint16_t material_index;
    TriangleGroup *groups;

    void adjust_offsets(void *base_addr);
    size_t gfx_length() const;
};

// A submesh of a joint for a single draw layer
// Contains some number of material draws
struct JointMeshLayer {
    uint32_t num_draws;
    MaterialDraw *draws;
    Gfx* gfx;

    void adjust_offsets(void *base_addr);
    size_t gfx_length() const;
    Gfx *setup_gfx(Gfx* gfx_pos, Vtx *verts, MaterialHeader **materials) const;
};

// One joint (aka bone) of a mesh
// Contains one submesh for each draw layer
struct Joint {
    float posX; // Base positional offset x component (float to save conversion time later on)
    float posY; // Base positional offset y component
    float posZ; // Base positional offset y component
    uint8_t parent;
    uint8_t reserved; // Would be automatically added for alignment
    uint16_t reserved2; // Ditto
    JointMeshLayer layers[gfx::draw_layers];

    void adjust_offsets(void *base_addr);
    size_t gfx_length() const;
};

struct Model {
    uint16_t num_joints;
    uint16_t num_materials;
    Joint *joints;
    MaterialHeader **materials; // pointer to array of material pointers
    Vtx *verts; // pointer to all vertices
    std::unique_ptr<Gfx[]> gfx;

    void adjust_offsets(void *base_addr);
    size_t gfx_length() const;
    void setup_gfx();
};

#endif
