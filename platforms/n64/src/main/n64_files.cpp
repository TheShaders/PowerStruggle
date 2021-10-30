#include <cstring>

#include <ultra64.h>

#include <files.h>
#include <mem.h>
#include <n64_mem.h>
#include <n64_model.h>

struct filerecord { const char *path; uint32_t offset; uint32_t size; };

class FileRecords
{
private:
  static inline unsigned int hash (const char *str, size_t len);
public:
  static const struct filerecord *get_offset (const char *str, size_t len);
};

extern uint8_t _binary_build_n64_release_assets_bin_start[];

void *load_file(const char *path)
{
    const struct filerecord *file_offset = FileRecords::get_offset(path, strlen(path));
    void *ret = allocRegion(file_offset->size, ALLOC_FILE);

    OSMesgQueue queue;
    OSMesg msg;
    OSIoMesg io_msg;
    // Set up the intro segment DMA
    io_msg.hdr.pri = OS_MESG_PRI_NORMAL;
    io_msg.hdr.retQueue = &queue;
    io_msg.dramAddr = ret;
    io_msg.devAddr = (u32)(_binary_build_n64_release_assets_bin_start + file_offset->offset);
    io_msg.size = (u32)file_offset->size;
    osCreateMesgQueue(&queue, &msg, 1);
    osEPiStartDma(g_romHandle, &io_msg, OS_READ);
    osRecvMesg(&queue, nullptr, OS_MESG_BLOCK);

    return ret;
}

template <typename T>
T* load_file(const char *path)
{
    return static_cast<T*>(load_file(path));
}

Model *load_model(const char *path)
{
    Model *ret = load_file<Model>(path);
    ret->adjust_offsets();
    ret->setup_gfx();
    return ret;
}
