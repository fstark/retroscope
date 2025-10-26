#include "mfs_partition.h"

// mfs_partition_t implementation
mfs_partition_t::mfs_partition_t(std::shared_ptr<data_source_t> source)
    : source_(source), root_folder_(nullptr)
{
}

bool mfs_partition_t::is_mfs(std::shared_ptr<data_source_t> source)
{
    // Need at least 1024 + some bytes to check for MFS MDB at offset 1024
    if (source->size() < 1536)
    {
        return false;
    }

    // MFS Master Directory Block is always at offset 1024 per the specification
    block_t mdb_block = source->read_block(1024, 512);
    const MFSMasterDirectoryBlock *mdb = static_cast<const MFSMasterDirectoryBlock *>(mdb_block.data());

    // Check MFS signature (0xD2D7 in big endian)
    return be16(mdb->drSigWord) == 0xD2D7;
}

std::vector<uint8_t> mfs_partition_t::read_file_fork(const MFSDirectoryEntry *entry, bool is_resource_fork) const
{
    // Get the starting block and length for the appropriate fork
    uint16_t start_block = is_resource_fork ? be16(entry->deRsrcABlk) : be16(entry->deDataABlk);
    uint32_t fork_size = is_resource_fork ? be32(entry->deRsrcLen) : be32(entry->deDataLen);
    
    if (fork_size == 0) {
        return {};  // Empty fork
    }
    
    // We need to read the MDB again to get allocation block size
    block_t mdb_block = source_->read_block(1024, 512);
    const MFSMasterDirectoryBlock *mdb = static_cast<const MFSMasterDirectoryBlock *>(mdb_block.data());
    
    uint32_t alloc_block_size = be32(mdb->drAlBlkSiz);
    uint16_t first_alloc_block = be16(mdb->drAlBlSt);

    // Calculate the actual disk offset
    // From MFS spec: "Like FAT, allocation blocks are numbered starting from 2. 
    // However, unlike FAT, the block map begins with allocation block 2 instead 
    // of skipping the first two entries."
    // This means allocation block numbers in directory entries start from 2
    uint32_t alloc_area_start = first_alloc_block * 512;
    
    // Subtract 2 because allocation blocks are numbered starting from 2
    uint32_t adjusted_alloc_block = (start_block >= 2) ? start_block - 2 : 0;
    uint32_t start_offset = alloc_area_start + (adjusted_alloc_block * alloc_block_size);
    
    // Read the fork content
    block_t fork_block = source_->read_block(start_offset, fork_size);
    const uint8_t* fork_data = static_cast<const uint8_t*>(fork_block.data());
    
    return std::vector<uint8_t>(fork_data, fork_data + fork_size);
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> mfs_partition_t::read_file_content(const MFSDirectoryEntry *entry) const
{
    auto data_fork = read_file_fork(entry, false);   // Data fork
    auto rsrc_fork = read_file_fork(entry, true);    // Resource fork
    
    return {std::move(data_fork), std::move(rsrc_fork)};
}

void mfs_partition_t::build_root_folder()
{
    ENTRY("");

    // Return early if already built
    if (root_folder_) {
        return;
    }

    // Read the Master Directory Block at offset 1024
    block_t mdb_block = source_->read_block(1024, 512);
    const MFSMasterDirectoryBlock *mdb = static_cast<const MFSMasterDirectoryBlock *>(mdb_block.data());

    // Extract directory information
    uint16_t dir_start = be16(mdb->drDirSt);
    uint16_t dir_length = be16(mdb->drBlLen);

    // Get volume name from MDB
    std::string volume_name = string_from_pstring(mdb->drVN);
    volume_name = from_macroman(volume_name);

    // Create disk and root folder
    auto disk = std::make_shared<Disk>(volume_name, source_->description());
    root_folder_ = std::make_shared<Folder>(volume_name);

    // Calculate directory offset (dir_start is in 512-byte blocks)
    uint32_t dir_offset = dir_start * 512;

    // Parse directory block by block (512 bytes each)
    size_t entries_found = 0;
    
    rs_log("Scanning MFS directory: {} files, dir_start={}, dir_length={}", be16(mdb->drNmFls), dir_start, dir_length);
    
    for (uint16_t block_num = 0; block_num < dir_length; block_num++) {
        uint32_t block_offset = dir_offset + (block_num * 512);
        block_t dir_block = source_->read_block(block_offset, 512);
        const uint8_t *block_data = static_cast<const uint8_t *>(dir_block.data());
        
        rs_log("Scanning MFS directory block {} at offset {}", block_num, block_offset);
        
        // Parse entries within this 512-byte block
        size_t offset_in_block = 0;
        
        while (offset_in_block < 512) {
            // Ensure we have at least the fixed part of a directory entry
            if (offset_in_block + sizeof(MFSDirectoryEntry) > 512) {
                break;
            }

            const MFSDirectoryEntry *entry = reinterpret_cast<const MFSDirectoryEntry *>(block_data + offset_in_block);
            uint8_t name_len = entry->deNameLen;

            // Check bit 7 of flags - if clear, no more entries in this block
            if ((entry->deFlags & 0x80) == 0) {
                // No more entries in this block per MFS specification
                break; 
            }

            // Check if we have enough space for the complete entry (including name)
            size_t total_entry_size = sizeof(MFSDirectoryEntry) + name_len;
            if (total_entry_size & 1) {
                total_entry_size++; // Pad to even boundary
            }
            
            if (offset_in_block + total_entry_size > 512) {
                // Entry would cross block boundary, skip to next block
                break;
            }

            // Bit 7 is set (valid entry), now check if it's a real file
            if (name_len > 0 && name_len <= 63) {
                // Extract filename - it starts right after the fixed structure
                const uint8_t *name_ptr = block_data + offset_in_block + sizeof(MFSDirectoryEntry);
                std::string filename(reinterpret_cast<const char *>(name_ptr), name_len);
                filename = from_macroman(filename);

                rs_log("Found file: '{}' (flags=0x{:02x})", filename, entry->deFlags);

                // Extract type and creator (stored in big-endian format)
                std::string type = string_from_code(be32(entry->deType));
                std::string creator = string_from_code(be32(entry->deCreator));

                uint32_t data_size = be32(entry->deDataLen);
                uint32_t rsrc_size = be32(entry->deRsrcLen);

                rs_log("  Type: {}, Creator: {}, Data: {} bytes, Resource: {} bytes",
                       type, creator, data_size, rsrc_size);

                // Read the actual file content
                auto [data_content, rsrc_content] = read_file_content(entry);

                // Create file forks
                std::unique_ptr<file_fork_t> data_fork;
                std::unique_ptr<file_fork_t> rsrc_fork;
                
                if (data_size > 0 && !data_content.empty()) {
                    data_fork = std::make_unique<mfs_file_fork_t>(std::move(data_content));
                }
                if (rsrc_size > 0 && !rsrc_content.empty()) {
                    rsrc_fork = std::make_unique<mfs_file_fork_t>(std::move(rsrc_content));
                }

                // Create File object
                auto file = std::make_shared<File>(disk, filename, type, creator,
                                                          std::move(data_fork), std::move(rsrc_fork));

                // Add file to root folder
                root_folder_->add_file(file);
                entries_found++;
            }
            
            // Move to next entry
            offset_in_block += total_entry_size;
        }
    }

    rs_log("Found {} files in use", entries_found);
}

std::shared_ptr<Folder> mfs_partition_t::get_root_folder()
{
    if (!root_folder_) {
        build_root_folder();
    }
    return root_folder_;
}