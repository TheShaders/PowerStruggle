#ifndef __ENEMY_BEAM_H__
#define __ENEMY_BEAM_H__

#define ARCHETYPE_BEAM (Bit_Position | Bit_Velocity | Bit_Collider | Bit_Rotation | Bit_Behavior | Bit_Model | Bit_AnimState | Bit_Gravity | Bit_Health | Bit_Control | Bit_Deactivatable)

// The per-subtype parameters for a beam-type enemy
struct BeamParams {
    // Maximum distance that the player can be seen from
    float sight_radius;
    // Distance that the beam will try to keep from the player
    float follow_distance;
    // Distance at which the enemy will start an attack
    float beam_fire_radius;
    // Length of the beam hitbox
    uint16_t beam_length;
    // Width of the beam hitbox
    uint16_t beam_width;
    // Height of the beam hitbox
    uint16_t beam_height;
    // Y-offset of the beam hitbox from the position of the enemy
    uint16_t beam_y_offset;
    // The amount of time between the beam being activated and showing up
    int16_t beam_start_lag;
    // The time duration of a beam attack (full attack is double this, as it takes this long to extend and then retract)
    uint16_t beam_duration;
};

struct BeamDefinition : public BaseEnemyDefinition {
    BeamParams params;
};

// The list of beam params for each beam subtypes
extern BeamDefinition beam_definitions[];

// The state that a beam-type enemy maintains
struct BeamState : public BaseEnemyState {
    Entity* beam_hitbox;
    uint16_t beam_timer;
    int16_t locked_rotation;
};

// Ensure that the beam state first in behavior data
static_assert(sizeof(BeamState) <= sizeof(BehaviorState::data), "BeamState does not fit in behavior data!");

// Creates a beam of the given beam
Entity* create_beam_enemy(float x, float y, float z, int subtype);
// Helper function for a beam's hitbox

void delete_beam_enemy(Entity *beam_enemy);

#endif