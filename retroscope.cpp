#include "retroscope.h"
#include "data.h"
#include "hfs_parser.h"
#include "file_set.h"
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
#include <tuple>

std::string string_from_sizes(uint32_t min, uint32_t max)
{
    if (min == max)
        return std::format("{}", min);
    return std::format("{} to {}", min, max);
}
std::string string_from_fork_sizes(uint32_t dmin, uint32_t dmax, uint32_t rmin, uint32_t rmax)
{
    if (dmin + dmax + rmin + rmax == 0)
        return "(empty file)";

    if (rmin + rmax == 0)
        return std::format("(DATA: {} bytes)", string_from_sizes(dmin, dmax));

    if (dmin + dmax == 0)
        return std::format("(RSRC: {} bytes)", string_from_sizes(rmin, rmax));

    return std::format("(DATA: {} bytes, RSRC: {} bytes)",
                       string_from_sizes(dmin, dmax),
                       string_from_sizes(rmin, rmax));
}

std::string string_from_fork_sizes(uint32_t data_size, uint32_t rsrc_size)
{
    return string_from_fork_sizes(data_size, data_size, rsrc_size, rsrc_size);
}

std::string short_sring_from_file(const File &file)
{
    return std::format("{} {}/{}",
                       file.name(), file.type(), file.creator());
}

std::string string_from_file(const File &file)
{
    return std::format("{} {}",
                       short_sring_from_file(file),
                       string_from_fork_sizes(file.data_size(), file.rsrc_size()));
}

std::string string_from_group(const FileSet::FileGroup &group)
{
    // Sort files by total size (data + resource)
    std::vector<std::shared_ptr<File>> files = group.files;
    // std::sort(files.begin(), files.end(),
    //           [](const std::shared_ptr<File> &a, const std::shared_ptr<File> &b)
    //           {
    //               return (a->data_size() + a->rsrc_size()) < (b->data_size() + b->rsrc_size());
    //           });

    // TODO: to be rewritten with algorithm
    uint32_t min_data_size = 0xffffffff; // should be max
    uint32_t min_rsrc_size = 0xffffffff; // should be max
    uint32_t max_data_size = 0;
    uint32_t max_rsrc_size = 0;

    for (auto &file : files)
    {
        assert(file);
        min_data_size = std::min(min_data_size, file->data_size());
        min_rsrc_size = std::min(min_rsrc_size, file->rsrc_size());
        max_data_size = std::max(max_data_size, file->data_size());
        max_rsrc_size = std::max(max_rsrc_size, file->rsrc_size());
    }

    // END TODO

    return std::format("{} {}/{} {} occurences {}",
                       group.name,
                       group.type,
                       group.creator,
                       group.files.size(),
                       string_from_fork_sizes(min_data_size, max_data_size, min_rsrc_size, max_rsrc_size));
}

std::string path_string(const std::vector<std::string> &path)
{
    if (path.empty())
    {
        return "";
    }
    std::string result;
    for (size_t i = 0; i < path.size(); ++i)
    {
        // Skip empty path components
        if (path[i].empty())
        {
            continue;
        }
        if (!result.empty())
        {
            result += ":";
        }
        result += path[i];
    }
    return result;
}

class filter_t
{
public:
    virtual ~filter_t() = default;
    virtual bool matches(const File &) = 0;
    virtual bool matches(const Folder &) { return true; }
};

class filter_visitor_t : public file_visitor_t
{
    std::shared_ptr<file_visitor_t> next_;
    std::vector<std::shared_ptr<filter_t>> filters_;

public:
    filter_visitor_t(std::shared_ptr<file_visitor_t> next, std::vector<std::shared_ptr<filter_t>> &filters) : next_(next),
                                                                                                              filters_(filters)
    {
    }

    void visit_file(std::shared_ptr<File> file) override
    {
        for (const auto &filter : filters_)
        {
            if (!filter->matches(*file))
            {
                return;
            }
        }
        next_->visit_file(file);
    }

    bool pre_visit_folder(std::shared_ptr<Folder> folder) override
    {
        for (const auto &filter : filters_)
        {
            if (!filter->matches(*folder))
            {
                return false;
            }
        }
        return true;
    }
};

class name_filter_t : public filter_t
{
    std::string pattern_;

public:
    name_filter_t(const std::string &pattern) : pattern_(pattern) {}
    bool matches(const File &file) override
    {
        return has_case_insensitive_substring(file.name(), pattern_);
    }
};

