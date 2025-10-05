#include "hfs_parser.h"
#include <map>
#include <algorithm>

// Free-standing functions to convert block_t to specific node types
btree_header_node_t as_btree_header_node(block_t &block)
{
    return btree_header_node_t(block);
}

master_directory_block_t as_master_directory_block(block_t &block)
{
    return master_directory_block_t(block);
}

// Function to check if a data source contains an HFS partition
bool is_hfs(std::shared_ptr<data_source_t> source)
{
    ENTRY("{}", source->description());

    // Need at least 1024 + 512 bytes to check for HFS MDB at block 2
    if (source->size() < 1536)
    {
        rs_log("Data source too small to be HFS");
        return false;
    }

    // Read block 2 (offset 1024) where HFS Master Directory Block should be
    block_t mdb_block = source->read_block(1024, 512);
    const uint8_t *data = static_cast<const uint8_t *>(mdb_block.data());

    // Check HFS signature (0x4244 = "BD" in big endian at offset 0)
    uint16_t signature = (uint16_t(data[0]) << 8) | uint16_t(data[1]);
    if (signature != 0x4244)
    {
        rs_log("Invalid HFS signature: 0x{:04X}", signature);
        return false;
    }

    // Additional basic sanity checks
    // Check that allocation block size is reasonable (typically 512, 1024, etc.)
    uint32_t alloc_block_size = (uint32_t(data[20]) << 24) | (uint32_t(data[21]) << 16) |
                                (uint32_t(data[22]) << 8) | uint32_t(data[23]);

    // Allocation block size should be a power of 2 and >= 512
    if (alloc_block_size == 0 || (alloc_block_size % 512) != 0)
    {
        rs_log("Unreasonable allocation block size: {}", alloc_block_size);
        return false;
    }

    rs_log("HFS volume detected with allocation block size: {}", alloc_block_size);
    return true;
}

// master_directory_block_t implementations
bool master_directory_block_t::isHFSVolume() const
{
    return be16(content->drSigWord) == 0x4244;
}

std::string master_directory_block_t::getVolumeName() const
{
    return string_from_pstring(content->drVN);
}

uint32_t master_directory_block_t::allocationBlockSize() const
{
    return be32(content->drAlBlkSiz);
}

uint16_t master_directory_block_t::allocationBlockStart() const
{
    return be16(content->drAlBlSt);
}

uint16_t master_directory_block_t::extendsExtendStart(int index) const
{
    if (index < 0 || index >= 3)
        throw std::out_of_range("Extent index out of range");
    return be16(content->drXTExtRec[index].startBlock);
}

uint16_t master_directory_block_t::extendsExtendCount(int index) const
{
    if (index < 0 || index >= 3)
        throw std::out_of_range("Extent index out of range");
    return be16(content->drXTExtRec[index].blockCount);
}

uint16_t master_directory_block_t::catalogExtendStart(int index) const
{
    if (index < 0 || index >= 3)
        throw std::out_of_range("Extent index out of range");
    return be16(content->drCTExtRec[index].startBlock);
}

uint16_t master_directory_block_t::catalogExtendCount(int index) const
{
    if (index < 0 || index >= 3)
        throw std::out_of_range("Extent index out of range");
    return be16(content->drCTExtRec[index].blockCount);
}

// hfs_file_t implementations
void hfs_file_t::add_extent(const extent_t &extend)
{
    extents_.push_back(extend);
}

void hfs_file_t::add_extent(uint16_t start_block, const extent_t &extent)
{
    //  check that we already have start_block blocks in the extent
    uint16_t total_blocks = 0;
    for (const auto &ext : extents_)
    {
        total_blocks += ext.count;
    }
    if (total_blocks != start_block)
    {
        throw std::runtime_error("Extent continuity error: expected " +
                                 std::to_string(start_block) + " blocks, but have " +
                                 std::to_string(total_blocks));
    }

    extents_.push_back(extent);
}

uint16_t hfs_file_t::to_absolute_block(uint16_t block) const
{
    uint16_t absolute_block = 0;
    for (const auto &extent : extents_)
    {
        if (block < extent.count)
        {
            return absolute_block + extent.start + block;
        }
        block -= extent.count;
        absolute_block += extent.count;
    }
    throw std::out_of_range("Block out of range");
}

uint32_t hfs_file_t::allocation_offset(uint32_t offset) const
{
    auto allocation_block_size = partition_.allocation_block_size();
    for (auto &ext : extents_)
    {
        if (offset < ext.count * allocation_block_size)
            return ext.start * allocation_block_size + offset;
        offset -= ext.count * allocation_block_size;
    }
    throw std::out_of_range("Offset out of range");
}

btree_file_t hfs_file_t::as_btree_file()
{
    return btree_file_t(*this);
}

std::vector<uint8_t> hfs_file_t::read() const
{
    return read(0, logical_size_);
}

