#include <ultra64.h>

#include <grid.h>
#include <cstring>
#include <files.h>
#include <camera.h>

#include <n64_gfx.h>

extern "C"
{
#include <debug.h>
}

struct filerecord { const char *path; uint32_t offset; uint32_t size; };

class FileRecords
{
private:
  static inline unsigned int hash (const char *str, size_t len);
public:
  static const struct filerecord *get_offset (const char *str, size_t len);
};

extern uint8_t _assetsSegmentStart[];
// extern OSPiHandle *g_romHandle;

// void load_asset_data(void *ret, uint32_t rom_pos, uint32_t size)
// {
//     OSMesgQueue queue;
//     OSMesg msg;
//     OSIoMesg io_msg;
//     // Set up the intro segment DMA
//     io_msg.hdr.pri = OS_MESG_PRI_NORMAL;
//     io_msg.hdr.retQueue = &queue;
//     io_msg.dramAddr = ret;
//     io_msg.devAddr = (u32)(_assetsSegmentStart + rom_pos);
//     io_msg.size = (u32)size;
//     osCreateMesgQueue(&queue, &msg, 1);
//     osEPiStartDma(g_romHandle, &io_msg, OS_READ);
//     osRecvMesg(&queue, nullptr, OS_MESG_BLOCK);
// }

void GridDefinition::get_chunk_offset_size(unsigned int x, unsigned int z, uint32_t *offset, uint32_t *size)
{
    unsigned int chunk_index = z + num_chunks_z * x;
    uint32_t rom_pos = chunk_index * 2 * sizeof(uint32_t) + chunk_array_rom_offset + (uint32_t)_assetsSegmentStart;
    uint32_t offset_tmp;
    osPiReadIo(rom_pos + 0 * sizeof(uint32_t), &offset_tmp);
    osPiReadIo(rom_pos + 1 * sizeof(uint32_t), size);
    *offset = offset_tmp + chunk_array_rom_offset;        
}

GridDefinition get_grid_definition(const char *file)
{
    // 16 bytes to DMA into as recommended by osEPiStartDma manual entry
    uint8_t buf[16] __attribute__((aligned(16)));
    GridDefinition ret;
    auto grid_def_offset = FileRecords::get_offset(file, strlen(file));
    load_data(buf, (u32)(_assetsSegmentStart + grid_def_offset->offset), sizeof(buf));
    ret = *(GridDefinition*)buf;
    ret.adjust_offsets(grid_def_offset->offset);
    return ret;
}

bool Grid::is_loaded_or_loading(chunk_pos pos)
{
    for (auto& entry : loading_chunks_)
    {
        if (entry.first == pos)
        {
            return true;
        }
    }
    for (auto& entry : loaded_chunks_)
    {
        if (entry.pos == pos)
        {
            return true;
        }
    }
    return false;
}

inline bool between_inclusive(int min, int max, int val)
{
    return val >= min && val <= max;
}

void Grid::get_loaded_chunks_in_area(int min_chunk_x, int min_chunk_z, int max_chunk_x, int max_chunk_z, bool* found)
{
    int num_chunks_z = max_chunk_z - min_chunk_z + 1;

    // debug_printf("%d chunks currently loading\n", loading_chunks_.size());
    for (auto& entry : loading_chunks_)
    {
        int x = entry.first.first;
        int z = entry.first.second;
        if (between_inclusive(min_chunk_x, max_chunk_x, x) && between_inclusive(min_chunk_z, max_chunk_z, z))
        {
            // debug_printf("  Chunk {%d, %d} currently loading\n", x, z);
            found[(z - min_chunk_z) + (x - min_chunk_x) * num_chunks_z] = true;
        }
    }
    // debug_printf("%d chunks loaded\n", loaded_chunks_.size());
    for (auto& entry : loaded_chunks_)
    {
        int x = entry.pos.first;
        int z = entry.pos.second;
        if (between_inclusive(min_chunk_x, max_chunk_x, x) && between_inclusive(min_chunk_z, max_chunk_z, z))
        {
            // debug_printf("  Chunk {%d, %d} is loaded\n", x, z);
            found[(z - min_chunk_z) + (x - min_chunk_x) * num_chunks_z] = true;
        }
    }
}

