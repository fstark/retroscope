#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Utility function declarations
std::string string_from_pstring(const uint8_t *pascalStr);
std::string string_from_code(uint32_t code);
std::string from_macroman(const std::string &macroman_str);
std::string sanitize_string(const std::string &str);
void dump(const std::vector<uint8_t> &data);
void dump(const uint8_t *data, size_t size);