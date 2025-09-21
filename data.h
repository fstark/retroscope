#pragma once

#include <vector>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include "utils.h"

// A block of continuous data
class block_t
{
	std::vector<uint8_t> data_;

public:
	block_t(const std::vector<uint8_t> &data) : data_(data) {}

	void *data() { return data_.data(); }
	void dump() const { ::dump(data_); }
};

//  The interface to a data source
class data_source_t
{
public:
    virtual block_t read_block(uint64_t offset, uint16_t size) = 0;
};

class file_data_source_t : public data_source_t
{
    std::ifstream file_;
    uint64_t file_size_;
    
public:
    file_data_source_t(const std::filesystem::path& file_path)
        : file_(file_path, std::ios::binary)
    {
        if (!file_.is_open()) {
            throw std::runtime_error("Cannot open file: " + file_path.string());
        }
        
        file_size_ = std::filesystem::file_size(file_path);
    }
    
    block_t read_block(uint64_t offset, uint16_t size) override
    {
        if (offset + size > file_size_) {
            throw std::out_of_range("Read beyond end of file");
        }
        
        std::vector<uint8_t> data(size);
        file_.seekg(offset);
        file_.read(reinterpret_cast<char*>(data.data()), size);
        
        if (!file_.good()) {
            throw std::runtime_error("Error reading from file");
        }
        
        return block_t(data);
    }
};

class offset_data_source_t : public data_source_t
{
    std::shared_ptr<data_source_t> source_;
    uint64_t offset_;
    
public:
    offset_data_source_t(std::shared_ptr<data_source_t> source, uint64_t offset)
        : source_(std::move(source)), offset_(offset) {}
    
    block_t read_block(uint64_t offset, uint16_t size) override
    {
        return source_->read_block(offset + offset_, size);
    }
};

