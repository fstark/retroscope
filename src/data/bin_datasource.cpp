#include "data/bin_datasource.h"
#include "data/stripped_datasource.h"
#include <format>
#include <iostream>

// CD-ROM Mode 1 sector structure:
// - 16 bytes: sync pattern + header
// - 2048 bytes: user data  
// - 288 bytes: error correction (EDC/ECC)
// Total: 2352 bytes per sector

static const size_t CDROM_SECTOR_SIZE = 2352;
static const size_t CDROM_SYNC_HEADER_SIZE = 16;
static const size_t CDROM_DATA_SIZE = 2048;

// Common CD-ROM sync patterns
static const std::vector<uint8_t> CDROM_SYNC_PATTERN = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00
};

bool is_bin_file(std::shared_ptr<datasource_t> source) {
    if (source->size() < CDROM_SECTOR_SIZE) {
        return false;
    }
    
    // Check if size is a multiple of CD-ROM sector size
    if (source->size() % CDROM_SECTOR_SIZE != 0) {
        return false;
    }
    
    // Read the first 16 bytes to check for sync pattern
    auto header_block = source->read_block(0, 16);
    uint8_t* header = static_cast<uint8_t*>(header_block.data());
    
    // Check for the standard sync pattern in the first 12 bytes
    for (size_t i = 0; i < CDROM_SYNC_PATTERN.size(); ++i) {
        if (header[i] != CDROM_SYNC_PATTERN[i]) {
            return false;
        }
    }
    
    std::clog << std::format("Detected CD-ROM BIN file: {} sectors of {} bytes each\n", 
                           source->size() / CDROM_SECTOR_SIZE, CDROM_SECTOR_SIZE);
    
    return true;
}

std::shared_ptr<datasource_t> make_bin_datasource(std::shared_ptr<datasource_t> source) {
    if (!is_bin_file(source)) {
        return source;  // Not a BIN file, return original
    }
    
    // Create stripped data source that extracts just the data portion
    return std::make_shared<stripped_datasource_t>(source, 
                                                   CDROM_SECTOR_SIZE, 
                                                   CDROM_SYNC_HEADER_SIZE, 
                                                   CDROM_DATA_SIZE);
}