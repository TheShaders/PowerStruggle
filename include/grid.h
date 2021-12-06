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
#include <mathutils.h>

#include <platform_grid.h>

constexpr int visible_inner_range_x = 3000;
constexpr int visible_inner_range_pos_z = 1500;
constexpr int visible_inner_range_neg_z = 4000;

constexpr int visible_outer_range_x     = visible_inner_range_x + 64;
constexpr int visible_outer_range_pos_z = visible_inner_range_pos_z + 64;
constexpr int visible_outer_range_neg_z = visible_inner_range_neg_z + 64;

template <typename T>
constexpr T round_away_divide(T x, T y)
{
    return (x - 1) / y + 1;
}

constexpr size_t chunk_size = 16;
constexpr size_t tile_size = 256;
constexpr size_t max_loaded_chunks = 64;
constexpr size_t max_loading_chunks = 16;

using tile_id = uint8_t;
using grid_pos = std::pair<unsigned int, unsigned int>;
using chunk_pos = std::pair<uint16_t, uint16_t>;

enum class TileCollision : uint8_t {
    none,
    floor,
    wall,
    slope,
};

struct TileType {
    Model *model;
    TileCollision flags;
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

enum class ObjectClass
{
    enemy = 0,
    interactable = 1,
};

struct LevelObject {
    uint16_t x;
    int16_t  y;
    uint16_t z;
    uint16_t object_class;
    uint16_t object_type;
    uint16_t object_subtype;
    uint32_t object_param;
};

// A grid definition contains information about the grid it refers to. This includes the dimensions of the grid in chunks,
// as well as the rom offset to the array of chunk rom offsets. (TODO PC) The chunk arrays in rom contain 2 32-bit values each, 
// the chunk's file offset and the chunk's data length
struct GridDefinition {
    uint16_t num_chunks_x;
    uint16_t num_chunks_z;
    // Not really necessary in the file, but nice for runtime usage and only costs 4 bytes in rom per grid
    uint32_t chunk_array_rom_offset; // TODO refactor for PC support
    uint32_t object_array_rom_offset; // TODO refactor for PC support
    uint32_t num_objects;
    void adjust_offsets(uint32_t base_addr)
    {
        chunk_array_rom_offset += base_addr;
        object_array_rom_offset += base_addr;
    }
    void get_chunk_offset_size(unsigned int x, unsigned int z, uint32_t *offset, uint32_t *size);
};

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

// Defined per-platform
struct ChunkGfx;

struct ChunkEntry {
    chunk_pos pos;
    std::unique_ptr<Chunk, alloc_deleter> chunk;
    ChunkGfx gfx;
};

// Represents the level grid, which contains a definition of tile types and an array containing all of the level's tiles
class Grid {
public:
    Grid() :
        definition_{}, tile_types_{}, loaded_chunks_{}, loading_chunks_{}
    {}
    Grid(GridDefinition definition, dynamic_array<TileType>&& tile_types) :
        definition_(definition), tile_types_{std::move(tile_types)}, loaded_chunks_{}, loading_chunks_{}
    {}
    ~Grid()
    {
        for (auto& tile_type : tile_types_)
        {
            freeAlloc(tile_type.model);
            tile_type.model = nullptr;
        }
    }

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
    bool is_loaded(chunk_pos pos);
    bool is_pos_loaded(float x, float z);
    bool is_loaded_or_loading(chunk_pos pos);
    void load_visible_chunks(Camera& camera);
    void unload_nonvisible_chunks(Camera& camera);
    void load_chunk(chunk_pos pos);
    void process_loading_chunks();
    void load_objects();
    float get_height(float x, float z, float radius, float min_y, float max_y);
    int get_wall_collisions(Vec3 hits[4], float dists[4], float x, float z, float radius, float y_min, float y_max);
    chunk_pos get_minimum_loaded_chunk();

    void draw(Camera *camera);

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
