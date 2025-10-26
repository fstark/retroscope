#pragma once

#include "file/file.h"
#include <memory>
#include <vector>
#include <map>
#include <string>

/**
 * A file set groups similar files together. Two files are considered similar
 * if they have the same name and the same type/creator combination.
 *
 * Files can be added to the set but not removed. Each group maintains a list
 * of all files that match the grouping criteria.
 */
class FileSet
{
public:
    /**
     * Represents a group of similar files.
     */
    struct FileGroup
    {
        std::string name;
        std::string type;
        std::string creator;
        std::vector<std::shared_ptr<File>> files;

        FileGroup(const std::string &name, const std::string &type, const std::string &creator)
            : name(name), type(type), creator(creator) {}
    };

private:
    // Key: "name|type|creator", Value: FileGroup
    std::map<std::string, std::unique_ptr<FileGroup>> groups_;

    /**
     * Generate a unique key for a file based on name, type, and creator.
     */
    std::string make_key(const std::string &name, const std::string &type, const std::string &creator) const;

public:
    /**
     * Default constructor.
     */
    FileSet() = default;

    /**
     * Add a file to the set. If a group with the same name/type/creator already exists,
     * the file is added to that group. Otherwise, a new group is created.
     */
    void add_file(std::shared_ptr<File> file);

    /**
     * Get all groups in the file set.
     */
    const std::map<std::string, std::unique_ptr<FileGroup>> &get_groups() const;

    /**
     * Get a specific group by name, type, and creator.
     * Returns nullptr if the group doesn't exist.
     */
    const FileGroup *get_group(const std::string &name, const std::string &type, const std::string &creator) const;

    /**
     * Get the total number of groups.
     */
    size_t group_count() const;

    /**
     * Get the total number of files across all groups.
     */
    size_t file_count() const;

    /**
     * Check if the set is empty.
     */
    bool empty() const;

    /**
     * Clear all groups and files from the set.
     */
    void clear();
};
