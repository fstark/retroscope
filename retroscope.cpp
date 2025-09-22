#include "retroscope.h"
#include "data.h"
#include "hfs_parser.h"
#include "apm_datasource.h"
#include "dc42_datasource.h"
#include "bin_datasource.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <format>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cstddef>
#include <vector>
#include <filesystem>
#include <map>
#include <tuple>

class terse_visitor_t : public file_visitor_t
{
public:
    void visit_file(std::shared_ptr<File> file) override
    {
        if (gType != "" && file->type() != gType)
            return;
        if (gCreator != "" && file->creator() != gCreator)
            return;
        if (gFind != "" && !has_case_insensitive_substring(file->name(), gFind))
            return;
        std::cout << file->name()
                  << " (" << file->type() << "/" << file->creator() << ")"
                  << " DATA:" << file->data_size() << " bytes"
                  << " RSRC:" << file->rsrc_size() << " bytes" << std::endl;
    }
};

class dump_visitor_t : public file_visitor_t
{
    size_t indent_ = 0;

public:
    void visit_file(std::shared_ptr<File> file) override
    {
        std::string indent_str(static_cast<size_t>(indent_ * 2), ' ');
        std::cout << indent_str << "File: " << file->name()
                  << " (" << file->type() << "/" << file->creator() << ")"
                  << " DATA: " << file->data_size() << " bytes"
                  << " RSRC: " << file->rsrc_size() << " bytes" << std::endl;
    }
    void pre_visit_folder(std::shared_ptr<Folder> folder) override
    {
        std::string indent_str(static_cast<size_t>(indent_ * 2), ' ');
        std::cout << indent_str << "Folder: " << folder->name() << std::endl;
        indent_++;
    }
    void post_visit_folder(std::shared_ptr<Folder>) override
    {
        indent_--;
    }
};

// Template function to get arguments from flags map with type conversion
template <typename T>
T get_arg(const std::map<std::string, std::string> &flags, const std::string &key, const T &default_value = T{})
{
    auto it = flags.find(key);
    if (it == flags.end())
    {
        return default_value;
    }

    // Generic implementation - should be specialized for specific types
    return default_value;
}

// Specialization for int
template <>
int get_arg<int>(const std::map<std::string, std::string> &flags, const std::string &key, const int &default_value)
{
    auto it = flags.find(key);
    if (it == flags.end())
    {
        return default_value;
    }

    try
    {
        return std::stoi(it->second);
    }
    catch (const std::exception &)
    {
        return default_value;
    }
}

// Specialization for bool
template <>
bool get_arg<bool>(const std::map<std::string, std::string> &flags, const std::string &key, const bool &default_value)
{
    auto it = flags.find(key);
    if (it == flags.end())
    {
        return default_value;
    }

    return it->second == "true";
}

// Specialization for std::string
template <>
std::string get_arg<std::string>(const std::map<std::string, std::string> &flags, const std::string &key, const std::string &default_value)
{
    auto it = flags.find(key);
    if (it == flags.end())
    {
        return default_value;
    }

    return it->second;
}

void process_disk_image(const std::filesystem::path &filepath)
{
    // Create initial data source
    auto file_source = std::make_shared<file_data_source_t>(filepath);
#ifdef VERBOSE
    std::cout << "Analyzing disk image: " << filepath << " (" << file_source->size() << " bytes)\n";
#endif

    // Start with the file itself
    std::vector<std::shared_ptr<data_source_t>> sources = {file_source};

    // Process all sources, expanding them if they contain sub-partitions
    for (size_t i = 0; i < sources.size(); ++i)
    {
        auto source = sources[i];

        // Try BIN unwrapping (CD-ROM format)
        auto bin_source = make_bin_data_source(source);
        if (bin_source != source)
        {
            sources.push_back(bin_source);
            continue;
        }

        // Try DC42 unwrapping
        auto dc42_source = make_dc42_data_source(source);
        if (dc42_source != source)
        {
            sources.push_back(dc42_source);
            continue;
        }

        // Try APM expansion
        auto apm_partitions = make_apm_data_source(source);
        if (!apm_partitions.empty())
        {
#ifdef VERBOSE
            std::cout << "Found Apple Partition Map with " << apm_partitions.size() << " partitions\n";
#endif
            for (auto &partition : apm_partitions)
            {
                sources.push_back(partition);
            }
        }
    }

    // Now check each source for HFS and parse if found
    for (auto &source : sources)
    {
        if (is_hfs(source))
        {
#ifdef VERBOSE
            std::cout << "Analyzing disk image: " << filepath << " (" << file_source->size() << " bytes)\n";
            std::cout << "Found HFS partition\n";
#endif
            try
            {
                partition_t partition(source);
                auto root = partition.get_root_folder();
                // dump_visitor_t dumper;
                terse_visitor_t dumper;
                visit_folder(root, dumper);
            }
            catch (const std::exception &hfs_error)
            {
                std::cerr << "In disk image: " << filepath << " (" << file_source->size() << " bytes)\n";
                std::cerr << "Error parsing HFS partition: " << hfs_error.what() << "\n";
                std::cerr << "Continuing with other partitions...\n";
            }
        }
    }
}

