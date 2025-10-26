#include "file/file.h"
#include "file/folder.h"
#include "utils.h"
#include "utils/md5.h"
#include "rsrc/rsrc_parser.h"

#include <iostream>
#include <stdexcept>

#define noVERBOSE

#ifdef DEBUG_MEMORY
static int sFileCount = 0;
#endif

File::File(const std::shared_ptr<Disk> &disk, const std::string &name, const std::string &type,
           const std::string &creator, 
           std::unique_ptr<fork_t> data_fork,
           std::unique_ptr<fork_t> rsrc_fork)
    : disk_(disk), name_(name), sane_name_(sanitize_string(name)), type_(type), creator_(creator),
      data_size_(data_fork ? data_fork->size() : 0), 
      rsrc_size_(rsrc_fork ? rsrc_fork->size() : 0), 
      parent_(nullptr),
      data_fork_(std::move(data_fork)), 
      rsrc_fork_(std::move(rsrc_fork))
{
#ifdef DEBUG_MEMORY
    sFileCount++;
#endif
}

File::~File()
{
#ifdef DEBUG_MEMORY
    sFileCount--;
    if (sFileCount==0)
        std::cout << std::format("**** All File deleted\n");
#endif
}

void File::retain_folder()
{
    if (parent_)
    {
        retained_folder_ = parent_->shared_from_this();
        retained_folder_->retain_folder();
    }
}

std::vector<std::shared_ptr<Folder>> File::retained_path() const
{
    std::vector<std::shared_ptr<Folder>> result;
    if (retained_folder_)
    {
        auto f = retained_folder_;
        while (f)
        {
            result.push_back(f);
            f = f->retained_folder();
        }
    }
    return result;
}

std::vector<uint8_t> File::read_data(uint32_t offset, uint32_t size)
{
    if (!data_fork_) {
        return {};
    }
    if (size == UINT32_MAX) {
        size = data_fork_->size() - offset;
    }
    return data_fork_->read(offset, size);
}

std::vector<uint8_t> File::read_rsrc(uint32_t offset, uint32_t size)
{
    if (!rsrc_fork_) {
        return {};
    }
    if (size == UINT32_MAX) {
        size = rsrc_fork_->size() - offset;
    }
    return rsrc_fork_->read(offset, size);
}

std::string File::content_key() const
{
    // Calculate MD5 of data fork
    std::string data_md5 = "0";  // Default for empty data fork
    if (data_size_ > 0) {
        try {
            auto data = read_data_all();
            if (!data.empty()) {
                std::string data_str(data.begin(), data.end());
                data_md5 = MD5(data_str).toStr();
            }
        } catch (const std::exception& e) {
            // If we can't read the data, use a hash based on the error
            data_md5 = std::format("error_{}", data_size_);
        }
    }
    
    // Calculate MD5 of resource fork (skipping padding between header and data)
    std::string rsrc_md5 = "0";  // Default for empty resource fork
    if (rsrc_size_ > 0) {
        try {
            auto rsrc = read_rsrc_all();
            if (!rsrc.empty()) {
                // Try to parse the resource fork to get the actual data offset
                auto read_func = [&rsrc](uint32_t offset, uint32_t size) -> std::vector<uint8_t> {
                    if (offset >= rsrc.size()) {
                        return {};
                    }
                    uint32_t available = rsrc.size() - offset;
                    uint32_t to_read = std::min(size, available);
                    return std::vector<uint8_t>(rsrc.begin() + offset, rsrc.begin() + offset + to_read);
                };
                
                rsrc_parser_t parser(rsrc.size(), read_func);
                if (parser.is_valid()) {
                    // Only hash the actual resource data and map, skipping the padding
                    uint32_t data_offset = parser.get_data_offset();
                    uint32_t data_length = parser.get_data_length();
                    uint32_t map_offset = parser.get_map_offset();
                    uint32_t map_length = parser.get_map_length();
                    
                    // Create a string with just the header + data + map (no padding)
                    std::string clean_rsrc;
                    
                    // Add header (first 16 bytes)
                    if (rsrc.size() >= 16) {
                        clean_rsrc.append(rsrc.begin(), rsrc.begin() + 16);
                    }
                    
                    // Add resource data (skip padding, go directly to data)
                    if (data_offset + data_length <= rsrc.size()) {
                        clean_rsrc.append(rsrc.begin() + data_offset, rsrc.begin() + data_offset + data_length);
                    }
                    
                    // Add resource map
                    if (map_offset + map_length <= rsrc.size()) {
                        clean_rsrc.append(rsrc.begin() + map_offset, rsrc.begin() + map_offset + map_length);
                    }
                    
                    rsrc_md5 = MD5(clean_rsrc).toStr();
                } else {
                    // If we can't parse it, fall back to hashing the entire fork
                    std::string rsrc_str(rsrc.begin(), rsrc.end());
                    rsrc_md5 = MD5(rsrc_str).toStr();
                }
            }
        } catch (const std::exception& e) {
            // If we can't read the resource, use a hash based on the error
            rsrc_md5 = std::format("error_{}", rsrc_size_);
        }
    }
    
    // Return enhanced key with content hashes (name excluded for content comparison)
    std::string key = std::format("{}|{}|{}|{}|{}|{}", 
                       type_, creator_, data_size_, rsrc_size_, data_md5, rsrc_md5);
    
    return key;
}
