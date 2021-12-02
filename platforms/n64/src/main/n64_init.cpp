// libultra
#include <ultra64.h>

// game code
extern "C" {
#include <debug.h>
}
#include <main.h>
#include <gfx.h>
#include <mem.h>
#include <audio.h>
#include <n64_task_sched.h>
#include <n64_init.h>
#include <n64_gfx.h>
#include <n64_mem.h>
#include <n64_audio.h>

void loadThreadFunc(void *);

#include <array>

#define static

void platformInit()
{
}

u8 idleThreadStack[IDLE_THREAD_STACKSIZE] __attribute__((aligned (16)));
u8 mainThreadStack[MAIN_THREAD_STACKSIZE] __attribute__((aligned (16)));
u8 audioThreadStack[AUDIO_THREAD_STACKSIZE] __attribute__((aligned (16)));
u8 loadThreadStack[LOAD_THREAD_STACKSIZE] __attribute__((aligned (16)));

static OSMesgQueue piMesgQueue;
static std::array<OSMesg, NUM_PI_MESSAGES> piMessages;

OSMesgQueue siMesgQueue;
static std::array<OSMesg, NUM_SI_MESSAGES> siMessages;

static std::array<OSThread, NUM_THREADS> g_threads;
OSPiHandle *g_romHandle;

u8 g_isEmulator;

extern "C" void init(void)
{
    bzero(_mainSegmentBssStart, (u32)_mainSegmentBssEnd - (u32)_mainSegmentBssStart);
    osInitialize();
    
    if (IO_READ(DPC_CLOCK_REG) == 0) {
        g_isEmulator = 1;
    }

    // TODO figure out what's uninitialized that requires this to work
    // bzero(_mainSegmentBssEnd, 0x80400000 -  (u32)_mainSegmentBssEnd);

    initMemAllocator(memPoolStart, (void*) MEM_END);
    g_romHandle = osCartRomInit();

    osCreateThread(&g_threads[IDLE_THREAD_INDEX], IDLE_THREAD, idle, nullptr, idleThreadStack + IDLE_THREAD_STACKSIZE, 10);
    osStartThread(&g_threads[IDLE_THREAD_INDEX]);
}

int main(int, char **);
extern "C" u32 __osSetFpcCsr(u32);

// void crash_screen_init();

void mainThreadFunc(void *)
{
    u32 fpccsr;

    // Read fpcsr
    fpccsr = __osSetFpcCsr(0);
    // Write back fpcsr with division by zero and invalid operations exceptions disabled
    __osSetFpcCsr(fpccsr & ~(FPCSR_EZ | FPCSR_EV));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    main(0, nullptr);
#pragma GCC diagnostic pop
}

void idle(__attribute__ ((unused)) void *arg)
{
    bzero(g_frameBuffers.data(), num_frame_buffers * screen_width * screen_height * sizeof(u16));

    initScheduler();
    
    // Set up PI
    osCreatePiManager(OS_PRIORITY_PIMGR, &piMesgQueue, piMessages.data(), NUM_PI_MESSAGES);

    // Set up SI
    osCreateMesgQueue(&siMesgQueue, siMessages.data(), NUM_SI_MESSAGES);
    osSetEventMesg(OS_EVENT_SI, &siMesgQueue, nullptr);

#ifdef DEBUG_MODE
    debug_initialize();
#endif

    // // Create the audio thread
    // osCreateThread(&g_threads[AUDIO_THREAD_INDEX], AUDIO_THREAD, audioThreadFunc, nullptr, audioThreadStack + AUDIO_THREAD_STACKSIZE, AUDIO_THREAD_PRI);
    // // Start the audio thread
    // osStartThread(&g_threads[AUDIO_THREAD_INDEX]);

    // Create the load thread
    osCreateThread(&g_threads[LOAD_THREAD_INDEX], LOAD_THREAD, loadThreadFunc, nullptr, loadThreadStack + LOAD_THREAD_STACKSIZE, LOAD_THREAD_PRI);
    // Start the load thread
    osStartThread(&g_threads[LOAD_THREAD_INDEX]);

    // Create the main thread
    osCreateThread(&g_threads[MAIN_THREAD_INDEX], MAIN_THREAD, mainThreadFunc, nullptr, mainThreadStack + MAIN_THREAD_STACKSIZE, MAIN_THREAD_PRI);
    // Start the main thread
    osStartThread(&g_threads[MAIN_THREAD_INDEX]);

    // crash_screen_init();

    // Set this thread's priority to 0, making it the idle thread
    osSetThreadPri(nullptr, 0);

    // idle
    while (1);
}
