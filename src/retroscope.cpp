#include "retroscope.h"
#include "data/data.h"
#include "partition.h"
#include "file/file_set.h"
#include "file/disk.h"
#include "file/folder.h"
#include "file/file_visitor.h"
#include "data/apm_datasource.h"
#include "data/dc42_datasource.h"
#include "data/bin_datasource.h"
#include "rsrc/rsrc_parser.hpp"

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
#include <unordered_set>
#include <cassert>

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

// Canonical way to describe a file
std::string string_from_file(const File &file)
{
    return std::format("{} {}",
                       short_sring_from_file(file),
                       string_from_fork_sizes(file.data_size(), file.rsrc_size()));
}

std::string string_from_path(const std::vector<std::shared_ptr<Folder>> &path_vector)
{
    if (path_vector.empty())
    {
        return "";
    }

    //  Copy and reverse the path to get root:first
    std::vector<std::shared_ptr<Folder>> reversed_path(path_vector.rbegin(), path_vector.rend());

    std::string result;
    for (size_t i = 0; i < reversed_path.size(); ++i)
    {
        if (i > 0)
        {
            result += ":";
        }
        result += reversed_path[i]->name();
    }
    return result;
}

std::string string_from_disk(const std::shared_ptr<Disk> &disk)
{
    if (!disk)
    {
        return "(no disk)";
    }
    return std::format("{} in {}", disk->name(), disk->path());
}

// Canonical way to describe a group of files
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
    filter_visitor_t(std::vector<std::shared_ptr<filter_t>> &filters, std::shared_ptr<file_visitor_t> next) : next_(next),
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
        // std::cout << std::format(" - .Keeping: {:p} {:p} {} {}\n",
        //                          (void *)this,
        //                          (void *)file.get(),
        //                          string_from_disk(file->disk()),
        //                          string_from_file(*file));

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

//  Accumulates all files in a list
//  Holds a set of files to exclude
class file_accumulator_t : public file_visitor_t
{
    std::vector<std::shared_ptr<File>> found_files_;
    std::unordered_set<std::string> exclude_keys_;

public:
    file_accumulator_t() {}
    file_accumulator_t(const std::vector<std::shared_ptr<File>> &exclusion)
    {
        for (const auto &file : exclusion)
        {
            exclude_keys_.insert(file->key());
        }
    }

    void visit_file(std::shared_ptr<File> file) override
    {
        if (!is_excluded(file))
        {
            found_files_.push_back(file);
            file->retain_folder();
        }
    }

    const std::vector<std::shared_ptr<File>> &get_found_files() const { return found_files_; }
    void clear() { found_files_.clear(); }

    void switch_exclusion()
    {
        exclude_keys_.clear();
        for (const auto &file : found_files_)
        {
            exclude_keys_.insert(file->key());
        }
        clear();
    }
    bool is_excluded(const std::shared_ptr<File> &file) const
    {
        // rs_log("Checking exclusion for file key: {}", file->key());
        return exclude_keys_.find(file->key()) != exclude_keys_.end();
    }
};

class file_printer_t : public file_visitor_t
{
    void dump_data( const std::vector<uint8_t> &data ) {
        for (size_t offset = 0; offset < data.size(); offset += 16) {
            std::cout << std::format("    {:08x}: ", offset);
            
            // Hex dump
            for (size_t i = 0; i < 16; ++i) {
                if (offset + i < data.size()) {
                    std::cout << std::format("{:02x} ", data[offset + i]);
                } else {
                    std::cout << "   ";
                }
            }
            
            std::cout << " |";
            
            // ASCII dump
            for (size_t i = 0; i < 16 && offset + i < data.size(); ++i) {
                char c = static_cast<char>(data[offset + i]);
                std::cout << (std::isprint(c) ? c : '.');
            }
            
            std::cout << "|" << std::endl;
        }
    }

public:
    void visit_file(std::shared_ptr<File> file) override
    {
        std::cout << string_from_file(*file) << std::endl;

        // Show first 16 bytes of data fork in hex + ASCII
        // auto data = file->read_data_all();
        // if (data.size()>0)
        // {
        //     std::cout << "DATA:" << std::endl;
        //     dump_data(data);
        // }
        // auto rsrc = file->read_rsrc_all();
        // if (rsrc.size()>0)
        // {
        //     std::cout << "RSRC:" << std::endl;
        //     dump_data(rsrc);
        // }

        // Dump resource fork if present
        if (file->rsrc_size() > 0) {
            auto rsrc_data = file->read_rsrc(0, file->rsrc_size());
            if (!rsrc_data.empty()) {
                try {
                    // Create a read function lambda for the resource parser
                    auto read_func = [&rsrc_data](uint32_t offset, uint32_t size) -> std::vector<uint8_t> {
                        if (offset >= rsrc_data.size()) {
                            return {};
                        }
                        uint32_t available = rsrc_data.size() - offset;
                        uint32_t to_read = std::min(size, available);
                        return std::vector<uint8_t>(rsrc_data.begin() + offset, 
                                                   rsrc_data.begin() + offset + to_read);
                    };

                    // Parse and dump the resource fork
                    rsrc_parser_t parser(rsrc_data.size(), read_func);
                    if (parser.is_valid()) {
                        parser.dump();
                        
                        // Test the new get_resources API
                        try {
                            auto resources = parser.get_resources();
                            
                            // Show a few examples of the new API
                            int shown = 0;
                            for (const auto& resource : resources) {
                                std::cout << std::format("    {} {} ({} bytes)", 
                                                        resource.type(), resource.id(), resource.size());
                                if (resource.has_name()) {
                                    std::cout << std::format(" \"{}\"", resource.name());
                                }
                                std::cout << std::endl;
                                shown++;
                            }
                        } catch (const std::exception& e) {
                            std::cout << std::format("    Error reading resources: {}", e.what()) << std::endl;
                        }
                    } else {
                        std::cout << "    Invalid resource fork" << std::endl;
                        // Show first 32 bytes of resource fork in hex for debugging
                    }
                } catch (const std::exception& e) {
                    std::cout << "    Error parsing resource fork: " << e.what() << std::endl;
                }
            }
        }
    }
};

