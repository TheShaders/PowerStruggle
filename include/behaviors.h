#ifndef __BEHAVIORS_H__
#define __BEHAVIORS_H__

#include <ecs.h>

#define ARCHETYPE_SHOOTER (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health)

// The per-subtype parameters for a shooter-type enemy
struct ShooterParams {
    // Maximum distance that the player can be seen from
    float sight_radius;
    // Distance that the shooter will try to keep from the player
    float follow_distance;
    // Speed the shooter will move at
    float move_speed;
    // Speed of the shot (units/frame)
    float shot_speed;
    // Minimum number of frames between shots
    uint8_t shot_rate;
};

struct ShooterDefinition {
    const char* model_name;
    Model* model;
    ShooterParams params;
};

// The list of shooter params for each shooter subtypes
extern ShooterDefinition shooter_definitions[];

// The state that a shooter-type enemy maintains
struct ShooterState {
    ShooterParams* params;
    // Number of frames since last shot
    uint8_t shot_timer;
};

// Ensure that the shooter state first in behavior data
static_assert(sizeof(ShooterState) <= sizeof(BehaviorState::data), "ShooterState does not fit in behavior data!");

// Creates a shooter of the given subtype
Entity* create_shooter(int subtype, float x, float y, float z);
// Check if the target is in the sight radius and if so moves towards being the given distance from it
void approach_target(float sight_radius, float follow_distance, float move_speed, Vec3 pos, Vec3 vel, Vec3 target_pos);

#endif
