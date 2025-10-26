#include "rsrc/rsrc.h"

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