// class dump_visitor_t : public file_visitor_t
// {
//     size_t indent_ = 0;

// public:
//     void visit_file(std::shared_ptr<File> file) override
//     {
//         std::string indent_str(static_cast<size_t>(indent_ * 2), ' ');
//         std::cout << indent_str << "File: " << file->name()
//                   << " (" << file->type() << "/" << file->creator() << ")"
//                   << " DATA: " << file->data_size() << " bytes"
//                   << " RSRC: " << file->rsrc_size() << " bytes" << std::endl;
//     }

//     bool pre_visit_folder(std::shared_ptr<Folder> folder) override
//     {
//         std::string indent_str(static_cast<size_t>(indent_ * 2), ' ');
//         std::cout << indent_str << "Folder: " << folder->name() << std::endl;
//         indent_++;
//         return true;
//     }

//     void post_visit_folder(std::shared_ptr<Folder>) override
//     {
//         indent_--;
//     }
// };

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

bool replace_source(const std::shared_ptr<datasource_t> source, std::vector<std::shared_ptr<datasource_t>> &result)
{
    // Try BIN unwrapping (CD-ROM format)
    auto bin_source = make_bin_datasource(source);
    if (bin_source != source)
    {
        result.push_back(bin_source);
        return true;
    }

    // Try DC42 unwrapping
    auto dc42_source = make_dc42_datasource(source);
    if (dc42_source != source)
    {
        result.push_back(dc42_source);
        return true;
    }

    // Try APM expansion
    auto apm_partitions = make_apm_datasource(source);
    if (!apm_partitions.empty())
    {
        rs_log("Found Apple Partition Map with {} partitions", apm_partitions.size());
        for (auto &partition : apm_partitions)
        {
            result.push_back(partition);
        }
        return true;
    }

    result.push_back(source);

    return false;
}

std::vector<std::shared_ptr<datasource_t>> expand_source(const std::shared_ptr<datasource_t> file_source)
{
    ENTRY("{}", file_source->description());

    std::vector<std::shared_ptr<datasource_t>> sources = {file_source};

    bool changed;

    // Process all sources, expanding them if they contain sub-partitions
    do
    {
        changed = false;
        std::vector<std::shared_ptr<datasource_t>> results;
        for (auto &source : sources)
        {
            changed |= replace_source(source, results);
        }
        if (changed)
        {
            sources = results;
        }
    } while (changed);

    rs_log("Expanded to {} data sources", sources.size());

    return sources;
}

void process_disk_image(const std::filesystem::path &filepath, file_visitor_t &visitor, std::vector<std::shared_ptr<Folder>> * = nullptr)
{
    ENTRY("{}", filepath.string());

    // Create initial data source
    auto file_source = std::make_shared<file_datasource_t>(filepath);
    rs_log("Analyzing disk image: {} ({})", filepath.c_str(), file_source->size());

    auto sources = expand_source(file_source);

    for (auto &source : sources)
    {
        auto partition = partition_t::create(source);
        if (partition)
        {
            try
            {
                auto root = partition->get_root_folder();
                visit_folder(root, visitor);
            }
            catch (const std::exception &error)
            {
                std::cerr << "\033[31mError parsing partition\033[0m : " << filepath << " (" << file_source->size() << " bytes) ";
                std::cerr << ": " << error.what() << "\n";
            }
        }
        else
        {
            rs_log("Unknown partition type for {}", source->description());
        }
    }
}

void process_single_path(const std::filesystem::path &path, file_visitor_t &visitor)
{
    ENTRY("{}", path.c_str());
    if (std::filesystem::is_directory(path))
    {
        try
        {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(path))
            {
                if (std::filesystem::is_regular_file(entry.path()))
                    process_disk_image(entry.path(), visitor);
            }
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "Error accessing directory " << path << ": " << e.what() << "\n";
        }
    }
    else if (std::filesystem::is_regular_file(path))
    {
        process_disk_image(path, visitor);
    }
    else
    {
        std::cerr << "Error: " << path << " is not a regular file or directory\n";
        throw std::runtime_error("Invalid path type");
    }
}

