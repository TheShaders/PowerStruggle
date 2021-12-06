#include <ultra64.h>

#include <cmath>

#include <grid.h>
#include <cstring>
#include <files.h>
#include <camera.h>
#include <behaviors.h>

#include <n64_gfx.h>
#include <n64_mathutils.h>

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
    // 16 byte DMA width alignment as recommended by osEPiStartDma manual entry
    uint8_t buf[16];
    GridDefinition ret;
    auto grid_def_offset = FileRecords::get_offset(file, strlen(file));
    load_data(buf, (u32)(_assetsSegmentStart + grid_def_offset->offset), sizeof(buf));
    ret = *(GridDefinition*)buf;
    ret.adjust_offsets(grid_def_offset->offset);
    return ret;
}

bool Grid::is_loaded(chunk_pos pos)
{
    for (auto& entry : loaded_chunks_)
    {
        if (entry.pos == pos)
        {
            return true;
        }
    }
    return false;
}

bool Grid::is_pos_loaded(float x, float z)
{
    int32_t x_cell = round_down_divide<tile_size>(lround(x));
    int32_t z_cell = round_down_divide<tile_size>(lround(z));

    int32_t x_chunk = round_down_divide<chunk_size>(x_cell);
    int32_t z_chunk = round_down_divide<chunk_size>(z_cell);

    return is_loaded({x_chunk, z_chunk});
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
    int min_chunk_x = round_down_divide<chunk_size * tile_size>(pos_x - visible_inner_range_x);
    min_chunk_x = std::clamp(min_chunk_x, 0, definition_.num_chunks_x - 1);

    int max_chunk_x = round_down_divide<chunk_size * tile_size>(pos_x + visible_inner_range_x);
    max_chunk_x = std::clamp(max_chunk_x, 0, definition_.num_chunks_x - 1);

    int min_chunk_z = round_down_divide<chunk_size * tile_size>(pos_z - visible_inner_range_neg_z);
    min_chunk_z = std::clamp(min_chunk_z, 0, definition_.num_chunks_z - 1);
    
    int max_chunk_z = round_down_divide<chunk_size * tile_size>(pos_z + visible_inner_range_pos_z);
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
                debug_printf("Loading chunk %d, %d\n", x + min_chunk_x, z + min_chunk_z);
                load_chunk({x + min_chunk_x, z + min_chunk_z});
            }
        }
    }
}

