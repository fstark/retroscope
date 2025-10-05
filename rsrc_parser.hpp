#pragma once

#include "rsrc.h"
#include <cstdint>
#include <functional>
#include <vector>
#include <string>
#include <memory>

// Individual resource representation
class rsrc_t
{
private:
    std::string type_;
    int16_t id_;
    std::string name_;
    std::shared_ptr<std::vector<uint8_t>> data_;

public:
    // Constructor
    rsrc_t(const std::string& type, int16_t id, const std::string& name, std::shared_ptr<std::vector<uint8_t>> data);

    // Getters
    const std::string& type() const { return type_; }
    int16_t id() const { return id_; }
    const std::string& name() const { return name_; }
    bool has_name() const { return !name_.empty(); }
    std::shared_ptr<std::vector<uint8_t>> data() const { return data_; }
    size_t size() const { return data_ ? data_->size() : 0; }

    // Comparison operator for sorting by type, then by ID
    bool operator<(const rsrc_t& other) const;
    
    // Alternative comparison function for use with std::sort
    static bool compare(const rsrc_t& a, const rsrc_t& b);
};

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