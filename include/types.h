#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#define KEEP __attribute__((section (".text.keep"), used))
#define NOINLINE __attribute__((noinline))
#define FORCEINLINE inline __attribute__((always_inline))
#elif _MSC_VER
#define UNUSED
#define KEEP
#define NOINLINE __declspec(noinline)
#define FORCEINLINE inline __forceinline
#endif

// Prototypes for model structs
struct Model;

// Prototypes for animation structs
struct Animation;
struct AnimState;

// Prototypes for collision structs
struct AABB;
typedef uint8_t SurfaceType;

constexpr SurfaceType surface_none = 0;

struct ColliderParams;

typedef float MtxF[4][4];
typedef float Vec3[3];
typedef int16_t Vec3s[4];

// Prototypes for entity component system
typedef struct Entity_t Entity;
typedef uint32_t archetype_t;
typedef struct MultiArrayListBlock_t MultiArrayListBlock;
typedef struct MultiArrayList_t MultiArrayList;
struct BehaviorState;

// Prototypes for input structs
typedef struct InputData_t InputData;

// Prototypes for camera structs
struct Camera;

// Prototypes for level structs
typedef struct LevelHeader_t LevelHeader;

// Prototypes for physics structs
typedef struct GravityParams_t GravityParams;

// Prototypes for player structs
struct PlayerState;

// Prototypes for various gameplay strucs
struct HealthState;
struct Hitbox;
struct Hit;
struct BaseEnemyDefinition;
struct BaseEnemyState;
struct ControlParams;
struct ControlHandler;

// Prototype for the grid
class Grid;

// Components
// #define COMPONENT(Name, Type) typedef Type Name;
// #include "components.inc.h"
// #undef COMPONENT

#endif