void Grid::load_visible_chunks(Camera& camera)
{
    int pos_x = static_cast<int>(camera.target[0]);
    int pos_z = static_cast<int>(camera.target[2]);
    int min_chunk_x = round_down_divide<chunk_size * 256>(pos_x - 256);
    min_chunk_x = std::clamp(min_chunk_x, 0, definition_.num_chunks_x - 1);

    int max_chunk_x = round_down_divide<chunk_size * 256>(pos_x + 256);
    max_chunk_x = std::clamp(max_chunk_x, 0, definition_.num_chunks_x - 1);

    int min_chunk_z = round_down_divide<chunk_size * 256>(pos_z - 256);
    min_chunk_z = std::clamp(min_chunk_z, 0, definition_.num_chunks_z - 1);
    
    int max_chunk_z = round_down_divide<chunk_size * 256>(pos_z + 256);
    max_chunk_z = std::clamp(max_chunk_z, 0, definition_.num_chunks_z - 1);
    
    // debug_printf("Visible chunk bounds: [%d, %d], [%d, %d]\n", min_chunk_x, max_chunk_x, min_chunk_z, max_chunk_z);
    
    int num_chunks_x = max_chunk_x - min_chunk_x + 1;
    int num_chunks_z = max_chunk_z - min_chunk_z + 1;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla" // I know what I'm doing
    bool mem[num_chunks_x * num_chunks_z];
#pragma GCC diagnostic pop
    memset(mem, 0, num_chunks_x * num_chunks_z);
    get_loaded_chunks_in_area(min_chunk_x, min_chunk_z, max_chunk_x, max_chunk_z, mem);

    for (int x = 0; x < num_chunks_x; x++)
    {
        for (int z = 0; z < num_chunks_z; z++)
        {
            if (!mem[x * num_chunks_z + z])
            {
                // debug_printf("Loading chunk %d, %d\n", x + min_chunk_x, z + min_chunk_z);
                load_chunk({x + min_chunk_x, z + min_chunk_z});
            }
        }
    }
}

void Grid::unload_nonvisible_chunks(Camera& camera)
{
    int pos_x = static_cast<int>(camera.target[0]);
    int pos_z = static_cast<int>(camera.target[2]);
    int min_chunk_x = round_down_divide<chunk_size * 256>(pos_x - 320);
    min_chunk_x = std::clamp(min_chunk_x, 0, definition_.num_chunks_x - 1);

    int max_chunk_x = round_down_divide<chunk_size * 256>(pos_x + 320);
    max_chunk_x = std::clamp(max_chunk_x, 0, definition_.num_chunks_x - 1);

    int min_chunk_z = round_down_divide<chunk_size * 256>(pos_z - 320);
    min_chunk_z = std::clamp(min_chunk_z, 0, definition_.num_chunks_z - 1);
    
    int max_chunk_z = round_down_divide<chunk_size * 256>(pos_z + 320);
    max_chunk_z = std::clamp(max_chunk_z, 0, definition_.num_chunks_z - 1);

    for (auto it = loaded_chunks_.begin(); it != loaded_chunks_.end(); )
    {
        int chunk_x = it->pos.first;
        int chunk_z = it->pos.second;
        if (!between_inclusive(min_chunk_x, max_chunk_x, chunk_x) || !between_inclusive(min_chunk_z, max_chunk_z, chunk_z))
        {
            // debug_printf("Unloading nonvisible chunk {%d, %d}\n", chunk_x, chunk_z);
            it = unload_chunk(it);
        }
        else
        {
            ++it;
        }
    }
}

void Grid::load_chunk(chunk_pos pos)
{
    uint32_t chunk_offset, chunk_length;
    definition_.get_chunk_offset_size(pos.first, pos.second, &chunk_offset, &chunk_length);

    // If the currently loading chunks skipfield is full, continuously process loading chunks until a slot opens up
    while (loading_chunks_.full())
    {
        process_loading_chunks();
    }

    LoadHandle handle = start_data_load(nullptr, (u32)(_assetsSegmentStart + chunk_offset), chunk_length);
    loading_chunks_.emplace(pos, std::move(handle));


    // Chunk *chunk = (Chunk*)load_data(nullptr, (u32)(_assetsSegmentStart + chunk_offset), chunk_length);
    // chunk->adjust_offsets();

    // loaded_chunks_.emplace({pos, std::unique_ptr<Chunk, alloc_deleter>(chunk)});
}

