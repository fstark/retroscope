#include "hfs_partition.h"
#include <map>
#include <algorithm>

template <class T>
class type_node_t
{
protected:
	block_t &block_;
	T *content;

public:
	type_node_t(block_t &block) : block_(block)
	{
		content = reinterpret_cast<T *>(block_.data());
	}

	// T *operator->() { return content; }
	// const T *operator->() const { return content; }
};

class master_directory_block_t : public type_node_t<HFSMasterDirectoryBlock>
{
public:
	master_directory_block_t(block_t &block) : type_node_t<HFSMasterDirectoryBlock>(block)
	{
#ifdef VERBOSE
		block_.dump();
		// print the contents of the HFSMasterDirectoryBlock structure
		std::cout << std::format("drSigWord: 0x{:04X}\n", be16(content->drSigWord));
		std::cout << std::format("drCrDate: {}\n", be32(content->drCrDate));
		std::cout << std::format("drLsMod: {}\n", be32(content->drLsMod));
		std::cout << std::format("drAtrb: 0x{:04X}\n", be16(content->drAtrb));
		std::cout << std::format("drNmFls: {}\n", be16(content->drNmFls));
		std::cout << std::format("drVBMSt: {}\n", be16(content->drVBMSt));
		std::cout << std::format("drAllocPtr: {}\n", be16(content->drAllocPtr));
		std::cout << std::format("drNmAlBlks: {}\n", be16(content->drNmAlBlks));
		std::cout << std::format("drAlBlkSiz: {}\n", be32(content->drAlBlkSiz));
		std::cout << std::format("drClpSiz: {}\n", be32(content->drClpSiz));
		std::cout << std::format("drAlBlSt: {}\n", be16(content->drAlBlSt));
		std::cout << std::format("drNxtCNID: {}\n", be32(content->drNxtCNID));
		std::cout << std::format("drFreeBks: {}\n", be16(content->drFreeBks));
		std::cout << std::format("drVN: {}\n", string_from_pstring(content->drVN));
		std::cout << std::format("drVolBkUp: {}\n", be32(content->drVolBkUp));
		std::cout << std::format("drVSeqNum: {}\n", be16(content->drVSeqNum));
		std::cout << std::format("drWrCnt: {}\n", be32(content->drWrCnt));
		std::cout << std::format("drXTClpSiz: {}\n", be32(content->drXTClpSiz));
		std::cout << std::format("drCTClpSiz: {}\n", be32(content->drCTClpSiz));
		std::cout << std::format("drNmRtDirs: {}\n", be16(content->drNmRtDirs));
		std::cout << std::format("drFilCnt: {}\n", be32(content->drFilCnt));
		std::cout << std::format("drDirCnt: {}\n", be32(content->drDirCnt));
		std::cout << std::format("drVCSize: {}\n", be16(content->drVCSize));
		std::cout << std::format("drVBMCSize: {}\n", be16(content->drVBMCSize));
		std::cout << std::format("drCtlCSize: {}\n", be16(content->drCtlCSize));
		std::cout << std::format("drXTFlSize: {}\n", be32(content->drXTFlSize));
		std::cout << std::format("drXTExtRec[0]: startBlock={}, blockCount={}\n",
								 be16(content->drXTExtRec[0].startBlock), be16(content->drXTExtRec[0].blockCount));
		std::cout << std::format("drXTExtRec[1]: startBlock={}, blockCount={}\n",
								 be16(content->drXTExtRec[1].startBlock), be16(content->drXTExtRec[1].blockCount));
		std::cout << std::format("drXTExtRec[2]: startBlock={}, blockCount={}\n",
								 be16(content->drXTExtRec[2].startBlock), be16(content->drXTExtRec[2].blockCount));
		std::cout << std::format("drCTFlSize: {}\n", be32(content->drCTFlSize));
		std::cout << std::format("drCTExtRec[0]: startBlock={}, blockCount={}\n",
								 be16(content->drCTExtRec[0].startBlock), be16(content->drCTExtRec[0].blockCount));
		std::cout << std::format("drCTExtRec[1]: startBlock={}, blockCount={}\n",
								 be16(content->drCTExtRec[1].startBlock), be16(content->drCTExtRec[1].blockCount));
		std::cout << std::format("drCTExtRec[2]: startBlock={}, blockCount={}\n",
								 be16(content->drCTExtRec[2].startBlock), be16(content->drCTExtRec[2].blockCount));

		// print offsetof(drXTExtRec)
		std::cout << std::format("Offset of drXTExtRec: 0x{:X}\n", offsetof(HFSMasterDirectoryBlock, drXTExtRec));
#endif
	}

