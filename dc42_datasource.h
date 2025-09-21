#pragma once

#include "data.h"
#include <memory>

// Function to check if a data source contains a DC42 file and wrap it appropriately
std::shared_ptr<data_source_t> make_dc42_data_source(std::shared_ptr<data_source_t> source);