class type_filter_t : public filter_t
{
    std::string type_;

public:
    type_filter_t(const std::string &type) : type_(type) {}
    bool matches(const File &file) override
    {
        return file.type() == type_;
    }
};

class creator_filter_t : public filter_t
{
    std::string creator_;

public:
    creator_filter_t(const std::string &creator) : creator_(creator) {}
    bool matches(const File &file) override
    {
        return file.creator() == creator_;
    }
};

class file_accumulator_t : public file_visitor_t
{
    std::vector<std::shared_ptr<File>> found_files_;

public:
    file_accumulator_t() {}

    void visit_file(std::shared_ptr<File> file) override
    {
        found_files_.push_back(file);
    }

    const std::vector<std::shared_ptr<File>> &get_found_files() const { return found_files_; }
    void clear() { found_files_.clear(); }
};

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

    bool pre_visit_folder(std::shared_ptr<Folder> folder) override
    {
        std::string indent_str(static_cast<size_t>(indent_ * 2), ' ');
        std::cout << indent_str << "Folder: " << folder->name() << std::endl;
        indent_++;
        return true;
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

//
std::vector<std::shared_ptr<data_source_t>> expand_source(const std::shared_ptr<file_data_source_t> file_source)
{
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
    return sources;
}

void process_disk_image(const std::filesystem::path &filepath, file_visitor_t &visitor, std::vector<std::shared_ptr<Folder>> *root_folders = nullptr)
{
    // Create initial data source
    auto file_source = std::make_shared<file_data_source_t>(filepath);
#ifdef VERBOSE
    std::cout << "Analyzing disk image: " << filepath << " (" << file_source->size() << " bytes)\n";
#endif

    auto sources = expand_source(file_source);
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

                // Keep root folder alive if we're using groups
                if (root_folders)
                {
                    root_folders->push_back(root);
                }

                visit_folder(root, visitor);
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

void process_single_path(const std::filesystem::path &path, file_visitor_t &visitor, std::vector<std::shared_ptr<Folder>> *root_folders = nullptr)
{
    if (std::filesystem::is_directory(path))
    {
        try
        {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(path))
            {
                process_single_path(entry.path(), visitor, root_folders);
            }
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "Error accessing directory " << path << ": " << e.what() << "\n";
        }
    }
    else if (std::filesystem::is_regular_file(path))
    {
        process_disk_image(path, visitor, root_folders);
    }
    else
    {
        std::cerr << "Error: " << path << " is not a regular file or directory\n";
        throw std::runtime_error("Invalid path type");
    }
}

void process_path(const std::vector<std::filesystem::path> &paths, file_visitor_t &visitor, std::vector<std::shared_ptr<Folder>> *root_folders = nullptr)
{
    for (const auto &path : paths)
    {
        process_single_path(path, visitor, root_folders);
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
        gGroup = get_arg(flags, "group", false);

        //  If gType is in the form of "XXXX/XXXX" split into type and creator
        size_t slash_pos = gType.find('/');
        if (slash_pos != std::string::npos)
        {
            gType = gType.substr(0, slash_pos);
            gCreator = gType.substr(slash_pos + 1);
        }

        std::vector<std::shared_ptr<filter_t>> filters;
        if (!gFind.empty())
        {
            filters.push_back(std::make_shared<name_filter_t>(gFind));
        }
        if (!gType.empty())
        {
            filters.push_back(std::make_shared<type_filter_t>(gType));
        }
        if (!gCreator.empty())
        {
            filters.push_back(std::make_shared<creator_filter_t>(gCreator));
        }

        std::shared_ptr<file_accumulator_t> accumulator;
        accumulator = std::make_shared<file_accumulator_t>();

        filter_visitor_t visitor{accumulator, filters};

        // Process all paths
        // Keep root folders alive to preserve parent relationships
        std::vector<std::shared_ptr<Folder>> root_folders;
        process_path(paths, visitor, &root_folders);

        if (!gGroup)
            for (auto &file : accumulator->get_found_files())
            {
                std::cout << string_from_file(*file) << "\n";
            }
        else
        {
            FileSet file_set;
            for (auto &file : accumulator->get_found_files())
            {
                file_set.add_file(file);
            }
            std::cout << "Found " << file_set.group_count() << " groups with a total of " << file_set.file_count() << " files.\n";
            for (const auto &[key, group] : file_set.get_groups())
            {
                std::cout << string_from_group(*group) << std::endl;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