	bool isHFSVolume() const;
	std::string getVolumeName() const;
	uint32_t allocationBlockSize() const;
	uint16_t allocationBlockStart() const;
	uint16_t extendsExtendStart(int index) const;
	uint16_t extendsExtendCount(int index) const;
	uint16_t catalogExtendStart(int index) const;
	uint16_t catalogExtendCount(int index) const;
};

class btree_header_node_t : public type_node_t<BTNodeDescriptor>
{
	BTHeaderRec *header_record_;

public:
	btree_header_node_t(block_t &block) : type_node_t<BTNodeDescriptor>(block)
	{
#ifdef VERBOSE
		std::cout << "Node kind: " << (int)content->kind << "\n";
#endif
		if (content->kind != ndHdrNode)
		{
			throw std::runtime_error("Not a valid B-tree header node");
		}
		header_record_ = reinterpret_cast<BTHeaderRec *>((char *)block.data() + sizeof(BTNodeDescriptor));
	}

	uint32_t first_leaf_node() const { return be32(header_record_->firstLeafNode); }
	uint16_t node_size() const { return be16(header_record_->nodeSize); }
	uint32_t node_count() const { return be32(header_record_->totalNodes); }
};

class btree_leaf_node_t : public type_node_t<BTNodeDescriptor>
{
public:
	btree_leaf_node_t(block_t &block) : type_node_t<BTNodeDescriptor>(block) {}

	uint16_t num_records() const { return be16(content->numRecords); }

	uint32_t f_link() const { return be32(content->fLink); }

	std::pair<void *, uint16_t> get_record(uint16_t record_index) const
	{
		uint16_t num = num_records();

		// Calculate offset table position (at end of node)
		uint16_t *offset_table = reinterpret_cast<uint16_t *>(
			reinterpret_cast<uint8_t *>(content) + 512 - 2 * (num + 1));

#ifdef VERBOSE
		// Debug: Print addresses
		std::cout << std::format("content address: {:p}\n", static_cast<void *>(content));
		std::cout << std::format("offset_table address: {:p}\n", static_cast<void *>(offset_table));
		std::cout << std::format("delta =: {}\n",
								 reinterpret_cast<uint8_t *>(offset_table) -
									 reinterpret_cast<uint8_t *>(content));
#endif

		// Get record start and end offsets
		uint16_t record_start = be16(offset_table[record_index + 1]);
		uint16_t record_end = be16(offset_table[record_index]);

#ifdef VERBOSE
		std::cout << std::format("Record start = {} end = {}\n", record_start, record_end);
#endif

		uint8_t *record_ptr = reinterpret_cast<uint8_t *>(content) + record_start;
		uint16_t record_size = record_end - record_start;

		return std::make_pair(record_ptr, record_size);
	}
};

class extents_record_t
{
public:
	struct extent_t
	{
		uint16_t start;
		uint16_t count;
	};

private:
	const HFSExtentsRecord *data_;

public:
	extents_record_t(const HFSExtentsRecord *data) : data_(data) {}

	uint8_t key_length() const
	{
		return data_->keyLength;
	};

	// 0x00 = data fork, 0xff = resource fork
	uint8_t fork_type() const
	{
		return data_->forkType;
	};

	uint32_t file_ID() const
	{
		return be32(data_->fileID);
	};

	uint16_t start_block() const
	{
		return be16(data_->startBlock);
	}
	hfs_file_t::extent_t get_extent(int index) const
	{
		if (index < 0 || index >= 3)
		{
			throw std::out_of_range("Extent index out of bounds");
		}
		return {
			be16(data_->extents[index].startBlock),
			be16(data_->extents[index].blockCount)};
	}
};

class catalog_record_t
{
	const HFSCatalogKey *key_;

public:
	catalog_record_t(const HFSCatalogKey *key) : key_(key) {}

	const uint8_t *data() const
	{
		const uint8_t *raw = reinterpret_cast<const uint8_t *>(key_);
		uint16_t offset = 1 + key_length(); // keyLength field (1 byte) + key content (reserved + parentID + nodeName)
		// 68k requires word alignment - round up to next even address if odd
		if (offset & 1)
		{
			offset++;
		}
		return raw + offset;
	}

	uint16_t type() const
	{
		return be16(*reinterpret_cast<const uint16_t *>(data()));
	}

