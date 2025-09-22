#include "file_set.h"
#include <stdexcept>
#include <iostream>

std::string FileSet::make_key(const std::string &name, const std::string &type, const std::string &creator) const
{
    // Use a delimiter that's unlikely to appear in file names, types, or creators
    return name + "|" + type + "|" + creator;
}

void FileSet::add_file(std::shared_ptr<File> file)
{
    if (!file)
    {
        throw std::invalid_argument("Cannot add null file to FileSet");
    }

    // Debug: Log the file path when adding to FileSet
    

    std::string key = make_key(file->name(), file->type(), file->creator());

    auto it = groups_.find(key);
    if (it != groups_.end())
    {
        // Group already exists, add file to it
        it->second->files.push_back(file);
    }
    else
    {
        // Create new group
        auto group = std::make_unique<FileGroup>(file->name(), file->type(), file->creator());
        group->files.push_back(file);
        groups_[key] = std::move(group);
    }
}

const std::map<std::string, std::unique_ptr<FileSet::FileGroup>> &FileSet::get_groups() const
{
    return groups_;
}

const FileSet::FileGroup *FileSet::get_group(const std::string &name, const std::string &type, const std::string &creator) const
{
    std::string key = make_key(name, type, creator);
    auto it = groups_.find(key);
    return (it != groups_.end()) ? it->second.get() : nullptr;
}

size_t FileSet::group_count() const
{
    return groups_.size();
}

size_t FileSet::file_count() const
{
    size_t total = 0;
    for (const auto &[key, group] : groups_)
    {
        total += group->files.size();
    }
    return total;
}

bool FileSet::empty() const
{
    return groups_.empty();
}

void FileSet::clear()
{
    groups_.clear();
}
