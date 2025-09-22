#include "file.h"
#include "utils.h"

#include <iostream>

File::File(const std::string &name, const std::string &type,
           const std::string &creator, uint32_t data_size, uint32_t rsrc_size)
    : name_(name), sane_name_(sanitize_string(name)), type_(type), creator_(creator),
      data_size_(data_size), rsrc_size_(rsrc_size) {}

Folder::Folder(const std::string &name) : name_(name), sane_name_(sanitize_string(name)) {}

void visit_folder(const Folder &folder, file_visitor_t &visitor)
{
    visitor.pre_visit_folder(folder);
    for (const auto &file : folder.files())
    {
        visitor.visit_file(*file);
    }
    for (const auto &subfolder : folder.folders())
    {
        visit_folder(*subfolder, visitor);
    }
    visitor.post_visit_folder(folder);
}
