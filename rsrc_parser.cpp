#include "rsrc_parser.hpp"
#include "utils.h"
#include <stdexcept>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>

// rsrc_t implementation
rsrc_t::rsrc_t(const std::string& type, int16_t id, const std::string& name, std::shared_ptr<std::vector<uint8_t>> data)
    : type_(type), id_(id), name_(name), data_(data)
{
}

bool rsrc_t::operator<(const rsrc_t& other) const
{
    // First compare by type
    if (type_ != other.type_) {
        return type_ < other.type_;
    }
    // If types are equal, compare by ID
    return id_ < other.id_;
}

bool rsrc_t::compare(const rsrc_t& a, const rsrc_t& b)
{
    return a < b;
}

// rsrc_parser_t implementation
rsrc_parser_t::rsrc_parser_t(uint32_t size, read_function_t read_func)
    : size_(size), read_func_(read_func), header_valid_(false)
{
    if (!read_func_) {
        throw std::invalid_argument("Read function cannot be null");
    }
    
    init_header();
}

void rsrc_parser_t::init_header()
{
    // Resource fork must be at least large enough for the header
    if (size_ < sizeof(ResourceHeader)) {
        header_valid_ = false;
        return;
    }

    // Read the header from the beginning of the resource fork
    auto header_data = read_func_(0, sizeof(ResourceHeader));
    if (header_data.size() != sizeof(ResourceHeader)) {
        header_valid_ = false;
        return;
    }

    // Copy header data
    std::memcpy(&header_, header_data.data(), sizeof(ResourceHeader));

    // Validate the header values
    uint32_t data_offset = get_data_offset();
    uint32_t map_offset = get_map_offset();
    uint32_t data_length = get_data_length();
    uint32_t map_length = get_map_length();

    // Basic sanity checks
    if (data_offset >= size_ || 
        map_offset >= size_ || 
        data_offset + data_length > size_ ||
        map_offset + map_length > size_ ||
        data_offset < sizeof(ResourceHeader) ||  // Data must come after header
        map_offset < sizeof(ResourceHeader)) {   // Map must come after header
        header_valid_ = false;
        return;
    }

    // Data and map areas should not overlap
    if ((data_offset < map_offset && data_offset + data_length > map_offset) ||
        (map_offset < data_offset && map_offset + map_length > data_offset)) {
        header_valid_ = false;
        return;
    }

    header_valid_ = true;
}

uint32_t rsrc_parser_t::get_data_offset() const
{
    return be32(header_.dataOffset);
}

uint32_t rsrc_parser_t::get_map_offset() const
{
    return be32(header_.mapOffset);
}

uint32_t rsrc_parser_t::get_data_length() const
{
    return be32(header_.dataLength);
}

uint32_t rsrc_parser_t::get_map_length() const
{
    return be32(header_.mapLength);
}

void rsrc_parser_t::dump() const
{
    if (!header_valid_) {
        rs_log("Resource fork not valid, cannot dump");
        return;
    }
    
    rs_log("Dumping resource fork:");
    
    try {
        auto resources = get_resources();
        
        for (const auto& resource : resources) {
            // Format output as requested: 'CODE ID=1 [opt name] 123 bytes'
            if (resource.has_name()) {
                rs_log("{} ID={} [{}] {} bytes", resource.type(), resource.id(), resource.name(), resource.size());
            } else {
                rs_log("{} ID={} {} bytes", resource.type(), resource.id(), resource.size());
            }
        }
    } catch (const std::exception& e) {
        rs_log("Error dumping resources: {}", e.what());
    }
}