void process_single_path(const std::filesystem::path &path)
{
    if (std::filesystem::is_directory(path))
    {
        std::cout << "Processing directory: " << path << "\n";
        try
        {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(path))
            {
                if (entry.is_regular_file())
                {
                    try
                    {
                        process_disk_image(entry.path());
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error processing file " << entry.path() << ": " << e.what() << "\n";
                        std::cerr << "Continuing with other files...\n";
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "Error accessing directory " << path << ": " << e.what() << "\n";
        }
    }
    else if (std::filesystem::is_regular_file(path))
    {
        process_disk_image(path);
    }
    else
    {
        std::cerr << "Error: " << path << " is not a regular file or directory\n";
        throw std::runtime_error("Invalid path type");
    }
}

void process_path(const std::vector<std::filesystem::path> &paths)
{
    for (const auto &path : paths)
    {
        try
        {
            process_single_path(path);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error processing " << path << ": " << e.what() << "\n";
            std::cerr << "Continuing with remaining paths...\n";
        }
    }
}

std::tuple<std::map<std::string, std::string>, std::vector<std::filesystem::path>> parse_arguments(int argc, char *argv[])
{
    std::map<std::string, std::string> flags;
    std::vector<std::filesystem::path> paths;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        // Check if argument starts with '--'
        if (arg.starts_with("--"))
        {
            std::string key;
            std::string value;

            // Check if it's in the format --xxx=yyy or --xxx=
            size_t equals_pos = arg.find('=');
            if (equals_pos != std::string::npos)
            {
                // Format: --xxx=yyy or --xxx=
                key = arg.substr(2, equals_pos - 2);
                value = arg.substr(equals_pos + 1); // This handles both --xxx=yyy and --xxx=
                flags[key] = value;
            }
            else
            {
                // Format: --xxx (no equals sign)
                key = arg.substr(2);
                flags[key] = "true";
            }
        }
        else
        {
            // Not a flag, treat as a path
            paths.emplace_back(arg);
        }
    }

    return std::make_tuple(flags, paths);
}

using namespace std::string_literals;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <disk_image_file_or_directory> [additional_paths...]\n";
        std::cerr << "Analyzes vintage Macintosh HFS disk images and list content.\n";
        std::cerr << "If a directory is provided, recursively processes all files in it.\n";
        std::cerr << "Multiple paths can be specified to process them all.\n";
        return 1;
    }

    try
    {
        // Parse command line arguments
        auto [flags, paths] = parse_arguments(argc, argv);

        // Check if we have any paths to process
        if (paths.empty())
        {
            std::cerr << "Error: No paths specified for processing\n";
            return 1;
        }

        gVerbose = get_arg(flags, "verbose", false);
        gType = get_arg(flags, "type", ""s);
        gCreator = get_arg(flags, "creator", ""s);
        gFind = get_arg(flags, "find", ""s);
        gDump = get_arg(flags, "dump", false);

        //  If gType is in the form of "XXXX/XXXX" split into type and creator
        size_t slash_pos = gType.find('/');
        if (slash_pos != std::string::npos)
        {
            gType = gType.substr(0, slash_pos);
            gCreator = gType.substr(slash_pos + 1);
        }

        // Process all paths
        process_path(paths);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
