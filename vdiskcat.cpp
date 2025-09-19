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

// HFS structures
#pragma pack(push, 1)

class HFSPartition
{
    const int8_t ndIndxNode = 0;
    const int8_t ndHdrNode = 1;
    const int8_t ndMapNode = 2;
    const int8_t ndLeafNode = -1;

    std::ifstream &file_;
    uint64_t partitionStart_;
    uint64_t partitionSize_;
    uint64_t allocationStart_ = 0;
    uint64_t blockSize_ = 512;

    struct extent_t
    {
        uint16_t start;
        uint16_t count;
    };

    class file_t
    {
        std::vector<extent_t> extents_;
        // uint32_t size_;

    public:
        file_t() = default;

        void add_extent(uint16_t start, uint16_t count)
        {
            extents_.push_back({start, count});
        }

        uint16_t to_absolute_block(uint16_t block) const
        {
            for (auto &ext : extents_)
            {
                if (block < ext.count)
                    return ext.start + block;
                block -= ext.count;
            }
            return 0xffff;
        }
    };

    file_t extents_;
    file_t catalog_;

    class btree_header_node_t;
    class master_record_t;

    class block_t
    {
    public:
        btree_header_node_t as_btree_node();
        master_record_t as_master_record();

        void *data() { return data_.data(); }
    };

    // HFS Master Directory Block (Volume Header)
    class master_record_t /* HFSMasterDirectoryBlock */
    {
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
        std::shared_ptr<block_t> block_;

        HFSMasterDirectoryBlock *content;

    public:
        master_record_t(std::shared_ptr<block_t> b) : block_(b)
        {
            content = reinterpret_cast<HFSMasterDirectoryBlock *>(block_->data());
        }

        bool isHFSVolume(std::string &volumeName)
        {
            return be16toh_custom(content->drSigWord) == 0x4244;
        }

        std::string getVolumeName() const
        {
            return pascalToString(content->drVN);
        }
    };

    class btree_header_node_t
    {
    };

    class disk_t
    {
    public:
        block_t read(uint16_t block);
    };

    // xxx this is where we are
    // we want to scan btree with a callback
    // to be used twice:
    // 1 - to create the extends for all the files (store in a map)
    // 2 - to create all the file objects (file and directories) in a map

    //	btree records
    //	type Thread: [File#, ParentID, Name]
    //	type File  : [ParentID, Name, File #]
    //	type Folder: [ParentID, Name, File #] (to check)

    //	This scans all the btree leaves in order
    // void btree_leaf_scan(uint16_t header, const file_t &file)
    // {
    //     //	1 read the header node
    //     //	2 find the first leaf node
    //     //	3 read each leaf node in order
    // }

    uint16_t rootNode_;

