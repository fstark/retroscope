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
    virtual uint64_t size() const = 0;
};

class file_data_source_t : public data_source_t
{
    std::ifstream file_;
    uint64_t size_;
    
public:
    file_data_source_t(const std::filesystem::path& file_path)
        : file_(file_path, std::ios::binary)
    {
        if (!file_.is_open()) {
            throw std::runtime_error("Cannot open file: " + file_path.string());
        }
        
        size_ = std::filesystem::file_size(file_path);
    }
    
    uint64_t size() const override
    {
        return size_;
    }
    
    block_t read_block(uint64_t offset, uint16_t size) override
    {
        if (offset + size > size_) {
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

class range_data_source_t : public data_source_t
{
    std::shared_ptr<data_source_t> source_;
    uint64_t offset_;
    uint64_t size_;
    
public:
    range_data_source_t(std::shared_ptr<data_source_t> source, uint64_t offset, uint64_t size)
        : source_(std::move(source)), offset_(offset), size_(size) 
    {
        if (offset + size > source_->size()) {
            throw std::out_of_range("Range exceeds source size");
        }
    }
    
    uint64_t size() const override
    {
        return size_;
    }
    
    block_t read_block(uint64_t offset, uint16_t size) override
    {
        if (offset + size > size_) {
            throw std::out_of_range("Read beyond end of range");
        }
        return source_->read_block(offset + offset_, size);
    }
};

