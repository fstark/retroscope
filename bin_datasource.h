#pragma once

#include "data.h"
#include <memory>
#include <vector>

// Creates a data source for CD-ROM BIN files
// BIN files contain raw CD-ROM sectors with sync patterns and error correction
// This function detects BIN format and returns a stripped data source containing just the data
std::shared_ptr<data_source_t> make_bin_data_source(std::shared_ptr<data_source_t> source);