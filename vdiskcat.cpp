#include "vdiskcat.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <format>
#include <iostream>
#include "vdiskcat.h"
#include <cstdint>
#include <string>
#include <fstream>
#include <format>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cstddef>
#include <endian.h>

// Convert big-endian to host byte order
// Note: Using system-provided be16toh and be32toh functions
// Custom implementations commented out for portability
/*
uint16_t be16toh(uint16_t val)
{
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

uint32_t be32toh(uint32_t val)
{
    return ((val & 0xFF) << 24) | (((val >> 8) & 0xFF) << 16) |
           (((val >> 16) & 0xFF) << 8) | ((val >> 24) & 0xFF);
}
*/

// Convert Pascal string to C++ string
std::string string_from_pstring(const uint8_t *pascalStr)
{
    if (pascalStr[0] == 0)
        return std::string();
    return std::string(reinterpret_cast<const char *>(pascalStr + 1), static_cast<size_t>(pascalStr[0]));
}

// Format type/creator codes as 4-character strings
std::string string_from_code(uint32_t code)
{
    if (code == 0) return "    ";
    
    char chars[5];
    chars[0] = (code >> 24) & 0xFF;
    chars[1] = (code >> 16) & 0xFF;
    chars[2] = (code >> 8) & 0xFF;
    chars[3] = code & 0xFF;
    chars[4] = '\0';
    
    // Replace non-printable characters with '.'
    for (int i = 0; i < 4; i++) {
        if (chars[i] < 32 || chars[i] > 126) {
            chars[i] = '.';
        }
    }
    
    return std::string(chars);
}

void dump(const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; i += 16)
    {
        std::cout << std::hex << std::setw(8) << std::setfill('0') << i << ": ";
        for (size_t j = 0; j < 16 && i + j < size; j++)
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<int>(data[i + j]) << " ";
        }
        std::cout << " ";
        for (size_t j = 0; j < 16 && i + j < size; j++)
        {
            char c = static_cast<char>(data[i + j]);
            if (std::isprint(static_cast<unsigned char>(c)))
                std::cout << c;
            else
                std::cout << ".";
        }
        std::cout << std::endl;
    }
}

void dump(const std::vector<uint8_t> &data)
{
    dump(data.data(), data.size());
}

// block_t implementations
btree_header_node_t block_t::as_btree_header_node()
{
	return btree_header_node_t(*this);
}

master_directory_block_t block_t::as_master_directory_block() 
{ 
	return master_directory_block_t(*this); 
}

bool master_directory_block_t::isHFSVolume() const
{
	return be16toh(content->drSigWord) == 0x4244;
}

std::string master_directory_block_t::getVolumeName() const
{
	return string_from_pstring(content->drVN);
}

uint32_t master_directory_block_t::allocationBlockSize() const
{	
	return be32toh(content->drAlBlkSiz);
}

uint16_t master_directory_block_t::allocationBlockStart() const
{
	return be16toh(content->drAlBlSt);
}

uint16_t master_directory_block_t::extendsExtendStart(int index) const
{
	if (index < 0 || index >= 3)
		throw std::out_of_range("Extent index out of range");
	return be16toh(content->drXTExtRec[index].startBlock);
}

uint16_t master_directory_block_t::extendsExtendCount(int index) const
{
	if (index < 0 || index >= 3)
		throw std::out_of_range("Extent index out of range");
	return be16toh(content->drXTExtRec[index].blockCount);
}

uint16_t master_directory_block_t::catalogExtendStart(int index) const
{
	if (index < 0 || index >= 3)
		throw std::out_of_range("Extent index out of range");
	return be16toh(content->drCTExtRec[index].startBlock);
}

uint16_t master_directory_block_t::catalogExtendCount(int index) const
{
	if (index < 0 || index >= 3)
		throw std::out_of_range("Extent index out of range");
	return be16toh(content->drCTExtRec[index].blockCount);
}

// file_t implementations
void file_t::add_extent( const extent_t &extent )
{
	extents_.push_back(extent);
}

void file_t::add_extent( uint16_t start_block, const extent_t &extent )
{
    //  check that we already have start_block blocks in the extent
    uint16_t total_blocks = 0;
    for (const auto &ext : extents_) {
        total_blocks += ext.count;
    }
    if (total_blocks != start_block) {
        throw std::runtime_error("Extent continuity error: expected " + 
                                std::to_string(start_block) + " blocks, but have " + 
                                std::to_string(total_blocks));
    }

    extents_.push_back(extent);
}

