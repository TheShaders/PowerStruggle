#include <n64_mem.h>

#include <ultra64.h>

#include <mem.h>

OSMesgQueue dmaMesgQueue;

OSMesg dmaMessage;

OSIoMesg dmaIoMessage;

void startDMA(void *targetVAddr, void *romAddr, int length)
{
    // Zero out the region being DMA'd to
    bzero(targetVAddr, length);

    // Create message queue for DMA reads/writes
    osCreateMesgQueue(&dmaMesgQueue, &dmaMessage, 1);
    
    // Invalidate the data cache for the region being DMA'd to
    osInvalDCache(targetVAddr, length); 

    // Set up the intro segment DMA
    dmaIoMessage.hdr.pri = OS_MESG_PRI_NORMAL;
    dmaIoMessage.hdr.retQueue = &dmaMesgQueue;
    dmaIoMessage.dramAddr = targetVAddr;
    dmaIoMessage.devAddr = (u32)romAddr;
    dmaIoMessage.size = (u32)length;

    // Start the DMA
    osEPiStartDma(g_romHandle, &dmaIoMessage, OS_READ);
}

void waitForDMA()
{
    // Wait for the DMA to complete
    osRecvMesg(&dmaMesgQueue, nullptr, OS_MESG_BLOCK);
}

uintptr_t segmentTable[SEGMENT_COUNT];

void setSegment(u32 segmentIndex, void* virtualAddress)
{
    segmentTable[segmentIndex] = (uintptr_t) virtualAddress;
}

void *getSegment(u32 segmentIndex)
{
    return (void *)segmentTable[segmentIndex];
}

void* segmentedToVirtual(void* segmentedAddress)
{
    u32 segmentIndex = (uintptr_t)segmentedAddress >> 24;
    if (segmentIndex == 0x80)
        return segmentedAddress;
    return (void*)(segmentTable[segmentIndex] + ((uintptr_t)segmentedAddress & 0xFFFFFF));
}

extern "C" void abort()
{
    while (1);
}
