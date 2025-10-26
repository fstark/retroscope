#include "hfs_file_fork.h"
#include "hfs_partition.h"
#include "utils.h"
#include <algorithm>
#include <cstring>

hfs_file_fork_t::hfs_file_fork_t(hfs_partition_t& partition, 
                                 const HFSExtentRecord extents[3], 
                                 uint32_t logical_size)
    : partition_(partition), logical_size_(logical_size), data_loaded_(false)
{
    // Copy the extent records (up to 3)
    for (int i = 0; i < 3; i++) {
        if (be16(extents[i].startBlock) != 0 || be16(extents[i].blockCount) != 0) {
            extents_.push_back(extents[i]);
        }
    }
}

uint32_t hfs_file_fork_t::size() const
{
    return logical_size_;
}

void hfs_file_fork_t::load_data()
{
    if (data_loaded_) {
        return;
    }

    if (logical_size_ == 0) {
        cached_data_.clear();
        data_loaded_ = true;
        return;
    }

    // Pre-allocate the result vector
    cached_data_.resize(logical_size_);
    uint32_t bytes_read = 0;

    for (const auto& extent : extents_) {
        uint16_t start_block = be16(extent.startBlock);
        uint16_t block_count = be16(extent.blockCount);

        if (start_block == 0 && block_count == 0) {
            break; // End of extents
        }

        uint32_t allocation_block_size = partition_.allocation_block_size();
        uint32_t extent_size = block_count * allocation_block_size;

        // Don't read beyond logical size
        uint32_t bytes_to_read = std::min(extent_size, logical_size_ - bytes_read);
        
        if (bytes_to_read == 0) {
            break;
        }

        // Read the data from this extent
        auto extent_data = partition_.read_allocated_block(start_block, block_count);
        
        // Get the underlying data from block_t
        const uint8_t* block_data = static_cast<const uint8_t*>(extent_data.data());
        uint32_t available_size = extent_size; // We know how much we requested
        
        // Copy the relevant portion to our result
        uint32_t copy_size = std::min(bytes_to_read, available_size);
        std::memcpy(cached_data_.data() + bytes_read, block_data, copy_size);
        
        bytes_read += copy_size;

        if (bytes_read >= logical_size_) {
            break;
        }
    }

    // Resize to actual logical size (in case we allocated too much)
    cached_data_.resize(logical_size_);
    data_loaded_ = true;
}

std::vector<uint8_t> hfs_file_fork_t::read(uint32_t offset, uint32_t size)
{
    // Lazy load the data if not already loaded
    load_data();

    // Handle the case where offset is beyond file size
    if (offset >= logical_size_) {
        return {};
    }

    // Determine actual size to read
    uint32_t actual_size = std::min(size, logical_size_ - offset);

    // Extract the requested portion
    auto start_iter = cached_data_.begin() + offset;
    auto end_iter = start_iter + actual_size;
    
    return std::vector<uint8_t>(start_iter, end_iter);
}