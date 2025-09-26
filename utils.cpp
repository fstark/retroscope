#include "utils.h"

#include <iostream>
#include <iomanip>
#include <format>
#include <cctype>
#include <algorithm>

std::string gType = "";
std::string gCreator = "";
std::string gName = "";
bool gGroup = false;

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
    if (code == 0)
        return "    ";

    char chars[5];
    chars[0] = (code >> 24) & 0xFF;
    chars[1] = (code >> 16) & 0xFF;
    chars[2] = (code >> 8) & 0xFF;
    chars[3] = code & 0xFF;
    chars[4] = '\0';

    // Replace non-printable characters with '.'
    for (int i = 0; i < 4; i++)
    {
        if (chars[i] < 32 || chars[i] > 126)
        {
            chars[i] = '.';
        }
    }

    return std::string(chars);
}

// Convert MacRoman encoded string to UTF-8
std::string from_macroman(const std::string &macroman_str)
{
    // MacRoman to Unicode mapping table for characters 0x80-0xFF
    // Characters 0x00-0x7F are identical to ASCII/UTF-8
    static const uint16_t macroman_to_unicode[128] = {
        // 0x80-0x8F
        0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1,
        0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
        // 0x90-0x9F
        0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3,
        0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC,
        // 0xA0-0xAF
        0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF,
        0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8,
        // 0xB0-0xBF
        0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211,
        0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x03A9, 0x00E6, 0x00F8,
        // 0xC0-0xCF
        0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB,
        0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153,
        // 0xD0-0xDF
        0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA,
        0x00FF, 0x0178, 0x2044, 0x20AC, 0x2039, 0x203A, 0xFB01, 0xFB02,
        // 0xE0-0xEF
        0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1,
        0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4,
        // 0xF0-0xFF
        0xF8FF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC,
        0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7};

    std::string utf8_result;
    utf8_result.reserve(macroman_str.size() * 2); // Reserve space for potential expansion

    for (unsigned char c : macroman_str)
    {
        if (c < 0x80)
        {
            // ASCII range - direct copy
            utf8_result += static_cast<char>(c);
        }
        else
        {
            // Extended MacRoman range - convert to Unicode then to UTF-8
            uint16_t unicode = macroman_to_unicode[c - 0x80];

            // Convert Unicode code point to UTF-8
            if (unicode < 0x80)
            {
                utf8_result += static_cast<char>(unicode);
            }
            else if (unicode < 0x800)
            {
                // 2-byte UTF-8 sequence
                utf8_result += static_cast<char>(0xC0 | (unicode >> 6));
                utf8_result += static_cast<char>(0x80 | (unicode & 0x3F));
            }
            else
            {
                // 3-byte UTF-8 sequence (covers Basic Multilingual Plane)
                utf8_result += static_cast<char>(0xE0 | (unicode >> 12));
                utf8_result += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                utf8_result += static_cast<char>(0x80 | (unicode & 0x3F));
            }
        }
    }

    return utf8_result;
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
    // First convert from MacRoman to UTF-8
    std::string utf8_str = from_macroman(str);

    std::string result;
    result.reserve(utf8_str.size() * 2); // Reserve space to avoid frequent reallocations

    for (char c : utf8_str)
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

// Check if source string contains substring (case-insensitive)
bool has_case_insensitive_substring(const std::string &source, const std::string &sub)
{
    // Use std::search with case-insensitive comparison
    auto it = std::search(source.begin(), source.end(),
                          sub.begin(), sub.end(),
                          [](char a, char b)
                          {
                              return std::tolower(static_cast<unsigned char>(a)) ==
                                     std::tolower(static_cast<unsigned char>(b));
                          });

    return it != source.end();
}

// Static variable for log indentation
static int log_indent = 0;

void rs_log_increment()
{
    log_indent++;
}

void rs_log_decrement()
{
    if (log_indent > 0)
        log_indent--;
}

int get_log_indent()
{
    return log_indent;
}
