#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

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