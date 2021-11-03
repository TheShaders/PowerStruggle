#include <cstdlib>
#include <filesystem>
#include <vector>
#include <fstream>
#include <tuple>
#include <typeinfo>

#include <fmt/core.h>
#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>

#include "dynamic_array.h"
#include "levelconv.h"

namespace fs = std::filesystem;
namespace js = nlohmann;


template <typename T>
constexpr T round_up_divide(T x, T y)
{
    return (x - 1) / y + 1;
}

struct Cell
{
    // int x;
    int y;
    // int z;
    int tile_id;
    int rotation;
};

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

struct pair_hash
{
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &pair) const {
        size_t ret = std::hash<T1>()(pair.first);
        hash_combine(ret, pair.second);
        return ret;
    }
};

struct ChunkArrayElement
{
    uint32_t offset;
    uint32_t size;
    void swap_endianness()
    {
        offset = ::swap_endianness(offset);
        size = ::swap_endianness(size);
    }
};

void write_grid(std::ofstream& output_file, dynamic_array_2d<Chunk>& chunks)
{
    size_t num_chunks_x = chunks.size().first;
    size_t num_chunks_z = chunks.size().second;

    // Create an array to hold the chunk offsets and sizes
    dynamic_array<ChunkArrayElement> chunk_offset_array(num_chunks_x * num_chunks_z);
    size_t chunk_offset_array_bytes = chunk_offset_array.size() * sizeof(ChunkArrayElement);

    uint32_t cur_offset = sizeof(OutputGridDefinition) + chunk_offset_array_bytes;

    // Write the grid definition
    OutputGridDefinition grid_def{(uint16_t)num_chunks_x, (uint16_t)num_chunks_z, sizeof(OutputGridDefinition)};
    grid_def.swap_endianness();
    output_file.write(reinterpret_cast<char*>(&grid_def), sizeof(OutputGridDefinition));

    output_file.seekp(cur_offset);

    // Create an array to hold the current chunk's tiles
    std::vector<OutputTile> cur_chunk_tiles{};
    // Reserve some space, assuming an average of 2 tiles per column
    // Doesn't affect output, but helps reduce memory allocations during writing the output file
    cur_chunk_tiles.reserve(2 * chunk_size * chunk_size);

    int chunk_index = 0;
    for (size_t chunk_x = 0; chunk_x < num_chunks_x; chunk_x++)
    {
        for (size_t chunk_z = 0; chunk_z < num_chunks_z; chunk_z++)
        {
            // Clear the current chunk tile vector
            cur_chunk_tiles.clear();
            // Write the current file offset as the current chunk's offset
            chunk_offset_array[chunk_index].offset = cur_offset - sizeof(OutputGridDefinition);
            // Get the input chunk and create the corresponding output chunk
            Chunk &cur_chunk = chunks[{chunk_x, chunk_z}];
            OutputChunk cur_output_chunk;
            // Keep track of the current byte offset in the chunk, skipping the chunk's column offset array
            uint32_t cur_chunk_offset = sizeof(OutputChunk);
            
            for (size_t sub_chunk_x = 0; sub_chunk_x < chunk_size; sub_chunk_x++)
            {
                for (size_t sub_chunk_z = 0; sub_chunk_z < chunk_size; sub_chunk_z++)
                {
                    auto& cur_column = cur_chunk.columns[sub_chunk_x][sub_chunk_z];
                    auto& cur_output_column = cur_output_chunk.columns[sub_chunk_x][sub_chunk_z];

                    // Copy the relevant data from the input chunk column to the output chunk column
                    cur_output_column.base_height = cur_column.base_height;
                    cur_output_column.num_tiles = cur_column.tiles.size();

                    // Record the current chunk tile count as the column's offset
                    cur_output_column.tiles_offset = cur_chunk_offset;

                    // Copy the current column into the chunk's tiles
                    std::copy(cur_column.tiles.begin(), cur_column.tiles.end(), std::back_inserter(cur_chunk_tiles));

                    // Move forward in the chunk's data by the column's tile byte count
                    cur_chunk_offset += cur_column.tiles.size() * sizeof(decltype(cur_column.tiles)::value_type);
                }
            }
            cur_output_chunk.swap_endianness();
            // Write the chunk tile offset array and the chunk's tiles
            output_file.write(reinterpret_cast<char*>(&cur_output_chunk), sizeof(cur_output_chunk));
            output_file.write(reinterpret_cast<char*>(cur_chunk_tiles.data()), cur_chunk_tiles.size() * sizeof(decltype(cur_chunk_tiles)::value_type));

            // Get the new file offset after writing
            uint32_t new_offset = output_file.tellp();
            
            // Calculate the chunk size from the two file offsets and correct the chunk's endianness
            chunk_offset_array[chunk_index].size = new_offset - cur_offset;
            chunk_offset_array[chunk_index].swap_endianness();

            // Update the file offset and advance to the next chunk
            cur_offset = new_offset;
            chunk_index++;
        }
    }

    // Seek back to the start of the chunk offset array
    output_file.seekp(sizeof(OutputGridDefinition));
    output_file.write(reinterpret_cast<char*>(chunk_offset_array.data()), chunk_offset_array.size() * sizeof(decltype(chunk_offset_array)::value_type));
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fmt::print("Usage: {} [input level file] [output level binary] [asset folder]\n", argv[0]);
        return EXIT_SUCCESS;
    }

    fs::path asset_folder(argv[3]);
    char *input_path = argv[1];
    char *output_path = argv[2];

    if (!fs::is_directory(asset_folder))
    {
        fmt::print(stderr, "Provided asset folder ({}) is not a valid directory\n", asset_folder.c_str());
        return EXIT_FAILURE;
    }

    if (!fs::is_regular_file(input_path))
    {
        fmt::print(stderr, "Input file {} does not exist or cannot be opened\n", input_path);
        return EXIT_FAILURE;
    }

    js::json input_json;
    try
    {
        std::ifstream input_file(input_path);
        input_file >> input_json;
    }
    catch (js::json::exception& err)
    {
        fmt::print("Error parsing json: {}\n", err.what());
        return EXIT_FAILURE;
    }


    try
    {
        fs::path assets_absolute = fs::canonical(asset_folder);
        fs::path input_dir = fs::path(input_path).parent_path() / fs::path{"./"};
        int min_x = input_json["MinX"];
        // int min_y = input_json["MinY"];
        int min_z = input_json["MinZ"];
        int max_x = input_json["MaxX"];
        // int max_y = input_json["MaxY"];
        int max_z = input_json["MaxZ"];
        std::vector<std::string> tile_paths;

        // Read tile model paths
        for (auto& entry : input_json["TilePaths"])
        {
            auto tile_path = entry.get<std::string>();
            auto real_tile_path = fs::relative(input_dir / tile_path, assets_absolute).replace_extension();
            tile_paths.emplace_back(real_tile_path.string());
        }

        using column_pos = std::pair<unsigned int, unsigned int>;
        std::unordered_map<column_pos, std::vector<Cell>, pair_hash> columns{};

        // Read the cell data into column vectors
        for (auto& cell : input_json["Cells"])
        {
            int x = cell["X"];
            x -= min_x;
            int y = cell["Y"];
            int z = cell["Z"];
            z -= min_z;
            int tile_index = cell["TileIndex"];
            int rotation = cell["Rotation"];
            auto column_vec = columns.find({x, z});
            if (column_vec == columns.end())
            {
                column_vec = columns.emplace(std::make_pair(column_pos{x,z}, std::vector<Cell>{})).first;
            }
            column_vec->second.push_back(Cell{/*x,*/ y, /*z,*/ tile_index, rotation});
        }
        // int sum = std::accumulate(columns.begin(), columns.end(), 0, 
        //     [](auto a, decltype(columns)::value_type& b)
        //     {
        //         return a + b.second.size();
        //     }
        // );
        size_t num_cells_x = max_x - min_x;
        size_t num_cells__z = max_z - min_z;
        size_t num_chunks_x = round_up_divide(num_cells_x, chunk_size);
        size_t num_chunks_z = round_up_divide(num_cells__z, chunk_size);

        dynamic_array_2d<Chunk> chunks(num_chunks_x, num_chunks_z);

        for (size_t chunk_x = 0; chunk_x < num_chunks_x; chunk_x++)
        {
            for (size_t chunk_z = 0; chunk_z < num_chunks_z; chunk_z++)
            {
                Chunk& cur_chunk = chunks[{chunk_x,chunk_z}];

                // fmt::print("chunk: {:<2} {:<2}\n", chunk_x, chunk_z);
                size_t start_x = chunk_x * chunk_size;
                size_t start_z = chunk_z * chunk_size;
                for (size_t subchunk_x = 0; subchunk_x < chunk_size; subchunk_x++)
                {
                    size_t cell_x = subchunk_x + start_x;
                    for (size_t subchunk_z = 0; subchunk_z < chunk_size; subchunk_z++)
                    {
                        size_t cell_z = subchunk_z + start_z;
                        auto column_iter = columns.find({cell_x, cell_z});
                        if (column_iter != columns.end())
                        {
                            std::vector<Cell>& column_vec = column_iter->second;
                            std::sort(column_vec.begin(), column_vec.end(),
                                [](const Cell& a, const Cell& b)
                                {
                                    return a.y < b.y;
                                }
                            );
                            int column_min_y = column_vec.front().y;
                            int column_max_y = column_vec.back().y;
                            size_t height = 1 + column_max_y - column_min_y;
                            dynamic_array<OutputTile> output_column(height);
                            size_t input_column_index = 0;
                            for (size_t output_column_index = 0; output_column_index < height; output_column_index++)
                            {
                                int y = output_column_index + column_min_y;
                                if (column_vec[input_column_index].y > y)
                                {
                                    output_column[output_column_index] = OutputTile{0xFF, 0};
                                }
                                else
                                {
                                    const auto& input_tile = column_vec[input_column_index];
                                    output_column[output_column_index] = OutputTile{(uint8_t)input_tile.tile_id, (uint8_t)input_tile.rotation};
                                    input_column_index++;
                                }
                            }
                            cur_chunk.columns[subchunk_x][subchunk_z].tiles = std::move(output_column);
                            cur_chunk.columns[subchunk_x][subchunk_z].base_height = column_min_y;
                        }
                        else
                        {
                            cur_chunk.columns[subchunk_x][subchunk_z].tiles = dynamic_array<OutputTile>();
                            cur_chunk.columns[subchunk_x][subchunk_z].base_height = 0;
                        }
                    }
                }
            }
        }

        std::ofstream output_file(output_path, std::ios_base::binary);
        write_grid(output_file, chunks);

        // fmt::print("Column count: {}\n", columns.size());
        // fmt::print("Tile count: {}\n", sum);
        // fmt::print("Min: {} {} {} Max: {} {} {}\n", min_x, min_y, min_z, max_x, max_y, max_z);
        // fmt::print("Chunks: {} by {}\n", num_chunks_x, num_chunks_z);
    }
    catch (js::json::exception& err)
    {
        fmt::print("Invalid data in level file: {}\n", err.what());
        return EXIT_FAILURE;
    }

    // std::vector<fs::path> all_files;
    // size_t total_size = gather_files(asset_folder, all_files);

    // std::ofstream packed_file(packed_file_path, std::ios_base::binary);
    // dynamic_array<size_offset_t> file_offsets = write_files(packed_file, all_files, total_size);
    // packed_file.close();

    // auto asset_table = fmt::output_file(asset_table_path);
    // for (size_t file_idx = 0; file_idx < file_offsets.size(); file_idx++)
    // {
    //     asset_table.print("{}, {}, {}\n",
    //         fs::relative(all_files[file_idx], asset_folder).c_str(),
    //         file_offsets[file_idx].offset,
    //         file_offsets[file_idx].size);
    // }
    // asset_table.close();

    return EXIT_SUCCESS;
}