extern "C" {
#include <debug.h>
}

void Grid::process_loading_chunks()
{
    int i = 0;
    // If there's no room for any more chunks, then don't bother checking the loading chunks
    if (loaded_chunks_.full())
    {
        // debug_printf("No room to load chunks\n");
        return;
    }
    for (auto it = loading_chunks_.begin(); it != loading_chunks_.end();)
    {
        if (it->second.is_finished() && !loaded_chunks_.full())
        {
            Chunk *chunk = reinterpret_cast<Chunk*>(it->second.join());
            // debug_printf("Chunk %08X {%d, %d} finished loading\n", chunk, it->first.first, it->first.second);
            chunk->adjust_offsets();
            std::unique_ptr<Chunk, alloc_deleter> chunk_ptr{chunk};
            loaded_chunks_.emplace(it->first, std::move(chunk_ptr));
            it = loading_chunks_.erase(it);
            i++;
            if (loaded_chunks_.full()) break;
        }
        else
        {
            ++it;
        }
    }
    if (i != 0)
    {
        // debug_printf("Chunks loaded: %d\n", i);
    }
}

float tile_rotations[] = {
    0.0f, 90.0f, 180.0f, 270.0f
};

MtxF tile_rotation_matrices[] = {
    {
        { 2.56f,  0.0f,  0.0f,  0.0f},
        {  0.0f, 2.56f,  0.0f,  0.0f},
        {  0.0f,  0.0f, 2.56f,  0.0f},
        {  0.0f,  0.0f,  0.0f,  1.0f},
    },
    {
        {  0.0f,  0.0f, 2.56f,  0.0f},
        {  0.0f, 2.56f,  0.0f,  0.0f},
        {-2.56f,  0.0f,  0.0f,  0.0f},
        {  0.0f,  0.0f,  0.0f,  1.0f},
    },
    {
        {-2.56f,  0.0f,  0.0f,  0.0f},
        {  0.0f, 2.56f,  0.0f,  0.0f},
        {  0.0f,  0.0f,-2.56f,  0.0f},
        {  0.0f,  0.0f,  0.0f,  1.0f},
    },
    {
        {  0.0f,  0.0f,-2.56f,  0.0f},
        {  0.0f, 2.56f,  0.0f,  0.0f},
        { 2.56f,  0.0f,  0.0f,  0.0f},
        {  0.0f,  0.0f,  0.0f,  1.0f},
    },
};

void Grid::draw()
{
    for (auto& entry : loaded_chunks_)
    {
        // debug_printf("Drawing chunk {%d, %d}\n", entry.pos.first, entry.pos.second);
        float chunk_world_x = entry.pos.first * (256.0f * chunk_size);
        float chunk_world_z = entry.pos.second * (256.0f * chunk_size);
        auto& chunk = entry.chunk;
        float cur_x = chunk_world_x;
        for (unsigned int x = 0; x < chunk_size; x++)
        {
            float cur_z = chunk_world_z;
            for (unsigned int z = 0; z < chunk_size; z++)
            {
                const ChunkColumn &col = chunk->columns[x][z];
                unsigned int tile_idx;
                int y;
                for (tile_idx = 0, y = col.base_height; y < col.base_height + col.num_tiles; tile_idx++, y++)
                {
                    const auto& tile = col.tiles[tile_idx];
                    if (tile.id != 0xFF)
                    {
                        // Combine the rotation and translation into one matrix to cut down on matrix multiplications
                        MtxF cur_mat;
                        memcpy(cur_mat, tile_rotation_matrices[tile.rotation], sizeof(MtxF));
                        cur_mat[3][0] = cur_x;
                        cur_mat[3][1] = 256.0f * y;
                        cur_mat[3][2] = cur_z;
                        gfx::push_mat();
                          gfx::apply_matrix(&cur_mat);
                          drawModel(tile_types_[tile.id].model, nullptr, 0);
                        gfx::pop_mat();
                    }
                }
                cur_z += 256.0f;
            }
            cur_x += 256.0f;
        }
    }
}
