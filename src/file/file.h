#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <format>
#include "fork.h"

// Forward declarations
class Disk;
class Folder;
class fork_t;

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
	std::unique_ptr<fork_t> data_fork_;
	std::unique_ptr<fork_t> rsrc_fork_;

public:
	File(const std::shared_ptr<Disk> &disk,
		 const std::string &name, const std::string &type,
		 const std::string &creator, 
		 std::unique_ptr<fork_t> data_fork,
		 std::unique_ptr<fork_t> rsrc_fork);
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
	// concatenation of name, type, creator, datasize and rscsize
	std::string key() const { return std::format( "{}|{}|{}|{}|{}", name_, type_, creator_, data_size_, rsrc_size_); }
	
	// concatenation of name, type, creator, datasize, rscsize and content MD5 hashes
	std::string content_key() const;

private:
	// Calculate MD5 hash of resource fork, skipping filesystem metadata padding
	std::string calculate_rsrc_md5() const;

public:
	// Read methods using fork_t
	std::vector<uint8_t> read_data(uint32_t offset = 0, uint32_t size = UINT32_MAX);
	std::vector<uint8_t> read_rsrc(uint32_t offset = 0, uint32_t size = UINT32_MAX);
	
	// Convenience methods
	std::vector<uint8_t> read_data_all() const { return const_cast<File*>(this)->read_data(0, data_size_); }
	std::vector<uint8_t> read_rsrc_all() const { return const_cast<File*>(this)->read_rsrc(0, rsrc_size_); }
};
