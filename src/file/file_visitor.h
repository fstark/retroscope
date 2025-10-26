#pragma once

#include <memory>
#include <vector>
#include <string>

// Forward declarations
class Disk;
class File;
class Folder;

class file_visitor_t
{
public:
	virtual ~file_visitor_t() = default;
	virtual void pre_visit(std::shared_ptr<Disk>) {}
	virtual void post_visit() {}
	virtual void visit_file(std::shared_ptr<File> file) = 0;
	virtual bool pre_visit_folder(std::shared_ptr<Folder>) { return true; }
	virtual void post_visit_folder(std::shared_ptr<Folder>) {}
};

void visit_folder(std::shared_ptr<Folder> folder, file_visitor_t &visitor);

// Helper function to convert path vector to string
std::string path_string(const std::vector<std::shared_ptr<Folder>> &path_vector);