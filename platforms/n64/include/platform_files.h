#ifndef __PLATFORM_FILES_H__
#define __PLATFORM_FILES_H__

#include <cstdint>
#include <memory>
#include <utility>

struct LoadRxSlot;

// Deleter class for use with load receive slots
class LoadRxSlotDeleter
{
public:
    void operator()(void* ptr);
};

class load_handle
{
public:
    // Default constructor
    load_handle() = default;
    // Default destructor thanks to unique_ptr
    ~load_handle() = default;
    // Not copyable or copy assignable
    load_handle(const load_handle&) = delete;
    load_handle& operator=(const load_handle&) = delete;
    // Move constructor
    load_handle(load_handle&& rhs) :
        handle_slot_(std::exchange(rhs.handle_slot_, nullptr))
    {}
    // Move assignment operator
    load_handle& operator=(load_handle&& rhs)
    {
        handle_slot_ = std::exchange(rhs.handle_slot_, nullptr);
        return *this;
    }

    // Checks if the corresponding load is complete
    bool is_finished();
    // Waits for the corresponding load to complete
    void* join();
private:
    std::unique_ptr<LoadRxSlot, LoadRxSlotDeleter> handle_slot_;
    load_handle(LoadRxSlot *handle_slot) :
        handle_slot_(handle_slot)
    {}

    friend load_handle start_data_load(void*, uint32_t, uint32_t);
};

#endif