std::vector<rsrc_t> rsrc_parser_t::get_resources() const
{
    std::vector<rsrc_t> resources;
    
    if (!header_valid_) {
        rs_log("Resource fork header is not valid");
        throw std::runtime_error("Resource fork header is not valid");
    }
    
    // Calculate offset to resource map
    uint32_t map_offset = get_map_offset();
    
    // Read the resource map header
    auto map_data = read_func_(map_offset, sizeof(ResourceMap));
    if (map_data.size() != sizeof(ResourceMap)) {
        rs_log("Failed to read resource map at offset {}", map_offset);
        throw std::runtime_error("Failed to read resource map");
    }
    
    ResourceMap res_map;
    std::memcpy(&res_map, map_data.data(), sizeof(ResourceMap));
    
    // Calculate offset to type list
    uint32_t type_list_offset = map_offset + be16(res_map.typeListOffset);
    
    // Read number of types
    auto num_types_data = read_func_(type_list_offset, sizeof(uint16_t));
    if (num_types_data.size() != sizeof(uint16_t)) {
        rs_log("Failed to read type count at offset {}", type_list_offset);
        throw std::runtime_error("Failed to read resource type count");
    }
    
    uint16_t num_types_minus_one;
    std::memcpy(&num_types_minus_one, num_types_data.data(), sizeof(uint16_t));
    uint16_t num_types = be16(num_types_minus_one) + 1;
    
    // Read each resource type
    for (int type_idx = 0; type_idx < num_types; type_idx++) {
        uint32_t type_entry_offset = type_list_offset + 2 + (type_idx * sizeof(ResourceType));
        
        auto type_data = read_func_(type_entry_offset, sizeof(ResourceType));
        if (type_data.size() != sizeof(ResourceType)) {
            rs_log("Failed to read resource type {} at offset {}", type_idx, type_entry_offset);
            throw std::runtime_error("Failed to read resource type data");
        }
        
        ResourceType res_type;
        std::memcpy(&res_type, type_data.data(), sizeof(ResourceType));
        
        // Convert type to string using utils function
        std::string type_str = string_from_code(be32(res_type.type));
        
        uint16_t num_resources = be16(res_type.numResources) + 1;
        uint16_t ref_list_offset = be16(res_type.refListOffset);
        
        // Calculate absolute offset to reference list for this type
        uint32_t ref_list_abs_offset = type_list_offset + ref_list_offset;
        
        // Read each resource reference for this type
        for (int res_idx = 0; res_idx < num_resources; res_idx++) {
            uint32_t ref_offset = ref_list_abs_offset + (res_idx * sizeof(ResourceReference));
            
            auto ref_data = read_func_(ref_offset, sizeof(ResourceReference));
            if (ref_data.size() != sizeof(ResourceReference)) {
                rs_log("Failed to read resource reference {} for type '{}' at offset {}", res_idx, type_str, ref_offset);
                throw std::runtime_error("Failed to read resource reference");
            }
            
            ResourceReference res_ref;
            std::memcpy(&res_ref, ref_data.data(), sizeof(ResourceReference));
            
            int16_t resource_id = be16(res_ref.id);
            uint16_t name_offset = be16(res_ref.nameOffset);
            
            // Extract 3-byte data offset and convert to uint32_t
            uint32_t data_offset = (static_cast<uint32_t>(res_ref.dataOffset[0]) << 16) |
                                   (static_cast<uint32_t>(res_ref.dataOffset[1]) << 8) |
                                   static_cast<uint32_t>(res_ref.dataOffset[2]);
            
            // Calculate absolute data offset
            uint32_t abs_data_offset = get_data_offset() + data_offset;
            
            // Read resource data length
            auto length_data = read_func_(abs_data_offset, sizeof(uint32_t));
            if (length_data.size() != sizeof(uint32_t)) {
                rs_log("Failed to read data length for {} ID={} at offset {}", type_str, resource_id, abs_data_offset);
                throw std::runtime_error("Failed to read resource data length");
            }
            
            uint32_t data_length;
            std::memcpy(&data_length, length_data.data(), sizeof(uint32_t));
            data_length = be32(data_length);
            
            // Read the actual resource data
            auto resource_data = read_func_(abs_data_offset + sizeof(uint32_t), data_length);
            if (resource_data.size() != data_length) {
                rs_log("Failed to read resource data for {} ID={}, expected {} bytes but got {}", 
                       type_str, resource_id, data_length, resource_data.size());
                throw std::runtime_error("Failed to read resource data");
            }
            
            // Create shared_ptr for the data
            auto data_ptr = std::make_shared<std::vector<uint8_t>>(std::move(resource_data));
            
            // Try to read resource name if it exists
            std::string name;
            if (name_offset != 0xFFFF) {
                uint32_t name_abs_offset = map_offset + be16(res_map.nameListOffset) + name_offset;
                
                // Read Pascal string using utils function
                auto pascal_data = read_func_(name_abs_offset, 256); // Read up to 256 bytes (max Pascal string length + 1)
                if (!pascal_data.empty()) {
                    uint8_t name_len = pascal_data[0];
                    if (name_len > 0 && pascal_data.size() >= static_cast<size_t>(name_len + 1)) {
                        // Use string_from_pstring and convert from MacRoman
                        std::string raw_name = string_from_pstring(pascal_data.data());
                        name = from_macroman(raw_name);
                    }
                }
            }
            
            // Create and add the resource
            resources.emplace_back(type_str, resource_id, name, data_ptr);
        }
    }
    
    // Sort resources by type, then by ID
    std::sort(resources.begin(), resources.end());
    
    return resources;
}