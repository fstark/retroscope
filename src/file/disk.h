#pragma once

#include <string>

class Disk
{
	std::string name_;
	std::string path_;

public:
	Disk(const std::string &name, const std::string &path) : name_(name), path_(path) {}
	const std::string &name() const { return name_; }
	const std::string &path() const { return path_; }
};