std::vector<uint8_t> hfs_file_t::read(uint32_t offset, uint32_t size) const
{
    if (offset >= logical_size_) {
        return {}; // Beyond file end
    }
    
    // Clamp size to not read beyond logical file size
    size = std::min(size, logical_size_ - offset);
    
    std::vector<uint8_t> result;
    result.reserve(size);
    
    uint32_t bytes_read = 0;
    
    // Use the existing allocation_offset method which handles the extent traversal
    while (bytes_read < size) {
        uint32_t current_offset = offset + bytes_read;
        uint32_t remaining = size - bytes_read;
        
        // Calculate how much we can read in one go
        // We need to be careful not to cross extent boundaries
        uint32_t bytes_in_this_read = remaining;
        
        // Find which extent this offset falls into and limit read to extent boundary
        uint32_t file_pos = 0;
        for (const auto& extent : extents_) {
            uint32_t extent_size = extent.count * partition_.allocation_block_size();
            if (current_offset < file_pos + extent_size) {
                // This offset is in this extent
                uint32_t offset_in_extent = current_offset - file_pos;
                uint32_t bytes_left_in_extent = extent_size - offset_in_extent;
                bytes_in_this_read = std::min(bytes_in_this_read, bytes_left_in_extent);
                break;
            }
            file_pos += extent_size;
        }
        
        // Use allocation_offset to get the absolute offset and read
        uint32_t alloc_offset = allocation_offset(current_offset);
        block_t block_data = partition_.read_allocation(alloc_offset, bytes_in_this_read);
        
        // Copy the data from the block
        const uint8_t* block_ptr = static_cast<const uint8_t*>(block_data.data());
        result.insert(result.end(), block_ptr, block_ptr + bytes_in_this_read);
        
        bytes_read += bytes_in_this_read;
    }
    
    return result;
}

// hfs_partition_t implementations
hfs_partition_t::hfs_partition_t(std::shared_ptr<data_source_t> data_source)
    : data_source_(data_source),
      extents_(*this), catalog_(*this)
{
    build_root_folder();
}

block_t hfs_partition_t::read(uint64_t blockOffset) const
{
    return data_source_->read_block(blockOffset * 512, 512);
}

block_t hfs_partition_t::read_allocated_block(uint16_t block, uint16_t size)
{
    if (size == 0xffff)
        size = allocationBlockSize_;

#ifdef VERBOSE
    std::cout << std::format("read_allocated_block({},{}) allocation start={}\n", block, size, allocationStart_);
#endif

    auto disk_offset = (allocationStart_ + block) * 512;
    return data_source_->read_block(disk_offset, size);
}

#define CNID_ROOT 2
#define CNID_CATALOG 4

