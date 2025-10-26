#pragma once

#include "rsrc/mac_rsrc.h"
#include "rsrc/rsrc_t.h"
#include <cstdint>
#include <functional>
#include <vector>

// Resource fork parser for Macintosh resource forks
class rsrc_parser_t
{
public:
    // Function type for reading bytes from the resource fork
    using read_function_t = std::function<std::vector<uint8_t>(uint32_t offset, uint32_t size)>;

private:
    uint32_t size_;
    read_function_t read_func_;
    ResourceHeader header_;
    bool header_valid_;

public:
    // Constructor
    rsrc_parser_t(uint32_t size, read_function_t read_func);

    // Check if the resource fork is valid
    bool is_valid() const { return header_valid_; }

    // Get resource header
    const ResourceHeader& get_header() const { return header_; }

    // Get data offset (converted from big-endian)
    uint32_t get_data_offset() const;

    // Get map offset (converted from big-endian)
    uint32_t get_map_offset() const;

    // Get data length (converted from big-endian)
    uint32_t get_data_length() const;

    // Get map length (converted from big-endian)
    uint32_t get_map_length() const;

    // Dump all resources using rs_log
    void dump() const;

    // Get all resources as a vector of rsrc_t objects
    // Throws std::runtime_error on parsing errors
    std::vector<rsrc_t> get_resources() const;

private:
    // Initialize and validate the resource header
    void init_header();
};