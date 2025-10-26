#include "partition.h"
#include "hfs_partition.h"
#include "mfs_partition.h"
#include "utils.h"

std::unique_ptr<partition_t> partition_t::create(std::shared_ptr<data_source_t> source)
{
    ENTRY("");

    // Try HFS first
    if (is_hfs(source)) {
        rs_log("Creating HFS partition");
        return std::make_unique<hfs_partition_t>(source);
    }

    // Try MFS
    if (is_mfs(source)) {
        rs_log("Creating MFS partition");
        return std::make_unique<mfs_partition_t>(source);
    }

    rs_log("Unknown partition type");
    return nullptr;
}

bool partition_t::is_hfs(std::shared_ptr<data_source_t> source)
{
    return hfs_partition_t::is_hfs(source);
}

bool partition_t::is_mfs(std::shared_ptr<data_source_t> source)
{
    return mfs_partition_t::is_mfs(source);
}