//  This is more than "building root", it is "mounting the partition and creating proxy for all the files"
void hfs_partition_t::build_root_folder()
{
    //  First step is to read the Master Directory Block
    //  To extract a few key informations
    block_t mdb_block = read(2); // MDB is at block 2
    master_directory_block_t mdb = as_master_directory_block(mdb_block);

    if (!mdb.isHFSVolume())
    {
        throw std::runtime_error("Not an HFS volume");
    }

    auto disk = std::make_shared<Disk>(from_macroman(mdb.getVolumeName()), data_source_->description());

    // std::cout << std::format("\n\n\n\nHFS Volume: {} (Disk: {})\n", mdb.getVolumeName(), data_source_->description());

    allocationBlockSize_ = mdb.allocationBlockSize();
    allocationStart_ = mdb.allocationBlockStart();

#ifdef VERBOSE
    std::cout << std::format("Allocation block size: {}\n", allocationBlockSize_);
    std::cout << std::format("Allocation start: {}\n", allocationStart_);
#endif

    // We now set up the extents file
    //  (we want to see where the extends file reside on disk)
    for (int i = 0; i < 3; i++)
    {
        uint16_t start = mdb.extendsExtendStart(i);
        uint16_t count = mdb.extendsExtendCount(i);
        if (count > 0)
        {
            extents_.add_extent({start, count});
#ifdef VERBOSE
            std::cout << std::format("Extents extent {}: start={}, count={}\n", i, start, count);
#endif
        }
    }

    // Set up catalog file
    //  (we want to see where the catalog file reside on disk)
    for (int i = 0; i < 3; i++)
    {
        uint16_t start = mdb.catalogExtendStart(i);
        uint16_t count = mdb.catalogExtendCount(i);
        if (count > 0)
        {
            catalog_.add_extent({start, count});
#ifdef VERBOSE
            std::cout << std::format("Catalog extent {}: start={}, count={}\n", i, start, count);
#endif
        }
    }

    //  However, the MDB only gives us the first 3 extents for each file
    //  It is always enough for extends [citation needed]
    //  but can be insufficient for the catalog file
    //  so we scan all the extends records to find additional extends for the catalog file (CNID 4, DATA fork)
    // Get additional extents from the extents B-tree
    btree_file_t extents_btree = extents_.as_btree_file();
    extents_btree.iterate_extents([&](const extents_record_t &extent_record)
                                  {
        uint8_t forkType = extent_record.fork_type();
        uint32_t file_ID = extent_record.file_ID();

        // We add the file_ID 4 data extends to the catalog file
        if (file_ID == CNID_CATALOG && forkType == 0x00)
        {
            for (int i = 0; i < 3; i++) {
                auto extent = extent_record.get_extent(i);
                if (extent.count > 0) {
                    catalog_.add_extent(extent);
                }
            }
        } });

    // Now we can read the catalog B-tree and build the folder/file hierarchy
    auto catalog_btree = catalog_.as_btree_file();

    // Root folder has ID 2
    root_folder = std::make_shared<Folder>(from_macroman(mdb.getVolumeName()));
    folders[CNID_ROOT] = root_folder;

    std::vector<hierarchy_t> hierarchy;

    catalog_btree.iterate_catalog([&](
                                      const catalog_record_t *catalog_record,
                                      const catalog_record_folder_t *folder_record,
                                      const catalog_record_file_t *file_record)
                                  {

        // Skip records with parent ID 1 (these are special system records)
        if (catalog_record->parent_id() == 1) {
            return;
        }

        auto parent_id = catalog_record->parent_id();

        if (folder_record) {
            // This is a folder
            auto folder = std::make_shared<Folder>(from_macroman(catalog_record->name()));
            auto folder_id = folder_record->folder_id();
            folders[folder_id] = folder;
            
            // Record the hierarchy relationship
            hierarchy.push_back({parent_id, folder_id});

#ifdef VERBOSE
            std::cout << std::format("Folder: {} (ID: {}, Parent: {})\n", 
                                    catalog_record->name(), folder_id, parent_id);
#endif
        } else if (file_record) {
            // This is a file
            std::shared_ptr<File> file = std::make_shared<HFSFile>(
                disk,
                from_macroman(catalog_record->name()),
                file_record->type(),
                file_record->creator(),
                file_record->dataLogicalSize(),
                file_record->rsrcLogicalSize());

            // Find parent folder and add file to it
            auto parent_it = folders.find(parent_id);
            if (parent_it != folders.end()) {
                parent_it->second->add_file(file);
            }

#ifdef VERBOSE
            std::cout << std::format("File: {} [{}/{}] (Parent: {}, Data: {}, Rsrc: {})\n",
                                    catalog_record->name(),
                                    file_record->type(),
                                    file_record->creator(),
                                    parent_id,
                                    file_record->dataLogicalSize(),
                                    file_record->rsrcLogicalSize());
#endif
        } });

    // Build the folder hierarchy
    for (const auto &h : hierarchy)
    {
        auto parent_it = folders.find(h.parent_id);
        auto child_it = folders.find(h.child_id);

        if (parent_it != folders.end() && child_it != folders.end())
        {
            parent_it->second->add_folder(child_it->second);
        }
    }

    // All the Folder shared_ptr should now be owned by their parents folders
}

void hfs_partition_t::readCatalogHeader(uint64_t /* catalogExtendStartBlock */)
{
    // Implementation moved to build_root_folder()
}

void hfs_partition_t::readCatalogRoot(uint16_t /* rootNode */)
{
    // Implementation integrated into build_root_folder()
}

void hfs_partition_t::dumpextentTree()
{
    std::cout << "=== Extents B-tree ===\n";

    auto extents_btree = extents_.as_btree_file();
    extents_btree.iterate_extents([&](const extents_record_t &record)
                                  {
        std::cout << std::format("File ID: {}, Fork: {}, Start: {}\n",
                                record.file_ID(),
                                record.fork_type() == 0x00 ? "data" : "resource",
                                record.start_block());
        
        for (int i = 0; i < 3; i++)
        {
            try {
                auto extent = record.get_extent(i);
                if (extent.count > 0)
                {
                    std::cout << std::format("  Extent {}: start={}, count={}\n", i, extent.start, extent.count);
                }
            } catch (const std::out_of_range&) {
                break;
            }
        } });
}

// btree_file_t implementations
btree_file_t::btree_file_t(hfs_file_t &file) : file_(file)
{
    auto start = file_.allocation_offset(0);
    // std::cout << "Btree file starts at absolute block: " << start+file_.partition().

#ifdef VERBOSE
    std::cout << std::format("Partition allocation start: {}\n", file_.partition().allocation_start());
    std::cout << std::format("File start: {}\n", start);
#endif

    //  We read the node as a 512 byte block, but it may be larger
    //  We look into the btree header to find the actual node size
    auto block1 = file_.partition().read_allocation(start, 512);
    // block.dump();
    auto header1 = as_btree_header_node(block1);

#ifdef VERBOSE
    std::cout << std::format("Node size: {}\n", header1.node_size());
    std::cout << std::format("Node count: {}\n", header1.node_count());
#endif

    //  Re-read with the proper node size
    auto block2 = file_.partition().read_allocation(start, header1.node_size());
    auto header2 = as_btree_header_node(block2);

    first_leaf_node_ = header2.first_leaf_node();
    node_size_ = header2.node_size();

#ifdef VERBOSE
    std::cout << std::format("First leaf node: {}\n", header2.first_leaf_node());
#endif
}