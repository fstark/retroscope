#include "retroscope.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <format>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cstddef>
#include <endian.h>

// Process Apple Partition Map
void processAPM(std::ifstream &file)
{
    std::vector<uint8_t> block(512);

    // Read block 1 (first partition map entry)
    file.seekg(512);
    file.read(reinterpret_cast<char *>(block.data()), 512);

    if (!file.good())
    {
        std::cerr << "Error reading partition map\n";
        return;
    }

    const ApplePartitionMapEntry *entry =
        reinterpret_cast<const ApplePartitionMapEntry *>(block.data());

    // Check partition map signature
    if (be16toh(entry->pmSig) != 0x504D)
    {
        std::cout << "Not a valid Apple Partition Map\n";
        return;
    }

    uint32_t mapBlocks = be32toh(entry->pmMapBlkCnt);
    std::cout << "Found Apple Partition Map with " << mapBlocks << " entries\n";

    // Process each partition entry
    for (uint32_t i = 1; i <= mapBlocks; i++)
    {
        file.seekg(i * 512);
        file.read(reinterpret_cast<char *>(block.data()), 512);

        if (!file.good())
            break;

        entry = reinterpret_cast<const ApplePartitionMapEntry *>(block.data());

        if (be16toh(entry->pmSig) != 0x504D)
            continue;

        std::string partName = std::string(reinterpret_cast<const char *>(entry->pmPartName), static_cast<size_t>(32));
        std::string partType = std::string(reinterpret_cast<const char *>(entry->pmParType), static_cast<size_t>(32));

        // Remove null terminators
        partName = partName.c_str();
        partType = partType.c_str();

        std::cout << "\nPartition " << i << ": " << partName << " (Type: " << partType << ")\n";

        // Check if this is an HFS partition
        if (partType == "Apple_HFS" || partType == "Apple_HFSX")
        {
            uint64_t startOffset = be32toh(entry->pmPyPartStart) * 512ULL;
            uint64_t size = be32toh(entry->pmPartBlkCnt) * 512ULL;

            std::cout << "  Start: " << startOffset << " bytes, Size: " << size << " bytes\n";
            partition_t hfs(file, startOffset, size);
            hfs.readMasterDirectoryBlock();
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <disk_image_file>\n";
        std::cerr << "Analyzes vintage Macintosh HFS disk images and prints volume names.\n";
        return 1;
    }

    const char *filename = argv[1];
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "Error: Cannot open file '" << filename << "'\n";
        return 1;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    uint64_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::cout << "Analyzing disk image: " << filename << " (" << fileSize << " bytes)\n";

    // Read first block to check for partition map
    std::vector<uint8_t> block0(512);
    file.read(reinterpret_cast<char *>(block0.data()), 512);

    if (!file.good())
    {
        std::cerr << "Error reading first block\n";
        return 1;
    }

    // Check if this looks like a partitioned disk
    // Block 0 should contain driver descriptor record
    // Block 1 should contain first partition map entry
    file.seekg(512);
    std::vector<uint8_t> block1(512);
    file.read(reinterpret_cast<char *>(block1.data()), 512);

    if (file.good())
    {
        const ApplePartitionMapEntry *entry =
            reinterpret_cast<const ApplePartitionMapEntry *>(block1.data());

        if (be16toh(entry->pmSig) == 0x504D)
        {
            // This appears to be a partitioned disk
            std::cout << "Detected partitioned disk image\n";
            processAPM(file);
            file.close();
            return 0;
        }
    }

    // Not a partitioned disk, check if it's a raw HFS partition
    std::cout << "Checking for raw HFS partition...\n";

    // Try block 2 (offset 1024) for HFS MDB
    file.seekg(1024);
    std::vector<uint8_t> hfsBlock(512);
    file.read(reinterpret_cast<char *>(hfsBlock.data()), 512);

    if (file.good())
    {
		block_t block(hfsBlock);
		auto mdb = block.as_master_directory_block();
        if (mdb.isHFSVolume())
        {
            std::cout << "Found raw HFS partition\n";
            partition_t hfs(file, 0, fileSize);
            hfs.readMasterDirectoryBlock();
        }
        else
        {
            std::cout << "File does not appear to be a valid HFS disk image\n";
            return 1;
        }
    }
    else
    {
        std::cout << "File too small or corrupted\n";
        return 1;
    }

    file.close();
    return 0;
}