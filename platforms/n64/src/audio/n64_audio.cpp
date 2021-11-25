#include <ultra64.h>
#include <PR/libmus.h>
extern "C" {
#include <PR/sched.h>
}

#include <n64_audio.h>
#include <n64_mem.h>
#include <audio.h>
#include <mem.h>

extern OSSched scheduler;

// OSMesgQueue audioMsgQueue;
// OSMesg audioMsg;

// s16 audioBuffers[NUM_AUDIO_BUFFERS][AUDIO_BUFFER_LEN] __attribute__((aligned (64)));
// u16 curAudioBuffer = 0;
// s32 realSampleRate;

// ptrbank ram address
extern u8 _ptrBankStart[];
// fxbank ram address
extern u8 _fxBankStart[];
// wavetable rom address
extern u8 _wavetableSegmentStart[];

void audioInit(void)
{
    // osCreateMesgQueue(&audioMsgQueue, &audioMsg, 1);
    // osSetEventMesg(OS_EVENT_AI, &audioMsgQueue, nullptr);

    // Initialize libmus
    musConfig mus_config;
    mus_config.control_flag = 0;
    mus_config.channels = 32;
    mus_config.sched = &scheduler;
    mus_config.thread_priority = 12;

    mus_config.heap = static_cast<uint8_t*>(allocRegion(audio_heap_size, ALLOC_AUDIO));
    mus_config.heap_length = audio_heap_size;
    bzero(mus_config.heap, mus_config.heap_length);

    mus_config.fifo_length = 64;
    mus_config.ptr = _ptrBankStart;
    mus_config.wbk = _wavetableSegmentStart;
    mus_config.default_fxbank = _fxBankStart;

    mus_config.syn_updates = 256;
    mus_config.syn_output_rate = audio_sample_rate;
    mus_config.syn_rsp_cmds = 4096;
    mus_config.syn_retraceCount = 2;
    mus_config.syn_num_dma_bufs = 36;
    mus_config.syn_dma_buf_size = 0x800;

    MusInitialize(&mus_config);
    MusSetMasterVolume(MUSFLAG_SONGS, 0x5FFF);
    MusSetMasterVolume(MUSFLAG_EFFECTS, 0x7FFF);
}

void playSound(uint32_t soundIndex)
{
    MusStartEffect2(soundIndex, 0x80, 0x80, 0, -1);
}

// void audioThreadFunc(UNUSED void *arg)
// {
//     audioInit();

//     // Set up the first audio buffer
//     // fillAudioBuffer(curAudioBuffer);
//     // Clear any pending messages from the queue
//     while (!MQ_IS_EMPTY(&audioMsgQueue))
//     {
//         osRecvMesg(&audioMsgQueue, nullptr, OS_MESG_BLOCK);
//     }
//     while (1)
//     {
//         // Initiate the current buffer's DMA
//         osAiSetNextBuffer(&audioBuffers[curAudioBuffer], AUDIO_BUFFER_LEN * sizeof(u16));

//         // Set up the next buffer
//         curAudioBuffer = (curAudioBuffer + 1) % NUM_AUDIO_BUFFERS;
//         // fillAudioBuffer(curAudioBuffer);
        
//         // Wait for the current buffer to run out
//         osRecvMesg(&audioMsgQueue, nullptr, OS_MESG_BLOCK);
//     }
// }
