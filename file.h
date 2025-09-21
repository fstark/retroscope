#pragma once

#include <string>
#include <vector>
#include <cstdint>

class File
{
	std::string name_;
	std::string sane_name_;
	std::string type_;
	std::string creator_;
	uint32_t data_size_;
	uint32_t rsrc_size_;
	public:
	File( const std::string &name, const std::string &type, 
		  const std::string &creator, uint32_t data_size, uint32_t rsrc_size );
	
	const std::string &name() const { return sane_name_; }
	const std::string &type() const { return type_; }
	const std::string &creator() const { return creator_; }
	uint32_t data_size() const { return data_size_; }
	uint32_t rsrc_size() const { return rsrc_size_; }
};

class Folder
{
	std::string name_;
	std::string sane_name_;
	std::vector<File*> files_;
	std::vector<Folder*> folders_;
public:
	Folder( const std::string &name );
	void add_file( File *file ) { files_.push_back(file); }
	void add_folder( Folder *folder ) { folders_.push_back(folder); }

	const std::string &name() const { return sane_name_; }
	const std::vector<File*> &files() const { return files_; }
	const std::vector<Folder*> &folders() const { return folders_; }
};

// Function declarations that depend on the classes above
void print_folder_hierarchy(const Folder& folder, int indent = 0);