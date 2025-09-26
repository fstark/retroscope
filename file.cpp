#include "file.h"
#include "utils.h"

#include <iostream>
#include <stdexcept>

#define noVERBOSE

static int sFileCount = 0;

File::File(const std::shared_ptr<Disk> &disk, const std::string &name, const std::string &type,
           const std::string &creator, uint32_t data_size, uint32_t rsrc_size)
    : disk_(disk), name_(name), sane_name_(sanitize_string(name)), type_(type), creator_(creator),
      data_size_(data_size), rsrc_size_(rsrc_size), parent_(nullptr)
{
    sFileCount++;
#ifdef VERBOSE
    std::cout << "**** Creating File: " << name_ << " (Total files: " << sFileCount << ")\n";
#endif
}

File::~File()
{
    sFileCount--;
#ifdef VERBOSE
    std::cout << "**** Destroying File: " << name_ << " (Remaining files: " << sFileCount << ")\n";
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

static int sFolderCount = 0;

Folder::Folder(const std::string &name) : name_(name), sane_name_(sanitize_string(name)), parent_(nullptr)
{
    sFolderCount++;
#ifdef VERBOSE
    std::cout << "**** Creating Folder: " << name_ << " (Total folders: " << sFolderCount << ")\n";
#endif
}

Folder::~Folder()
{
    sFolderCount--;
#ifdef VERBOSE
    std::cout << "**** Destroying Folder: " << name_ << " (Remaining folders: " << sFolderCount << ")\n";
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
