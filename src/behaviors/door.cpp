#include <behaviors.h>
#include <collision.h>
#include <files.h>
#include <gameplay.h>

#define ARCHETYPE_DOOR (Bit_Position | Bit_Rotation | Bit_Model | Bit_Hitbox | Bit_Behavior)

constexpr int door_move_speed = 16;

struct DoorState {
    uint16_t movedAmount;
    bool moving;
};

static_assert(sizeof(DoorState) <= sizeof(BehaviorState::data), "DoorState does not fit in behavior data!");

void door_bhv_callback(void** components, UNUSED void* data)
{
    DoorState* state = reinterpret_cast<DoorState*>(data);
    Entity* door = get_entity(components);
    Hitbox& hitbox = *get_component<Bit_Hitbox, Hitbox>(components, ARCHETYPE_DOOR);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_DOOR);

    if (state->moving)
    {
        state->movedAmount += door_move_speed;
        pos[1] -= door_move_speed;
        if (state->movedAmount > tile_size)
        {
            queue_entity_deletion(door);
        }
    }
    else if (hitbox.hits != nullptr)
    {
        if (num_keys() > 0)
        {
            // play door open sound
            use_key();
            state->moving = true;
        }
    }
}

Model* door_model = nullptr;

Entity* create_door(float x, float y, float z, uint32_t param)
{
    Entity* door = createEntity(ARCHETYPE_DOOR);
    void* components[NUM_COMPONENTS(ARCHETYPE_DOOR) + 1];
    getEntityComponents(door, components);

    Model** model_out = get_component<Bit_Model, Model*>(components, ARCHETYPE_DOOR);
    BehaviorState& bhv = *get_component<Bit_Behavior, BehaviorState>(components, ARCHETYPE_DOOR);
    Hitbox& hitbox = *get_component<Bit_Hitbox, Hitbox>(components, ARCHETYPE_DOOR);
    Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_DOOR);
    Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_DOOR);
    
    DoorState* state = reinterpret_cast<DoorState*>(bhv.data.data());
    state->movedAmount = 0;
    state->moving = false;

    if (door_model == nullptr)
    {
        door_model = load_model("models/Door");
    }
    *model_out = door_model;

    bhv.callback = door_bhv_callback;

    hitbox.hits = nullptr;
    hitbox.mask = collision_hitbox_mask;
    hitbox.radius = 256;
    hitbox.size_y = 256;
    hitbox.size_z = 64;

    pos[0] = x;
    pos[1] = y;
    pos[2] = z;

    rot[0] = 0;
    rot[1] = 0x4000 * (param & 0b11);
    rot[2] = 0;

    return door;
}
