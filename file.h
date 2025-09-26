#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

// Forward declaration
class Disk;

class Folder;

class Folder;

class File
{
	std::shared_ptr<Disk> disk_;
	std::string name_;
	std::string sane_name_;
	std::string type_;
	std::string creator_;
	uint32_t data_size_;
	uint32_t rsrc_size_;
	Folder *parent_;
	std::shared_ptr<Folder> retained_folder_;

public:
	File(const std::shared_ptr<Disk> &disk,
		 const std::string &name, const std::string &type,
		 const std::string &creator, uint32_t data_size, uint32_t rsrc_size);
	~File();
	const std::shared_ptr<Disk> &disk() const { return disk_; }
	const std::string &name() const { return sane_name_; }
	const std::string &type() const { return type_; }
	const std::string &creator() const { return creator_; }
	uint32_t data_size() const { return data_size_; }
	uint32_t rsrc_size() const { return rsrc_size_; }
	Folder *parent() const { return parent_; }
	void set_parent(Folder *parent) { parent_ = parent; }
	std::vector<std::shared_ptr<Folder>> retained_path() const;
	void retain_folder();
};

class Folder : public std::enable_shared_from_this<Folder>
{
	std::string name_;
	std::string sane_name_;
	std::vector<std::shared_ptr<File>> files_;
	std::vector<std::shared_ptr<Folder>> folders_;
	Folder *parent_;
	std::shared_ptr<Folder> retained_folder_;

public:
	Folder(const std::string &name);
	~Folder();
	void add_file(std::shared_ptr<File> file);
	void add_folder(std::shared_ptr<Folder> folder);

	const std::string &name() const { return sane_name_; }
	const std::vector<std::shared_ptr<File>> &files() const { return files_; }
	const std::vector<std::shared_ptr<Folder>> &folders() const { return folders_; }
	Folder *parent() const { return parent_; }
	void set_parent(Folder *parent) { parent_ = parent; }
	std::shared_ptr<Folder> retained_folder() const { return retained_folder_; }
	void retain_folder()
	{
		if (parent_)
		{
			retained_folder_ = parent_->shared_from_this();
			retained_folder_->retain_folder();
		}
	}
	void release()
	{
		files_.clear();
		for (auto &folder : folders_)
		{
			folder->release();
		}
		folders_.clear();
	}

	std::vector<std::shared_ptr<Folder>> path();
};

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

class Disk
{
	std::string name_;
	std::string path_;

public:
	Disk(const std::string &name, const std::string &path) : name_(name), path_(path) {}
	const std::string &name() const { return name_; }
	const std::string &path() const { return path_; }
};

void visit_folder(std::shared_ptr<Folder> folder, file_visitor_t &visitor);

// Helper function to convert path vector to string
std::string path_string(const std::vector<std::shared_ptr<Folder>> &path_vector);
