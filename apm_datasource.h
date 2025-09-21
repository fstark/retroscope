#pragma once

#include "data.h"
#include <memory>
#include <vector>

// Function to check if a data source contains an Apple Partition Map
// Returns a vector of data sources for each partition found
std::vector<std::shared_ptr<data_source_t>> make_apm_data_source(std::shared_ptr<data_source_t> source);