#pragma once

#include "fork.h"
#include <vector>
#include <cstdint>

// Forward declarations
class hfs_file_t;

/**
 * HFS implementation of fork_t interface.
 * Provides access to HFS file forks using the hfs_file_t infrastructure
 * for complete extent handling including extents overflow file support.
 */
class hfs_fork_t : public fork_t {
private:
    const hfs_file_t* file_;
    uint32_t logical_size_;

public:
    /**
     * Construct HFS fork from hfs_file_t.
     * @param file Pointer to hfs_file_t (must remain valid during fork lifetime)
     * @param logical_size Logical size of the fork in bytes
     */
    hfs_fork_t(const hfs_file_t* file, uint32_t logical_size);

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