#ifndef __ENEMY_SLASH_H__
#define __ENEMY_SLASH_H__

#define ARCHETYPE_SLASH (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health | Bit_Control)

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
Entity* create_slash_enemy(float x, float y, float z, int subtype);
// Helper function for a slasher's hitbox
int update_slash_hitbox(const Vec3& slasher_pos, const Vec3s& slasher_rot, SlasherParams* params, SlasherState* state, int first = false);

#endif