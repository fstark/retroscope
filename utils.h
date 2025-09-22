#pragma once

#include <string>
#include <vector>
#include <cstdint>

extern bool gVerbose;
extern bool gTerse;
extern std::string gType;
extern std::string gCreator;
extern std::string gFind;
extern bool gDump;

// Utility function declarations
std::string string_from_pstring(const uint8_t *pascalStr);
std::string string_from_code(uint32_t code);
std::string from_macroman(const std::string &macroman_str);
std::string sanitize_string(const std::string &str);
bool has_case_insensitive_substring(const std::string &source, const std::string &sub);
void dump(const std::vector<uint8_t> &data);
void dump(const uint8_t *data, size_t size);

inline uint16_t be16(const uint8_t *b)
{
    return (uint16_t(b[0]) << 8) | uint16_t(b[1]);
}

inline uint16_t be16(const uint16_t b)
{
    return (uint16_t(b >> 8) & 0x00ff) | (uint16_t(b & 0x00ff) << 8);
}

inline uint32_t be32(const uint8_t *b)
{
    return (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | uint32_t(b[3]);
}

inline uint32_t be32(const uint32_t b)
{
    return (uint32_t(b >> 24) & 0x000000ff) | (uint32_t(b >> 8) & 0x0000ff00) |
           (uint32_t(b & 0x0000ff00) << 8) | (uint32_t(b & 0x000000ff) << 24);
}
