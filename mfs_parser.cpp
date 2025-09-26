#include "mfs_parser.h"

#define noVERBOSE
#ifndef VERBOSE
#undef ENTRY
#define ENTRY(...)
#define rs_log(...)
#endif

// Function to check if a data source contains an MFS partition
bool is_mfs(std::shared_ptr<data_source_t> source)
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

// mfs_partition_t implementation
mfs_partition_t::mfs_partition_t(std::shared_ptr<data_source_t> source)
    : source_(source)
{
}

void mfs_partition_t::scan_partition(file_visitor_t &visitor)
{
    ENTRY("");
    
    // Read the Master Directory Block at offset 1024
    block_t mdb_block = source_->read_block(1024, 512);
    const MFSMasterDirectoryBlock *mdb = static_cast<const MFSMasterDirectoryBlock *>(mdb_block.data());
    
    // Extract directory information
    // uint16_t num_files = be16(mdb->drNmFls);
    uint16_t dir_start = be16(mdb->drDirSt);
    uint16_t dir_length = be16(mdb->drBlLen);
    
    // Get volume name from MDB
    std::string volume_name = string_from_pstring(mdb->drVN);
    volume_name = from_macroman(volume_name);
    
    // Create disk and root folder
    auto disk = std::make_shared<Disk>(volume_name, source_->description());
    auto root_folder = std::make_shared<Folder>(volume_name);
    
    // Calculate directory offset (dir_start is in 512-byte blocks)
    uint32_t dir_offset = dir_start * 512;
    uint32_t dir_size = dir_length * 512;
    
    // Read the entire directory
    block_t dir_block = source_->read_block(dir_offset, dir_size);
    const uint8_t *dir_data = static_cast<const uint8_t *>(dir_block.data());
    
    // Parse variable-length directory entries
    size_t offset = 0;
    size_t entries_found = 0;
    
    rs_log("Scanning MFS directory: {} files, dir_start={}, dir_length={}", num_files, dir_start, dir_length);
    
    while (offset < dir_size) {
        // Ensure we have at least the fixed part of a directory entry
        if (offset + sizeof(MFSDirectoryEntry) > dir_size) {
            break; 
        }
        
        const MFSDirectoryEntry *entry = reinterpret_cast<const MFSDirectoryEntry *>(dir_data + offset);
        
        // Name length is at the end of the fixed structure
        uint8_t name_len = entry->deNameLen;
        
        // Check if file is in use (bit 7 of deFlags)
        if (entry->deFlags & 0x80) {
            rs_log("Directory entry at offset {}: flags=0x{:02x}, name_len={}", offset, entry->deFlags, name_len);
            
            if (name_len > 0 && name_len <= 63) {
                // Extract filename - it starts right after the fixed structure
                const uint8_t *name_ptr = dir_data + offset + sizeof(MFSDirectoryEntry);
                std::string filename(reinterpret_cast<const char*>(name_ptr), name_len);
                filename = from_macroman(filename);
                
                rs_log("Found file: '{}'", filename);
                
                // Extract type and creator (stored in big-endian format)
                std::string type = string_from_code(be32(entry->deType));
                std::string creator = string_from_code(be32(entry->deCreator));
                
                uint32_t data_size = be32(entry->deDataLen);
                uint32_t rsrc_size = be32(entry->deRsrcLen);
                
                rs_log("  Type: {}, Creator: {}, Data: {} bytes, Resource: {} bytes", 
                       type, creator, data_size, rsrc_size);
                
                // Create File object
                auto file = std::make_shared<File>(disk, filename, type, creator, data_size, rsrc_size);
                
                // Add file to root folder
                root_folder->add_file(file);
                entries_found++;
            } else {
                rs_log("  Invalid name length: {}", name_len);
            }
        }
        
        // Calculate total entry size: fixed part + name + padding to even  
        size_t entry_size = sizeof(MFSDirectoryEntry) + name_len;
        if (entry_size & 1) {
            entry_size++; // Pad to even boundary
        }
        
        // Check if the complete entry fits in the directory
        if (offset + entry_size > dir_size) {
            rs_log("Entry at offset {} extends beyond directory, stopping", offset);
            break;
        }
        
        offset += entry_size;
    }
    
    rs_log("Found {} files in use", entries_found);
    
    // Visit the root folder, which will visit all files
    visit_folder(root_folder, visitor);
}