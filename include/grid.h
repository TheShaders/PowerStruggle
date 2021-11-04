#ifndef __GRID_H__
#define __GRID_H__

#include <memory>
#include <tuple>
#include <type_traits>

#include <dynamic_array.h>
#include <skipfield.h>
#include <model.h>
#include <mem.h>
#include <files.h>

template <typename T>
constexpr T round_up_divide(T x, T y)
{
    return (x - 1) / y + 1;
}

constexpr size_t chunk_size = 16;
constexpr size_t max_loaded_chunks = 64;
constexpr size_t max_loading_chunks = 16;

using tile_id = uint8_t;
using grid_pos = std::pair<unsigned int, unsigned int>;
using chunk_pos = std::pair<uint16_t, uint16_t>;

struct TileType {
    Model *model;
};

// A tile is a single element in the grid. It has an associated tile id and a rotation.
struct Tile {
    tile_id id;
    uint8_t rotation;
};

// A ChunkColumn contains a contiguous column of tiles starting at a certain height
// Each subsequent tile in the column is one higher than the previous
struct ChunkColumn {
    int16_t base_height;
    uint16_t num_tiles;
    Tile *tiles;
};

#include <n64_model.h>

// A chunk is a collection of columns that can be loaded or unloaded independently of other chunks
// Each chunk is assigned to a given chunk position, which is the position of the first column divided by chunk_size
struct Chunk {
    std::array<std::array<ChunkColumn, chunk_size>, chunk_size> columns;
    void adjust_offsets()
    {
        for (auto& x : columns)
        {
            for (auto &xz : x)
            {
                xz.tiles = ::add_offset(xz.tiles, this);
            }
        }
    }
};

// A grid definition contains information about the grid it refers to. This includes the dimensions of the grid in chunks,
// as well as the rom offset to the array of chunk rom offsets. (TODO PC) The chunk arrays in rom contain 2 32-bit values each, 
// the chunk's file offset and the chunk's data length
struct GridDefinition {
    uint16_t num_chunks_x;
    uint16_t num_chunks_z;
    // Not really necessary in the file, but nice for runtime usage and only costs 4 bytes in rom per grid
    uint32_t chunk_array_rom_offset; // TODO refactor for PC support
    void adjust_offsets(uint32_t base_addr)
    {
        chunk_array_rom_offset += base_addr;
    }
    void get_chunk_offset_size(unsigned int x, unsigned int z, uint32_t *offset, uint32_t *size);
};

// Division in C rounds towards 0, rather than towards negative infinity
// Rounding towards negative infinity is quicker for powers of two (just a right arithmetic shift)
// This also ensure correctness for negative chunk positions (which don't exist currently, but might in the future)
template <size_t D, typename T>
constexpr T round_down_divide(T x)
{
    static_assert(D && !(D & (D - 1)), "Can only round down divide by a power of 2!");
    // This log is evaluated at compile time
    size_t log = static_cast<size_t>(std::log2(D));
    return x >> log;
}

constexpr chunk_pos chunk_from_pos(grid_pos pos)
{
    return chunk_pos{round_down_divide<chunk_size>(pos.first), round_down_divide<chunk_size>(pos.second)};
}

class Grid;
struct GridPosProxy {
    Grid& grid;
    unsigned int row;

    inline Tile& operator[](unsigned int col);
};

struct ChunkEntry {
    chunk_pos pos;
    std::unique_ptr<Chunk, alloc_deleter> chunk;
};

// Represents the level grid, which contains a definition of tile types and an array containing all of the level's tiles
class Grid {
public:
    Grid() :
        definition_{}, tile_types_{}, loaded_chunks_{}
    {}
    Grid(GridDefinition definition, dynamic_array<TileType>&& tile_types) :
        definition_(definition), tile_types_{std::move(tile_types)}, loaded_chunks_{}
    {}

    // // [{row, col}]
    // Tile& operator[](grid_pos pos) { return tiles_[pos.first * width_ + pos.second]; }
    // // [row][col]
    // GridPosProxy operator[](unsigned int row) { return GridPosProxy{*this, row}; }

    // Tile& get_tile(unsigned int row, unsigned int col)
    // {
    //     return tiles_[row * width_ + col];
    // }
    // void set_tile(unsigned int row, unsigned int col, Tile type) { tiles_[row * width_ + col] = type; }
    void get_loaded_chunks_in_area(int min_chunk_x, int min_chunk_z, int max_chunk_x, int max_chunk_z, bool* found);
    bool is_loaded_or_loading(chunk_pos pos);
    void load_visible_chunks(Camera& camera);
    void unload_nonvisible_chunks(Camera& camera);
    void load_chunk(chunk_pos pos);
    void process_loading_chunks();

    void draw();

    // Grids cannot be copied, only moved, therefore the two following methods are deleted
    // Copy constructor
    Grid(const Grid&) = delete;
    // Copy assignment
    Grid& operator=(const Grid&) = delete;
    // Move assignment
    Grid& operator=(Grid&&) = default;

private:
    GridDefinition definition_;
    dynamic_array<TileType> tile_types_;
    skipfield<ChunkEntry, max_loaded_chunks> loaded_chunks_;
    skipfield<std::pair<chunk_pos, LoadHandle>, max_loading_chunks> loading_chunks_;

    using loaded_chunk_iterator = decltype(loaded_chunks_)::iterator;
    loaded_chunk_iterator unload_chunk(loaded_chunk_iterator it)
    {
        // unique_ptr will handle freeing the chunk data
        return loaded_chunks_.erase(it);
    }
};

// Tile& GridPosProxy::operator[](unsigned int col) { return grid.get_tile(row, col); }

#endif
