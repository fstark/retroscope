#pragma once

#include "data.h"
#include <memory>

// A data source that extracts data from another data source using a repeating pattern
// For example, extracting 2048 bytes of data from every 2352-byte CD-ROM sector
class stripped_data_source_t : public data_source_t
{
private:
    std::shared_ptr<data_source_t> source_;
    size_t sector_size_;     // Total size of each sector (e.g., 2352 for CD-ROM)
    size_t skip_bytes_;      // Bytes to skip at start of each sector (e.g., 16 for sync/header)
    size_t data_bytes_;      // Bytes of actual data per sector (e.g., 2048)
    size_t total_data_size_; // Cached total size of stripped data

public:
    stripped_data_source_t(std::shared_ptr<data_source_t> source,
                           size_t sector_size,
                           size_t skip_bytes,
                           size_t data_bytes);

    std::string description() const override
    {
        return source_->description() + std::format(" [sector={}, skip={}, data={}]", sector_size_, skip_bytes_, data_bytes_);
    }

    block_t read_block(uint64_t offset, uint64_t length) override;
    uint64_t size() const override;

private:
    // Helper method for easier reading
    std::vector<uint8_t> read(uint64_t offset, uint64_t length) const;
};
