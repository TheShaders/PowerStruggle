#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <types.h>

void playSound(uint32_t soundIndex);

enum Sfx {
    metal_hit,
    zap,
    bootup,
};

#endif
