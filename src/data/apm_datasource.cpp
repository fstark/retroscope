#include "data/apm_datasource.h"
#include <cstring>
#include <iostream>

// Apple Partition Map Entry structure (simplified version inspired by retroscope.cpp)
struct ApplePartitionMapEntry
{
    uint16_t pmSig;         // 0x00: Partition signature (0x504D)
    uint16_t pmSigPad;      // 0x02: Reserved
    uint32_t pmMapBlkCnt;   // 0x04: Number of blocks in partition map
    uint32_t pmPyPartStart; // 0x08: First physical block of partition
    uint32_t pmPartBlkCnt;  // 0x0C: Number of blocks in partition
    char pmPartName[32];    // 0x10: Partition name
    char pmParType[32];     // 0x30: Partition type
    // ... other fields not needed for basic parsing
};

static bool isAPM(std::shared_ptr<datasource_t> source)
{
    // Need at least 1024 bytes (2 blocks) to check for APM
    if (source->size() < 1024)
    {
        return false;
    }

    // Read block 1 (first partition map entry)
    block_t block1 = source->read_block(512, 512);
    const uint8_t *data = static_cast<const uint8_t *>(block1.data());

    // Check partition map signature (0x504D = "PM" in big endian)
    uint16_t signature = be16(data);
    return signature == 0x504D;
}

std::vector<std::shared_ptr<datasource_t>> make_apm_datasource(std::shared_ptr<datasource_t> source)
{
    std::vector<std::shared_ptr<datasource_t>> partitions;

    if (!isAPM(source))
    {
        // Not an APM disk, return empty vector
        return partitions;
    }

    // Read block 1 to get the number of partition entries
    block_t block1 = source->read_block(512, 512);
    const uint8_t *data = static_cast<const uint8_t *>(block1.data());

    uint32_t mapBlocks = be32(data + 4); // pmMapBlkCnt at offset 4

    // Process each partition entry
    for (uint32_t i = 1; i <= mapBlocks; i++)
    {
        // Check if we have enough data for this block
        if ((i + 1) * 512 > source->size())
        {
            break;
        }

        block_t block = source->read_block(i * 512, 512);
        const uint8_t *entry_data = static_cast<const uint8_t *>(block.data());

        // Check partition signature
        uint16_t sig = be16(entry_data);
        if (sig != 0x504D)
        {
            continue;
        }

        uint32_t startOffset = be32(entry_data + 8) * 512ULL; // pmPyPartStart
        uint32_t size = be32(entry_data + 12) * 512ULL;       // pmPartBlkCnt

        // Validate partition bounds
        if (startOffset + size <= source->size())
        {
            auto partition_source = std::make_shared<range_datasource_t>(source, startOffset, size);
            partitions.push_back(partition_source);
        }
    }

    return partitions;
}