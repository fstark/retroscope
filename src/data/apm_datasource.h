#pragma once

#include "data/data.h"
#include <memory>
#include <vector>

// Function to check if a data source contains an Apple Partition Map
// Returns a vector of data sources for each partition found
std::vector<std::shared_ptr<datasource_t>> make_apm_datasource(std::shared_ptr<datasource_t> source);