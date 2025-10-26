#pragma once

#include "data/data.h"
#include <memory>
#include <vector>

// Creates a data source for CD-ROM BIN files
// BIN files contain raw CD-ROM sectors with sync patterns and error correction
// This function detects BIN format and returns a stripped data source containing just the data
std::shared_ptr<datasource_t> make_bin_datasource(std::shared_ptr<datasource_t> source);