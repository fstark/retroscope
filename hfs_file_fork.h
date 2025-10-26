#pragma once

#include "file_fork.h"
#include "hfs.h"
#include <vector>
#include <memory>

// Forward declarations
class hfs_partition_t;

/**
 * HFS implementation of file_fork_t interface.
 * Uses lazy loading strategy - data is read on demand from the HFS partition
 * using extent records to locate data blocks.
 */
class hfs_file_fork_t : public file_fork_t {
private:
    hfs_partition_t& partition_;
    std::vector<HFSExtentRecord> extents_;
    uint32_t logical_size_;
    
    // Cache for loaded data (mutable for lazy loading in non-const methods)
    mutable std::vector<uint8_t> cached_data_;
    mutable bool data_loaded_;

    // Helper method to load data from extents
    void load_data();

public:
    /**
     * Construct HFS file fork from extent records and logical size.
     * @param partition Reference to the HFS partition
     * @param extents Array of up to 3 extent records from catalog
     * @param logical_size Logical size of the fork in bytes
     */
    hfs_file_fork_t(hfs_partition_t& partition, 
                    const HFSExtentRecord extents[3], 
                    uint32_t logical_size);

    /**
     * Get the size of this fork in bytes.
     * @return Size in bytes
     */
    uint32_t size() const override;

    /**
     * Read data from this fork.
     * @param offset Offset in bytes from start of fork
     * @param size Number of bytes to read
     * @return Vector containing the requested data
     */
    std::vector<uint8_t> read(uint32_t offset, uint32_t size) override;
};