void Grid::unload_nonvisible_chunks(Camera& camera)
{
    int pos_x = static_cast<int>(camera.target[0]);
    int pos_z = static_cast<int>(camera.target[2]);
    int min_chunk_x = round_down_divide<chunk_size * tile_size>(pos_x - visible_outer_range_x);
    min_chunk_x = std::clamp(min_chunk_x, 0, definition_.num_chunks_x - 1);

    int max_chunk_x = round_down_divide<chunk_size * tile_size>(pos_x + visible_outer_range_x);
    max_chunk_x = std::clamp(max_chunk_x, 0, definition_.num_chunks_x - 1);

    int min_chunk_z = round_down_divide<chunk_size * tile_size>(pos_z - visible_outer_range_neg_z);
    min_chunk_z = std::clamp(min_chunk_z, 0, definition_.num_chunks_z - 1);
    
    int max_chunk_z = round_down_divide<chunk_size * tile_size>(pos_z + visible_outer_range_pos_z);
    max_chunk_z = std::clamp(max_chunk_z, 0, definition_.num_chunks_z - 1);

    for (auto it = loaded_chunks_.begin(); it != loaded_chunks_.end(); )
    {
        int chunk_x = it->pos.first;
        int chunk_z = it->pos.second;
        if (!between_inclusive(min_chunk_x, max_chunk_x, chunk_x) || !between_inclusive(min_chunk_z, max_chunk_z, chunk_z))
        {
            debug_printf("Unloading nonvisible chunk {%d, %d}\n", chunk_x, chunk_z);
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

constinit Mtx fixed_tile_rotation_matrices[] = {
    float_to_fixed(
    {
        { 2.56f,  0.0f,  0.0f,  0.0f},
        {  0.0f, 2.56f,  0.0f,  0.0f},
        {  0.0f,  0.0f, 2.56f,  0.0f},
        {  0.0f,  0.0f,  0.0f,  1.0f},
    }),
    float_to_fixed({
        {  0.0f,  0.0f, 2.56f,  0.0f},
        {  0.0f, 2.56f,  0.0f,  0.0f},
        {-2.56f,  0.0f,  0.0f,  0.0f},
        {  0.0f,  0.0f,  0.0f,  1.0f},
    }),
    float_to_fixed({
        {-2.56f,  0.0f,  0.0f,  0.0f},
        {  0.0f, 2.56f,  0.0f,  0.0f},
        {  0.0f,  0.0f,-2.56f,  0.0f},
        {  0.0f,  0.0f,  0.0f,  1.0f},
    }),
    float_to_fixed({
        {  0.0f,  0.0f,-2.56f,  0.0f},
        {  0.0f, 2.56f,  0.0f,  0.0f},
        { 2.56f,  0.0f,  0.0f,  0.0f},
        {  0.0f,  0.0f,  0.0f,  1.0f},
    }),
};

// Copies the first 3 rows of the input matrix into the output one (the rows containing the rotation and scale)
// Doubleword sized copies instead of memcpy, which is 1 byte at a time with the implementation available
// Word copies are just as fast because of 32-bit data bus, but this is fewer instructions and so better icache usage
void copy_rotation_scale(Mtx* out, Mtx* in)
{
    uint64_t *out_int = (uint64_t*)&out->m[0][0];
    uint64_t *in_int = (uint64_t*)&in->m[0][0];
    uint64_t a;
    uint64_t b;
    uint64_t c;

    __asm__ __volatile__(".set gp=64");
    __asm__ __volatile__("ld %0,  0(%1)" : "=r"(a) : "r"(in_int));
    __asm__ __volatile__("ld %0,  8(%1)" : "=r"(b) : "r"(in_int));
    __asm__ __volatile__("ld %0, 16(%1)" : "=r"(c) : "r"(in_int));
    __asm__ __volatile__("sd %0,  0(%1)" : : "r"(a), "r"(out_int));
    __asm__ __volatile__("sd %0,  8(%1)" : : "r"(b), "r"(out_int));
    __asm__ __volatile__("sd %0, 16(%1)" : : "r"(c), "r"(out_int));
    __asm__ __volatile__("ld %0, 32(%1)" : "=r"(a) : "r"(in_int));
    __asm__ __volatile__("ld %0, 40(%1)" : "=r"(b) : "r"(in_int));
    __asm__ __volatile__("ld %0, 48(%1)" : "=r"(c) : "r"(in_int));
    __asm__ __volatile__("sd %0, 32(%1)" : : "r"(a), "r"(out_int));
    __asm__ __volatile__("sd %0, 40(%1)" : : "r"(b), "r"(out_int));
    __asm__ __volatile__("sd %0, 48(%1)" : : "r"(c), "r"(out_int));
    __asm__ __volatile__(".set gp=32");
}

std::array<block_vector<Mtx>, num_frame_buffers> get_matrices(Chunk* chunk)
{
    std::array<block_vector<Mtx>, num_frame_buffers> ret{};

    for (unsigned int x = 0; x < chunk_size; x++)
    {
        for (unsigned int z = 0; z < chunk_size; z++)
        {
            const ChunkColumn &col = chunk->columns[x][z];
            unsigned int tile_idx;
            for (tile_idx = 0; tile_idx < col.num_tiles; tile_idx++)
            {
                const auto& tile = col.tiles[tile_idx];
                if (tile.id != 0xFF)
                {
                    for (unsigned int i = 0; i < num_frame_buffers; i++)
                    {
                        // Allocate the matrix
                        Mtx* curMtx = &(*ret[i].emplace_back());
                        // Copy from the template matrix based on the rotation
                        copy_rotation_scale(curMtx, &fixed_tile_rotation_matrices[tile.rotation]);
                        curMtx->m[3][2] = 0;
                        curMtx->m[3][3] = 0;
                    }
                }
            }
        }
    }

    return ret;
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
            debug_printf("Chunk %08X {%d, %d} finished loading\n", chunk, it->first.first, it->first.second);
            chunk->adjust_offsets();
            std::unique_ptr<Chunk, alloc_deleter> chunk_ptr{chunk};
            std::array<block_vector<Mtx>, num_frame_buffers> chunk_matrices = get_matrices(chunk);
            loaded_chunks_.emplace(it->first, std::move(chunk_ptr), ChunkGfx{std::move(chunk_matrices)});
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

void drawTileModel(Model *toDraw, Mtx* curMtx);

void Grid::draw(Camera *camera)
{
    int pos_x = static_cast<int>(g_Camera.target[0]) - camera->model_offset[0];
    int pos_z = static_cast<int>(g_Camera.target[2]) - camera->model_offset[2];
    int min_draw_x = pos_x - visible_inner_range_x;
    int max_draw_x = pos_x + visible_inner_range_x;
    int min_draw_z = pos_z - visible_inner_range_neg_z;
    int max_draw_z = pos_z + visible_inner_range_pos_z;
    for (auto& entry : loaded_chunks_)
    {
        // debug_printf("Drawing chunk {%d, %d}\n", entry.pos.first, entry.pos.second);
        int32_t chunk_world_x = entry.pos.first  * static_cast<int32_t>(tile_size * chunk_size) + tile_size / 2 - camera->model_offset[0];
        int32_t chunk_world_z = entry.pos.second * static_cast<int32_t>(tile_size * chunk_size) + tile_size / 2 - camera->model_offset[2];
        auto& chunk = entry.chunk;
        int32_t cur_x = chunk_world_x;
        auto& cur_matrices = entry.gfx.matrices[g_curGfxContext];
        auto mtx_iter = cur_matrices.begin();
        for (unsigned int x = 0; x < chunk_size; x++)
        {
            bool outside_x = cur_x > max_draw_x || cur_x < min_draw_x;
            int32_t cur_z = chunk_world_z;
            for (unsigned int z = 0; z < chunk_size; z++)
            {
                bool outside_z = cur_z > max_draw_z || cur_z < min_draw_z;
                const ChunkColumn &col = chunk->columns[x][z];
                unsigned int tile_idx;
                int y;
                for (tile_idx = 0, y = col.base_height; y < col.base_height + col.num_tiles; tile_idx++, y++)
                {
                    const auto& tile = col.tiles[tile_idx];
                    if (tile.id != 0xFF)
                    {
                        Mtx* cur_mtx = &(*mtx_iter);

                        if (!outside_x && !outside_z)
                        {
                            // Set up the translation in the matrix
                            cur_mtx->m[1][2] = ((uint32_t)cur_x & 0xFFFF) << 16 | ((uint32_t)(y * tile_size) & 0xFFFF);
                            cur_mtx->m[1][3] = ((uint32_t)cur_z & 0xFFFF) << 16 | (1 & 0xFFFF);

                            drawTileModel(tile_types_[tile.id].model, cur_mtx);
                        }

                        ++mtx_iter;
                    }
                }
                cur_z += tile_size;
            }
            cur_x += tile_size;
        }
        for (auto block_it = cur_matrices.blocks_begin(); block_it != cur_matrices.blocks_end(); ++block_it)
        {
            osWritebackDCache(&(*block_it), mem_block_size);
        }
    }
}

float Grid::get_height(float x, float z, float radius, float min_y, float max_y)
{
    int x_int = lround(x);
    int z_int = lround(z);
    // Default to an unreasonably low grid height
    float found_y = std::numeric_limits<decltype(ChunkColumn::base_height)>::min() * static_cast<float>(tile_size);

    // debug_printf("get_height([%5.2f, %5.2f], %5.2f, [%5.2f, %5.2f])\n", x, z, radius, min_y, max_y);
    // debug_printf("-> pos (%d, %d)\n", x_int, z_int);

    // This would be 1 more, but we need to find slopes too which can take up the entire tile they're in
    int min_pos_y = round_down_divide<tile_size>(lfloor(min_y));
    int max_pos_y = round_down_divide<tile_size>(lceil(max_y));

    // debug_printf("  y tile bounds: [%d, %d]\n", min_pos_y, max_pos_y);

    if (min_pos_y > max_pos_y) return found_y; // If no tile boundaries are crossed, there's nothing that can be found

    // Get the positional bounds of the query
    int min_x = lfloor(x - radius);
    int max_x = lceil(x + radius);
    int min_z = lfloor(z - radius);
    int max_z = lceil(z + radius);

    // Get the tile bounds of the query
    int min_pos_x = round_down_divide<tile_size>(min_x);
    int max_pos_x = round_down_divide<tile_size>(max_x);
    int min_pos_z = round_down_divide<tile_size>(min_z);
    int max_pos_z = round_down_divide<tile_size>(max_z);

    // Get the chunk bounds of the query
    int min_chunk_x = round_down_divide<chunk_size>(min_pos_x);
    int max_chunk_x = round_down_divide<chunk_size>(max_pos_x);
    int min_chunk_z = round_down_divide<chunk_size>(min_pos_z);
    int max_chunk_z = round_down_divide<chunk_size>(max_pos_z);

    // Convert the tile bounds to exclusive upper bounds
    max_pos_x += 1;
    max_pos_y += 1;
    max_pos_z += 1;

    // debug_printf("  Looking for tiles in region [%d, %d], (%d, %d)\n", min_pos_x, min_pos_z, max_pos_x, max_pos_z);
    
    // Iterate over every loaded chunk to see which ones exist that contain tiles in the query
    for (const auto& chunk_entry : loaded_chunks_)
    {
        // Get the position of this chunk
        int chunk_x = chunk_entry.pos.first;
        int chunk_z = chunk_entry.pos.second;
        // Check if the current chunk is within the calculated chunk bounds
        if (between_inclusive(min_chunk_x, max_chunk_x, chunk_x) && between_inclusive(min_chunk_z, max_chunk_z, chunk_z))
        {
            // debug_printf("    Chunk %d, %d intersects with the region\n", chunk_x, chunk_z);
            // Get the tile bounds of this chunk
            int cur_chunk_min_x = chunk_x * chunk_size;
            int cur_chunk_max_x = cur_chunk_min_x + chunk_size;
            int cur_chunk_min_z = chunk_z * chunk_size;
            int cur_chunk_max_z = cur_chunk_min_z + chunk_size;

            // Get the tile bounds of the intersection of the chunk and the query
            int cur_check_min_x = std::max(cur_chunk_min_x, min_pos_x);
            int cur_check_max_x = std::min(cur_chunk_max_x, max_pos_x);
            int cur_check_min_z = std::max(cur_chunk_min_z, min_pos_z);
            int cur_check_max_z = std::min(cur_chunk_max_z, max_pos_z);

            // Convert the bounds to local coordinates in the chunk
            int cur_local_min_x = cur_check_min_x - cur_chunk_min_x;
            int cur_local_max_x = cur_check_max_x - cur_chunk_min_x;
            int cur_local_min_z = cur_check_min_z - cur_chunk_min_z;
            int cur_local_max_z = cur_check_max_z - cur_chunk_min_z;

            // debug_printf("    Checking local tile region [%d, %d], (%d %d)\n", cur_local_min_x, cur_local_min_z, cur_local_max_x, cur_local_max_z);

            const auto& chunk_columns = chunk_entry.chunk->columns;

            // Check the columns in the current chunk to see if they contain any floors in the given bounds
            for (int local_tile_x = cur_local_min_x; local_tile_x < cur_local_max_x; local_tile_x++)
            {
                for (int local_tile_z = cur_local_min_z; local_tile_z < cur_local_max_z; local_tile_z++)
                {
                    const auto& cur_column = chunk_columns[local_tile_x][local_tile_z];

                    // Get the start and end tiles to search through in this column
                    int min_tile_index = std::max(0, min_pos_y - cur_column.base_height);
                    int num_tiles = std::min(cur_column.num_tiles - min_tile_index, max_pos_y + 1 - cur_column.base_height);

                    int tile_x = (local_tile_x + cur_chunk_min_x) * static_cast<int>(tile_size);
                    int tile_z = (local_tile_z + cur_chunk_min_z) * static_cast<int>(tile_size);
                    int local_x = x_int - tile_x;
                    int local_z = z_int - tile_z;

                    // debug_printf("      Checking tiles %d through %d in column [%d, %d]\n", min_tile_index, num_tiles, local_tile_x, local_tile_z);
                    // debug_printf("      -> local (%d, %d)\n", local_x, local_z);

                    for (int tile_index = min_tile_index + num_tiles - 1; tile_index >= min_tile_index; tile_index--)
                    {
                        int tile_id = cur_column.tiles[tile_index].id;
                        int rotation = cur_column.tiles[tile_index].rotation;
                        if (tile_id != -1)
                        {
                            TileCollision tile_collision = tile_types_[tile_id].flags;
                            int tile_y = tile_index + cur_column.base_height;
                            float tile_y_world;
                            int tile_bottom = tile_y * static_cast<int>(tile_size);
                            switch (tile_collision)
                            {
                                case TileCollision::floor:
                                    tile_y_world = static_cast<float>(tile_bottom);
                                    break;
                                case TileCollision::slope:
                                    {
                                        int radius_int = lround(radius);
                                        int slope_y;
                                        switch (rotation)
                                        {
                                            default:
                                                slope_y = static_cast<int>(tile_size) + tile_bottom - local_z;
                                                break;
                                            case 1:
                                                slope_y = tile_bottom + local_x;
                                                break;
                                            case 2:
                                                slope_y = tile_bottom + local_z;
                                                break;
                                            case 3:
                                                slope_y = static_cast<int>(tile_size) + tile_bottom - local_x;
                                                break;
                                        }
                                        slope_y += radius_int;
                                        if (slope_y > tile_bottom && slope_y < (tile_bottom + static_cast<int>(tile_size)))
                                        {
                                            tile_y_world = static_cast<float>(slope_y);
                                        }
                                        else
                                        {
                                            tile_y_world = static_cast<float>(tile_bottom);
                                        }
                                    }
                                    break;
                                default:
                                    tile_y_world = found_y;
                                    break;
                            }
                            if (tile_y_world > found_y && tile_y_world >= min_y && tile_y_world < max_y)
                            {
                                // debug_printf("        Found new max height at %5.2f\n", tile_y_world);
                                found_y = tile_y_world;
                                // This is the highest tile in the column that fits the query, so don't check any more in the column
                                break;
                            }
                        }
                    }

                    
                    // const auto& cur_column = chunk_columns[x][z];

                    // // Get the start and end tiles to search through in this column
                    // int min_tile_index = std::max(0, min_pos_y - cur_column.base_height);
                    // int max_tile_index = std::min(static_cast<int>(cur_column.num_tiles), max_pos_y - cur_column.base_height);

                    // debug_printf("      Checking tiles [%d, %d) in column [%d, %d]\n", min_tile_index, max_tile_index, x, z);

                    // for (int tile_index = max_tile_index - 1; tile_index >= min_tile_index; tile_index--)
                    // {
                    //     int tile_id = cur_column.tiles[tile_index].id;
                    //     if (tile_id != -1 && tile_types_[tile_id].flags == TileCollision::floor)
                    //     {
                    //         int y = tile_index + cur_column.base_height;

                    //         if (y > found_y)
                    //         {
                    //             debug_printf("        Found new max height at %d\n", y);
                    //             found_y = y;
                    //         }

                    //         // This is the highest tile in the column that fits the query, so don't check any more in the column
                    //         break;
                    //     }
                    // }
                }
            }
        }
        else
        {
            // debug_printf("    Chunk %d, %d does not intersect with the region\n", chunk_x, chunk_z);
        }
    }

    // debug_printf("-> found_y: %d\n", found_y);
    return found_y;
}

int circle_aabb_intersect(float x, float z, float min_x, float max_x, float min_z, float max_z, float rad_sq, float* dist_out, Vec3 hit_out)
{
    float closest_x = std::max(min_x, std::min(x, max_x));
    float closest_z = std::max(min_z, std::min(z, max_z));
    float dx = closest_x - x;
    float dz = closest_z - z;
    float dist_sq = dx * dx + dz * dz;
    if (dist_sq < rad_sq)
    {
        *dist_out = sqrtf(dist_sq);
        hit_out[0] = closest_x;
        hit_out[2] = closest_z;
        return true;
    }
    return false;
}

int Grid::get_wall_collisions(Vec3 hits[4], float dists[4], float x, float z, float radius, float min_y, float max_y)
{
    int num_hits = 0;
    float rad_sq = radius * radius;

    // debug_printf("get_height([%5.2f, %5.2f], %5.2f, [%5.2f, %5.2f])\n", x, z, radius, min_y, max_y);
    // debug_printf("-> pos (%d, %d)\n", x_int, z_int);

    // This would be 1 more, but we need to find slopes too which can take up the entire tile they're in
    int min_pos_y = round_down_divide<tile_size>(lfloor(min_y));
    int max_pos_y = round_down_divide<tile_size>(lceil(max_y));

    // debug_printf("  y tile bounds: [%d, %d]\n", min_pos_y, max_pos_y);

    if (min_pos_y > max_pos_y) return 0; // If no tile boundaries are crossed, there's nothing that can be found

    // Get the positional bounds of the query
    int min_x = lfloor(x - radius);
    int max_x = lceil(x + radius);
    int min_z = lfloor(z - radius);
    int max_z = lceil(z + radius);

    // Get the tile bounds of the query
    int min_pos_x = round_down_divide<tile_size>(min_x);
    int max_pos_x = round_down_divide<tile_size>(max_x);
    int min_pos_z = round_down_divide<tile_size>(min_z);
    int max_pos_z = round_down_divide<tile_size>(max_z);

    // Get the chunk bounds of the query
    int min_chunk_x = round_down_divide<chunk_size>(min_pos_x);
    int max_chunk_x = round_down_divide<chunk_size>(max_pos_x);
    int min_chunk_z = round_down_divide<chunk_size>(min_pos_z);
    int max_chunk_z = round_down_divide<chunk_size>(max_pos_z);

    // Convert the tile bounds to exclusive upper bounds
    max_pos_x += 1;
    max_pos_y += 1;
    max_pos_z += 1;

    // debug_printf("  Looking for tiles in region [%d, %d], (%d, %d)\n", min_pos_x, min_pos_z, max_pos_x, max_pos_z);
    
    // Iterate over every loaded chunk to see which ones exist that contain tiles in the query
    for (const auto& chunk_entry : loaded_chunks_)
    {
        // Get the position of this chunk
        int chunk_x = chunk_entry.pos.first;
        int chunk_z = chunk_entry.pos.second;
        // Check if the current chunk is within the calculated chunk bounds
        if (between_inclusive(min_chunk_x, max_chunk_x, chunk_x) && between_inclusive(min_chunk_z, max_chunk_z, chunk_z))
        {
            // debug_printf("    Chunk %d, %d intersects with the region\n", chunk_x, chunk_z);
            // Get the tile bounds of this chunk
            int cur_chunk_min_x = chunk_x * chunk_size;
            int cur_chunk_max_x = cur_chunk_min_x + chunk_size;
            int cur_chunk_min_z = chunk_z * chunk_size;
            int cur_chunk_max_z = cur_chunk_min_z + chunk_size;

            // Get the tile bounds of the intersection of the chunk and the query
            int cur_check_min_x = std::max(cur_chunk_min_x, min_pos_x);
            int cur_check_max_x = std::min(cur_chunk_max_x, max_pos_x);
            int cur_check_min_z = std::max(cur_chunk_min_z, min_pos_z);
            int cur_check_max_z = std::min(cur_chunk_max_z, max_pos_z);

            // Convert the bounds to local coordinates in the chunk
            int cur_local_min_x = cur_check_min_x - cur_chunk_min_x;
            int cur_local_max_x = cur_check_max_x - cur_chunk_min_x;
            int cur_local_min_z = cur_check_min_z - cur_chunk_min_z;
            int cur_local_max_z = cur_check_max_z - cur_chunk_min_z;

            // debug_printf("    Checking local tile region [%d, %d], (%d %d)\n", cur_local_min_x, cur_local_min_z, cur_local_max_x, cur_local_max_z);

            const auto& chunk_columns = chunk_entry.chunk->columns;

            // Check the columns in the current chunk to see if they contain any floors in the given bounds
            for (int local_tile_x = cur_local_min_x; local_tile_x < cur_local_max_x; local_tile_x++)
            {
                for (int local_tile_z = cur_local_min_z; local_tile_z < cur_local_max_z; local_tile_z++)
                {
                    const auto& cur_column = chunk_columns[local_tile_x][local_tile_z];

                    // Get the start and end tiles to search through in this column
                    int min_tile_index = std::max(0, min_pos_y - cur_column.base_height);
                    int num_tiles = std::min(cur_column.num_tiles - min_tile_index, max_pos_y + 1 - cur_column.base_height);

                    int tile_x = (local_tile_x + cur_chunk_min_x) * static_cast<int>(tile_size);
                    int tile_z = (local_tile_z + cur_chunk_min_z) * static_cast<int>(tile_size);

                    // debug_printf("      Checking tiles %d through %d in column [%d, %d]\n", min_tile_index, num_tiles, local_tile_x, local_tile_z);
                    // debug_printf("      -> local (%d, %d)\n", local_x, local_z);

                    for (int tile_index = min_tile_index + num_tiles - 1; tile_index >= min_tile_index; tile_index--)
                    {
                        int tile_id = cur_column.tiles[tile_index].id;
                        if (tile_id != -1)
                        {
                            TileCollision tile_collision = tile_types_[tile_id].flags;
                            if (tile_collision == TileCollision::wall)
                            {
                                int tile_y = tile_index + cur_column.base_height;
                                float tile_y_world = tile_y * static_cast<int>(tile_size);
                                if (tile_y_world >= min_y && tile_y_world < max_y)
                                {

                                    if (circle_aabb_intersect(
                                        x, z,
                                        tile_x, tile_x + static_cast<int>(tile_size),
                                        tile_z, tile_z + static_cast<int>(tile_size),
                                        rad_sq, &dists[num_hits], hits[num_hits]))
                                    {
                                        num_hits++;
                                        hits[num_hits][1] = tile_y_world;
                                        if (num_hits >= 4)
                                        {
                                            return num_hits;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // debug_printf("    Chunk %d, %d does not intersect with the region\n", chunk_x, chunk_z);
        }
    }

    return num_hits;
}

chunk_pos Grid::get_minimum_loaded_chunk()
{
    chunk_pos ret {std::numeric_limits<decltype(chunk_pos::first)>::max(), std::numeric_limits<decltype(chunk_pos::second)>::max()};
    for (const auto& entry : loaded_chunks_)
    {
        if (entry.pos.first < ret.first)
        {
            ret.first = entry.pos.first;
        }
        if (entry.pos.second < ret.second)
        {
            ret.second = entry.pos.second;
        }
    }
    return ret;
}

void Grid::load_objects()
{
    if (definition_.num_objects == 0) return;
    dynamic_array<LevelObject> level_objs(definition_.num_objects);
    uint32_t asset_address = (uint32_t)_assetsSegmentStart;
    load_data(level_objs.data(), asset_address + definition_.object_array_rom_offset, sizeof(LevelObject) * definition_.num_objects);

    for (size_t obj_index = 0; obj_index < definition_.num_objects; obj_index++)
    {
        const LevelObject& cur_obj = level_objs[obj_index];
        switch (static_cast<ObjectClass>(cur_obj.object_class))
        {
            case ObjectClass::enemy:
                create_enemy(
                    static_cast<float>(static_cast<int>(cur_obj.x * tile_size + tile_size / 2)),
                    static_cast<float>(static_cast<int>(cur_obj.y * tile_size)),
                    static_cast<float>(static_cast<int>(cur_obj.z * tile_size + tile_size / 2)),
                    static_cast<EnemyType>(cur_obj.object_type),
                    cur_obj.object_subtype);
                break;
            case ObjectClass::interactable:
                create_interactable(
                    static_cast<float>(static_cast<int>(cur_obj.x * tile_size + tile_size / 2)),
                    static_cast<float>(static_cast<int>(cur_obj.y * tile_size)),
                    static_cast<float>(static_cast<int>(cur_obj.z * tile_size + tile_size / 2)),
                    static_cast<InteractableType>(cur_obj.object_type),
                    cur_obj.object_subtype,
                    cur_obj.object_param);
                break;
        }
    }
}
