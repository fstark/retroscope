#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <map>
#include <algorithm>
#include <array>
#include <memory>
#include <functional>
#include "data.h"
#include <format>
#include <cassert>

#include "hfs.h"
#include "utils.h"
#include "file.h"
#include "data.h"

#define noVERBOSE

// Forward declarations
class btree_header_node_t;
class master_directory_block_t;
class partition_t;
class btree_file_t;

// Function to check if a data source contains an HFS partition
bool is_hfs(std::shared_ptr<data_source_t> source);

struct hierarchy_t
{
	uint32_t parent_id;
	uint32_t child_id;
};

struct extent_t
{
	uint16_t start;
	uint16_t count;
};

// Free-standing functions to convert block_t to specific node types
btree_header_node_t as_btree_header_node(block_t &block);
master_directory_block_t as_master_directory_block(block_t &block);

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
	extent_t get_extent(int index) const
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
};

class file_t
{
	std::vector<extent_t> extents_;
	partition_t &partition_;

public:
	file_t(partition_t &partition) : partition_(partition) {}

	void add_extent(const extent_t &extend);
	// same with sanity check
	void add_extent(uint16_t start_block, const extent_t &extent);
	partition_t &partition() { return partition_; }
	uint16_t to_absolute_block(uint16_t block) const;

	uint32_t allocation_offset(uint32_t offset) const;

	btree_file_t as_btree_file();

	// Getters for accessing extents
	const std::vector<extent_t> &extents() const { return extents_; }
	bool has_extents() const { return !extents_.empty(); }
};

class partition_t
{
	std::shared_ptr<data_source_t> data_source_;
	uint64_t allocationStart_ = 0;
	uint64_t allocationBlockSize_ = 512;

	file_t extents_;
	file_t catalog_;

	block_t read(uint64_t blockOffset) const;
	void build_root_folder();

	std::shared_ptr<Folder> root_folder;
	std::map<uint32_t, std::shared_ptr<Folder>> folders;

public:
	partition_t(std::shared_ptr<data_source_t> data_source);

	std::shared_ptr<Folder> get_root_folder() { return root_folder; }

	uint64_t allocation_start() { return allocationStart_; }
	uint32_t allocation_block_size() { return allocationBlockSize_; }
	block_t read_allocated_block(uint16_t block, uint16_t size = 0xffff);

	//	reads in the allocation zone
	block_t read_allocation(uint32_t offset, uint16_t size)
	{
#ifdef VERBOSE
		std::cout << std::format("read_allocation({},{})\n", offset, size);
#endif

		auto disk_offset = allocationStart_ * 512 + offset;
		return data_source_->read_block(disk_offset, size);
	}

	void readCatalogHeader(uint64_t catalogExtendStartBlock);
	void readCatalogRoot(uint16_t rootNode);
	void dumpextentTree();
};

class btree_file_t
{
	file_t &file_;

	uint32_t first_leaf_node_; //	first leaf node position relative to file
	uint16_t node_size_;	   //	node size in bytes

public:
	btree_file_t(file_t &file);

	//	Callback gets a btree_leaf_node_t&
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

	//	Callback gets a uint8_t pointer to records
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