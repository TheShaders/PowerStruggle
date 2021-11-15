#ifndef __BEHAVIORS_H__
#define __BEHAVIORS_H__

#include <ecs.h>

#define ARCHETYPE_SHOOTER (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health)
#define ARCHETYPE_SLASHER (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health)

struct BaseEnemyDefinition {
    const char* model_name;
    Model* model;
};

// Check if the target is in the sight radius and if so moves towards being the given distance from it
float approach_target(float sight_radius, float follow_distance, float move_speed, Vec3 pos, Vec3 vel, Vec3s rot, Vec3 target_pos);

/////////////
// Shooter //
/////////////

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
    BaseEnemyDefinition base;
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

/////////////
// Slasher //
/////////////

// The per-subtype parameters for a slasher-type enemy
struct SlasherParams {
    // Maximum distance that the player can be seen from
    float sight_radius;
    // Distance that the slasher will try to keep from the player
    float follow_distance;
    // Speed the slasher will move at
    float move_speed;
    // Length of the slash hitbox
    uint16_t slash_length;
    // Width of the slash hitbox
    uint16_t slash_width;
    // Height of the slash hitbox
    uint16_t slash_height;
    // Y-offset of the slash hitbox from the position of the enemy
    uint16_t slash_y_offset;
    // The total angle that the slasher will swing across
    uint16_t slash_angle;
    // The angle per physics frame that the slasher will rotate their weapon at
    uint16_t slash_angular_rate;
};

struct SlasherDefinition {
    BaseEnemyDefinition base;
    SlasherParams params;
};

// The list of slasher params for each slasher subtypes
extern SlasherDefinition slasher_definitions[];

// The state that a slasher-type enemy maintains
struct SlasherState {
    SlasherParams* params;
    Entity* slash_hitbox;
    uint16_t cur_slash_angle;
};

// Ensure that the slasher state first in behavior data
static_assert(sizeof(SlasherState) <= sizeof(BehaviorState::data), "SlasherState does not fit in behavior data!");

// Creates a slasher of the given slasher
Entity* create_slasher(int subtype, float x, float y, float z);

#endif
