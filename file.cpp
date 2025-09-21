#include "file.h"
#include "utils.h"

#include <iostream>

File::File( const std::string &name, const std::string &type, 
		    const std::string &creator, uint32_t data_size, uint32_t rsrc_size )
	: name_(name), sane_name_(sanitize_string(name)), type_(type), creator_(creator),
	  data_size_(data_size), rsrc_size_(rsrc_size) {}

Folder::Folder( const std::string &name ) : name_(name), sane_name_(sanitize_string(name)) {}

void print_folder_hierarchy(const Folder& folder, int indent) {
    // Print current folder
    std::string indent_str(static_cast<size_t>(indent * 2), ' ');
    std::cout << indent_str << "Folder: " << folder.name() << std::endl;
    
    // Print files in this folder
    for (const File* file : folder.files()) {
        std::cout << indent_str << "  File: " << file->name() 
                  << " (" << file->type() << "/" << file->creator() << ")"
                  << " DATA: " << file->data_size() << " bytes"
                  << " RSRC: " << file->rsrc_size() << " bytes" << std::endl;
    }
    
    // Recursively print subfolders
    for (const Folder* subfolder : folder.folders()) {
        print_folder_hierarchy(*subfolder, indent + 1);
    }
}