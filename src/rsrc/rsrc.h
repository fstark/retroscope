#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

/**
 * Individual Macintosh Resource.
 * Represents a single resource extracted from a Macintosh resource fork.
 * Each resource has a 4-character type code, numeric ID, optional name,
 * and binary data payload.
 * 
 * Resources are the fundamental unit of data storage in Mac resource forks,
 * containing everything from application code and dialog templates to
 * icons, sounds, and localized strings.
 */
class rsrc_t
{
private:
    std::string type_;                            ///< 4-character resource type (e.g., "CODE", "PICT")
    int16_t id_;                                  ///< Resource ID number
    std::string name_;                            ///< Optional resource name (empty if unnamed)
    std::shared_ptr<std::vector<uint8_t>> data_; ///< Resource data payload

public:
    /**
     * Construct a resource object.
     * @param type 4-character resource type code
     * @param id Resource ID number
     * @param name Optional resource name (empty string if none)
     * @param data Shared pointer to the resource data
     */
    rsrc_t(const std::string& type, int16_t id, const std::string& name, std::shared_ptr<std::vector<uint8_t>> data);

    /**
     * Get the resource type.
     * @return 4-character type code (e.g., "CODE", "PICT", "STR ")
     */
    const std::string& type() const { return type_; }
    
    /**
     * Get the resource ID.
     * @return Numeric resource identifier
     */
    int16_t id() const { return id_; }
    
    /**
     * Get the resource name.
     * @return Resource name string (may be empty)
     */
    const std::string& name() const { return name_; }
    
    /**
     * Check if this resource has a name.
     * @return True if the resource has a non-empty name
     */
    bool has_name() const { return !name_.empty(); }
    
    /**
     * Get the resource data.
     * @return Shared pointer to vector containing the resource payload
     */
    std::shared_ptr<std::vector<uint8_t>> data() const { return data_; }
    
    /**
     * Get the size of the resource data.
     * @return Number of bytes in the resource payload (0 if no data)
     */
    size_t size() const { return data_ ? data_->size() : 0; }

    /**
     * Compare resources for ordering.
     * Resources are ordered first by type, then by ID.
     * @param other Resource to compare against
     * @return True if this resource should sort before other
     */
    bool operator<(const rsrc_t& other) const;
    
    /**
     * Static comparison function for use with std::sort.
     * @param a First resource to compare
     * @param b Second resource to compare
     * @return True if a should sort before b
     */
    static bool compare(const rsrc_t& a, const rsrc_t& b);
};