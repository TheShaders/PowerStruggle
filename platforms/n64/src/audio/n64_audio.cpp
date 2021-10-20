#include <ultra64.h>

#include <n64_audio.h>
#include <n64_mem.h>
#include <audio.h>
#include <mem.h>

void audioInit(void)
{
}

void playSound(uint32_t soundIndex)
{
    (void)soundIndex;
}

void audioThreadFunc(UNUSED void *arg)
{
    audioInit();
}
