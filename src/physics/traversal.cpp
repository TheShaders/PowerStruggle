#include <cfloat>

#include <gfx.h>
#include <mem.h>
#include <ecs.h>
#include <collision.h>
#include <surface_types.h>

float raycastVertical(Vec3 rayOrigin, float rayLength, float tmin, float tmax, SurfaceType *floorSurface)
{
    // TODO Search the grid

    *floorSurface = -1;
    return FLT_MAX;
}

float raycast(Vec3 rayOrigin, Vec3 rayDir, float tmin, float tmax, SurfaceType *floorSurface)
{
    // TODO search the grid

    *floorSurface = -1;
    return FLT_MAX;
}
