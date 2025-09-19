#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <map>
#include <algorithm>
#include <array>
#include <memory>

// HFS structures
#pragma pack(push, 1)

struct HFSMasterDirectoryBlock
{
	uint16_t drSigWord;        // 0x00: Volume signature (0x4244 for HFS)
	uint32_t drCrDate;         // 0x02: Volume creation date
	uint32_t drLsMod;          // 0x06: Date of last modification
	uint16_t drAtrb;           // 0x0A: Volume attributes
	uint16_t drNmFls;          // 0x0C: Number of files in root directory
	uint16_t drVBMSt;          // 0x0E: Start of volume bitmap
	uint16_t drAllocPtr;       // 0x10: Start of next allocation search
	uint16_t drNmAlBlks;       // 0x12: Number of allocation blocks
	uint32_t drAlBlkSiz;       // 0x14: Size of allocation blocks
	uint32_t drClpSiz;         // 0x18: Default clump size
	uint16_t drAlBlSt;         // 0x1C: First allocation block
	uint32_t drNxtCNID;        // 0x1E: Next unused catalog node ID
	uint16_t drFreeBks;        // 0x22: Number of unused allocation blocks
	uint8_t drVN[28];          // 0x24: Volume name (Pascal string)
	uint32_t drVolBkUp;        // 0x40: Date of last backup
	uint16_t drVSeqNum;        // 0x44: Volume backup sequence number
	uint32_t drWrCnt;          // 0x46: Volume write count
	uint32_t drXTClpSiz;       // 0x4A: Extents overflow clump size
	uint32_t drCTClpSiz;       // 0x4E: Catalog clump size
	uint16_t drNmRtDirs;       // 0x52: Number of directories in root
	uint32_t drFilCnt;         // 0x54: Number of files in volume
	uint32_t drDirCnt;         // 0x58: Number of directories in volume
	uint32_t drFndrInfo[8];    // 0x5C: Finder information
	uint16_t drVCSize;         // 0x7C: Volume cache size
	uint16_t drVBMCSize;       // 0x7E: Volume bitmap cache size
	uint16_t drCtlCSize;       // 0x80: Common volume cache size
	uint32_t drXTFlSize;       // 0x82: Extents overflow file size
	uint16_t drXTExtRec[3][2]; // 0x86: Extents overflow extents (3 extent descriptors)
	uint32_t drCTFlSize;       // 0x92: Catalog file size
	uint16_t drCTExtRec[3][2]; // 0x96: Catalog file extents (3 extent descriptors)
};

// HFS Extent Record
struct HFSExtentRecord
{
    uint16_t startBlock;
    uint16_t blockCount;
};

// HFS B-tree node header
struct BTNodeDescriptor
{
    uint32_t fLink;      // Forward link
    uint32_t bLink;      // Backward link
    int8_t kind;         // Node type
    uint8_t height;      // Node height
    uint16_t numRecords; // Number of records in node
    uint16_t reserved;
};

// HFS B-tree header record
struct BTHeaderRec
{
    uint16_t treeDepth;     // Current depth of tree
    uint32_t rootNode;      // Node number of root node
    uint32_t leafRecords;   // Number of leaf records
    uint32_t firstLeafNode; // Node number of first leaf
    uint32_t lastLeafNode;  // Node number of last leaf
    uint16_t nodeSize;      // Node size in bytes
    uint16_t maxKeyLength;  // Maximum key length
    uint32_t totalNodes;    // Total number of nodes
    uint32_t freeNodes;     // Number of free nodes
    uint16_t reserved1;
    uint32_t clumpSize;     // Clump size
    uint8_t btreeType;      // B-tree type
    uint8_t keyCompareType; // Key compare type
    uint32_t attributes;    // B-tree attributes
    uint32_t reserved3[16];
};

// HFS Catalog Key
struct HFSCatalogKey
{
    uint8_t keyLength;    // Key length
    uint8_t reserved;     // Reserved
    uint32_t parentID;    // Parent directory ID
    uint8_t nodeName[32]; // Node name (Pascal string)
};

// HFS Catalog Directory Record
struct HFSCatalogFolder
{
    int16_t recordType;     // Record type (1 = folder, 2 = file)
    uint16_t flags;         // Flags
    uint16_t valence;       // Number of items in folder
    uint32_t dirID;         // Directory ID
    uint32_t createDate;    // Creation date
    uint32_t modifyDate;    // Modification date
    uint32_t backupDate;    // Backup date
    uint32_t userInfo[8];   // User info
    uint32_t finderInfo[8]; // Finder info
    uint32_t textEncoding;  // Text encoding
    uint32_t reserved;      // Reserved
};

// HFS Catalog File Record
struct HFSCatalogFile
{
    int16_t recordType;             // Record type (2 = file)
    uint8_t flags;                  // Flags
    uint8_t fileType;               // File type
    uint32_t fileCreator;           // File creator
    uint16_t finderFlags;           // Finder flags
    uint32_t location[2];           // File location
    uint16_t reserved;              // Reserved
    uint32_t dataLogicalSize;       // Data fork logical size
    uint32_t dataPhysicalSize;      // Data fork physical size
    HFSExtentRecord dataExtents[3]; // Data fork extents
    uint32_t rsrcLogicalSize;       // Resource fork logical size
    uint32_t rsrcPhysicalSize;      // Resource fork physical size
    HFSExtentRecord rsrcExtents[3]; // Resource fork extents
    uint32_t createDate;            // Creation date
    uint32_t modifyDate;            // Modification date
    uint32_t backupDate;            // Backup date
    uint32_t userInfo[8];           // User info
    uint32_t finderInfo[8];         // Finder info
    uint16_t clumpSize;             // Clump size
    HFSExtentRecord extents[3];     // Additional extents
    uint32_t textEncoding;          // Text encoding
    uint32_t reserved2;             // Reserved
};

