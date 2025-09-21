#include "retroscope.h"
#include "data.h"
#include "hfs_parser.h"
#include "apm_datasource.h"
#include "dc42_datasource.h"
#include "bin_datasource.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <format>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cstddef>
#include <endian.h>
#include <vector>
#include <filesystem>

void process_disk_image(const std::filesystem::path& filepath)
{
    // Create initial data source
    auto file_source = std::make_shared<file_data_source_t>(filepath);
#ifdef VERBOSE
    std::cout << "Analyzing disk image: " << filepath << " (" << file_source->size() << " bytes)\n";
#endif

    // Start with the file itself
    std::vector<std::shared_ptr<data_source_t>> sources = { file_source };
    
    // Process all sources, expanding them if they contain sub-partitions
    for (size_t i = 0; i < sources.size(); ++i) {
        auto source = sources[i];
        
        // Try BIN unwrapping (CD-ROM format)
        auto bin_source = make_bin_data_source(source);
        if (bin_source != source) {
            sources.push_back(bin_source);
            continue;
        }
        
        // Try DC42 unwrapping
        auto dc42_source = make_dc42_data_source(source);
        if (dc42_source != source) {
            sources.push_back(dc42_source);
            continue;
        }
        
        // Try APM expansion
        auto apm_partitions = make_apm_data_source(source);
        if (!apm_partitions.empty()) {
            std::cout << "Found Apple Partition Map with " << apm_partitions.size() << " partitions\n";
            for (auto& partition : apm_partitions) {
                sources.push_back(partition);
            }
        }
    }
    
    // Now check each source for HFS and parse if found
    for (auto& source : sources) {
        if (is_hfs(source)) {
    std::cout << "Analyzing disk image: " << filepath << " (" << file_source->size() << " bytes)\n";
            std::cout << "Found HFS partition\n";
            try {
                parse_hfs(source);
            } catch (const std::exception& hfs_error) {
                std::cerr << "In disk image: " << filepath << " (" << file_source->size() << " bytes)\n";
                std::cerr << "Error parsing HFS partition: " << hfs_error.what() << "\n";
                std::cerr << "Continuing with other partitions...\n";
            }
        }
    }
}

void process_path(const std::filesystem::path& path)
{
    if (std::filesystem::is_directory(path)) {
        std::cout << "Processing directory: " << path << "\n";
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    try {
                        process_disk_image(entry.path());
                    } catch (const std::exception& e) {
                        std::cerr << "Error processing file " << entry.path() << ": " << e.what() << "\n";
                        std::cerr << "Continuing with other files...\n";
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error accessing directory " << path << ": " << e.what() << "\n";
        }
    } else if (std::filesystem::is_regular_file(path)) {
        process_disk_image(path);
    } else {
        std::cerr << "Error: " << path << " is not a regular file or directory\n";
        throw std::runtime_error("Invalid path type");
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <disk_image_file_or_directory> [additional_paths...]\n";
        std::cerr << "Analyzes vintage Macintosh HFS disk images and list content.\n";
        std::cerr << "If a directory is provided, recursively processes all files in it.\n";
        std::cerr << "Multiple paths can be specified to process them all.\n";
        return 1;
    }

    try {
        // Process each path argument
        for (int i = 1; i < argc; ++i) {
            std::filesystem::path input_path = argv[i];
            
            try {
                process_path(input_path);
            } catch (const std::exception& e) {
                std::cerr << "Error processing " << input_path << ": " << e.what() << "\n";
                if (i < argc - 1) {
                    std::cerr << "Continuing with remaining paths...\n";
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