	uint8_t key_length() const { return key_->keyLength; }

	uint32_t parent_id() const { return be32(key_->parentID); }
	std::string name() const { return string_from_pstring(key_->nodeName); }
};

class catalog_record_folder_t
{
	const HFSCatalogFolder *folder_;

public:
	catalog_record_folder_t(const HFSCatalogFolder *folder)
		: folder_(folder) {}

	uint32_t folder_id() const { return be32(folder_->folderID); }
	uint16_t item_count() const { return be16(folder_->valence); }
};

class catalog_record_file_t
{
	const HFSCatalogFile *file_;

public:
	catalog_record_file_t(const HFSCatalogFile *file)
		: file_(file) {}

	uint32_t file_id() const { return be32(file_->fileID); }
	std::string type() const { return string_from_code(be32(file_->userInfo.fdType)); }
	std::string creator() const { return string_from_code(be32(file_->userInfo.fdCreator)); }
	uint32_t fileID() const { return be32(file_->fileID); }
	int32_t dataLogicalSize() const { return be32(file_->dataLogicalSize); }
	int32_t dataPhysicalSize() const { return be32(file_->dataPhysicalSize); }
	int32_t rsrcLogicalSize() const { return be32(file_->rsrcLogicalSize); }
	int32_t rsrcPhysicalSize() const { return be32(file_->rsrcPhysicalSize); }
	
	// Access to extent records
	const HFSExtentRecord* dataExtents() const { return file_->dataExtents; }
	const HFSExtentRecord* rsrcExtents() const { return file_->rsrcExtents; }
};

class btree_file_t
{
	hfs_file_t &file_;

	uint32_t first_leaf_node_; //	first leaf node position relative to file
	uint16_t node_size_;	   //	node size in bytes

public:
	btree_file_t(hfs_file_t &file);

	//	This will iterate all the leaf nodes in the btree
	//	and call the callback for each one
	//	with a btree_leaf_node_t& as parameter
	//	To perform the iteration, it uses the fLink field
	//	of each leaf node to find the next one
	template <typename F>
	void iterate_leaves(F &&callback)
	{
		auto block_index = first_leaf_node_;

		while (block_index)
		{
			uint32_t file_offset = block_index * node_size_;
			uint32_t allocation_offset = file_.allocation_offset(file_offset);
#ifdef VERBOSE
			std::cout << std::format("**** {} IS THE BTREE BLOC #\n", block_index);
			std::cout << std::format("**** {} IS THE LOCAL OFFSET\n", file_offset);
			std::cout << std::format("**** {} IS THE ALLOC OFFSET\n", allocation_offset);
#endif
			//	We find the "allocation" block corresponding to this btree block

// convert to allocation block
// uint32_t allocation_index = file.
// auto local_index = file_.to_absolute_block(block_index);
#ifdef VERBOSE
			std::cout << std::format("iterate_leaves() - first leaf node: {}, allocation offset:{} node size: {}\n", first_leaf_node_, allocation_offset, node_size_);
#endif
			block_t block = file_.partition().read_allocation(allocation_offset, node_size_);
			btree_leaf_node_t leaf(block);
			callback(leaf);
			block_index = leaf.f_link();
		}
	}

	//	This iterates all records in the btree
	//	Using the iterate_leaves function
	//	In each leaf, it iterates all records
	//	Callback gets a uint8_t pointer to records and its size
	template <typename F>
	void iterate_records(F &&callback)
	{
		iterate_leaves([&](btree_leaf_node_t &leaf)
					   {
			auto num_records = leaf.num_records();
#ifdef VERBOSE
            std::cout << std::format( "Leaf node with {} records\n", num_records );
#endif

			for (uint16_t i=0;i!=num_records;i++)
			{
				auto [ptr, size] = leaf.get_record( i );
				callback( ptr, size );
			} });
	}

	//	Callback to iterate extends_btree
	template <typename F>
	void iterate_extents(F &&callback)
	{
		iterate_records([&](const void *ptr, uint16_t size)
						{
			assert( size == sizeof(HFSExtentsRecord));
			const HFSExtentsRecord *record = reinterpret_cast<const HFSExtentsRecord*>(ptr);
			callback( extents_record_t{record} ); });
	}

