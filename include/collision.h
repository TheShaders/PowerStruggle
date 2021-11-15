#ifndef __COLLISION_H__
#define __COLLISION_H__

#include <grid.h>
#include <types.h>

#define IS_NOT_LEAF_NODE(bvhNode) ((bvhNode).triCount != 0)
#define IS_LEAF_NODE(bvhNode) ((bvhNode).triCount == 0)

struct AABB {
    Vec3 min;
    Vec3 max;
};

uint32_t testVerticalRayVsAABB(Vec3 rayStart, float lengthInv, AABB *box, float tmin, float tmax);
float verticalRayVsAABB(Vec3 rayStart, float lengthInv, AABB *box, float tmin, float tmax);
uint32_t testRayVsAABB(Vec3 rayStart, Vec3 rayDirInv, AABB *box, float tmin, float tmax);
float rayVsAABB(Vec3 rayStart, Vec3 rayDirInv, AABB *box, float tmin, float tmax);

float raycastVertical(Vec3 rayOrigin, float rayLength, float tmin, float tmax, SurfaceType *floorSurface);
float raycast(Vec3 rayOrigin, Vec3 rayDir, float tmin, float tmax, SurfaceType *floorSurface);

void handleWalls(Grid* grid, ColliderParams *collider, Vec3 pos, Vec3 vel);
SurfaceType handleFloorOnGround(Grid* grid, ColliderParams *collider, Vec3 pos, Vec3 vel, float stepUp, float stepDown);
SurfaceType handleFloorInAir(Grid* grid, ColliderParams *collider, Vec3 pos, Vec3 vel);
struct ColliderParams {
    float radius; // Radius of the collision cylinder
    float height; // Height of the collision cylinder
    float friction_damping; // The fraction of velocity maintained while on the ground each physics frame (e.g. if it's 0 then the object will instantly stop)
    float floor_height; // The height of the floor that the collider is on
    SurfaceType floor_surface_type; // The surface type of the floor
    Hit* hits; // The list of hitboxes this collider is intersecting with
    uint16_t mask; // The mask applied to hitboxes to restrict intersections
};

struct Hitbox
{
    uint16_t size_z; // z length if rectangle hitbox, 0 if cylinder hitbox
    uint16_t radius; // x length if rectangle hitbox, radius if cylinder hitbox
    uint16_t size_y;
    uint16_t mask;
};

constexpr uint16_t player_hitbox_mask = 0x0001;
constexpr uint16_t enemy_hitbox_mask  = 0x0002;

struct Hit
{
    Hit* next;
    Entity* entity;
    Hitbox* hitbox;
};

void find_collisions(Grid& grid);

#endif
