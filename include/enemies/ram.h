#ifndef __ENEMY_RAM_H__
#define __ENEMY_RAM_H__

#define ARCHETYPE_RAM (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health | Bit_Control | Bit_Deactivatable)

// The per-subtype parameters for a ram-type enemy
struct RamParams {
    // Maximum distance that the player can be seen from
    float sight_radius;
    // Distance that the ram will try to keep from the player
    float follow_distance;
    // Movement speed while ramming
    float ram_speed;
    // Distance from the player at which the enemy will start a ram
    float ram_range;
    // Forward offset from the enemy's position to the ramming hitbox position
    uint16_t hitbox_forward_offset;
    // Width of the ramming hitbox
    uint16_t hitbox_width;
    // Length of the ramming hitbox
    uint16_t hitbox_length;
    // Height of the ramming hitbox
    uint16_t hitbox_height;
    // Number of frames of cooldown after hitting something during a ram
    uint16_t cooldown_length;
};

struct RamDefinition : public BaseEnemyDefinition {
    RamParams params;
};

// The list of ram params for each ram subtypes
extern RamDefinition ram_definitions[];

// The state that a ram-type enemy maintains
struct RamState : public BaseEnemyState {
    Entity* ram_hitbox;
    uint16_t ram_angle;
    uint16_t cooldown_timer;
    uint8_t is_ramming;
};

// Ensure that the ram state first in behavior data
static_assert(sizeof(RamState) <= sizeof(BehaviorState::data), "RamState does not fit in behavior data!");

// Creates a ram of the given subtype
Entity* create_ram_enemy(float x, float y, float z, int subtype);

#endif
