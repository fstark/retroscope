#pragma once

#include <vector>
#include <cstdint>

/**
 * Abstract interface for accessing the content of a file fork (data or resource).
 * Each fork is responsible for knowing its own size and providing read access
 * to its content with proper bounds checking.
 */
class file_fork_t
{
public:
    virtual ~file_fork_t() = default;

    /**
     * Get the logical size of this fork in bytes.
     * @return The size of the fork content
     */
    virtual uint32_t size() const = 0;

    /**
     * Read data from this fork.
     * @param offset Byte offset from the start of the fork (0-based)
     * @param size Maximum number of bytes to read (will be clamped to available data)
     * @return Vector containing the requested data (may be shorter than requested size)
     */
    virtual std::vector<uint8_t> read(uint32_t offset, uint32_t size) = 0;
};