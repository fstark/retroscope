#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <format>
#include <iostream>

#define noDEBUG_MEMORY

extern std::string gType;
extern std::string gCreator;
extern std::string gName;
extern bool gGroup;

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

void rs_log_increment();
void rs_log_decrement();
int get_log_indent();

template <typename... Args>
void rs_log(const std::format_string<Args...> format_str, Args &&...args)
{
    // Add indentation based on current log_indent level
    int indent = get_log_indent();
    std::string indent_str(static_cast<size_t>(indent * 4), ' ');
    std::clog << indent_str << std::format(format_str, std::forward<Args>(args)...) << std::endl;
}

template <typename... Args>
void rs_log_function(const std::string &function, const std::string &filename, int lineno, const std::format_string<Args...> format_str, Args &&...args)
{
    // Add indentation based on current log_indent level
    int indent = get_log_indent();
    std::string indent_str(static_cast<size_t>(indent * 4), ' ');
    std::clog
        << indent_str
        << std::format("--> {} (", function)
        << std::format(format_str, std::forward<Args>(args)...)
        << std::format(") at {}:{}", filename, lineno)
        << std::endl;
    rs_log_increment();
}

// RAII helper class for automatic decrement on scope exit
class rs_log_scope_guard
{
    std::string function_name;

public:
    rs_log_scope_guard(const std::string &func_name) : function_name(func_name) {}
    ~rs_log_scope_guard()
    {
        rs_log_decrement();
        rs_log("<-- {}", function_name);
    }
    // Non-copyable, non-movable
    rs_log_scope_guard(const rs_log_scope_guard &) = delete;
    rs_log_scope_guard &operator=(const rs_log_scope_guard &) = delete;
    rs_log_scope_guard(rs_log_scope_guard &&) = delete;
    rs_log_scope_guard &operator=(rs_log_scope_guard &&) = delete;
};

// ENTRY macro for function entry logging with automatic exit logging
#define ENTRY(...)                                              \
    rs_log_function(__func__, __FILE__, __LINE__, __VA_ARGS__); \
    rs_log_scope_guard _rs_log_guard(__func__);
