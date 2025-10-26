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
#include "partition.h"
#include "mfs_file_fork.h"

// Forward declarations
class mfs_partition_t;

// Function to check if a data source contains an MFS partition
bool is_mfs(std::shared_ptr<data_source_t> source);

// MFS partition class
class mfs_partition_t : public partition_t
{
private:
    std::shared_ptr<data_source_t> source_;
    std::shared_ptr<Folder> root_folder_;  // Cached root folder
    
    // Helper functions for reading file content
    std::vector<uint8_t> read_file_fork(const MFSDirectoryEntry *entry, bool is_resource_fork) const;
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> read_file_content(const MFSDirectoryEntry *entry) const;
    
    // Helper function to build the root folder structure
    void build_root_folder();

public:
    explicit mfs_partition_t(std::shared_ptr<data_source_t> source);
    
    // Abstract interface implementation
    std::shared_ptr<Folder> get_root_folder() override;
    
    // Static method to check if data source is MFS
    static bool is_mfs(std::shared_ptr<data_source_t> source);
};