// Apple Partition Map entry
struct ApplePartitionMapEntry
{
    uint16_t pmSig;          // Partition signature (0x504M)
    uint16_t pmSigPad;       // Reserved
    uint32_t pmMapBlkCnt;    // Number of blocks in partition map
    uint32_t pmPyPartStart;  // First physical block of partition
    uint32_t pmPartBlkCnt;   // Number of blocks in partition
    uint8_t pmPartName[32];  // Partition name
    uint8_t pmParType[32];   // Partition type
    uint32_t pmLgDataStart;  // First logical block of data area
    uint32_t pmDataCnt;      // Number of blocks in data area
    uint32_t pmPartStatus;   // Partition status
    uint32_t pmLgBootStart;  // First logical block of boot code
    uint32_t pmBootSize;     // Size of boot code (bytes)
    uint32_t pmBootAddr;     // Boot code load address
    uint32_t pmBootAddr2;    // Reserved
    uint32_t pmBootEntry;    // Boot code entry point
    uint32_t pmBootEntry2;   // Reserved
    uint32_t pmBootCksum;    // Boot code checksum
    uint8_t pmProcessor[16]; // Processor type
    // ... padding to 512 bytes
    uint8_t padding[376];
};

#pragma pack(pop)

// Structure to hold file/directory information
struct CatalogEntry
{
    std::string name;
    uint32_t parentID;
    uint32_t cnid;
    bool isDirectory;
    uint32_t size;
    uint32_t createDate;
    uint32_t modifyDate;
};

// HFS volume context
struct HFSVolume
{
    std::ifstream *file;
    uint64_t partitionStart;
    uint16_t allocationBlockSize;
    uint16_t firstAllocationBlock;

    HFSVolume(std::ifstream *f, uint64_t start) : file(f), partitionStart(start) {}
};

// Forward declarations
class btree_header_node_t;
class master_directory_block_t;
class partition_t;
class btree_file_t;

// Utility functions
uint16_t be16toh_custom(uint16_t val);
uint32_t be32toh_custom(uint32_t val);
std::string pascalToString(const uint8_t *pascalStr);
void dump(const std::vector<uint8_t> &data);

// Classes
class block_t
{
	std::vector<uint8_t> data_;

public:
	block_t(const std::vector<uint8_t> &data) : data_(data) {}

	btree_header_node_t as_btree_node();
	master_directory_block_t as_master_directory_block();

	void *data() { return data_.data(); }
	void dump() const { ::dump(data_); }
};

class master_directory_block_t
{
	block_t &block_;
	HFSMasterDirectoryBlock *content;

public:
	master_directory_block_t(block_t &block);

	bool isHFSVolume() const;
	std::string getVolumeName() const;
	uint32_t allocationBlockSize() const;
	uint16_t allocationBlockStart() const;
	uint16_t extendsExtendStart(int index) const;
	uint16_t extendsExtendCount(int index) const;
	uint32_t catalogExtendStart(int index) const;
	uint16_t catalogExtendCount(int index) const;
};

class btree_header_node_t
{
};

class file_t
{
	struct extent_t
	{
		uint16_t start;
		uint16_t count;
	};

	std::vector<extent_t> extents_;
	partition_t &partition_;
	uint32_t block_size_ = 512;	//	For now fixed

public:
	file_t(partition_t &partition) : partition_(partition) {}

	void add_extent(uint16_t start, uint16_t count);
	partition_t &partition() { return partition_; }
	uint16_t to_absolute_block(uint16_t block) const;
	btree_file_t as_btree_file();
};

class btree_file_t
{
	file_t &file_;
public:
	btree_file_t(file_t &file);
};

class partition_t
{
    static const int8_t ndIndxNode = 0;
    static const int8_t ndHdrNode = 1;
    static const int8_t ndMapNode = 2;
    static const int8_t ndLeafNode = -1;

    std::ifstream &file_;
    uint64_t partitionStart_;
    uint64_t partitionSize_;
    uint64_t allocationStart_ = 0;
    uint64_t allocationBlockSize_ = 512;

    file_t extents_;
    file_t catalog_;
    uint16_t rootNode_;

	block_t read(uint64_t blockOffset) const;
    std::vector<uint8_t> readBlock512(uint64_t blockOffset);
    std::vector<uint8_t> readBlock(uint64_t blockOffset);

public:
    partition_t(std::ifstream &file, uint64_t partitionStart, uint64_t partitionSize);

	uint64_t allocation_start() { return allocationStart_; }
    void readMasterDirectoryBlock();
    void readCatalogHeader(uint64_t catalogExtendStartBlock);
    void readCatalogRoot(uint16_t rootNode);
    void dumpextentTree();
};

// Function declarations
void processAPM(std::ifstream &file);