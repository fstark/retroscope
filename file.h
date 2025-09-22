#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Forward declaration
class Folder;

class File
{
	std::string name_;
	std::string sane_name_;
	std::string type_;
	std::string creator_;
	uint32_t data_size_;
	uint32_t rsrc_size_;
	Folder *parent_;

	friend class Folder;

public:
	File(const std::string &name, const std::string &type,
		 const std::string &creator, uint32_t data_size, uint32_t rsrc_size);

	const std::string &name() const { return sane_name_; }
	const std::string &type() const { return type_; }
	const std::string &creator() const { return creator_; }
	uint32_t data_size() const { return data_size_; }
	uint32_t rsrc_size() const { return rsrc_size_; }
	Folder *parent() const { return parent_; }
	std::vector<std::string> path() const;
};

class Folder
{
	std::string name_;
	std::string sane_name_;
	std::vector<std::shared_ptr<File>> files_;
	std::vector<std::shared_ptr<Folder>> folders_;
	Folder *parent_;

	friend class Folder;

public:
	Folder(const std::string &name);
	void add_file(std::shared_ptr<File> file);
	void add_folder(std::shared_ptr<Folder> folder);

	const std::string &name() const { return sane_name_; }
	const std::vector<std::shared_ptr<File>> &files() const { return files_; }
	const std::vector<std::shared_ptr<Folder>> &folders() const { return folders_; }
	Folder *parent() const { return parent_; }
};

class file_visitor_t
{
public:
	virtual ~file_visitor_t() = default;
	virtual void pre_visit() {}
	virtual void post_visit() {}
	virtual void visit_file(std::shared_ptr<File> file) = 0;
	virtual void pre_visit_folder(std::shared_ptr<Folder>) {}
	virtual void post_visit_folder(std::shared_ptr<Folder>) {}
};

void visit_folder(std::shared_ptr<Folder> folder, file_visitor_t &visitor);

// Helper function to convert path vector to string
std::string path_string(const std::vector<std::shared_ptr<Folder>> &path_vector);
