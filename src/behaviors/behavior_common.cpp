#include <array>
#include <cmath>

#include <files.h>
#include <mathutils.h>
#include <interaction.h>
#include <behaviors.h>
#include <control.h>
#include <collision.h>
#include <main.h>
#include <audio.h>

float approach_target(float sight_radius, float follow_distance, float move_speed, Vec3 pos, Vec3 vel, Vec3s rot, Vec3 target_pos)
{
    // Get the displacement vector from the player to this enemy
    float dx = pos[0] - target_pos[0];
    float dz = pos[2] - target_pos[2];
    float dist_sq = dx * dx + dz * dz;
    if (dist_sq < EPSILON) return 0.0f;
    float dist = sqrtf(dist_sq);
    // Check if the player can be seen
    if (dist < sight_radius && dist > EPSILON)
    {
        // If so, determine where to move towards

        // Get the normalized vector from the player to this enemy
        float dx_norm = dx * (1.0f / dist);
        float dz_norm = dz * (1.0f / dist);

        // Calculate the target position
        float target_x = target_pos[0] + dx_norm * follow_distance;
        float target_z = target_pos[2] + dz_norm * follow_distance;

        // Get the displacement vector from the enemy to the target position
        float target_dx = target_x - pos[0];
        float target_dz = target_z - pos[2];
        float target_dist_sq = target_dx * target_dx + target_dz * target_dz;
        float target_dist = sqrtf(target_dist_sq);

        float target_vel_x;
        float target_vel_z;
        if (target_dist > EPSILON)
        {
            // Calculate the normal vector from the enemy to the target position
            float target_dx_norm = target_dx * (1.0f / target_dist);
            float target_dz_norm = target_dz * (1.0f / target_dist);

            // Calculate the target speed
            float target_speed = std::min(move_speed, target_dist * 0.25f);
            target_vel_x = target_speed * target_dx_norm;
            target_vel_z = target_speed * target_dz_norm;
        }
        else
        {
            target_vel_x = 0.0f;
            target_vel_z = 0.0f;
        }

        vel[0] = approachFloatLinear(vel[0], target_vel_x, 1.0f);
        vel[2] = approachFloatLinear(vel[2], target_vel_z, 1.0f);

        rot[1] = atan2s(-dz, -dx);
    }
    else
    {
        vel[0] = approachFloatLinear(vel[0], 0.0f, 1.0f);
        vel[2] = approachFloatLinear(vel[2], 0.0f, 1.0f);
    }
    return dist;
}

Entity* placeholder_create(float, float, float, int)
{
    *(volatile int*)0 = 0;
    while (1);
}

Entity* placeholder_create2(float, float, float, int)
{
    return nullptr;
}


std::array create_enemy_funcs {
    create_shoot_enemy,
    create_slash_enemy,
    create_spinner_enemy,
    create_ram_enemy,
    create_bomb_enemy,
    create_beam_enemy, // beam
    create_multishot_enemy,
    placeholder_create2, // jet
    create_stab_enemy,
    placeholder_create2, // slam
    create_mortar_enemy,
    placeholder_create2, // flamethrower
};

using delete_enemy_func_t = void(Entity*);

