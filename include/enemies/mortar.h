#ifndef __ENEMY_MORTAR_H__
#define __ENEMY_MORTAR_H__

#define ARCHETYPE_MORTAR (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health | Bit_Control | Bit_Deactivatable)

// The per-subtype parameters for a mortar-type enemy
struct MortarParams {
    // Name of the model used by the shots of this subtype
    const char* mortar_model_name;
    // Pointer to the model used by the shots of this subtype
    Model* mortar_model;
    // Maximum distance that the player can be seen from
    float sight_radius;
    // Distance that the mortar will try to keep from the player
    float follow_distance;
    // Radius of the shell's explosion hitbox
    uint16_t mortar_explosion_radius;
    // Height of the shell's explosion hitbox
    uint16_t mortar_explosion_height;
    // Maximum distance at which the mortar will fire a shell
    float mortar_fire_radius;
    // Number of frames before the mortar explodes after hitting the ground
    uint16_t mortar_time;
    // Minimum number of frames between mortar shots
    uint16_t mortar_rate;
};

struct MortarDefinition : public BaseEnemyDefinition {
    MortarParams params;
};

// The list of mortar params for each mortar subtypes
extern MortarDefinition mortar_definitions[];

// The state that a mortar-type enemy maintains
struct MortarState : public BaseEnemyState {
    // Number of frames since the mortar was placed
    uint16_t mortar_timer;
    // Angle to run from the mortar
    uint16_t run_angle;
    // Location the mortar was placed
    float mortar_x;
    float mortar_z;
};

// Ensure that the mortar state first in behavior data
static_assert(sizeof(MortarState) <= sizeof(BehaviorState::data), "MortarState does not fit in behavior data!");

// Creates a mortar of the given subtype
Entity* create_mortar_enemy(float x, float y, float z, int subtype);

#endif
