#ifndef __ENEMY_STAB_H__
#define __ENEMY_STAB_H__

#define ARCHETYPE_STAB (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health | Bit_Control | Bit_Deactivatable)

// The per-subtype parameters for a stab-type enemy
struct StabParams {
    // Maximum distance that the player can be seen from
    float sight_radius;
    // Distance that the stab will try to keep from the player
    float follow_distance;
    // Length of the stab hitbox
    uint16_t stab_length;
    // Width of the stab hitbox
    uint16_t stab_width;
    // Height of the stab hitbox
    uint16_t stab_height;
    // Y-offset of the stab hitbox from the position of the enemy
    uint16_t stab_y_offset;
    // The starting forward offset of the stab hitbox
    int16_t stab_start_pos;
    // The forward offset the stab hitbox moves per frame
    uint16_t stab_forward_speed;
    // The time duration of a stab attack (full attack is double this, as it takes this long to extend and then retract)
    uint16_t stab_duration;
};

struct StabDefinition : public BaseEnemyDefinition {
    StabParams params;
};

// The list of stab params for each stab subtypes
extern StabDefinition stab_definitions[];

// The state that a stab-type enemy maintains
struct StabState : public BaseEnemyState {
    Entity* stab_hitbox;
    int16_t stab_offset;
    uint16_t stab_timer;
    uint16_t recoil_timer;
};

// Ensure that the stab state first in behavior data
static_assert(sizeof(StabState) <= sizeof(BehaviorState::data), "StabState does not fit in behavior data!");

// Creates a stab of the given stab
Entity* create_stab_enemy(float x, float y, float z, int subtype);
// Helper function for a stab's hitbox

void delete_stab_enemy(Entity *stab_enemy);

#endif