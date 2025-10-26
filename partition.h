#pragma once

#include <memory>
#include <string>
#include "data.h"
#include "file.h"

class file_visitor_t;
class data_source_t;

/**
 * Abstract base class for filesystem partitions.
 * Provides a unified interface for both MFS and HFS partitions.
 */
class partition_t {
public:
    virtual ~partition_t() = default;

    /**
     * Get the root folder of the partition.
     * @return Shared pointer to the root folder
     */
    virtual std::shared_ptr<Folder> get_root_folder() = 0;

    /**
     * Factory method to create the appropriate partition type based on the data source.
     * Automatically detects whether the partition is MFS or HFS.
     * @param source The data source to analyze
     * @return Unique pointer to the appropriate partition implementation, or nullptr if not recognized
     */
    static std::unique_ptr<partition_t> create(std::shared_ptr<data_source_t> source);

    /**
     * Check if a data source contains an HFS partition.
     * @param source The data source to check
     * @return True if HFS partition detected
     */
    static bool is_hfs(std::shared_ptr<data_source_t> source);

    /**
     * Check if a data source contains an MFS partition.
     * @param source The data source to check
     * @return True if MFS partition detected
     */
    static bool is_mfs(std::shared_ptr<data_source_t> source);

protected:
    partition_t() = default;
};