    void dump(const std::vector<uint8_t> &data)
    {
        for (size_t i = 0; i < data.size(); i += 16)
        {
            std::cout << std::hex << std::setw(8) << std::setfill('0') << i << ": ";
            for (size_t j = 0; j < 16 && i + j < data.size(); j++)
            {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(data[i + j]) << " ";
            }
            std::cout << " ";
            for (size_t j = 0; j < 16 && i + j < data.size(); j++)
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

    //	For reading 512 bytes blocks relative to the partition start
    std::vector<uint8_t> readBlock512(uint64_t blockOffset)
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

    //	For reading blockSize_ bytes blocks relative to the allocationStart_
    std::vector<uint8_t> readBlock(uint64_t blockOffset)
    {
        std::vector<uint8_t> block(blockSize_);
        auto offset = partitionStart_ + allocationStart_ * 512 /* ? */ + blockOffset * blockSize_;
        std::cout << "Reading block at offset: " << offset << std::endl;

        file_.seekg(offset);
        file_.read(reinterpret_cast<char *>(block.data()), blockSize_);
        if (!file_.good())
        {
            throw std::runtime_error("Error reading block");
            return {};
        }
        return block;
    }

public:
    HFSPartition(std::ifstream &file, uint64_t partitionStart, uint64_t partitionSize)
        : file_(file), partitionStart_(partitionStart), partitionSize_(partitionSize)
    {
        std::cout << "Partition start: " << partitionStart_ << ", size: " << partitionSize_ << std::endl;
    }

    void readMasterDirectoryBlock();
    void readCatalogHeader(uint64_t catalogStartBlock);
    void readCatalogRoot(uint16_t rootNode);

    void dumpextentTree();
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
    uint16_t pmSig;          // Partition signature (0x504D)
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

// Convert big-endian to host byte order
uint16_t be16toh_custom(uint16_t val)
{
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

uint32_t be32toh_custom(uint32_t val)
{
    return ((val & 0xFF) << 24) | (((val >> 8) & 0xFF) << 16) |
           (((val >> 16) & 0xFF) << 8) | ((val >> 24) & 0xFF);
}

// Convert Pascal string to C++ string
std::string pascalToString(const uint8_t *pascalStr)
{
    if (pascalStr[0] == 0)
        return "";
    return std::string(reinterpret_cast<const char *>(pascalStr + 1), pascalStr[0]);
}

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

// Check if a block contains a valid HFS Master Directory Block
bool isHFSVolume(const std::vector<uint8_t> &block, std::string &volumeName, HFSMasterDirectoryBlock &mdb)
{
    if (block.size() < sizeof(HFSMasterDirectoryBlock))
    {
        return false;
    }

    const HFSMasterDirectoryBlock *mdbPtr =
        reinterpret_cast<const HFSMasterDirectoryBlock *>(block.data());

    // Check HFS signature (0x4244 = "BD" in big-endian)
    if (be16toh_custom(mdbPtr->drSigWord) == 0x4244)
    {
        volumeName = pascalToString(mdbPtr->drVN);
        mdb = *mdbPtr;
        return true;
    }

    return false;
}

void HFSPartition::dumpextentTree()
{
    std::cout << "\n\nDumping Catalog Extent Tree:\n";
    // for (size_t i = 0; i < ca.size(); i++) {
    // 	std::cout << "Extent " << i << ": Start Block = " << catalogExtents_[i].start
    // 	          << ", Block Count = " << catalogExtents_[i].count << std::endl;
    // }
}

void HFSPartition::readCatalogRoot(uint16_t rootNode)
{
    std::cout << "\n\nReading Catalog B-tree root node at block " << rootNode << std::endl;

    auto block = readBlock(rootNode - 4);

    if (block.size() < sizeof(BTNodeDescriptor))
    {
        std::cerr << "Catalog block too small\n";
        return;
    }

    dump(block);

    const BTNodeDescriptor *nodeDesc =
        reinterpret_cast<const BTNodeDescriptor *>(block.data());

    std::cout << "NODE TYPE: " << (int)nodeDesc->kind << "\n";

    if (nodeDesc->kind == ndIndxNode)
    {
        std::cout << "Index NODE\n";
    }
    else
        return;

    uint16_t numRecords = be16toh_custom(nodeDesc->numRecords);
    std::cout << "Number of records in root node: " << numRecords << std::endl;
    int8_t nodeType = nodeDesc->kind;
    std::cout << "Node type: " << (int)nodeType << std::endl;

    // Record offsets are stored at the end of the node
    const uint8_t *recordOffsets = block.data() + blockSize_ - (numRecords + 2) * sizeof(uint16_t);

    for (int i = 0; i < numRecords; i++)
    {
        uint16_t offset = be16toh_custom(*reinterpret_cast<const uint16_t *>(recordOffsets + i * sizeof(uint16_t)));
        if (offset >= blockSize_)
        {
            std::cerr << "Invalid record offset\n";
            continue;
        }

        const uint8_t *record = block.data() + offset;

        // Read key length
        uint16_t keyLength = be16toh_custom(*reinterpret_cast<const uint16_t *>(record));
        if (keyLength + sizeof(uint16_t) > blockSize_ - offset)
        {
            std::cerr << "Invalid key length\n";
            continue;
        }

        const HFSCatalogKey *key =
            reinterpret_cast<const HFSCatalogKey *>(record + sizeof(uint16_t));

        std::string nodeName = pascalToString(key->nodeName);
        uint32_t parentID = be32toh_custom(key->parentID);

        std::cout << "Record " << i << ": Key length: " << keyLength
                  << ", Parent ID: " << parentID
                  << ", Name: \"" << nodeName << "\"\n";
    }
}

void HFSPartition::readCatalogHeader(uint64_t catalogStartBlock)
{
    std::cout << "\n\nReading Catalog B-tree header at block " << catalogStartBlock << std::endl;

    auto block = readBlock(catalogStartBlock);

    if (block.size() < sizeof(BTNodeDescriptor) + sizeof(BTHeaderRec))
    {
        std::cerr << "Catalog block too small\n";
        return;
    }

    const BTNodeDescriptor *nodeDesc =
        reinterpret_cast<const BTNodeDescriptor *>(block.data());

    std::cout << "NODE TYPE: " << (int)nodeDesc->kind << "\n";

    if (nodeDesc->kind != 1)
    { // 1 = header node
        std::cerr << "Not a valid B-tree header node\n";
        return;
    }

    const BTHeaderRec *headerRec =
        reinterpret_cast<const BTHeaderRec *>(block.data() + sizeof(BTNodeDescriptor));

    uint16_t nodeSize = be16toh_custom(headerRec->nodeSize);
    uint32_t totalNodes = be32toh_custom(headerRec->totalNodes);
    uint32_t freeNodes = be32toh_custom(headerRec->freeNodes);
    uint16_t treeDepth = be16toh_custom(headerRec->treeDepth);
    uint32_t rootNode = be32toh_custom(headerRec->rootNode);
    uint32_t leafRecords = be32toh_custom(headerRec->leafRecords);

    dumpextentTree();

    exit(0);

    // rootNode_ =  to_absolute_block(rootNode);

    // std::cout << "=== HFS Catalog B-tree Header ===" << std::endl;
    // std::cout << "Node size: " << nodeSize << " bytes" << std::endl;
    // std::cout << "Total nodes: " << totalNodes << std::endl;
    // std::cout << "Free nodes: " << freeNodes << std::endl;
    // std::cout << "Tree depth: " << (int)treeDepth << std::endl;
    // std::cout << "Root node: " << rootNode << " absolute sector:" << to_absolute_block(rootNode) << std::endl;
    // std::cout << "Leaf records: " << leafRecords << std::endl;
    // std::cout << "=================================" << std::endl;

    std::cout << "\n\n";
}

// Process a single HFS partition
void HFSPartition::readMasterDirectoryBlock()
{
    auto block = readBlock512(2); // Read block 2 (offset 1024)

    std::string volumeName;
    HFSMasterDirectoryBlock mdb;
    if (isHFSVolume(block, volumeName, mdb))
    {
        // Print MDB information as requested
        std::cout << "=== HFS Master Directory Block ===" << std::endl;

        // MDB structure size in memory
        std::cout << "MDB structure size: " << sizeof(HFSMasterDirectoryBlock) << " bytes" << std::endl;

        // SIGNATURE
        uint16_t signature = be16toh_custom(mdb.drSigWord);
        std::cout << "SIGNATURE: 0x" << std::hex << signature << std::dec << " (\"BD\")" << std::endl;

        // Volume name
        std::cout << "Volume name: \"" << volumeName << "\"" << std::endl;

        // Number of files in root dir
        uint16_t filesInRoot = be16toh_custom(mdb.drNmFls);
        std::cout << "# of files in root dir: " << filesInRoot << std::endl;

        // Number of directories in root dir
        uint16_t dirsInRoot = be16toh_custom(mdb.drNmRtDirs);
        std::cout << "# of directories in root dir: " << dirsInRoot << std::endl;

        // Number of files in volume
        uint32_t filesInVolume = be32toh_custom(mdb.drFilCnt);
        std::cout << "# of files in volume: " << filesInVolume << std::endl;

        // Number of directories in volume
        uint32_t dirsInVolume = be32toh_custom(mdb.drDirCnt);
        std::cout << "# of directories: " << dirsInVolume << std::endl;

        // Logical block size
        uint32_t drAlBlkSiz = be32toh_custom(mdb.drAlBlkSiz);
        std::cout << "Logical block size: " << drAlBlkSiz << " bytes" << std::endl;
        blockSize_ = drAlBlkSiz;

        uint16_t drAlBlSt = be16toh_custom(mdb.drAlBlSt);
        std::cout << "First allocation block: " << drAlBlSt << std::endl;
        allocationStart_ = drAlBlSt;

        extents_.add_extent(drAlBlSt, be16toh_custom(mdb.drNmAlBlks));

        // Allocation block size
        uint32_t allocationBlockSize = be32toh_custom(mdb.drAlBlkSiz);
        std::cout << "Allocation block size: " << allocationBlockSize << " bytes" << std::endl;

        // Number of allocation blocks
        uint16_t numAllocBlocks = be16toh_custom(mdb.drNmAlBlks);
        std::cout << "Number of allocation blocks: " << numAllocBlocks << std::endl;

        // Extents overflow file size (prob useless)
        uint32_t extOverflowSize = be32toh_custom(mdb.drXTFlSize);
        std::cout << "Extents overflow file size (drXTFlSize): " << extOverflowSize << " bytes" << std::endl;

        //	extents size
        uint32_t extentsSize = be32toh_custom(mdb.drXTClpSiz);
        std::cout << "Extents overflow clump size (drXTClpSiz): " << extentsSize << " bytes" << std::endl;

        // Extents overflow extents (3 extent descriptors)
        std::cout << "drXTExtRec (extents extents):\n";
        for (int i = 0; i < 3; i++)
        {
            uint16_t startBlock = be16toh_custom(mdb.drXTExtRec[i][0]);
            uint16_t blockCount = be16toh_custom(mdb.drXTExtRec[i][1]);
            if (blockCount)
            {
                std::cout << "  [" << i << "]=" << startBlock << "-" << blockCount << "\n";
                extents_.add_extent(startBlock, blockCount);
            }
        }

        //	Catalog size (prob useless)
        uint16_t catalogSize = be32toh_custom(mdb.drCTFlSize);
        std::cout << "Catalog file size (drCTFlSize): " << catalogSize << " bytes" << std::endl;

        // Catalog file extents (first node of catalog btree)
        std::cout << "drCTExtRec (catalog extents):\n";
        for (int i = 0; i < 3; i++)
        {
            uint16_t startBlock = be16toh_custom(mdb.drCTExtRec[i][0]);
            uint16_t blockCount = be16toh_custom(mdb.drCTExtRec[i][1]);
            if (blockCount)
            {
                std::cout << "  [" << i << "]=" << startBlock << "-" << blockCount << "\n";
                catalog_.add_extent(startBlock, blockCount);
            }
        }
        std::cout << std::endl;

        std::cout << "===================================" << std::endl;

        readCatalogHeader(be16toh_custom(mdb.drCTExtRec[0][0]));
        readCatalogRoot(rootNode_);

        std::cout << "===================================" << std::endl;
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
    if (be16toh_custom(entry->pmSig) != 0x504D)
    {
        std::cout << "Not a valid Apple Partition Map\n";
        return;
    }

    uint32_t mapBlocks = be32toh_custom(entry->pmMapBlkCnt);
    std::cout << "Found Apple Partition Map with " << mapBlocks << " entries\n";

    // Process each partition entry
    for (uint32_t i = 1; i <= mapBlocks; i++)
    {
        file.seekg(i * 512);
        file.read(reinterpret_cast<char *>(block.data()), 512);

        if (!file.good())
            break;

        entry = reinterpret_cast<const ApplePartitionMapEntry *>(block.data());

        if (be16toh_custom(entry->pmSig) != 0x504D)
            continue;

        std::string partName = std::string(reinterpret_cast<const char *>(entry->pmPartName), 32);
        std::string partType = std::string(reinterpret_cast<const char *>(entry->pmParType), 32);

        // Remove null terminators
        partName = partName.c_str();
        partType = partType.c_str();

        std::cout << "\nPartition " << i << ": " << partName << " (Type: " << partType << ")\n";

        // Check if this is an HFS partition
        if (partType == "Apple_HFS" || partType == "Apple_HFSX")
        {
            uint64_t startOffset = be32toh_custom(entry->pmPyPartStart) * 512ULL;
            uint64_t size = be32toh_custom(entry->pmPartBlkCnt) * 512ULL;

            std::cout << "  Start: " << startOffset << " bytes, Size: " << size << " bytes\n";
            HFSPartition hfs(file, startOffset, size);
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

        if (be16toh_custom(entry->pmSig) == 0x504D)
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
        std::string volumeName;
        HFSMasterDirectoryBlock mdb;
        if (isHFSVolume(hfsBlock, volumeName, mdb))
        {
            std::cout << "Found raw HFS partition\n";
            HFSPartition hfs(file, 0, fileSize);
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