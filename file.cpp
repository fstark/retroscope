#include "file.h"
#include "utils.h"

#include <iostream>
#include <stdexcept>

File::File(const std::string &name, const std::string &type,
           const std::string &creator, uint32_t data_size, uint32_t rsrc_size)
    : name_(name), sane_name_(sanitize_string(name)), type_(type), creator_(creator),
      data_size_(data_size), rsrc_size_(rsrc_size), parent_(nullptr) {}

std::vector<std::string> File::path() const
{
    std::vector<std::string> parts;
    Folder *current = parent_;
    while (current)
    {
        parts.insert(parts.begin(), current->name());
        current = current->parent();
    }
    return parts;
}

Folder::Folder(const std::string &name) : name_(name), sane_name_(sanitize_string(name)), parent_(nullptr) {}

void Folder::add_file(std::shared_ptr<File> file)
{
    if (file->parent() != nullptr)
    {
        throw std::runtime_error("File '" + file->name() + "' is already in a folder");
    }
    file->parent_ = this;
    files_.push_back(file);
}

void Folder::add_folder(std::shared_ptr<Folder> folder)
{
    if (folder->parent() != nullptr)
    {
        throw std::runtime_error("Folder '" + folder->name() + "' is already in a folder");
    }
    folder->parent_ = this;
    folders_.push_back(folder);
}

void visit_folder(std::shared_ptr<Folder> folder, file_visitor_t &visitor)
{
    if (visitor.pre_visit_folder(folder))
    {
        for (const auto &file : folder->files())
        {
            visitor.visit_file(file);
        }
        for (const auto &subfolder : folder->folders())
        {
            visit_folder(subfolder, visitor);
        }
        visitor.post_visit_folder(folder);
    }
}

std::string path_string(const std::vector<std::shared_ptr<Folder>> &path_vector)
{
    if (path_vector.empty())
    {
        return "";
    }

    std::string result;
    for (size_t i = 0; i < path_vector.size(); ++i)
    {
        if (i > 0)
        {
            result += ":";
        }
        result += path_vector[i]->name();
    }
    return result;
}
