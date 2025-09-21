#include "utils.h"

#include <iostream>
#include <iomanip>
#include <format>
#include <cctype>

// Convert Pascal string to C++ string
std::string string_from_pstring(const uint8_t *pascalStr)
{
    if (pascalStr[0] == 0)
        return std::string();
    return std::string(reinterpret_cast<const char *>(pascalStr + 1), static_cast<size_t>(pascalStr[0]));
}

// Format type/creator codes as 4-character strings
std::string string_from_code(uint32_t code)
{
    if (code == 0) return "    ";
    
    char chars[5];
    chars[0] = (code >> 24) & 0xFF;
    chars[1] = (code >> 16) & 0xFF;
    chars[2] = (code >> 8) & 0xFF;
    chars[3] = code & 0xFF;
    chars[4] = '\0';
    
    // Replace non-printable characters with '.'
    for (int i = 0; i < 4; i++) {
        if (chars[i] < 32 || chars[i] > 126) {
            chars[i] = '.';
        }
    }
    
    return std::string(chars);
}

void dump(const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; i += 16)
    {
        std::cout << std::hex << std::setw(8) << std::setfill('0') << i << ": ";
        for (size_t j = 0; j < 16 && i + j < size; j++)
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<int>(data[i + j]) << " ";
        }
        std::cout << " ";
        for (size_t j = 0; j < 16 && i + j < size; j++)
        {
            char c = static_cast<char>(data[i + j]);
            if (std::isprint(static_cast<unsigned char>(c)))
                std::cout << c;
            else
                std::cout << ".";
        }
        std::cout << std::endl;
    }
}

void dump(const std::vector<uint8_t> &data)
{
    dump(data.data(), data.size());
}

// Sanitize string by replacing control characters with escape codes and doubling backslashes
std::string sanitize_string(const std::string &str)
{
    std::string result;
    result.reserve(str.size() * 2); // Reserve space to avoid frequent reallocations
    
    for (char c : str)
    {
        if (c == '\\')
        {
            result += "\\\\"; // Double backslashes
        }
        else if (c == '\n')
        {
            result += "\\n";
        }
        else if (c == '\r')
        {
            result += "\\r";
        }
        else if (c == '\t')
        {
            result += "\\t";
        }
        else if (c == '\0')
        {
            result += "\\0";
        }
        else if (c == '\a')
        {
            result += "\\a";
        }
        else if (c == '\b')
        {
            result += "\\b";
        }
        else if (c == '\f')
        {
            result += "\\f";
        }
        else if (c == '\v')
        {
            result += "\\v";
        }
        else if (c == '\"')
        {
            result += "\\\"";
        }
        // else if (c == '\'')
        // {
        //     result += "\\'";
        // }
        else if (std::iscntrl(static_cast<unsigned char>(c)))
        {
            // For other control characters, use hex escape
            result += "\\x";
            result += std::format("{:02x}", static_cast<unsigned char>(c));
        }
        else
        {
            result += c; // Regular printable character
        }
    }
    
    return result;
}