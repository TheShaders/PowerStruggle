#include <cstring>

#include <ultra64.h>

#include <files.h>
#include <mem.h>
#include <n64_mem.h>
#include <n64_model.h>
#include <skipfield.h>

constexpr size_t num_load_slots = 16;

constexpr FORCEINLINE void free_rx_slot(LoadRxSlot *slot);

// Represents a slot in the receieve skipfield that a load handle points to
// Owned by the load handle that points to it
struct LoadRxSlot
{
    OSMesgQueue queue;
    OSMesg mesg_buf;
    uint32_t rom_pos;
    uint32_t data_size;
    void *dest;
    uint32_t id;
    // Non-default constructor purposefully leaves queue and mesg_buf alone
    // They will have been initialized by a first pass during startup
    LoadRxSlot(uint32_t rom_pos, uint32_t data_size, void *dest, uint32_t id) :
        rom_pos(rom_pos),
        data_size(data_size),
        dest(dest),
        id(id)
    {
        // TODO figure out why this is needed, since the queues are all created in the load thread startup
        osCreateMesgQueue(&queue, &mesg_buf, 1);
    }
    constexpr FORCEINLINE ~LoadRxSlot()
    {
        id = 0;
    }
private:
    constexpr LoadRxSlot() = default;

    // Can't declare partially specialized friend, otherwise this would be skipfield<LoadRxSlot, N>
    template <typename T, size_t N>
    friend class skipfield;
    friend void loadThreadFunc(void*);
};

// Represents a slot in the transmit skipfield that gets sent to the load thread
// Gets free by the load thread once the load is complete, points to a tx slot with matching id
struct LoadTxSlot
{
    LoadRxSlot *rx_slot;
    uint32_t id;
};

constinit skipfield<LoadRxSlot, num_load_slots> load_rx_slots{};
constinit skipfield<LoadTxSlot, num_load_slots> load_tx_slots{};

OSMesgQueue loading_queue;
OSMesg loading_mesg_buf[num_load_slots];
OSMesgQueue dma_queue;
OSMesg dma_mesg_buf;

struct filerecord { const char *path; uint32_t offset; uint32_t size; };

class FileRecords
{
private:
  static inline unsigned int hash (const char *str, size_t len);
public:
  static const struct filerecord *get_offset (const char *str, size_t len);
};

extern uint8_t _binary_build_n64_release_assets_bin_start[];

uint32_t load_id_counter = 1;

load_handle start_data_load(void *ret, uint32_t rom_pos, uint32_t size)
{
    // Get a new load transaction id
    uint32_t new_id = load_id_counter++;
    // Wait for a free rx slot
    while (load_rx_slots.full()) { osYieldThread(); }
    // Allocate memory for the destination if none was provided
    if (ret == nullptr)
    {
        ret = allocRegion(size, ALLOC_FILE);
    }
    // Get an rx slot and set its parameters
    auto rx_iter = load_rx_slots.emplace({rom_pos, size, ret, new_id});
    LoadRxSlot* rx_slot = &(*rx_iter);
    // Wait for a free tx slot
    while (load_tx_slots.full()) { osYieldThread(); }
    // Get a tx slot and point it at the current rx slot
    auto tx_iter = load_tx_slots.emplace({rx_slot, new_id});
    LoadTxSlot* tx_slot = &(*tx_iter);
    // Send the tx slot to the loading thread
    // Block in case the message queue is full, as this message needs to go through
    osSendMesg(&loading_queue, tx_slot, OS_MESG_BLOCK);
    // Return a handle pointing to the current rx slot
    return load_handle{rx_slot};
}

load_handle start_file_load(const char *path)
{
    const struct filerecord *file_record = FileRecords::get_offset(path, strlen(path));
    uint32_t rom_pos = (uint32_t)(_binary_build_n64_release_assets_bin_start + file_record->offset);
    return start_data_load(nullptr, rom_pos, file_record->size);
}

