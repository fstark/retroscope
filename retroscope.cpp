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

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <disk_image_file>\n";
        std::cerr << "Analyzes vintage Macintosh HFS disk images and prints volume names.\n";
        return 1;
    }

    const char *filename = argv[1];
    
    try {
        // Create initial data source
        auto file_source = std::make_shared<file_data_source_t>(filename);
        std::cout << "Analyzing disk image: " << filename << " (" << file_source->size() << " bytes)\n";
        
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
                std::cout << "Found HFS partition\n";
                try {
                    parse_hfs(source);
                } catch (const std::exception& hfs_error) {
                    std::cerr << "Error parsing HFS partition: " << hfs_error.what() << "\n";
                    std::cerr << "Continuing with other partitions...\n";
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}