	//	Callback to iterate catalog
	//	callback passed a pointer to HFSCatalogKey, HFSCatalogFolder and HFSCatalogFile
	//	(one of the last two is nullptr)
	template <typename F>
	void iterate_catalog(F &&callback)
	{
		iterate_records([&](const void *ptr, uint16_t)
						{
							const HFSCatalogKey *key = reinterpret_cast<const HFSCatalogKey *>(ptr);
							catalog_record_t catalog_record{key};

							if (catalog_record.type() == kHFSFolderRecord) // folder
							{
								const HFSCatalogFolder *folder = reinterpret_cast<const HFSCatalogFolder *>(catalog_record.data());
								catalog_record_folder_t folder_record{folder};
								callback(&catalog_record, &folder_record, nullptr);
								return;
							}
							else if (catalog_record.type() == kHFSFileRecord) // file
							{
								const HFSCatalogFile *file = reinterpret_cast<const HFSCatalogFile *>(catalog_record.data());
								catalog_record_file_t file_record{file};
								callback(&catalog_record, nullptr, &file_record);
								return;
							}
#ifdef VERBOSE
							else
								std::cout << std::format("  {} [{}]\n", catalog_record.type(), catalog_record.name());
#endif
							// others (thread, etc...) are ignored
						});
	}
};

// Free-standing functions to convert block_t to specific node types
btree_header_node_t as_btree_header_node(block_t &block)
{
    return btree_header_node_t(block);
}

master_directory_block_t as_master_directory_block(block_t &block)
{
    return master_directory_block_t(block);
}

bool hfs_partition_t::is_hfs(std::shared_ptr<datasource_t> source)
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
void hfs_file_t::add_extent(const hfs_file_t::extent_t &extend)
{
    extents_.push_back(extend);
}

