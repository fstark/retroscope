#include "hfs/hfs_fork.h"
#include "hfs/hfs_partition.h"
#include <algorithm>

hfs_fork_t::hfs_fork_t(const hfs_file_t* file, uint32_t logical_size)
    : file_(file), logical_size_(logical_size) {}

uint32_t hfs_fork_t::size() const
{
    return logical_size_;
}

std::vector<uint8_t> hfs_fork_t::read(uint32_t offset, uint32_t size)
{
    if (!file_ || logical_size_ == 0) {
        return {};
    }
    
    // Clamp size to logical size
    if (offset >= logical_size_) {
        return {};
    }
    
    uint32_t actual_size = std::min(size, logical_size_ - offset);
    return file_->read(offset, actual_size);
}