uint16_t file_t::to_absolute_block(uint16_t block) const
{
	for (auto &ext : extents_)
	{
		if (block < ext.count)
			return ext.start + block;
		block -= ext.count;
	}
	return 0xffff;
}

uint32_t file_t::allocation_offset(uint32_t offset) const
{
    auto allocation_block_size = partition_.allocation_block_size();
    for (auto &ext : extents_)
    {
        if (offset < ext.count*allocation_block_size)
            return ext.start*allocation_block_size + offset;
        offset -= ext.count*allocation_block_size;
    }
    return 0xffffffff;
}



btree_file_t file_t::as_btree_file() 
{ 
	return btree_file_t(*this); 
}

// btree_file_t implementations
btree_file_t::btree_file_t(file_t &file) : file_(file)
{
	auto start = file_.to_absolute_block(0);
	// std::cout << "Btree file starts at absolute block: " << start+file_.partition().

#ifdef VERBOSE
    std::cout << std::format("Partition allocation start: {}\n", file_.partition().allocation_start() );
    std::cout << std::format("File start: {}\n", start );
#endif

    //  We read the node as a 512 byte block, but it may be larger
    //  We look into the btree header to find the actual node size
    auto block1 = file_.partition().read_allocated_block(start,512);
    // block.dump();
    auto header1 = block1.as_btree_header_node();

#ifdef VERBOSE
    std::cout << std::format( "Node size: {}\n", header1.node_size() );
    std::cout << std::format( "Node count: {}\n", header1.node_count() );
#endif

    //  Re-read with the proper node size
    auto block2 = file_.partition().read_allocated_block(start,header1.node_size());
    auto header2 = block2.as_btree_header_node();

    first_leaf_node_= header2.first_leaf_node();
    node_size_= header2.node_size();

#ifdef VERBOSE
    std::cout << std::format( "First leaf node: {}\n", header2.first_leaf_node() );
#endif
}

// partition_t implementations
partition_t::partition_t(std::ifstream &file, uint64_t partitionStart, uint64_t partitionSize)
    : file_(file), partitionStart_(partitionStart), partitionSize_(partitionSize), extents_(*this), catalog_(*this)
{
    std::cout << "Partition start: " << partitionStart_ << ", size: " << partitionSize_ << std::endl;
}

block_t partition_t::read(uint64_t blockOffset) const
{
	std::vector<uint8_t> block(512);
	file_.seekg(partitionStart_ + blockOffset * 512);
    file_.read(reinterpret_cast<char *>(block.data()), 512);
    if (!file_.good())
    {
        throw std::runtime_error("Error reading block");
    }
	return block_t(block);
}

std::vector<uint8_t> partition_t::readBlock512(uint64_t blockOffset)
{
    std::vector<uint8_t> block(512);
    file_.seekg(partitionStart_ + blockOffset * 512);
    file_.read(reinterpret_cast<char *>(block.data()), 512);
    if (!file_.good())
    {
        throw std::runtime_error("Error reading block");
        return {};
    }
    return block;
}

std::vector<uint8_t> partition_t::readBlock(uint64_t blockOffset)
{
    std::vector<uint8_t> block(allocationBlockSize_);
    auto offset = partitionStart_ + allocationStart_ * 512 /* ? */ + blockOffset * allocationBlockSize_;
    std::cout << "Reading block at offset: " << offset << std::endl;

    file_.seekg(offset);
    file_.read(reinterpret_cast<char *>(block.data()), allocationBlockSize_);
    if (!file_.good())
    {
        throw std::runtime_error("Error reading block");
        return {};
    }
    return block;
}

block_t partition_t::read_allocated_block( uint16_t blockIndex , uint16_t size )
{
#ifdef VERBOSE
    std::cout << std::format("read_allocated_block({},{})\n", blockIndex, size);
#endif

    if (size == 0xffff)
        size = allocation_block_size();

    std::vector<uint8_t> block(size);
    auto offset = partitionStart_ + allocationStart_ * 512 /* ? */ + blockIndex * allocation_block_size();
#ifdef VERBOSE
    std::cout << std::format("  Allocation start: {} block index: {} block size: {} => offset: {}\n",
        allocationStart_, blockIndex, allocation_block_size(), offset );
#endif

    file_.seekg(offset);
    file_.read(reinterpret_cast<char *>(block.data()), allocationBlockSize_);
    if (!file_.good())
    {
        throw std::runtime_error("Error reading block");
    }

    return block;
}

