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

#include "mfs.h"
#include "utils.h"
#include "file.h"
#include "data.h"

// Forward declarations
class mfs_partition_t;

// Function to check if a data source contains an MFS partition
bool is_mfs(std::shared_ptr<data_source_t> source);

// MFS partition class
class mfs_partition_t
{
private:
    std::shared_ptr<data_source_t> source_;
    
    // Helper functions for reading file content
    std::vector<uint8_t> read_file_fork(const MFSDirectoryEntry *entry, bool is_resource_fork) const;
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> read_file_content(const MFSDirectoryEntry *entry) const;

public:
    explicit mfs_partition_t(std::shared_ptr<data_source_t> source);
    
    // Scan the MFS partition and visit all files
    void scan_partition(file_visitor_t &visitor);
};

// MFS-specific File implementation
class MFSFile : public File
{
	std::vector<uint8_t> data_content_;
	std::vector<uint8_t> rsrc_content_;

public:
	MFSFile(const std::shared_ptr<Disk> &disk,
			const std::string &name, const std::string &type,
			const std::string &creator, uint32_t data_size, uint32_t rsrc_size,
			std::vector<uint8_t> data_content, std::vector<uint8_t> rsrc_content)
		: File(disk, name, type, creator, data_size, rsrc_size),
		  data_content_(std::move(data_content)), rsrc_content_(std::move(rsrc_content)) {}

	std::vector<uint8_t> read_data(uint32_t offset = 0, uint32_t size = UINT32_MAX) override
	{
		if (offset >= data_content_.size()) return {};
		size = std::min(size, static_cast<uint32_t>(data_content_.size()) - offset);
		return {data_content_.begin() + offset, data_content_.begin() + offset + size};
	}

	std::vector<uint8_t> read_rsrc(uint32_t offset = 0, uint32_t size = UINT32_MAX) override
	{
		if (offset >= rsrc_content_.size()) return {};
		size = std::min(size, static_cast<uint32_t>(rsrc_content_.size()) - offset);
		return {rsrc_content_.begin() + offset, rsrc_content_.begin() + offset + size};
	}
};