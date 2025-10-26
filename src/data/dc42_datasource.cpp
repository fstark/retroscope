#include "data/dc42_datasource.h"
#include <cstring>
#include <iostream>

struct DC42Header
{
    uint8_t nameLen;         // 0x00
    char name[63];           // 0x01..0x3F (not null-terminated)
    uint32_t dataSizeBE;     // 0x40
    uint32_t tagSizeBE;      // 0x44
    uint32_t dataChecksumBE; // 0x48
    uint32_t tagChecksumBE;  // 0x4C
    uint8_t format;          // 0x50
    uint8_t flags;           // 0x51
    uint8_t reserved[2];     // 0x52..0x53
};

static bool isDC42(std::shared_ptr<datasource_t> source)
{
    // Need at least 84 bytes for header
    if (source->size() < 84)
    {
        return false;
    }

    // Read the 84-byte header
    block_t header_block = source->read_block(0, 84);
    const uint8_t *buf = static_cast<const uint8_t *>(header_block.data());

    DC42Header h{};
    h.nameLen = buf[0];
    std::memcpy(h.name, buf + 1, 63);
    h.dataSizeBE = be32(buf + 0x40);
    h.tagSizeBE = be32(buf + 0x44);
    h.dataChecksumBE = be32(buf + 0x48);
    h.tagChecksumBE = be32(buf + 0x4C);
    h.format = buf[0x50];
    h.flags = buf[0x51];
    h.reserved[0] = buf[0x52];
    h.reserved[1] = buf[0x53];

    // Basic header sanity
    if (h.nameLen > 63)
        return false;
    uint32_t dataSize = h.dataSizeBE;
    uint32_t tagSize = h.tagSizeBE;

    // Data size must be multiple of 512; tag size multiple of 12 (or 0)
    if ((dataSize % 512) != 0)
        return false;
    if (tagSize != 0 && (tagSize % 12) != 0)
        return false;

    // Total size must match exactly: 84 + data + tags
    uint64_t expect = 84ull + uint64_t(dataSize) + uint64_t(tagSize);
    if (source->size() != expect)
        return false;

    // Format byte sanity: zero is unlikely for valid DC42
    if (h.format == 0)
        return false;

    return true;
}

std::shared_ptr<datasource_t> make_dc42_datasource(std::shared_ptr<datasource_t> source)
{
    if (!isDC42(source))
    {
        // Not a DC42 file, return original source unchanged
        return source;
    }

    // DC42 files have an 84-byte header, so we create a range source
    // that skips the first 84 bytes to access the actual disk data
    uint64_t dc42_header_size = 84;
    uint64_t data_size = source->size() - dc42_header_size;

    return std::make_shared<range_datasource_t>(source, dc42_header_size, data_size);
}