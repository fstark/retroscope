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
#include "data/data.h"
#include <format>
#include <cassert>

#include "hfs/mac_hfs.h"
#include "utils.h"
#include "file/file.h"
#include "file/folder.h"
#include "file/disk.h"
#include "data/data.h"
#include "partition.h"
#include "hfs/hfs_fork.h"

#define noVERBOSE

/**
 * Check if a data source contains an HFS partition.
 * @param source Data source to examine
 * @return True if the source appears to contain a valid HFS filesystem
 */
bool is_hfs(std::shared_ptr<datasource_t> source);

// Forward declarations for internal types
class btree_file_t;
class hfs_partition_t;

/**
 * Represents a file in an HFS filesystem with extent management.
 * Handles files that may span multiple non-contiguous allocation blocks,
 * including support for extents overflow file for large files.
 */

class hfs_file_t
{
public:
	/**
	 * Represents a contiguous block extent in HFS allocation space.
	 */
	struct extent_t
	{
		uint16_t start;  ///< Starting allocation block number
		uint16_t count;  ///< Number of consecutive blocks
	};

private:
	std::vector<extent_t> extents_;
	hfs_partition_t &partition_;
	uint32_t logical_size_ = 0;

public:
	/**
	 * Construct an HFS file associated with a partition.
	 * @param partition Reference to the HFS partition containing this file
	 */
	hfs_file_t(hfs_partition_t &partition) : partition_(partition) {}
	
	/**
	 * Construct an HFS file with known logical size.
	 * @param partition Reference to the HFS partition containing this file
	 * @param logical_size Logical size of the file in bytes
	 */
	hfs_file_t(hfs_partition_t &partition, uint32_t logical_size) 
		: partition_(partition), logical_size_(logical_size) {}

	/**
	 * Add an extent to this file.
	 * @param extend The extent to add
	 */
	void add_extent(const extent_t &extend);
	
	/**
	 * Add an extent with continuity validation.
	 * @param start_block Expected starting block number for validation
	 * @param extent The extent to add
	 */
	void add_extent(uint16_t start_block, const extent_t &extent);
	
	/**
	 * Get the partition containing this file.
	 * @return Reference to the HFS partition
	 */
	hfs_partition_t &partition() { return partition_; }
	
	/**
	 * Convert a file-relative block number to absolute disk block.
	 * @param block File-relative block number
	 * @return Absolute block number on disk
	 */
	uint16_t to_absolute_block(uint16_t block) const;

	/**
	 * Convert a byte offset to allocation space offset.
	 * @param offset Byte offset within the file
	 * @return Offset within the allocation space
	 */
	uint64_t allocation_offset(uint32_t offset) const;

	/**
	 * Set the logical size of this file.
	 * @param size Logical size in bytes
	 */
	void set_logical_size(uint32_t size) { 
		logical_size_ = size; 
	}
	
	/**
	 * Get the logical size of this file.
	 * @return Logical size in bytes
	 */
	uint32_t logical_size() const { return logical_size_; }

	/**
	 * Read the entire file content.
	 * @return Vector containing all file data
	 */
	std::vector<uint8_t> read() const;
	
	/**
	 * Read a portion of the file.
	 * @param offset Byte offset from start of file
	 * @param size Number of bytes to read
	 * @return Vector containing the requested data
	 */
	std::vector<uint8_t> read(uint32_t offset, uint32_t size) const;

	/**
	 * Create a B-tree file accessor for this file.
	 * @return B-tree file interface
	 */
	btree_file_t as_btree_file();

	/**
	 * Get the extents list for this file.
	 * @return Const reference to the extents vector
	 */
	const std::vector<extent_t> &extents() const { return extents_; }
	
	/**
	 * Check if this file has any extents.
	 * @return True if the file has extents
	 */
	bool has_extents() const { return !extents_.empty(); }
};

/**
 * HFS (Hierarchical File System) partition implementation.
 * Provides access to files and folders in HFS volumes, supporting
 * hierarchical directory structure, B-tree catalog and extents files,
 * and unlimited file extents via overflow handling.
 */
class hfs_partition_t : public partition_t, public std::enable_shared_from_this<hfs_partition_t>
{
	std::shared_ptr<datasource_t> datasource_;
	uint64_t allocationStart_ = 0;
	uint64_t allocationBlockSize_ = 512;

	hfs_file_t extents_;
	hfs_file_t catalog_;
	