std::array<delete_enemy_func_t*, create_enemy_funcs.size()> delete_enemy_funcs {
    nullptr, // shoot
    delete_slash_enemy,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

Entity* create_enemy(float x, float y, float z, EnemyType type, int subtype)
{
    return create_enemy_funcs[static_cast<int>(type)](x, y, z, subtype);
}

void init_enemy_common(BaseEnemyInfo* base_info, Model** model_out, HealthState* health_out)
{
    // Set up the enemy's model
    // Load the model if it isn't already loaded
    // Don't try to load the model if the model name is null
    if (base_info->model == nullptr && base_info->model_name != nullptr)
    {
        base_info->model = load_model(base_info->model_name);
    }
    *model_out = base_info->model;

    // Set up the enemy's health
    health_out->max_health = base_info->max_health;
}

extern ControlHandler shoot_control_handler;
extern ControlHandler slash_control_handler;
extern ControlHandler spinner_control_handler;
extern ControlHandler ram_control_handler;
extern ControlHandler bomb_control_handler;
extern ControlHandler beam_control_handler;
extern ControlHandler multishot_control_handler;
// jet
extern ControlHandler stab_control_handler;
// slam
extern ControlHandler mortar_control_handler;
extern ControlHandler mainframe_control_handler;

ControlHandler* control_handlers[] = {
    &shoot_control_handler,
    &slash_control_handler,
    &spinner_control_handler,
    &ram_control_handler,
    &bomb_control_handler,
    &beam_control_handler,
    &multishot_control_handler,
    nullptr, // jet
    &stab_control_handler, // stab
    nullptr, // slam
    &mortar_control_handler,
    nullptr, // flamethrower
    &mainframe_control_handler,
};

extern DeleteHandler delete_slash_enemy;
extern DeleteHandler delete_spinner_enemy;
extern DeleteHandler delete_ram_enemy;
extern DeleteHandler delete_beam_enemy;
extern DeleteHandler delete_stab_enemy;

DeleteHandler* delete_handlers[] = {
    nullptr, // shoot
    delete_slash_enemy, // slash
    delete_spinner_enemy, // spinner
    delete_ram_enemy, // ram
    nullptr, // bomb
    delete_beam_enemy, // beam
    nullptr, // multishot
    nullptr, // jet
    delete_stab_enemy, // stab
    nullptr, // slam
    nullptr, // mortar
    nullptr, // flamethrower
    nullptr, // mainframe
};

int take_damage(Entity* hit_entity, HealthState& health_state, int damage)
{
    if (damage >= health_state.health)
    {
        health_state.health = 0;
        // playSound(1);
        queue_entity_deletion(hit_entity);
        return true;
    }
    health_state.health -= damage;
    return false;
}

int handle_enemy_hits(Entity* enemy, ColliderParams& collider, HealthState& health_state)
{
    ColliderHit* cur_hit = collider.hits;
    while (cur_hit != nullptr)
    {
        if (g_gameTimer - health_state.last_hit_time < 15)
        {
            break;
        }
        int ret = take_damage(enemy, health_state, 10);
        health_state.last_hit_time = g_gameTimer;
        // queue_entity_deletion(cur_hit->entity);
        cur_hit = cur_hit->next;
        return ret;
    }
    return false;
}

void apply_recoil(const Vec3& pos, Vec3& vel, Entity* hit, float recoil_strength)
{
    archetype_t hit_archetype = hit->archetype;
    // Get the hit entity's components
    dynamic_array<void*> hit_components(NUM_COMPONENTS(hit_archetype) + 1);
    getEntityComponents(hit, hit_components.data());
    Vec3& hit_pos = *get_component<Bit_Position, Vec3>(hit_components.data(), hit_archetype);

    // Get the normal vector between the ram and the hit
    float dx = hit_pos[0] - pos[0];
    float dz = hit_pos[2] - pos[2];
    float magnitude = sqrtf(dx * dx + dz * dz);
    if (magnitude < EPSILON)
    {
        dx = 1.0f;
        dz = 0.0f;
        magnitude = 1.0f;
    }
    float nx = dx * (1.0f / magnitude);
    float nz = dz * (1.0f / magnitude);

    vel[0] -= recoil_strength * nx;
    vel[2] -= recoil_strength * nz;

    // If the hit entity has a velocity, apply recoil to it too
    if (hit->archetype & Bit_Velocity)
    {
        Vec3& hit_vel = *get_component<Bit_Position, Vec3>(hit_components.data(), hit_archetype);

        hit_vel[0] += recoil_strength * nx;
        hit_vel[2] += recoil_strength * nz;
    }
}

Model* explosion_model = nullptr;
const char explosion_model_name[] = "models/Explosion";

void create_explosion(Vec3 pos, int radius, int time, int mask)
{
    Entity* explosion = createEntity(ARCHETYPE_EXPLOSION);
    void* explosion_components[NUM_COMPONENTS(ARCHETYPE_EXPLOSION) + 1];
    getEntityComponents(explosion, explosion_components);

    if (explosion_model == nullptr)
    {
        explosion_model = load_model(explosion_model_name);
    }

    Model** model_out = get_component<Bit_Model, Model*>(explosion_components, ARCHETYPE_EXPLOSION);
    Vec3& explosion_pos = *get_component<Bit_Position, Vec3>(explosion_components, ARCHETYPE_EXPLOSION);
    Vec3& explosion_scale = *get_component<Bit_Scale, Vec3>(explosion_components, ARCHETYPE_EXPLOSION);
    Vec3s& explosion_rot = *get_component<Bit_Rotation, Vec3s>(explosion_components, ARCHETYPE_EXPLOSION);
    uint16_t& explosion_timer = *get_component<Bit_DestroyTimer, uint16_t>(explosion_components, ARCHETYPE_EXPLOSION);
    Hitbox& explosion_hitbox = *get_component<Bit_Hitbox, Hitbox>(explosion_components, ARCHETYPE_EXPLOSION);

    *model_out = explosion_model;
    VEC3_COPY(explosion_pos, pos);
    explosion_rot[0] = 0;
    explosion_rot[1] = 0;
    explosion_rot[2] = 0;

    explosion_scale[0] = explosion_scale[1] = explosion_scale[2] = radius / 100.0f;

    explosion_timer = time;
    explosion_hitbox.radius = radius;
    explosion_hitbox.size_y = radius;
    explosion_hitbox.size_z = 0;
    explosion_hitbox.mask = mask;
    explosion_hitbox.hits = nullptr;
}

Entity* create_load_trigger(float x, float y, float z, UNUSED int size)
{
    Entity* ret = createEntity(ARCHETYPE_LOAD_TRIGGER);
    void* components[NUM_COMPONENTS(ARCHETYPE_LOAD_TRIGGER) + 1];
    getEntityComponents(ret, components);
    Vec3* pos = get_component<Bit_Position, Vec3>(components, ARCHETYPE_LOAD_TRIGGER);
    Hitbox* hitbox = get_component<Bit_Hitbox, Hitbox>(components, ARCHETYPE_LOAD_TRIGGER);

    (*pos)[0] = x;
    (*pos)[1] = y;
    (*pos)[2] = z;

    hitbox->size_z = hitbox->radius = (1 * tile_size) - 5;
    hitbox->size_y = tile_size;
    hitbox->mask = load_hitbox_mask;

    return ret;
}

Entity* create_interactable(float x, float y, float z, InteractableType type, UNUSED int subtype, uint32_t param)
{
    switch (type)
    {
        case InteractableType::LoadTrigger:
            return create_load_trigger(x, y, z, param);
            break;
        case InteractableType::Door:
            return nullptr;
            return create_door(x, y, z, param);
            break;
        case InteractableType::Key:
            return create_key(x, y, z);
            break;
    }
    return nullptr;
}