void process_paths(const std::vector<std::filesystem::path> &paths, file_visitor_t &visitor)
{
    ENTRY("{}", paths.size());
    for (const auto &path : paths)
    {
        process_single_path(path, visitor);
    }
}

/*
    All arguments in the for --xxx=yyy or --xxx form are returned in the map 1st argument
    For the rest:
        1st argument is always 'list' or 'diff'
        It is returned as the tuple first argument
        The rest of the arguments are a list of filesystem paths
*/
std::tuple<std::string, std::map<std::string, std::string>, std::vector<std::filesystem::path>> parse_arguments(int argc, char *argv[])
{
    std::map<std::string, std::string> flags;
    std::vector<std::filesystem::path> paths;
    std::string command;

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
            // Not a flag - first non-flag argument is the command, rest are paths
            if (command.empty())
            {
                command = arg;
            }
            else
            {
                paths.emplace_back(arg);
            }
        }
    }

    return std::make_tuple(command, flags, paths);
}

using namespace std::string_literals;

/*
    retroscope list <file_or_directories> [--type=XXXX] [--creator=XXXX] [--name=substring] [--group]
*/

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " {list|diff} <disk_image_file_or_directory> [additional_paths...]\n";
        std::cerr << "Analyzes vintage Macintosh HFS disk images and list content.\n";
        std::cerr << "Commands:\n";
        std::cerr << "  list - List files in the disk images\n";
        std::cerr << "  diff - Show files that differ between disk images\n";
        std::cerr << "If a directory is provided, recursively processes all files in it.\n";
        std::cerr << "Multiple paths can be specified to process them all.\n";
        return 1;
    }

    try
    {
        // Parse command line arguments
        auto [command, flags, paths] = parse_arguments(argc, argv);

        if (command != "list" && command != "diff")
        {
            std::cerr << "Error: First argument must be 'list' or 'diff'\n";
            return 1;
        }

        // Check if we have any paths to process
        if (paths.empty())
        {
            std::cerr << "Error: No paths specified for processing\n";
            return 1;
        }

        gType = get_arg(flags, "type", ""s);
        gCreator = get_arg(flags, "creator", ""s);
        gName = get_arg(flags, "name", ""s);
        gGroup = get_arg(flags, "group", false);

        //  If gType is in the form of "XXXX/XXXX" split into type and creator
        size_t slash_pos = gType.find('/');
        if (slash_pos != std::string::npos)
        {
            gCreator = gType.substr(slash_pos + 1);
            gType = gType.substr(0, slash_pos);
        }

        std::vector<std::shared_ptr<filter_t>> filters;
        if (!gName.empty())
        {
            filters.push_back(std::make_shared<name_filter_t>(gName));
        }
        if (!gType.empty())
        {
            filters.push_back(std::make_shared<type_filter_t>(gType));
        }
        if (!gCreator.empty())
        {
            filters.push_back(std::make_shared<creator_filter_t>(gCreator));
        }

        if (command == "list" && !gGroup)
        {
            auto printer = std::make_shared<file_printer_t>();
            filter_visitor_t visitor{filters, printer};
            process_paths(paths, visitor);
            return 0;
        }

        //  We will need to keep the files somewhere
        auto accumulator = std::make_shared<file_accumulator_t>();
        filter_visitor_t visitor{filters, accumulator};

        if (command == "list")
        {
            process_paths(paths, visitor);
        }
        else
        {
            //  We pick out the last path
            if (paths.size() < 1)
            {
                std::cerr << "Error: 'diff' command requires at least two paths\n";
                return 1;
            }
            auto last_path = paths.back();
            paths.pop_back();
            process_paths(paths, visitor);
            accumulator->switch_exclusion();
            process_paths({last_path}, visitor);
        }

        if (!gGroup)
        {
            // we print the files [should be done with a printer visitor]
            for (auto &file : accumulator->get_found_files())
            {
                std::cout << string_from_file(*file) << std::endl;
            }

            return 0;
        }

        //  We process the grouping command
        FileSet file_set;
        for (auto &file : accumulator->get_found_files())
        {
            file_set.add_file(file);
        }
        std::cout << "Found " << file_set.group_count() << " groups with a total of " << file_set.file_count() << " files.\n";
        for (const auto &[key, group] : file_set.get_groups())
        {
            std::cout << string_from_group(*group) << std::endl;
            for (auto &file : group->files)
            {
                std::cout << "    Disk: " << string_from_disk(file->disk()) << std::endl;
                std::cout << "          Path: " << string_from_path(file->retained_path()) << std::endl;
            }
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "Filesystem Error: " << e.what() << "\n";
        std::cerr << "Path: " << e.path1() << "\n";
        return 1;
    }
    catch (const std::out_of_range &e)
    {
        std::cerr << "Range Error: " << e.what() << "\n";
        return 1;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Runtime Error: " << e.what() << "\n";
        return 1;
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << "Invalid Argument: " << e.what() << "\n";
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
