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

public:
    explicit mfs_partition_t(std::shared_ptr<data_source_t> source);
    
    // Scan the MFS partition and visit all files
    void scan_partition(file_visitor_t &visitor);
};