void print_folder_hierarchy(const Folder& folder, int indent = 0) {
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

void partition_t::readMasterDirectoryBlock()
{
	auto block = read(2);
	auto mdb = block.as_master_directory_block();

    std::string volumeName;
    if (mdb.isHFSVolume())
    {
        // Print MDB information as requested
        std::cout << "=== HFS Master Directory Block ===" << std::endl;

		std::cout << "Volume Name = " << mdb.getVolumeName() << std::endl;

        // SIGNATURE
        // uint16_t signature = be16toh(mdb.drSigWord);
        // std::cout << "SIGNATURE: 0x" << std::hex << signature << std::dec << " (\"BD\")" << std::endl;

        // // Number of files in root dir
        // uint16_t filesInRoot = be16toh(mdb.drNmFls);
        // std::cout << "# of files in root dir: " << filesInRoot << std::endl;

        // // Number of directories in root dir
        // uint16_t dirsInRoot = be16toh(mdb.drNmRtDirs);
        // std::cout << "# of directories in root dir: " << dirsInRoot << std::endl;

        // // Number of files in volume
        // uint32_t filesInVolume = be32toh_custom(mdb.drFilCnt);
        // std::cout << "# of files in volume: " << filesInVolume << std::endl;

        // // Number of directories in volume
        // uint32_t dirsInVolume = be32toh_custom(mdb.drDirCnt);
        // std::cout << "# of directories: " << dirsInVolume << std::endl;

        // Logical block size
        allocationBlockSize_ = mdb.allocationBlockSize();
		std::cout << "Allocation block size: " << allocationBlockSize_ << " bytes" << std::endl;

        allocationStart_ = mdb.allocationBlockStart();
        std::cout << "Allocation starts at: " << allocationStart_ << std::endl;

        // extents_.add_extent(drAlBlSt, be16toh_custom(mdb.drNmAlBlks));

        // // Number of allocation blocks
        // uint16_t numAllocBlocks = be16toh_custom(mdb.drNmAlBlks);
        // std::cout << "Number of allocation blocks: " << numAllocBlocks << std::endl;

        // // Extents overflow file size (prob useless)
        // uint32_t extOverflowSize = be32toh_custom(mdb.drXTFlSize);
        // std::cout << "Extents overflow file size (drXTFlSize): " << extOverflowSize << " bytes" << std::endl;

        // //	extents size
        // uint32_t extentsSize = be32toh_custom(mdb.drXTClpSiz);
        // std::cout << "Extents overflow clump size (drXTClpSiz): " << extentsSize << " bytes" << std::endl;

        // Extents overflow extents (3 extent descriptors)
        std::cout << "drXTExtRec (extents extents):\n";
        for (int i = 0; i < 3; i++)
        {
            auto startBlock = mdb.extendsExtendStart(i);
            auto blockCount = mdb.extendsExtendCount(i);
            if (blockCount)
            {
                std::cout << "  [" << i << "]=" << startBlock << "-" << blockCount << "\n";
                extents_.add_extent( {startBlock, blockCount } );
            }
        }

        // Catalog file extents (first node of catalog btree)
        // (we have to do the catalog before scanning the extent leaves
        //  because the first 3 extents are not in the btree)
        std::cout << "drCTExtRec (catalog extents):\n";
        for (int i = 0; i < 3; i++)
        {
            auto startBlock = mdb.catalogExtendStart(i);
            auto blockCount = mdb.catalogExtendCount(i);
            if (blockCount)
            {
                std::cout << "  [" << i << "]=" << startBlock << "-" << blockCount << "\n";
                catalog_.add_extent({startBlock, blockCount});
            }
        }
        std::cout << std::endl;


		btree_file_t extents_btree = extents_.as_btree_file();

        extents_btree.iterate_extents( [&]( const extents_record_t &extent_record )
        {
            // Extract key from the extents record
            uint8_t keyLength = extent_record.key_length();
            uint8_t forkType = extent_record.fork_type();
            uint32_t file_ID = extent_record.file_ID();
            uint16_t startBlock = extent_record.start_block();

            std::cout << "Key length: " << static_cast<int>(keyLength) << std::endl;
            std::cout << "Fork type: " << static_cast<int>(forkType) << " (" << (forkType == 0 ? "data fork" : "resource fork") << ")" << std::endl;
            std::cout << "File ID: " << file_ID << std::endl;
            std::cout << "Start block: " << startBlock << std::endl;
    
            // prints the content of the extends
            std::cout << "Extents Record:" << std::endl;
            for (int i = 0; i < 3; i++) {
                auto extent = extent_record.get_extent( i );
                if (extent.count > 0) {
                    std::cout << "  Extent " << i << ": Start=" << extent.start 
                              << ", Count=" << extent.count << std::endl;
                }
            }

            //  We add the file_ID 4 data extends to the catalog file
            if (file_ID==4 && forkType==0x00)
            {
                for (int i = 0; i < 3; i++) {
                    auto extent = extent_record.get_extent( i );
                    if (extent.count > 0) {
                        catalog_.add_extent( startBlock, extent );
                    }
                }
            }
        });

        std::cout << "===================================" << std::endl;

        //  We now have a proper catalog file
        //  We also (later) will know all the extents of all the files

        btree_file_t catalog_btree = catalog_.as_btree_file();

        // extents_btree.iterate_files( [&]( const catalog_record_file_t &record ) )
        // {

        // }


        //  We have a uint32_t -> File map for the files
        //  and a uint32_t -> Folder map for the folders
        //  and a vector of hierachy_t for the folder -> file hierarchy
        //  and one for the folder -> folder hierarchy

        std::map<uint32_t, Folder> folders;
        std::map<uint32_t, File> files;
        std::vector<hierarchy_t> folder_hierarchy;
        std::vector<hierarchy_t> file_hierarchy;

        folders.emplace(1, Folder("/")); // root folder

        catalog_btree.iterate_catalog( [&]( const catalog_record_t *record, const catalog_record_folder_t *folder, const catalog_record_file_t *file )
        {
            if (folder)
            {
                std::cout << std::format( " FOLDER: {} / {} [{}] {} items)\n",
                    folder->folder_id(),
                    record->parent_id(),
                    record->name(),
                    folder->item_count()
                );
                folders.emplace( folder->folder_id(), Folder( record->name() ) );
                folder_hierarchy.push_back( { record->parent_id(), folder->folder_id() } );
            }
            if (file)
            {
                std::cout << std::format( " FILE  : {} / {} [{}] {}/{} DATA: {} bytes RSRC: {} bytes\n", 
                    file->file_id(),
                    record->parent_id(),
                    record->name(),
                    file->type(), 
                    file->creator(),
                    file->dataLogicalSize(),
                    file->rsrcLogicalSize()
                );
                files.emplace( file->file_id(), File( record->name(), file->type(), file->creator(), file->dataLogicalSize(), file->rsrcLogicalSize() ) );
                file_hierarchy.push_back( { record->parent_id(), file->file_id() } );
            }
        });

        //  We now add the folders to their parents
        for (const auto &h : folder_hierarchy)
        {
            auto parent_it = folders.find(h.parent_id);
            auto child_it = folders.find(h.child_id);
            if (parent_it != folders.end() && child_it != folders.end())
            {
                parent_it->second.add_folder( &child_it->second );
            }
            else
            {
                std::cerr << "Error: Folder hierarchy references unknown folder ID\n";
            }   
        }   

        //  We now add the files to their parents
        for (const auto &h : file_hierarchy)
        {
            auto parent_it = folders.find(h.parent_id);
            auto child_it = files.find(h.child_id);
            if (parent_it != folders.end() && child_it != files.end())
            {
                parent_it->second.add_file( &child_it->second );
            }
            else
            {
                std::cerr << "Error: File hierarchy references unknown folder or file ID\n";
            }   
        }

        // Print the complete folder hierarchy starting from root
        std::cout << "\n=== FOLDER HIERARCHY ===" << std::endl;
        auto root_it = folders.find(1);
        if (root_it != folders.end()) {
            print_folder_hierarchy(root_it->second);
        }

        // readCatalogHeader(be16toh_custom(mdb.drCTExtRec[0][0]));
        // readCatalogRoot(rootNode_);

        // std::cout << "===================================" << std::endl;
    }
    else
    {
        std::cout << "Not a valid HFS volume or corrupted header" << std::endl;
    }
}

// Process Apple Partition Map
void processAPM(std::ifstream &file)
{
    std::vector<uint8_t> block(512);

    // Read block 1 (first partition map entry)
    file.seekg(512);
    file.read(reinterpret_cast<char *>(block.data()), 512);

    if (!file.good())
    {
        std::cerr << "Error reading partition map\n";
        return;
    }

    const ApplePartitionMapEntry *entry =
        reinterpret_cast<const ApplePartitionMapEntry *>(block.data());

    // Check partition map signature
    if (be16toh(entry->pmSig) != 0x504D)
    {
        std::cout << "Not a valid Apple Partition Map\n";
        return;
    }

    uint32_t mapBlocks = be32toh(entry->pmMapBlkCnt);
    std::cout << "Found Apple Partition Map with " << mapBlocks << " entries\n";

    // Process each partition entry
    for (uint32_t i = 1; i <= mapBlocks; i++)
    {
        file.seekg(i * 512);
        file.read(reinterpret_cast<char *>(block.data()), 512);

        if (!file.good())
            break;

        entry = reinterpret_cast<const ApplePartitionMapEntry *>(block.data());

        if (be16toh(entry->pmSig) != 0x504D)
            continue;

        std::string partName = std::string(reinterpret_cast<const char *>(entry->pmPartName), static_cast<size_t>(32));
        std::string partType = std::string(reinterpret_cast<const char *>(entry->pmParType), static_cast<size_t>(32));

        // Remove null terminators
        partName = partName.c_str();
        partType = partType.c_str();

        std::cout << "\nPartition " << i << ": " << partName << " (Type: " << partType << ")\n";

        // Check if this is an HFS partition
        if (partType == "Apple_HFS" || partType == "Apple_HFSX")
        {
            uint64_t startOffset = be32toh(entry->pmPyPartStart) * 512ULL;
            uint64_t size = be32toh(entry->pmPartBlkCnt) * 512ULL;

            std::cout << "  Start: " << startOffset << " bytes, Size: " << size << " bytes\n";
            partition_t hfs(file, startOffset, size);
            hfs.readMasterDirectoryBlock();
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <disk_image_file>\n";
        std::cerr << "Analyzes vintage Macintosh HFS disk images and prints volume names.\n";
        return 1;
    }

    const char *filename = argv[1];
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "Error: Cannot open file '" << filename << "'\n";
        return 1;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    uint64_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::cout << "Analyzing disk image: " << filename << " (" << fileSize << " bytes)\n";

    // Read first block to check for partition map
    std::vector<uint8_t> block0(512);
    file.read(reinterpret_cast<char *>(block0.data()), 512);

    if (!file.good())
    {
        std::cerr << "Error reading first block\n";
        return 1;
    }

    // Check if this looks like a partitioned disk
    // Block 0 should contain driver descriptor record
    // Block 1 should contain first partition map entry
    file.seekg(512);
    std::vector<uint8_t> block1(512);
    file.read(reinterpret_cast<char *>(block1.data()), 512);

    if (file.good())
    {
        const ApplePartitionMapEntry *entry =
            reinterpret_cast<const ApplePartitionMapEntry *>(block1.data());

        if (be16toh(entry->pmSig) == 0x504D)
        {
            // This appears to be a partitioned disk
            std::cout << "Detected partitioned disk image\n";
            processAPM(file);
            file.close();
            return 0;
        }
    }

    // Not a partitioned disk, check if it's a raw HFS partition
    std::cout << "Checking for raw HFS partition...\n";

    // Try block 2 (offset 1024) for HFS MDB
    file.seekg(1024);
    std::vector<uint8_t> hfsBlock(512);
    file.read(reinterpret_cast<char *>(hfsBlock.data()), 512);

    if (file.good())
    {
		block_t block(hfsBlock);
		auto mdb = block.as_master_directory_block();
        if (mdb.isHFSVolume())
        {
            std::cout << "Found raw HFS partition\n";
            partition_t hfs(file, 0, fileSize);
            hfs.readMasterDirectoryBlock();
        }
        else
        {
            std::cout << "File does not appear to be a valid HFS disk image\n";
            return 1;
        }
    }
    else
    {
        std::cout << "File too small or corrupted\n";
        return 1;
    }

    file.close();
    return 0;
}