#ifndef __BEHAVIORS_H__
#define __BEHAVIORS_H__

#include <ecs.h>

enum class EnemyType : uint8_t {
    Shooter,
    Slasher
};

#define ARCHETYPE_SHOOTER (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health | Bit_Control)
#define ARCHETYPE_SLASHER (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health | Bit_Control)

// The basic info that every enemy definition has
struct BaseEnemyInfo {
    const char* model_name;
    Model* model;
    uint16_t max_health;
    uint16_t controllable_health;
    // Speed the enemy will move at
    float move_speed;
    EnemyType enemy_type;
};

// The base definition that all enemy definitions inherit from
struct BaseEnemyDefinition {
    BaseEnemyInfo base;
};

struct BaseEnemyState {
    BaseEnemyDefinition* definition;
};

// Check if the target is in the sight radius and if so moves towards being the given distance from it
float approach_target(float sight_radius, float follow_distance, float move_speed, Vec3 pos, Vec3 vel, Vec3s rot, Vec3 target_pos);
// Creates an enemy of the given type and subtype at the given position
Entity* create_enemy(float x, float y, float z, EnemyType type, int subtype);
// Common initialiation routine for enemies
void init_enemy_common(BaseEnemyInfo *base_info, Model** model_out, HealthState* health_out);
// Common hitbox handling routine for enemies, returns true if the entity has run out of health
int handle_enemy_hits(Entity* enemy, ColliderParams& collider, HealthState& health_state);

/////////////
// Shooter //
/////////////

// The per-subtype parameters for a shooter-type enemy
struct ShooterParams {
    // Name of the model used by the shots of this subtype
    const char* shot_model_name;
    // Pointer to the model used by the shots of this subtype
    Model* shot_model;
    // Maximum distance that the player can be seen from
    float sight_radius;
    // Distance that the shooter will try to keep from the player
    float follow_distance;
    // Speed of the shot (units/frame)
    float shot_speed;
    // Radius of the shot's hitbox
    uint16_t shot_radius;
    // Height of the shot's hitbox
    uint16_t shot_height;
    // Y offset of the shot from the shooter's position
    float shot_y_offset;
    // Minimum distance at which the shooter will start shooting
    float fire_radius;
    // Minimum number of frames between shots
    uint8_t shot_rate;
};

struct ShooterDefinition : public BaseEnemyDefinition {
    ShooterParams params;
};

// The list of shooter params for each shooter subtypes
extern ShooterDefinition shooter_definitions[];

// The state that a shooter-type enemy maintains
struct ShooterState : public BaseEnemyState {
    // Number of frames since last shot
    uint8_t shot_timer;
};

// Ensure that the shooter state first in behavior data
static_assert(sizeof(ShooterState) <= sizeof(BehaviorState::data), "ShooterState does not fit in behavior data!");

// Creates a shooter of the given subtype
Entity* create_shooter(float x, float y, float z, int subtype);

/////////////
// Slasher //
/////////////

// The per-subtype parameters for a slasher-type enemy
struct SlasherParams {
    // Maximum distance that the player can be seen from
    float sight_radius;
    // Distance that the slasher will try to keep from the player
    float follow_distance;
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

struct SlasherDefinition : public BaseEnemyDefinition {
    SlasherParams params;
};

// The list of slasher params for each slasher subtypes
extern SlasherDefinition slasher_definitions[];

// The state that a slasher-type enemy maintains
struct SlasherState : public BaseEnemyState {
    Entity* slash_hitbox;
    uint16_t cur_slash_angle;
};

// Ensure that the slasher state first in behavior data
static_assert(sizeof(SlasherState) <= sizeof(BehaviorState::data), "SlasherState does not fit in behavior data!");

// Creates a slasher of the given slasher
Entity* create_slasher(float x, float y, float z, int subtype);
// Helper function for a slasher's hitbox
int update_slash_hitbox(const Vec3& slasher_pos, const Vec3s& slasher_rot, SlasherParams* params, SlasherState* state, int first = false);

#endif
