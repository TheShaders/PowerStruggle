#ifndef __ENEMY_BOMB_H__
#define __ENEMY_BOMB_H__

#define ARCHETYPE_BOMBER (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health | Bit_Control | Bit_Deactivatable)

// The per-subtype parameters for a bomber-type enemy
struct BombParams {
    // Name of the model used by the shots of this subtype
    const char* bomb_model_name;
    // Pointer to the model used by the shots of this subtype
    Model* bomb_model;
    // Maximum distance that the player can be seen from
    float sight_radius;
    // Distance that the bomber will try to keep from the player
    float follow_distance;
    // Radius of the shot's hitbox
    uint16_t bomb_explosion_radius;
    // Height of the shot's hitbox
    uint16_t bomb_explosion_height;
    // Minimum distance at which the bomber will place a bomb
    float bomb_place_radius;
    // Number of frames before the bomb explodes
    uint16_t bomb_time;
    // Minimum number of frames between bomb placements
    uint16_t bomb_rate;
};

struct BombDefinition : public BaseEnemyDefinition {
    BombParams params;
};

// The list of bomber params for each bomber subtypes
extern BombDefinition bomber_definitions[];

// The state that a bomber-type enemy maintains
struct BombState : public BaseEnemyState {
    // Number of frames since the bomb was placed
    uint16_t bomb_timer;
    // Angle to run from the bomb
    uint16_t run_angle;
    // Location the bomb was placed
    float bomb_x;
    float bomb_z;
};

// Ensure that the bomber state first in behavior data
static_assert(sizeof(BombState) <= sizeof(BehaviorState::data), "BombState does not fit in behavior data!");

// Creates a bomber of the given subtype
Entity* create_bomb_enemy(float x, float y, float z, int subtype);

#endif
