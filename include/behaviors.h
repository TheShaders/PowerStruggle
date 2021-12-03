#ifndef __BEHAVIORS_H__
#define __BEHAVIORS_H__

#include <ecs.h>

enum class EnemyType : uint8_t {
    Shoot, // e.g. Grease-E
    Slash, // e.g. Mend-E
    Spinner, // e.g. Harv-E
    Ram, // e.g. Till-R
    Bomb, // e.g. Herb-E
    Beam, // e.g. Zap-E
    Multishot, // e.g. Gas-E
    Jet, // e.g. Wave-E
    Stab, // e.g. Drill-R
    Slam, // e.g. Smash-O
    Mortar, // e.g. Blast-E
    Flame, // e.g. Heat-O
};

enum class InteractableType: uint8_t {
    LoadTrigger,
    Key,
    Door
};

#define ARCHETYPE_EXPLOSION (ARCHETYPE_SCALED_MODEL | Bit_Hitbox | Bit_DestroyTimer)
#define ARCHETYPE_LOAD_TRIGGER (ARCHETYPE_RECTANGLE_HITBOX)

#include <enemies/base.h>
#include <enemies/shoot.h>
#include <enemies/slash.h>
#include <enemies/spinner.h>
#include <enemies/ram.h>
#include <enemies/bomb.h>
// beam
#include <enemies/multishot.h>
// jet
#include <enemies/stab.h>
// mortar
// flame

// Check if the target is in the sight radius and if so moves towards being the given distance from it
float approach_target(float sight_radius, float follow_distance, float move_speed, Vec3 pos, Vec3 vel, Vec3s rot, Vec3 target_pos);
// Creates an enemy of the given type and subtype at the given position
Entity* create_enemy(float x, float y, float z, EnemyType type, int subtype);
// Creates an enemy of the given type and subtype at the given position
Entity* create_interactable(float x, float y, float z, InteractableType type, int subtype, uint32_t param);
// Common initialiation routine for enemies
void init_enemy_common(BaseEnemyInfo *base_info, Model** model_out, HealthState* health_out);
// Common hitbox handling routine for enemies, returns true if the entity has run out of health
int handle_enemy_hits(Entity* enemy, ColliderParams& collider, HealthState& health_state);
// Applies recoil to the given position and velocity based on the hit entity's position
// Also applies recoil to the hit entity if it has a velocity component
void apply_recoil(const Vec3& pos, Vec3& vel, Entity* hit, float recoil_strength);

#endif
