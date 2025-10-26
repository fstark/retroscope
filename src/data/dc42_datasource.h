#pragma once

#include "data/data.h"
#include <memory>

// Function to check if a data source contains a DC42 file and wrap it appropriately
std::shared_ptr<datasource_t> make_dc42_datasource(std::shared_ptr<datasource_t> source);