#include "partition.h"
#include "hfs/hfs_partition.h"
#include "mfs/mfs_partition.h"
#include "utils.h"

std::shared_ptr<partition_t> partition_t::create(std::shared_ptr<datasource_t> source)
{
    ENTRY("");

    // Try HFS first
    if (is_hfs(source)) {
        rs_log("Creating HFS partition");
        auto partition = std::make_shared<hfs_partition_t>(source);
        // Now that we have a shared_ptr, we can build the files
        std::static_pointer_cast<hfs_partition_t>(partition)->complete_initialization();
        return partition;
    }

    // Try MFS
    if (is_mfs(source)) {
        rs_log("Creating MFS partition");
        return std::make_shared<mfs_partition_t>(source);
    }

    rs_log("Unknown partition type");
    return nullptr;
}

bool partition_t::is_hfs(std::shared_ptr<datasource_t> source)
{
    return hfs_partition_t::is_hfs(source);
}

bool partition_t::is_mfs(std::shared_ptr<datasource_t> source)
{
    return mfs_partition_t::is_mfs(source);
}