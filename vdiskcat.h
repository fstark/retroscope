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

#include "hfs.h"

// Forward declarations
class btree_header_node_t;
class master_directory_block_t;
class partition_t;
class btree_file_t;

// Utility functions
uint16_t be16toh_custom(uint16_t val);
uint32_t be32toh_custom(uint32_t val);
std::string pascalToString(const uint8_t *pascalStr);
void dump(const std::vector<uint8_t> &data);

// Classes
class block_t
{
	std::vector<uint8_t> data_;

public:
	block_t(const std::vector<uint8_t> &data) : data_(data) {}

	btree_header_node_t as_btree_header_node();
	master_directory_block_t as_master_directory_block();

	void *data() { return data_.data(); }
	void dump() const { ::dump(data_); }
};

template <class T> class type_node_t
{
protected:
	block_t &block_;
	T *content;
public:
	type_node_t(block_t &block) : block_(block)
	{
		content = reinterpret_cast<T *>(block_.data());
	}

	T *operator->() { return content; }
	const T *operator->() const { return content; }
};

class master_directory_block_t : public type_node_t<HFSMasterDirectoryBlock>
{
public:
	master_directory_block_t(block_t &block) : type_node_t<HFSMasterDirectoryBlock>(block) {}

	bool isHFSVolume() const;
	std::string getVolumeName() const;
	uint32_t allocationBlockSize() const;
	uint16_t allocationBlockStart() const;
	uint16_t extendsExtendStart(int index) const;
	uint16_t extendsExtendCount(int index) const;
	uint32_t catalogExtendStart(int index) const;
	uint16_t catalogExtendCount(int index) const;
};

class btree_header_node_t : public type_node_t<BTNodeDescriptor>
{
	BTHeaderRec *header_record_;

	public:
		btree_header_node_t(block_t &block) : type_node_t<BTNodeDescriptor>(block)
		{
			std::cout << "Node kind: " << (int)content->kind << "\n";
			if (content->kind != ndHdrNode)
			{
				throw std::runtime_error("Not a valid B-tree header node");
			}
		    header_record_ = reinterpret_cast<BTHeaderRec *>((char *)block.data() + sizeof(BTNodeDescriptor));
		}

		uint32_t first_leaf_node() const { return be32toh_custom(header_record_->firstLeafNode); }
		uint16_t node_size() const { return be16toh_custom(header_record_->nodeSize); }
	};

class file_t
{
	struct extent_t
	{
		uint16_t start;
		uint16_t count;
	};

	std::vector<extent_t> extents_;
	partition_t &partition_;
	uint32_t block_size_ = 512;	//	For now fixed

public:
	file_t(partition_t &partition) : partition_(partition) {}

	void add_extent(uint16_t start, uint16_t count);
	partition_t &partition() { return partition_; }
	uint16_t to_absolute_block(uint16_t block) const;
	btree_file_t as_btree_file();
};

class btree_file_t
{
	file_t &file_;
public:
	btree_file_t(file_t &file);
};

class partition_t
{
    std::ifstream &file_;
    uint64_t partitionStart_;
    uint64_t partitionSize_;
    uint64_t allocationStart_ = 0;
    uint64_t allocationBlockSize_ = 512;

    file_t extents_;
    file_t catalog_;
    uint16_t rootNode_;

	block_t read(uint64_t blockOffset) const;
    std::vector<uint8_t> readBlock512(uint64_t blockOffset);
    std::vector<uint8_t> readBlock(uint64_t blockOffset);

public:
    partition_t(std::ifstream &file, uint64_t partitionStart, uint64_t partitionSize);

	uint64_t allocation_start() { return allocationStart_; }
	uint32_t allocation_block_size() { return allocationBlockSize_; }
	block_t read_allocated_block( uint16_t block , uint16_t size=0xffff );
    void readMasterDirectoryBlock();

    void readCatalogHeader(uint64_t catalogExtendStartBlock);
    void readCatalogRoot(uint16_t rootNode);
    void dumpextentTree();
};

// Function declarations
void processAPM(std::ifstream &file);

// Structure to hold file/directory information
struct CatalogEntry
{
    std::string name;
    uint32_t parentID;
    uint32_t cnid;
    bool isDirectory;
    uint32_t size;
    uint32_t createDate;
    uint32_t modifyDate;
};

// HFS volume context
struct HFSVolume
{
    std::ifstream *file;
    uint64_t partitionStart;
    uint16_t allocationBlockSize;
    uint16_t firstAllocationBlock;

    HFSVolume(std::ifstream *f, uint64_t start) : file(f), partitionStart(start) {}
};
