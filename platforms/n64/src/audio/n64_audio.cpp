#include <ultra64.h>

#include <n64_audio.h>
#include <n64_mem.h>
#include <audio.h>
#include <mem.h>

OSMesgQueue audioMsgQueue;
OSMesg audioMsg;

s16 audioBuffers[NUM_AUDIO_BUFFERS][AUDIO_BUFFER_LEN] __attribute__((aligned (64)));
u16 curAudioBuffer = 0;
s32 realSampleRate;

void audioInit(void)
{
    osCreateMesgQueue(&audioMsgQueue, &audioMsg, 1);
    osSetEventMesg(OS_EVENT_AI, &audioMsgQueue, nullptr);

    // Set up a zero buffer to prevent any audio pop at startup
    bzero(audioBuffers[2], AUDIO_BUFFER_LEN * sizeof(u16));
    osAiSetNextBuffer(&audioBuffers[2], AUDIO_BUFFER_LEN * sizeof(u16));

    // Set up the audio frequency
    realSampleRate = osAiSetFrequency(SAMPLE_RATE);
}

void playSound(uint32_t soundIndex)
{
    (void)soundIndex;
}

void audioThreadFunc(UNUSED void *arg)
{
    audioInit();

    // Set up the first audio buffer
    // fillAudioBuffer(curAudioBuffer);
    // Clear any pending messages from the queue
    while (!MQ_IS_EMPTY(&audioMsgQueue))
    {
        osRecvMesg(&audioMsgQueue, nullptr, OS_MESG_BLOCK);
    }
    while (1)
    {
        // Initiate the current buffer's DMA
        osAiSetNextBuffer(&audioBuffers[curAudioBuffer], AUDIO_BUFFER_LEN * sizeof(u16));

        // Set up the next buffer
        curAudioBuffer = (curAudioBuffer + 1) % NUM_AUDIO_BUFFERS;
        // fillAudioBuffer(curAudioBuffer);
        
        // Wait for the current buffer to run out
        osRecvMesg(&audioMsgQueue, nullptr, OS_MESG_BLOCK);
    }
}
