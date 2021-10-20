#ifndef __INPUT_H__
#define __INPUT_H__

#include <types.h>

#define INPUT_DEADZONE 0.1f

typedef struct InputData_t {
    float x;
    float y;
    float magnitude;
    int16_t angle;
    uint16_t buttonsHeld;
    uint16_t buttonsPressed;
} InputData;

inline InputData g_PlayerInput;

void initInput();
void beginInputPolling();
void readInput();


#endif
