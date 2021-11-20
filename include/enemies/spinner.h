#ifndef __ENEMY_SPINNER_H__
#define __ENEMY_SPINNER_H__

#define ARCHETYPE_SPINNER (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health | Bit_Control)

// The per-subtype parameters for a spinner-type enemy
struct SpinnerParams {
    // Name of the model used by the blade
    const char* blade_model_name;
    // Pointer to the model used by the blade
    Model* blade_model;
    // Maximum distance that the player can be seen from
    float sight_radius;
    // Distance that the spinner will try to keep from the player
    float follow_distance;
    // Y offset of the blade from the spinner's position
    float blade_y_offset;
    // Rotational speed of the blade (angular units/frame)
    uint16_t blade_speed;
    // Length of the blade's hitbox
    uint16_t blade_length;
    // Height of the blade's hitbox
    uint16_t blade_height;
    // Width of the blade's hitbox
    uint16_t blade_width;
};

struct SpinnerDefinition : public BaseEnemyDefinition {
    SpinnerParams params;
};

// The list of spinner params for each spinner subtypes
extern SpinnerDefinition spinnerer_definitions[];

// The state that a spinner-type enemy maintains
struct SpinnerState : public BaseEnemyState {
    Entity* blade_entity;
};

// Ensure that the spinner state first in behavior data
static_assert(sizeof(SpinnerState) <= sizeof(BehaviorState::data), "SpinnerState does not fit in behavior data!");

// Creates a spinner of the given subtype
Entity* create_spinner(float x, float y, float z, int subtype);

#endif
