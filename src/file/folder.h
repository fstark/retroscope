#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward declarations
class File;

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