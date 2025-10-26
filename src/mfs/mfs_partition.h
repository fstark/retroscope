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

#include "mfs/mac_mfs.h"
#include "utils.h"
#include "file/file.h"
#include "file/folder.h"
#include "file/disk.h"
#include "data/data.h"
#include "partition.h"
#include "mfs/mfs_fork.h"

// Forward declarations
class mfs_partition_t;

/**
 * Check if a data source contains an MFS partition.
 * @param source Data source to examine
 * @return True if the source appears to contain a valid MFS filesystem
 */
bool is_mfs(std::shared_ptr<datasource_t> source);

/**
 * MFS (Macintosh File System) partition implementation.
 * Provides access to files and folders in classic Mac MFS volumes.
 * MFS is the original flat filesystem used by early Macintosh systems.
 */
class mfs_partition_t : public partition_t
{
private:
    std::shared_ptr<datasource_t> source_;
    std::shared_ptr<Folder> root_folder_;  // Cached root folder
    
    /**
     * Read data from a specific fork of an MFS file.
     * @param entry MFS directory entry for the file
     * @param is_resource_fork True to read resource fork, false for data fork
     * @return Vector containing the fork data
     */
    std::vector<uint8_t> read_file_fork(const MFSDirectoryEntry *entry, bool is_resource_fork) const;
    
    /**
     * Read both data and resource forks of an MFS file.
     * @param entry MFS directory entry for the file
     * @return Pair of vectors: first=data fork, second=resource fork
     */
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> read_file_content(const MFSDirectoryEntry *entry) const;
    
    /**
     * Build the root folder structure by parsing the MFS directory.
     * Creates File objects for all entries in the MFS directory.
     */
    void build_root_folder();

public:
    /**
     * Construct MFS partition from a data source.
     * @param source Data source containing the MFS volume data
     */
    explicit mfs_partition_t(std::shared_ptr<datasource_t> source);
    
    /**
     * Get the root folder of this MFS volume.
     * @return Shared pointer to the root folder containing all files
     */
    std::shared_ptr<Folder> get_root_folder() override;
    
    /**
     * Check if a data source contains an MFS partition.
     * @param source Data source to examine
     * @return True if the source appears to contain a valid MFS filesystem
     */
    static bool is_mfs(std::shared_ptr<datasource_t> source);
};