	// Map to store complete files: (fileID, forkType) -> hfs_file_t
	// forkType: 0 = data fork, 0xFF = resource fork
	std::map<std::pair<uint32_t, uint8_t>, hfs_file_t> files_;

	/**
	 * Read a block from the partition.
	 * @param blockOffset Block offset within the partition
	 * @return Block data
	 */
	block_t read(uint64_t blockOffset) const;
	
	/**
	 * Initialize basic partition metadata (allocation info, extents, catalog setup).
	 * This is safe to call during construction.
	 */
	void initialize_partition();

	/**
	 * Build the root folder by parsing HFS catalog and extents B-trees.
	 * Creates the complete file and folder hierarchy.
	 * Must be called after the object is managed by a shared_ptr.
	 */
	void build_root_folder();

	std::shared_ptr<Folder> root_folder;
	std::map<uint32_t, std::shared_ptr<Folder>> folders;

	/**
	 * Get a complete file with all extents.
	 * @param fileID HFS file ID
	 * @param forkType Fork type (0=data, 0xFF=resource)
	 * @return Pointer to hfs_file_t or nullptr if not found
	 */
	const hfs_file_t* get_file(uint32_t fileID, uint8_t forkType) const;

public:
	/**
	 * Construct HFS partition from a data source.
	 * @param datasource Data source containing the HFS volume
	 */
	hfs_partition_t(std::shared_ptr<datasource_t> datasource);

	/**
	 * Complete the initialization by building files and folders.
	 * Must be called after the object is managed by a shared_ptr.
	 */
	void complete_initialization() { build_root_folder(); }

	/**
	 * Get the underlying data source.
	 * @return Reference to the data source
	 */
	const datasource_t &datasource() const { return *datasource_; }

	/**
	 * Get the root folder of this HFS volume.
	 * @return Shared pointer to the root folder
	 */
	std::shared_ptr<Folder> get_root_folder() override { return root_folder; }
	
	/**
	 * Check if a data source contains an HFS partition.
	 * @param source Data source to examine
	 * @return True if the source appears to contain a valid HFS filesystem
	 */
	static bool is_hfs(std::shared_ptr<datasource_t> source);

	/**
	 * Get the allocation area start block.
	 * @return Starting block number of allocation area
	 */
	uint64_t allocation_start() { return allocationStart_; }
	
	/**
	 * Get the allocation block size.
	 * @return Size of allocation blocks in bytes
	 */
	uint32_t allocation_block_size() { return allocationBlockSize_; }
	
	/**
	 * Read an allocated block from the volume.
	 * @param block Allocation block number
	 * @param size Size to read (default: full allocation block)
	 * @return Block data
	 */
	block_t read_allocated_block(uint16_t block, uint16_t size = 0xffff);

	/**
	 * Read data from the allocation zone.
	 * 
	 * Note: This method may return fewer bytes than requested if:
	 * - The request would exceed partition boundaries
	 * - The request crosses allocation block boundaries
	 * - End of file/extent is reached
	 * 
	 * Callers must check the actual size of the returned block_t
	 * using block_t.size() rather than assuming the requested size.
	 * 
	 * @param offset Byte offset within allocation zone
	 * @param size Number of bytes to read (may receive less)
	 * @return Block containing the actual data read (use .size() for actual bytes)
	 */
	block_t read_allocation(uint64_t offset, uint32_t size) {
#ifdef VERBOSE
		std::cout << std::format("read_allocation({},{})\n", offset, size);
#endif
		
		if (!datasource_) {
			throw std::runtime_error("Null datasource");
		}

		uint64_t disk_offset = static_cast<uint64_t>(allocationStart_) * 512 + offset;
		
		// Check bounds to prevent reading beyond partition
		if (disk_offset + size > datasource_->size()) {
			throw std::out_of_range("Read beyond partition bounds");
		}
		
		return datasource_->read_block(disk_offset, size);
	}

	/**
	 * Read catalog header (legacy method).
	 * @param catalogExtendStartBlock Starting block of catalog
	 */
	void readCatalogHeader(uint64_t catalogExtendStartBlock);
	
	/**
	 * Read catalog root (legacy method).
	 * @param rootNode Root node number
	 */
	void readCatalogRoot(uint16_t rootNode);
	
	/**
	 * Dump the extents B-tree for debugging.
	 */
	void dumpextentTree();
};

