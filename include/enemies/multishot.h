#ifndef __ENEMY_MULTISHOT_H__
#define __ENEMY_MULTISHOT_H__

#define ARCHETYPE_MULTISHOT (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health | Bit_Control | Bit_Deactivatable)

// The per-subtype parameters for a multishoter-type enemy
struct MultishotParams {
    // Name of the model used by the shots of this subtype
    const char* shot_model_name;
    // Pointer to the model used by the shots of this subtype
    Model* shot_model;
    // Maximum distance that the player can be seen from
    float sight_radius;
    // Distance that the multishoter will try to keep from the player
    float follow_distance;
    // Speed of the shot (units/frame)
    float shot_speed;
    // Radius of the shot's hitbox
    uint16_t shot_radius;
    // Height of the shot's hitbox
    uint16_t shot_height;
    // Y offset of the shot from the multishoter's position
    float shot_y_offset;
    // Minimum distance at which the multishoter will start multishoting
    float fire_radius;
    // Minimum number of frames between shots
    uint8_t shot_rate;
};

struct MultishotDefinition : public BaseEnemyDefinition {
    MultishotParams params;
};

// The list of multishoter params for each multishoter subtypes
extern MultishotDefinition multishoter_definitions[];

// The state that a multishoter-type enemy maintains
struct MultishotState : public BaseEnemyState {
    // Number of frames since last shot
    uint8_t shot_timer;
};

// Ensure that the multishoter state first in behavior data
static_assert(sizeof(MultishotState) <= sizeof(BehaviorState::data), "MultishotState does not fit in behavior data!");

// Creates a multishoter of the given subtype
Entity* create_multishot_enemy(float x, float y, float z, int subtype);

#endif
