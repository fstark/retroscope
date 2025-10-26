#include "file/file_visitor.h"
#include "file/folder.h"
#include "file/file.h"

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