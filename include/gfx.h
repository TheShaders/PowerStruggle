#ifndef __GFX_H__
#define __GFX_H__

#include <type_traits>

#include <config.h>
#include <types.h>
#include <mathutils.h>

// Draw layers
enum class DrawLayer : unsigned int {
    background,
    opa_surf,
    // opa_inter,
    // opa_line,
    tex_edge,
    opa_decal,
    xlu_decal,
    xlu_surf,
    // xlu_inter,
    // xlu_line,
    count
};

constexpr unsigned int draw_layer_buffer_len = 32;

constexpr unsigned int matf_stack_len = 16;
constexpr unsigned int mat_buffer_len = 128;

extern MtxF *g_curMatFPtr;

void initGfx(void);

void startFrame(void);
void setupCameraMatrices(Camera *camera);
void setLightDirection(Vec3 lightDir);
void endFrame(void);

void drawModel(Model *toDraw, Animation* anim, uint32_t frame);
void drawAABB(DrawLayer layer, AABB *toDraw, uint32_t color);
void drawLine(DrawLayer layer, Vec3 start, Vec3 end, uint32_t color);
void drawColTris(DrawLayer layer, ColTri *tris, uint32_t count, uint32_t color);

void drawAllEntities(void);

void scrollTextures();
void animateTextures();
void shadeScreen(float alphaPercent);

float get_aspect_ratio();

extern "C" void guLookAtF(float mf[4][4], float xEye, float yEye, float zEye,
		      float xAt,  float yAt,  float zAt,
		      float xUp,  float yUp,  float zUp);
extern "C" void guPositionF(float mf[4][4], float r, float p, float h, float s,
			float x, float y, float z);
extern "C" void guTranslateF(float mf[4][4], float x, float y, float z);
extern "C" void guScaleF(float mf[4][4], float x, float y, float z);
extern "C" void guRotateF(float mf[4][4], float a, float x, float y, float z);
extern "C" void guMtxIdentF(float mf[4][4]);

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gfx
{
    constexpr unsigned int draw_layers = static_cast<unsigned int>(DrawLayer::count);

    inline void copy_mat(MtxF *dst, const MtxF *src)
    {
        float* srcPtr = (float*)(&(*src)[0][0]);
        float* dstPtr = (float*)(&(*dst)[0][0]);

        dstPtr[ 0] = srcPtr[ 0];
        dstPtr[ 1] = srcPtr[ 1];
        dstPtr[ 2] = srcPtr[ 2];
        dstPtr[ 3] = srcPtr[ 3];
        dstPtr[ 4] = srcPtr[ 4];
        dstPtr[ 5] = srcPtr[ 5];
        dstPtr[ 6] = srcPtr[ 6];
        dstPtr[ 7] = srcPtr[ 7];
        dstPtr[ 8] = srcPtr[ 8];
        dstPtr[ 9] = srcPtr[ 9];
        dstPtr[10] = srcPtr[10];
        dstPtr[11] = srcPtr[11];
        dstPtr[12] = srcPtr[12];
        dstPtr[13] = srcPtr[13];
        dstPtr[14] = srcPtr[14];
        dstPtr[15] = srcPtr[15];
    }

    inline void push_mat()
    {
        MtxF *nextMat = g_curMatFPtr + 1;
        gfx::copy_mat(nextMat, g_curMatFPtr);
        g_curMatFPtr++;
    }

    inline void load_mat(const MtxF *src)
    {
        gfx::copy_mat(g_curMatFPtr, src);
    }

    inline void push_load_mat(const MtxF *src)
    {
        g_curMatFPtr++;
        gfx::copy_mat(g_curMatFPtr, src);
    }

    inline void pop_mat()
    {
        g_curMatFPtr--;
    }

    inline void save_mat(MtxF *dst)
    {
        gfx::copy_mat(dst, g_curMatFPtr);
    }

    inline void load_identity()
    {
        guMtxIdentF(*g_curMatFPtr);
    }    

    inline uint16_t calc_perspnorm(float near, float far)
    {
        if (near + far <= 2.0f) {
            return 65535;
        }
        uint16_t ret = static_cast<int16_t>(float{1 << 17} / (near + far));
        if (ret <= 0) {
            ret = 1;
        }
        return ret;
    }

    void load_perspective(float fov, float aspect, float near, float far, UNUSED float scale);

    inline void mul_lookat(float eyeX, float eyeY, float eyeZ, float lookX, float lookY, float lookZ, float upX, float upY, float upZ)
    {
        MtxF tmp;
        guLookAtF(tmp,
            eyeX, eyeY, eyeZ,
            lookX, lookY, lookZ,
            upX, upY, upZ);
        mtxfMul(*g_curMatFPtr, *g_curMatFPtr, tmp);
    }

    inline void rotate_axis_angle(float angle, float axisX, float axisY, float axisZ)
    {
        MtxF tmp;
        guRotateF(tmp, angle, axisX, axisY, axisZ);
        mtxfMul(*g_curMatFPtr, *g_curMatFPtr, tmp);
    }

    inline void rotate_euler_xyz(int16_t rx, int16_t ry, int16_t rz)
    {
        MtxF tmp;
        mtxfEulerXYZ(tmp, rx, ry, rz);
        mtxfMul(*g_curMatFPtr, *g_curMatFPtr, tmp);
    }

    inline void apply_matrix(MtxF *mat)
    {
        mtxfMul(*g_curMatFPtr, *g_curMatFPtr, *mat);
    }

    inline void apply_translation(float x, float y, float z)
    {
        MtxF tmp;
        guTranslateF(tmp, x, y, z);
        mtxfMul(*g_curMatFPtr, *g_curMatFPtr, tmp);
    }

    inline void apply_scale(float sx, float sy, float sz)
    {
        MtxF tmp;
        guScaleF(tmp, sx, sy, sz);
        mtxfMul(*g_curMatFPtr, *g_curMatFPtr, tmp);
    }

    inline void apply_position(float pitch, float rx, float ry, float rz, float x, float y, float z)
    {
        MtxF tmp;
        guPositionF(tmp, pitch, rx, ry, rz, x, y, z);
        mtxfMul(*g_curMatFPtr, *g_curMatFPtr, tmp);
    }
}

#endif