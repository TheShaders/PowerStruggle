#ifndef __ENEMY_SHOOT_H__
#define __ENEMY_SHOOT_H__

#define ARCHETYPE_SHOOT (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health | Bit_Control)

// The per-subtype parameters for a shooter-type enemy
struct ShootParams {
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

struct ShootDefinition : public BaseEnemyDefinition {
    ShootParams params;
};

// The list of shooter params for each shooter subtypes
extern ShootDefinition shooter_definitions[];

// The state that a shooter-type enemy maintains
struct ShootState : public BaseEnemyState {
    // Number of frames since last shot
    uint8_t shot_timer;
};

// Ensure that the shooter state first in behavior data
static_assert(sizeof(ShootState) <= sizeof(BehaviorState::data), "ShootState does not fit in behavior data!");

// Creates a shooter of the given subtype
Entity* create_shoot(float x, float y, float z, int subtype);

#endif