void hfs_file_t::add_extent(uint16_t start_block, const hfs_file_t::extent_t &extent)
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
    auto allocation_block_size = static_cast<uint64_t>(partition_.allocation_block_size());
    for (auto &ext : extents_)
    {
        uint64_t extent_size = static_cast<uint64_t>(ext.count) * allocation_block_size;
        if (offset < extent_size)
        {
            uint64_t result = static_cast<uint64_t>(ext.start) * allocation_block_size + offset;
            return static_cast<uint32_t>(result);
        }
        offset -= extent_size;
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
        uint64_t file_pos = 0;
        for (const auto& extent : extents_) {
            uint64_t extent_size = static_cast<uint64_t>(extent.count) * partition_.allocation_block_size();
            if (current_offset < file_pos + extent_size) {
                // This offset is in this extent
                uint64_t offset_in_extent = current_offset - file_pos;
                uint64_t bytes_left_in_extent = extent_size - offset_in_extent;
                bytes_in_this_read = std::min(bytes_in_this_read, static_cast<uint32_t>(bytes_left_in_extent));
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
hfs_partition_t::hfs_partition_t(std::shared_ptr<datasource_t> datasource)
    : datasource_(datasource),
      extents_(*this), catalog_(*this)
{
    build_root_folder();
}

block_t hfs_partition_t::read(uint64_t blockOffset) const
{
    return datasource_->read_block(blockOffset * 512, 512);
}

block_t hfs_partition_t::read_allocated_block(uint16_t block, uint16_t size)
{
    if (size == 0xffff)
        size = allocationBlockSize_;

#ifdef VERBOSE
    std::cout << std::format("read_allocated_block({},{}) allocation start={}\n", block, size, allocationStart_);
#endif

    auto disk_offset = (allocationStart_ + block) * 512;
    return datasource_->read_block(disk_offset, size);
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

    auto disk = std::make_shared<Disk>(from_macroman(mdb.getVolumeName()), datasource_->description());

    // std::cout << std::format("\n\n\n\nHFS Volume: {} (Disk: {})\n", mdb.getVolumeName(), datasource_->description());

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
    
    // First, build complete extent information for ALL files
    extents_btree.iterate_extents([&](const extents_record_t &extent_record)
                                  {
        uint8_t forkType = extent_record.fork_type();
        uint32_t file_ID = extent_record.file_ID();
        
        // Create key for this file/fork combination
        auto key = std::make_pair(file_ID, forkType);
        
        // Create or get existing hfs_file_t for this file/fork
        auto it = files_.find(key);
        if (it == files_.end()) {
            // Create new hfs_file_t in the map
            it = files_.emplace(key, hfs_file_t(*this)).first;
        }

        // Add all extents from this record to the hfs_file_t
        for (int i = 0; i < 3; i++) {
            auto extent = extent_record.get_extent(i);
            if (extent.count > 0) {
                it->second.add_extent(extent);
#ifdef VERBOSE
                std::cout << std::format("Added extent for file {} fork {}: start={}, count={}\n", 
                                        file_ID, forkType, extent.start, extent.count);
#endif
            }
        }
        // Special handling: also add catalog extents to catalog_ for backward compatibility
        if (file_ID == CNID_CATALOG && forkType == 0x00) {
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

    // Local struct for tracking folder hierarchy relationships
    struct hierarchy_t {
        uint32_t parent_id;
        uint32_t child_id;
    };
    
    std::vector<hierarchy_t> hierarchy;

    // As files needs folders to be retained, we first create all folders
    catalog_btree.iterate_catalog([&](
                                      const catalog_record_t *catalog_record,
                                      const catalog_record_folder_t *folder_record,
                                      const catalog_record_file_t *)
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
        }
    }
    );

    
    // We then do all the files
    catalog_btree.iterate_catalog([&](
                                      const catalog_record_t *catalog_record,
                                      const catalog_record_folder_t *,
                                      const catalog_record_file_t *file_record)
                                  {

        // Skip records with parent ID 1 (these are special system records)
        if (catalog_record->parent_id() == 1) {
            return;
        }

        auto parent_id = catalog_record->parent_id();

        if (file_record) {
            // This is a file
            uint32_t fileID = file_record->file_id();
            
            // Build complete file extents (catalog + overflow) for each fork
            std::unique_ptr<fork_t> data_fork;
            std::unique_ptr<fork_t> rsrc_fork;
            
            if (file_record->dataLogicalSize() > 0) {
                // Create key for data fork
                auto data_key = std::make_pair(fileID, static_cast<uint8_t>(0x00));
                
                // Get or create hfs_file_t for data fork
                auto it = files_.find(data_key);
                if (it == files_.end()) {
                    // Create new hfs_file_t with logical size
                    it = files_.emplace(data_key, hfs_file_t(*this, file_record->dataLogicalSize())).first;
                    
                    // Add catalog extents
                    for (int i = 0; i < 3; i++) {
                        uint16_t start = be16(file_record->dataExtents()[i].startBlock);
                        uint16_t count = be16(file_record->dataExtents()[i].blockCount);
                        if (count > 0) {
                            it->second.add_extent({start, count});
                        }
                    }
                } else {
                    // Set logical size if not already set
                    it->second.set_logical_size(file_record->dataLogicalSize());
                }
                
                // Create adapter using the complete extents
                data_fork = std::make_unique<hfs_fork_t>(&it->second, 
                                                         file_record->dataLogicalSize());
            }
            
            if (file_record->rsrcLogicalSize() > 0) {
                // Create key for resource fork
                auto rsrc_key = std::make_pair(fileID, static_cast<uint8_t>(0xFF));
                
                // Get or create hfs_file_t for resource fork
                auto it = files_.find(rsrc_key);
                if (it == files_.end()) {
                    // Create new hfs_file_t with logical size
                    it = files_.emplace(rsrc_key, hfs_file_t(*this, file_record->rsrcLogicalSize())).first;
                    
                    // Add catalog extents
                    for (int i = 0; i < 3; i++) {
                        uint16_t start = be16(file_record->rsrcExtents()[i].startBlock);
                        uint16_t count = be16(file_record->rsrcExtents()[i].blockCount);
                        if (count > 0) {
                            it->second.add_extent({start, count});
                        }
                    }
                } else {
                    // Set logical size if not already set
                    it->second.set_logical_size(file_record->rsrcLogicalSize());
                }
                
                // Create adapter using the complete extents
                rsrc_fork = std::make_unique<hfs_fork_t>(&it->second, 
                                                         file_record->rsrcLogicalSize());
            }
            
            std::shared_ptr<File> file = std::make_shared<File>(
                disk,
                from_macroman(catalog_record->name()),
                file_record->type(),
                file_record->creator(),
                std::move(data_fork),
                std::move(rsrc_fork));

                // Find parent folder and add file to it
            auto parent_it = folders.find(parent_id);
            if (parent_it != folders.end()) {
                parent_it->second->add_file(file);
            }
            else
            {
                std::cerr << std::format("Warning: Parent folder ID {} not found for file {}\n",
                                         parent_id,
                                         catalog_record->name());
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

// Helper method implementation
const hfs_file_t* hfs_partition_t::get_file(uint32_t fileID, uint8_t forkType) const
{
    auto key = std::make_pair(fileID, forkType);
    auto it = files_.find(key);
    return (it != files_.end()) ? &it->second : nullptr;
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