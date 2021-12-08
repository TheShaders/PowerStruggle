#include <behaviors.h>
#include <collision.h>
#include <files.h>
#include <gameplay.h>
#include <audio.h>

#define ARCHETYPE_KEY (Bit_Position | Bit_Rotation | Bit_Model | Bit_Hitbox | Bit_Behavior)

void key_bhv_callback(void** components, void* data)
{
    Entity* key = get_entity(components);
    Hitbox& hitbox = *get_component<Bit_Hitbox, Hitbox>(components, ARCHETYPE_KEY);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_KEY);

    rot[1] += 0x100;

    if (hitbox.hits != nullptr)
    {
        collect_key();
        playSound(Sfx::card_get);
        queue_entity_deletion(key);
    }
}

Model* key_model = nullptr;

Entity* create_key(float x, float y, float z)
{
    Entity* key = createEntity(ARCHETYPE_KEY);
    void* components[NUM_COMPONENTS(ARCHETYPE_KEY) + 1];
    getEntityComponents(key, components);

    Model** model_out = get_component<Bit_Model, Model*>(components, ARCHETYPE_KEY);
    BehaviorState& bhv = *get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_KEY);
    Hitbox& hitbox = *get_component<Bit_Hitbox, Hitbox>(components, ARCHETYPE_KEY);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_KEY);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_KEY);

    if (key_model == nullptr)
    {
        key_model = load_model("models/Keycard");
    }
    *model_out = key_model;

    bhv.callback = key_bhv_callback;

    hitbox.hits = nullptr;
    hitbox.mask = interact_hitbox_mask;
    hitbox.radius = 60;
    hitbox.size_y = 256;
    hitbox.size_z = 0;

    pos[0] = x;
    pos[1] = y;
    pos[2] = z;

    rot[0] = 0;
    rot[1] = 0;
    rot[2] = 0;

    return key;
}
