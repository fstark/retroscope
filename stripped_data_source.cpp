#include "stripped_data_source.h"
#include <algorithm>

stripped_data_source_t::stripped_data_source_t(std::shared_ptr<data_source_t> source, 
                                               size_t sector_size, 
                                               size_t skip_bytes, 
                                               size_t data_bytes)
    : source_(source), sector_size_(sector_size), skip_bytes_(skip_bytes), data_bytes_(data_bytes) {
    
    // Calculate total stripped data size
    size_t source_size = source_->size();
    size_t num_complete_sectors = source_size / sector_size_;
    total_data_size_ = num_complete_sectors * data_bytes_;
    
    // Handle partial last sector if present
    size_t remaining_bytes = source_size % sector_size_;
    if (remaining_bytes > skip_bytes_) {
        total_data_size_ += std::min(remaining_bytes - skip_bytes_, data_bytes_);
    }
}

std::vector<uint8_t> stripped_data_source_t::read(uint64_t offset, uint16_t length) const {
    if (offset >= total_data_size_) {
        return {};
    }
    
    length = std::min((uint64_t)length, total_data_size_ - offset);
    std::vector<uint8_t> result;
    result.reserve(length);
    
    uint64_t bytes_read = 0;
    uint64_t current_offset = offset;
    
    while (bytes_read < length) {
        // Find which sector this offset belongs to
        uint64_t sector_index = current_offset / data_bytes_;
        uint64_t offset_in_sector = current_offset % data_bytes_;
        
        // Calculate source offset for this sector
        uint64_t source_offset = sector_index * sector_size_ + skip_bytes_ + offset_in_sector;
        
        // How much to read from this sector
        uint16_t bytes_in_this_sector = std::min((uint64_t)(data_bytes_ - offset_in_sector), length - bytes_read);
        
        // Read from source
        auto sector_block = source_->read_block(source_offset, bytes_in_this_sector);
        uint8_t* sector_data = static_cast<uint8_t*>(sector_block.data());
        result.insert(result.end(), sector_data, sector_data + bytes_in_this_sector);
        
        bytes_read += bytes_in_this_sector;
        current_offset += bytes_in_this_sector;
        
        // If we couldn't read the full requested amount, we've hit the end
        if (bytes_in_this_sector < (data_bytes_ - offset_in_sector) && bytes_read < length) {
            break;
        }
    }
    
    return result;
}

block_t stripped_data_source_t::read_block(uint64_t offset, uint16_t length) {
    return block_t(read(offset, length));
}

uint64_t stripped_data_source_t::size() const {
    return total_data_size_;
}