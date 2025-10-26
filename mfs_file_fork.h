#pragma once

#include "file_fork.h"
#include <vector>
#include <cstdint>
#include <algorithm>

/**
 * MFS implementation of file_fork_t that stores fork content in memory.
 * This implementation uses eager loading - the entire fork content is
 * read and stored when the object is created.
 */
class mfs_file_fork_t : public file_fork_t
{
    std::vector<uint8_t> content_;

public:
    /**
     * Create an MFS file fork with the given content.
     * @param content The complete fork content (moved into this object)
     */
    explicit mfs_file_fork_t(std::vector<uint8_t> content)
        : content_(std::move(content)) {}

    /**
     * Get the size of this fork.
     * @return The number of bytes in the fork content
     */
    uint32_t size() const override
    {
        return static_cast<uint32_t>(content_.size());
    }

    /**
     * Read data from this fork.
     * @param offset Byte offset from the start of the fork
     * @param size Maximum number of bytes to read
     * @return Vector containing the requested data (properly bounds-checked)
     */
    std::vector<uint8_t> read(uint32_t offset, uint32_t size) override
    {
        // Handle out-of-bounds offset
        if (offset >= content_.size()) {
            return {};
        }

        // Clamp size to not read beyond end of content
        size = std::min(size, static_cast<uint32_t>(content_.size()) - offset);

        // Return slice of content vector
        return {content_.begin() + offset, content_.begin() + offset + size};
    }
};