#include "file.h"
#include "utils.h"

#include <iostream>
#include <stdexcept>

#define noVERBOSE

#ifdef DEBUG_MEMORY
static int sFileCount = 0;
#endif

File::File(const std::shared_ptr<Disk> &disk, const std::string &name, const std::string &type,
           const std::string &creator, 
           std::unique_ptr<fork_t> data_fork,
           std::unique_ptr<fork_t> rsrc_fork)
    : disk_(disk), name_(name), sane_name_(sanitize_string(name)), type_(type), creator_(creator),
      data_size_(data_fork ? data_fork->size() : 0), 
      rsrc_size_(rsrc_fork ? rsrc_fork->size() : 0), 
      parent_(nullptr),
      data_fork_(std::move(data_fork)), 
      rsrc_fork_(std::move(rsrc_fork))
{
#ifdef DEBUG_MEMORY
    sFileCount++;
#endif
}

File::~File()
{
#ifdef DEBUG_MEMORY
    sFileCount--;
    if (sFileCount==0)
        std::cout << std::format("**** All File deleted\n");
#endif
}

void File::retain_folder()
{
    if (parent_)
    {
        retained_folder_ = parent_->shared_from_this();
        retained_folder_->retain_folder();
    }
}

std::vector<std::shared_ptr<Folder>> File::retained_path() const
{
    std::vector<std::shared_ptr<Folder>> result;
    if (retained_folder_)
    {
        auto f = retained_folder_;
        while (f)
        {
            result.push_back(f);
            f = f->retained_folder();
        }
    }
    return result;
}

std::vector<uint8_t> File::read_data(uint32_t offset, uint32_t size)
{
    if (!data_fork_) {
        return {};
    }
    if (size == UINT32_MAX) {
        size = data_fork_->size() - offset;
    }
    return data_fork_->read(offset, size);
}

std::vector<uint8_t> File::read_rsrc(uint32_t offset, uint32_t size)
{
    if (!rsrc_fork_) {
        return {};
    }
    if (size == UINT32_MAX) {
        size = rsrc_fork_->size() - offset;
    }
    return rsrc_fork_->read(offset, size);
}

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
