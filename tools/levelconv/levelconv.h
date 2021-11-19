#ifndef __LEVELCONV_H__
#define __LEVELCONV_H__

#include <array>
#include <cstdint>
#include "dynamic_array.h"
#include "../gltf64/include/bswap.h"

constexpr size_t chunk_size = 16;
using tile_id = uint8_t;

// A tile is a single element in the grid. It has an associated tile id and a rotation.
struct OutputTile {
    tile_id id;
    uint8_t rotation;
};

struct ChunkColumn {
    int16_t base_height;
    dynamic_array<OutputTile> tiles;
};

struct Chunk {
    std::array<std::array<ChunkColumn, chunk_size>, chunk_size> columns;
};

// A ChunkColumn contains a contiguous column of tiles starting at a certain height
// Each subsequent tile in the column is one higher than the previous
struct OutputChunkColumn {
    int16_t base_height;
    uint16_t num_tiles;
    uint32_t tiles_offset;
    void swap_endianness()
    {
        base_height = ::swap_endianness(base_height);
        num_tiles = ::swap_endianness(num_tiles);
        tiles_offset = ::swap_endianness(tiles_offset);
    }
};

// A chunk is a collection of columns that can be loaded or unloaded independently of other chunks
// Each chunk is assigned to a given chunk position, which is the position of the first column divided by chunk_size
struct OutputChunk {
    std::array<std::array<OutputChunkColumn, chunk_size>, chunk_size> columns;
    void swap_endianness()
    {
        for (auto& z_array : columns)
        {
            for (auto& column : z_array)
            {
                column.swap_endianness();
            }
        }
    }
};

// A grid definition contains information about the grid it refers to. This includes the dimensions of the grid in chunks,
// as well as the rom offset to the array of chunk rom offsets. (TODO PC) The chunk arrays in rom contain 2 32-bit values each, 
// the chunk's file offset and the chunk's data length
struct OutputGridDefinition {
    uint16_t num_chunks_x;
    uint16_t num_chunks_z;
    uint32_t chunk_array_rom_offset;
    uint32_t object_array_rom_offset;
    uint32_t num_objects;
    void swap_endianness()
    {
        num_chunks_x = ::swap_endianness(num_chunks_x);
        num_chunks_z = ::swap_endianness(num_chunks_z);
        chunk_array_rom_offset = ::swap_endianness(chunk_array_rom_offset);
        object_array_rom_offset = ::swap_endianness(object_array_rom_offset);
        num_objects = ::swap_endianness(num_objects);
    }
};

struct OutputObject {
    uint16_t x;
    int16_t  y;
    uint16_t z;
    uint16_t object_class;
    uint16_t object_type;
    uint16_t object_subtype;
    uint32_t object_param;
    void swap_endianness()
    {
        x = ::swap_endianness(x);
        y = ::swap_endianness(y);
        z = ::swap_endianness(z);
        object_class = ::swap_endianness(object_class);
        object_type = ::swap_endianness(object_type);
        object_subtype = ::swap_endianness(object_subtype);
        object_param = ::swap_endianness(object_param);
    }
};

// Quick and dirty 2D wrapper for a dynamic array, only implements functionality that was used
template <typename T>
class dynamic_array_2d
{
public:
    dynamic_array_2d(size_t size_x, size_t size_y) :
        size_x_(size_x), size_y_(size_y), data_(dynamic_array<T>(size_x * size_y))
    {}

          T& operator[](std::pair<size_t, size_t> pos)       { return data_[pos.first + pos.second * size_x_]; }
    const T& operator[](std::pair<size_t, size_t> pos) const { return data_[pos.first + pos.second * size_x_]; }
    
    std::pair<size_t, size_t> size() { return {size_x_, size_y_}; }
private:
    size_t size_x_, size_y_;
    dynamic_array<T> data_;
};

#endif
