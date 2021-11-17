#ifndef __CONTROL_H__
#define __CONTROL_H__

#include <types.h>

struct ControlParams {
    uint8_t placeholder;
};

using control_func_t = void(BaseEnemyState* state, InputData* input, void** player_components);

struct ControlHandler {
    control_func_t* on_enter;
    control_func_t* on_update;
    control_func_t* on_leave;
};

extern ControlHandler* control_handlers[];

#endif