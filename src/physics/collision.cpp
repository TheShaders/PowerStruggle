#include <cfloat>

#include <mathutils.h>
#include <collision.h>
#include <physics.h>
#include <types.h>

extern "C" {
#include <debug.h>
}

uint32_t testVerticalRayVsAABB(Vec3 rayStart, float lengthInv, AABB *box, float tmin, float tmax)
{
    float t0, t1;

    if (rayStart[0] > box->max[0] || rayStart[0] < box->min[0] || rayStart[2] > box->max[2] || rayStart[2] < box->min[2])
        return 0;

    // y
    t0 = (box->min[1] - rayStart[1]) * lengthInv;
    t1 = (box->max[1] - rayStart[1]) * lengthInv;

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return 0;
    
    return 1;
}

float verticalRayVsAABB(Vec3 rayStart, float lengthInv, AABB *box, float tmin, float tmax)
{
    float t0, t1;

    if (rayStart[0] > box->max[0] || rayStart[0] < box->min[0] || rayStart[2] > box->max[2] || rayStart[2] < box->min[2])
        return FLT_MAX;

    // y
    t0 = (box->min[1] - rayStart[1]) * lengthInv;
    t1 = (box->max[1] - rayStart[1]) * lengthInv;

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return FLT_MAX;
    
    return tmin;
}

uint32_t testRayVsAABB(Vec3 rayStart, Vec3 rayDirInv, AABB *box, float tmin, float tmax)
{
    float t0, t1;

    // x
    t0 = (box->min[0] - rayStart[0]) * rayDirInv[0];
    t1 = (box->max[0] - rayStart[0]) * rayDirInv[0];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return 0;

    // y
    t0 = (box->min[1] - rayStart[1]) * rayDirInv[1];
    t1 = (box->max[1] - rayStart[1]) * rayDirInv[1];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return 0;

    // z
    t0 = (box->min[2] - rayStart[2]) * rayDirInv[2];
    t1 = (box->max[2] - rayStart[2]) * rayDirInv[2];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return 0;
    
    return 1;
}

float rayVsAABB(Vec3 rayStart, Vec3 rayDirInv, AABB *box, float tmin, float tmax)
{
    float t0, t1;

    // x
    t0 = (box->min[0] - rayStart[0]) * rayDirInv[0];
    t1 = (box->max[0] - rayStart[0]) * rayDirInv[0];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return FLT_MAX;

    // y
    t0 = (box->min[1] - rayStart[1]) * rayDirInv[1];
    t1 = (box->max[1] - rayStart[1]) * rayDirInv[1];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return FLT_MAX;

    // z
    t0 = (box->min[2] - rayStart[2]) * rayDirInv[2];
    t1 = (box->max[2] - rayStart[2]) * rayDirInv[2];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return FLT_MAX;
    
    return tmin;
}

#define VEL_THRESHOLD 0.1f

void handleWalls(Grid* grid, ColliderParams *collider, Vec3 pos, Vec3 vel)
{
    // if (std::abs(vel[0]) >= VEL_THRESHOLD || std::abs(vel[2]) >= VEL_THRESHOLD || collider->floor_surface_type == surface_none)
    {
        float radius = collider->radius;
        Vec3 wall_hits[4];
        float wall_dists[4];
        int num_hits = grid->get_wall_collisions(wall_hits, wall_dists, pos[0], pos[2], radius, pos[1], pos[1] + collider->height);

        for (int i = 0; i < num_hits; i++)
        {
            float dx = wall_hits[i][0] - pos[0];
            float dz = wall_hits[i][2] - pos[2];
            if (wall_dists[i] > EPSILON)
            {
                float nx = dx * (1.0f / wall_dists[i]);
                float nz = dz * (1.0f / wall_dists[i]);

                float vel_dot_norm = nx * vel[0] + nz * vel[2];
                vel[0] -= vel_dot_norm * nx;
                vel[2] -= vel_dot_norm * nz;

                float push_dist = radius - wall_dists[i];
                pos[0] -= push_dist * nx;
                pos[2] -= push_dist * nz;
            }
            else
            {
                vel[0] = 0;
                vel[2] = 0;
            }
        }
    }
}

SurfaceType handleFloorOnGround(Grid* grid, ColliderParams *collider, Vec3 pos, Vec3 vel, float stepUp, float stepDown)
{
    SurfaceType floor_surface_type = 1; // TODO get surface type from get_height
    float hit_height = grid->get_height(pos[0], pos[2], collider->radius, pos[1] - stepDown, pos[1] + stepUp);
    if (hit_height < -16384.0f)
    {
        floor_surface_type = surface_none;
    }
    if (floor_surface_type != surface_none)
    {
        pos[1] = hit_height;
        vel[1] = 0.0f;

        return floor_surface_type;
    }
    return surface_none;
}

SurfaceType handleFloorInAir(Grid* grid, ColliderParams *collider, Vec3 pos, Vec3 vel)
{
    SurfaceType floor_surface_type = 1; // TODO get surface type from get_height
    float hit_height = grid->get_height(pos[0], pos[2], collider->radius, pos[1] - EPSILON, pos[1] - vel[1] + 100.0f);
    if (hit_height < -16384.0f)
    {
        floor_surface_type = surface_none;
    }
    if (floor_surface_type != surface_none)
    {
        pos[1] = hit_height;
        vel[1] = 0.0f;

        return floor_surface_type;
    }
    return surface_none;
}
