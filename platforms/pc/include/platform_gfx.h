#ifndef __PLATFORM_GFX_H__
#define __PLATFORM_GFX_H__

#include <cstdint>
#include <glm/glm.hpp>
#include <types.h>

constexpr glm::vec3 constexpr_normalize(const glm::vec3& in)
{
    glm::vec3 squared = in * in;
    float len = std::sqrt(squared.x + squared.y + squared.z);
    return in / len;
}

constexpr glm::vec3 color_to_normal(const glm::ivec4& rgba)
{
    glm::ivec3 signed_vec = glm::ivec3((int8_t)rgba.x, (int8_t)rgba.y, (int8_t)rgba.z);
    glm::vec3 norm = glm::vec3{signed_vec};
    return constexpr_normalize(norm);
}

class VtxInner {
public:
    glm::ivec3 pos;
    int flag;
    glm::ivec2 st;
    glm::ivec4 rgba;
};

class Vtx {
public:
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 rgba;
    constexpr Vtx(const VtxInner& inner) :
        pos(inner.pos),
        // Subtract 128 from each component, convert to float and normalize
        normal(color_to_normal(inner.rgba)),
        // Convert colors in [0,255] to [0.0, 1.0]
        rgba(glm::vec4(inner.rgba) * (1.0f / 255.0f)) {}
};

class Gfx {
public:
    uint32_t b0;
    uint32_t b1;
};

typedef uint8_t u8;
typedef int16_t s16;

#define gsDPSetTextureFilter(...) {0, 0}
#define gsSPTexture(...) {0, 0}
#define gsDPTileSync() {0, 0}
#define gsDPLoadSync() {0, 0}
#define gsDPPipeSync() {0, 0}
#define gsDPSetTextureImage(...) {0, 0}
#define gsSPEndDisplayList() {0, 0}
#define gsSPVertex(...) {0, 0}
#define gsSP2Triangles(...) {0, 0}
#define gsSP1Triangle(...) {0, 0}
#define gsDPSetTile(...) {0, 0}
#define gsDPSetPrimColor(...) {0, 0}
#define gsSPClearGeometryMode(...) {0, 0}
#define gsDPSetCombineLERP(...) {0, 0}
#define gsDPSetRenderMode(...) {0, 0}
#define gsDPSetAlphaCompare(...) {0, 0}
#define gsDPSetBlendColor(...) {0, 0}
#define gsSPClearGeometryMode(...) {0, 0}
#define gsDPSetEnvColor(...) {0, 0}
#define gsSPSetGeometryMode(...) {0, 0}
#define gsSPDisplayList(...) {0, 0}
#define gsSPCullDisplayList(...) {0, 0}
#define gsDPSetTileSize(...) {0, 0}
#define gsDPLoadTile(...) {0, 0}
#define gsDPSetTextureLUT(...) {0, 0}
#define gsDPLoadTLUTCmd(...) {0, 0}

#endif
