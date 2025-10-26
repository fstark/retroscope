#include "file/folder.h"
#include "file/file.h"
#include "utils.h"

#include <iostream>
#include <stdexcept>
#include <format>
#include <algorithm>

#ifdef DEBUG_MEMORY
static int sFolderCount = 0;
#endif

Folder::Folder(const std::string &name) : name_(name), sane_name_(sanitize_string(name)), parent_(nullptr)
{
#ifdef DEBUG_MEMORY
    sFolderCount++;
#endif
}

Folder::~Folder()
{
#ifdef DEBUG_MEMORY
    sFolderCount--;
    if (sFolderCount==0)
        std::cout << std::format("**** All Folder deleted\n");
#endif
}

void Folder::add_file(std::shared_ptr<File> file)
{
    // std::cout << "Adding file '" << file->name() << "' to folder '" << name_ << "'\n";
    if (file->parent() != nullptr)
    {
        throw std::runtime_error("File '" + file->name() + "' is already in a folder");
    }
    file->set_parent(this);
    files_.push_back(file);
}

void Folder::add_folder(std::shared_ptr<Folder> folder)
{
    if (folder->parent() != nullptr)
    {
        throw std::runtime_error("Folder '" + folder->name() + "' is already in a folder");
    }
    folder->set_parent(this);
    folders_.push_back(folder);
}

std::vector<std::shared_ptr<Folder>> Folder::path()
{
    std::vector<std::shared_ptr<Folder>> result;
    Folder *current = this;
    while (current->parent_)
    {
        result.push_back(current->parent_->shared_from_this());
        current = current->parent_;
    }
    std::reverse(result.begin(), result.end());
    return result;
}