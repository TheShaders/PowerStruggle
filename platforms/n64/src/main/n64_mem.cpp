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
    *(volatile int*)0 = 0;
    while (1);
}

extern "C" void* memset(void* ptr, int value, size_t num)
{
    uint8_t val_8 = static_cast<uint8_t>(value);
    uint32_t val_word = 
        (val_8 << 24) |
        (val_8 << 16) |
        (val_8 <<  8) |
        (val_8);

    uintptr_t bytes_end = reinterpret_cast<uintptr_t>(ptr) + num;
    uintptr_t words_end = bytes_end & ~(0b11);
    uintptr_t ptr_uint = reinterpret_cast<uintptr_t>(ptr);

    while (ptr_uint & 0b11)
    {
        *reinterpret_cast<uint8_t*>(ptr_uint) = value;
        ptr_uint += 1;
    }

    while (ptr_uint != words_end)
    {
        *reinterpret_cast<uint32_t*>(ptr_uint) = val_word;
        ptr_uint += 4;
    }

    while (ptr_uint != bytes_end)
    {
        *reinterpret_cast<uint8_t*>(ptr_uint) = value;
        ptr_uint += 1;
    }

	return ptr;
}