void *load_file(const char *path)
{

    // const struct filerecord *file_offset = FileRecords::get_offset(path, strlen(path));
    // void *ret = allocRegion(file_offset->size, ALLOC_FILE);

    // OSMesgQueue queue;
    // OSMesg msg;
    // OSIoMesg io_msg;
    // // Set up the intro segment DMA
    // io_msg.hdr.pri = OS_MESG_PRI_NORMAL;
    // io_msg.hdr.retQueue = &queue;
    // io_msg.dramAddr = ret;
    // io_msg.devAddr = (u32)(_binary_build_n64_release_assets_bin_start + file_offset->offset);
    // io_msg.size = (u32)file_offset->size;
    // osCreateMesgQueue(&queue, &msg, 1);
    // osEPiStartDma(g_romHandle, &io_msg, OS_READ);
    // osRecvMesg(&queue, nullptr, OS_MESG_BLOCK);

    // return ret;

    load_handle handle = start_file_load(path);
    return handle.join();
}

void *load_data(uint32_t rom_pos, uint32_t size)
{
    load_handle handle = start_data_load(nullptr, rom_pos, size);
    return handle.join();
}

void *load_data(void *ret, uint32_t rom_pos, uint32_t size)
{
    load_handle handle = start_data_load(ret, rom_pos, size);
    return handle.join();
}

Model *load_model(const char *path)
{
    Model *ret = load_file<Model>(path);
    ret->adjust_offsets();
    ret->setup_gfx();
    return ret;
}

// Deleter for load_handle's LoadRxSlot unique_ptr
// Invalidates the slot by setting the id to zero and frees the slot
void LoadRxSlotDeleter::operator()(void *ptr)
{
    LoadRxSlot *rx_ptr = static_cast<LoadRxSlot*>(ptr);
    rx_ptr->id = 0;
    load_rx_slots.erase(rx_ptr);
}

void *load_handle::join()
{
    // Wait for the message from the loading thread
    osRecvMesg(&handle_slot_->queue, nullptr, OS_MESG_BLOCK);
    // Return the rx handle's destination pointer
    return handle_slot_->dest;
}

void loadThreadFunc(UNUSED void *arg)
{
    // Set up the message queues in the rx slots
    for (size_t i = 0; i < num_load_slots; i++)
    {
        auto &rx_slot = *load_rx_slots.emplace({});
        osCreateMesgQueue(&rx_slot.queue, &rx_slot.mesg_buf, 1);
    }
    load_rx_slots.clear();

    // Set up the load request message queue
    osCreateMesgQueue(&loading_queue, loading_mesg_buf, num_load_slots);
    // Set up the DMA message queue
    osCreateMesgQueue(&dma_queue, &dma_mesg_buf, 1);

    // Set up the DMA read io message
    OSIoMesg io_msg;
    io_msg.hdr.pri = OS_MESG_PRI_NORMAL;
    io_msg.hdr.retQueue = &dma_queue;

    while (1)
    {
        // Wait for incoming load requests
        LoadTxSlot *cur_tx_slot;
        osRecvMesg(&loading_queue, (OSMesg*)&cur_tx_slot, OS_MESG_BLOCK);

        uint32_t cur_id = cur_tx_slot->id;
        LoadRxSlot *cur_rx_slot = cur_tx_slot->rx_slot;
        
        // Set up the DMA parameters
        io_msg.dramAddr = cur_rx_slot->dest;
        io_msg.devAddr = cur_rx_slot->rom_pos;
        io_msg.size = cur_rx_slot->data_size;

        // Start the DMA
        osEPiStartDma(g_romHandle, &io_msg, OS_READ);
        osRecvMesg(&dma_queue, nullptr, OS_MESG_BLOCK);

        // Check if the rx slot's id still matches the tx slot
        // If it does, send a completed message to the rx slot
        if (cur_id == cur_rx_slot->id)
        {
            osSendMesg(&cur_rx_slot->queue, nullptr, OS_MESG_NOBLOCK);
        }

        // Free the tx slot
        load_tx_slots.erase(cur_tx_slot);
    }
}
