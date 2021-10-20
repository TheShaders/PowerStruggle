#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <types.h>

typedef struct Camera_t {
    Vec3 target;
    float yOffset;
    float fov;
    float distance;
    int16_t pitch;
    int16_t yaw;
    int16_t roll;
} Camera;

extern Camera g_Camera;

#endif
