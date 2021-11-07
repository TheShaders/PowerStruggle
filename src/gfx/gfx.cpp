#include <cstdint>

#include <mathutils.h>
#include <gfx.h>
#include <mem.h>
#include <collision.h>
#include <camera.h>
#include <audio.h>

#include <array>

void setupCameraMatrices(Camera *camera)
{
    Vec3 eyePos;
    Vec3 targetPos;
    Vec3 eyeOffset = {
        camera->distance * sinsf(camera->yaw) * cossf(camera->pitch),
        camera->distance * sinsf(camera->pitch) + camera->yOffset,
        camera->distance * cossf(camera->yaw) * cossf(camera->pitch)
    };
    float cameraHitDist;
    // ColTri *cameraHitTri;
    SurfaceType cameraHitSurface;

    VEC3_COPY(targetPos, camera->target);
    targetPos[1] += camera->yOffset;

    // cameraHitDist = raycast(targetPos, eyeOffset, 0.0f, 1.1f, &cameraHitTri, &cameraHitSurface);

    // if (cameraHitTri != nullptr)
    // {
    //     VEC3_SCALE(eyeOffset, eyeOffset, MIN(MAX(cameraHitDist - 0.1f, 0.1f), 1.0f));
    // }
    VEC3_ADD(eyePos, eyeOffset, targetPos);

    // // Set up view matrix
    gfx::mul_lookat(
        eyePos[0], eyePos[1], eyePos[2], // Eye pos
        camera->target[0], camera->target[1] + camera->yOffset, camera->target[2], // Look pos
        0.0f, 1.0f, 0.0f); // Up vector
    
    // gfx::mul_lookat(
    //     camera->target[0], camera->target[1] + camera->yOffset, camera->target[2], // Eye pos
    //     camera->target[0] - eyeOffset[0], camera->target[1] + camera->yOffset - eyeOffset[1], camera->target[2] - eyeOffset[2], // Look pos
    //     0.0f, 1.0f, 0.0f); // Up vector

    // Perpsective
    gfx::load_perspective(camera->fov, get_aspect_ratio(), 100.0f, 20000.0f, 1.0f);
}
