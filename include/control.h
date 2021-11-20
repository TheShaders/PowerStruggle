#ifndef __CONTROL_H__
#define __CONTROL_H__

#include <types.h>

struct ControlParams {
    uint16_t controllable_health; // Max health at which this entity can be taken control of
};

using control_func_t = void(BaseEnemyState* state, InputData* input, void** player_components);

struct ControlHandler {
    control_func_t* on_enter;
    control_func_t* on_update;
    control_func_t* on_leave;
};

extern ControlHandler* control_handlers[];

Entity* get_controllable_entity_at_position(Vec3 pos, float radius, Vec3 foundPos, float& found_dist);
void control_update();

#endif