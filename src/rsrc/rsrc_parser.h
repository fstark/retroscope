#pragma once

#include "rsrc/mac_rsrc.h"
#include "rsrc/rsrc.h"
#include <cstdint>
#include <functional>
#include <vector>

/**
 * Macintosh Resource Fork Parser.
 * Provides parsing and access to resources stored in Macintosh resource forks.
 * 
 * Resource forks contain structured data including application code, dialog templates,
 * icons, strings, and other resources organized by type and ID. This parser handles
 * the complex resource fork format with proper validation and error handling.
 * 
 * Usage:
 *   auto parser = rsrc_parser_t(fork_size, read_function);
 *   if (parser.is_valid()) {
 *       auto resources = parser.get_resources();
 *   }
 */
class rsrc_parser_t
{
public:
    /**
     * Function type for reading bytes from the resource fork.
     * @param offset Byte offset from start of resource fork
     * @param size Number of bytes to read
     * @return Vector containing the requested data
     */
    using read_function_t = std::function<std::vector<uint8_t>(uint32_t offset, uint32_t size)>;

private:
    uint32_t size_;              ///< Total size of the resource fork
    read_function_t read_func_;  ///< Function to read data from the fork
    ResourceHeader header_;      ///< Cached resource fork header
    bool header_valid_;          ///< True if header passed validation

    /**
     * Initialize and validate the resource header.
     * Reads the header from offset 0 and performs sanity checks
     * on offsets, sizes, and structural consistency.
     */
    void init_header();

public:
    /**
     * Construct a resource fork parser.
     * @param size Total size of the resource fork in bytes
     * @param read_func Function to read data from the resource fork
     * @throws std::invalid_argument if read_func is null
     */
    rsrc_parser_t(uint32_t size, read_function_t read_func);

    /**
     * Check if the resource fork has a valid header.
     * @return True if the header is structurally valid and passed all checks
     */
    bool is_valid() const { return header_valid_; }

    /**
     * Get the raw resource header structure.
     * @return Reference to the parsed ResourceHeader (big-endian values)
     */
    const ResourceHeader& get_header() const { return header_; }

    /**
     * Get the offset to the resource data area.
     * @return Byte offset from start of fork to resource data (host byte order)
     */
    uint32_t get_data_offset() const;

    /**
     * Get the offset to the resource map area.
     * @return Byte offset from start of fork to resource map (host byte order)
     */
    uint32_t get_map_offset() const;

    /**
     * Get the length of the resource data area.
     * @return Size of resource data area in bytes (host byte order)
     */
    uint32_t get_data_length() const;

    /**
     * Get the length of the resource map area.
     * @return Size of resource map area in bytes (host byte order)
     */
    uint32_t get_map_length() const;

    /**
     * Debug dump all resources to the log.
     * Logs each resource with format: "TYPE ID=n [name] size bytes"
     * Only works if the resource fork is valid.
     */
    void dump() const;

    /**
     * Parse and extract all resources from the fork.
     * Performs complete parsing of the resource map, type list, reference lists,
     * and resource data. Resources are returned sorted by type then by ID.
     * 
     * @return Vector of rsrc_t objects representing all resources
     * @throws std::runtime_error if parsing fails or fork is invalid
     */
    std::vector<rsrc_t> get_resources() const;
};