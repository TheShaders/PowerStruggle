#include <ultra64.h>
#include <grid.h>
#include <cstring>
#include <n64_gfx.h>
#include <files.h>


struct filerecord { const char *path; uint32_t offset; uint32_t size; };

class FileRecords
{
private:
  static inline unsigned int hash (const char *str, size_t len);
public:
  static const struct filerecord *get_offset (const char *str, size_t len);
};

extern uint8_t _binary_build_n64_release_assets_bin_start[];
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
//     io_msg.devAddr = (u32)(_binary_build_n64_release_assets_bin_start + rom_pos);
//     io_msg.size = (u32)size;
//     osCreateMesgQueue(&queue, &msg, 1);
//     osEPiStartDma(g_romHandle, &io_msg, OS_READ);
//     osRecvMesg(&queue, nullptr, OS_MESG_BLOCK);
// }

void GridDefinition::get_chunk_offset_size(unsigned int x, unsigned int z, uint32_t *offset, uint32_t *size)
{
    unsigned int chunk_index = z + num_chunks_z * x;
    uint32_t rom_pos = chunk_index * 2 * sizeof(uint32_t) + chunk_array_rom_offset + (uint32_t)_binary_build_n64_release_assets_bin_start;
    uint32_t offset_tmp;
    osPiReadIo(rom_pos + 0 * sizeof(uint32_t), &offset_tmp);
    osPiReadIo(rom_pos + 1 * sizeof(uint32_t), size);
    *offset = offset_tmp + chunk_array_rom_offset;        
}

GridDefinition get_grid_definition(const char *file)
{
    GridDefinition ret;
    auto grid_def_offset = FileRecords::get_offset(file, strlen(file));
    load_data(&ret, (u32)(_binary_build_n64_release_assets_bin_start + grid_def_offset->offset), sizeof(GridDefinition));
    ret.adjust_offsets(grid_def_offset->offset);
    return ret;
}

void Grid::load_chunk(chunk_pos pos)
{
    uint32_t chunk_offset, chunk_length;
    definition_.get_chunk_offset_size(pos.first, pos.second, &chunk_offset, &chunk_length);

    Chunk *chunk = (Chunk*)allocRegion(chunk_length, ALLOC_FILE);

    load_data(chunk, (u32)(_binary_build_n64_release_assets_bin_start + chunk_offset), chunk_length);
    chunk->adjust_offsets();

    loaded_chunks_.emplace({pos, std::unique_ptr<Chunk, alloc_deleter>(chunk)});
}

float tile_rotations[] = {
    0.0f, 90.0f, 180.0f, 270.0f
};

void Grid::draw()
{
    for (auto& entry : loaded_chunks_)
    {
        gfx::push_mat();
        gfx::apply_translation(entry.pos.first * (100.0f * chunk_size), 0, entry.pos.second * (100.0f * chunk_size));
        auto& chunk = entry.chunk;
        for (unsigned int x = 0; x < chunk_size; x++)
        {
            for (unsigned int z = 0; z < chunk_size; z++)
            {
                const ChunkColumn &col = chunk->columns[x][z];
                unsigned int tile_idx;
                int y;
                for (tile_idx = 0, y = col.base_height; y < col.base_height + col.num_tiles; tile_idx++, y++)
                {
                    const auto& tile = col.tiles[tile_idx];
                    gfx::push_mat();
                    gfx::apply_translation(100.0f * x, 100.0f * y, 100.0f * z);
                    gfx::rotate_axis_angle(tile_rotations[tile.rotation], 0.0f, 1.0f, 0.0f);
                    drawModel(tile_types_[tile.id].model, nullptr, 0);
                    gfx::pop_mat();
                }
            }
        }
        gfx::pop_mat();
    }
}
