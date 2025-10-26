#pragma once

// HFS structures
#pragma pack(push, 1)

//	HFS Extent Record (contigous portion of a file)
//	It is counted from the "allocation start" of the partition
//	and is counted in allocation blocks (may not be 512 byte blocks)
struct HFSExtentRecord
{
    uint16_t startBlock;
    uint16_t blockCount;
};

//	This is the master directory block, at block 2 of an HFS volume (512 bytes block size)
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
	HFSExtentRecord drXTExtRec[3]; // 0x86: Extents overflow extents (3 extent descriptors)
	uint32_t drCTFlSize;       // 0x92: Catalog file size
	HFSExtentRecord drCTExtRec[3]; // 0x96: Catalog file extents (3 extent descriptors)
};

//	The type of b-tree nodes
const int8_t ndIndxNode = 0;		//	An index node
const int8_t ndHdrNode = 1;			//	Header, describes the b-tree
const int8_t ndMapNode = 2;
const int8_t ndLeafNode = -1;		//	A leaf node (contains actual data, chained together)

// HFS Catalog record types
const uint16_t kHFSFolderRecord = 0x0100;		// Folder record
const uint16_t kHFSFileRecord = 0x0200;			// File record
const uint16_t kHFSFolderThreadRecord = 0x0300;	// Folder thread record
const uint16_t kHFSFileThreadRecord = 0x0400;		// File thread record

// HFS B-tree node (all nodes)
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

// HFS Extents B-tree Leaf Record
struct HFSExtentsRecord
{
	uint8_t keyLength;              // Key length
	uint8_t forkType;               // Fork type (0 = data fork, 0xFF = resource fork)
	uint32_t fileID;                // File ID
	uint16_t startBlock;            // Starting allocation block
	HFSExtentRecord extents[3];     // Extent descriptors
};

// HFS Catalog Key
struct HFSCatalogKey
{
    uint8_t keyLength;    // Key length
    uint8_t reserved;     // Reserved
    uint32_t parentID;    // Parent directory ID
    uint8_t nodeName[32]; // Node name (Pascal string)
};

// HFS Catalog Directory Record - 70 bytes
struct HFSCatalogFolder
{
    int16_t recordType;     // Record type (0x0100 = folder)
    uint16_t flags;         // Folder flags
    uint16_t valence;       // Number of items in folder
    uint32_t folderID;      // Folder ID (same as dirID)
    uint32_t createDate;    // Creation date
    uint32_t modifyDate;    // Modification date
    uint32_t backupDate;    // Backup date
    struct {                // Finder information - 16 bytes
        struct {            // Folder's window rectangle
            int16_t top;
            int16_t left;
            int16_t bottom;
            int16_t right;
        } frRect;
        uint16_t frFlags;   // Finder flags
        struct {
            uint16_t v;     // Folder's location vertical
            uint16_t h;     // Folder's location horizontal
        } frLocation;
        int16_t opaque;     // Reserved
    } userInfo;
    struct {                // Additional Finder information - 16 bytes
        int8_t opaque[16];
    } finderInfo;
    uint32_t reserved[4];   // Reserved - initialized as zero
};

// HFS Catalog File Record - 102 bytes
struct HFSCatalogFile
{
    int16_t recordType;             // Record type (0x0200 = file)
    uint8_t flags;                  // File flags
    int8_t fileType;                // File type (unused, set to zero)
    struct {                        // Finder information - 16 bytes
        uint32_t fdType;            // File type
        uint32_t fdCreator;         // File creator
        uint16_t fdFlags;           // Finder flags
        struct {
            int16_t v;              // File location vertical
            int16_t h;              // File location horizontal
        } fdLocation;
        int16_t opaque;             // Reserved
    } userInfo;
    uint32_t fileID;                // File ID
    uint16_t dataStartBlock;        // Data fork start block (not used - set to zero)
    int32_t dataLogicalSize;        // Data fork logical size
    int32_t dataPhysicalSize;       // Data fork physical size
    uint16_t rsrcStartBlock;        // Resource fork start block (not used - set to zero)
    int32_t rsrcLogicalSize;        // Resource fork logical size
    int32_t rsrcPhysicalSize;       // Resource fork physical size
    uint32_t createDate;            // Creation date
    uint32_t modifyDate;            // Modification date
    uint32_t backupDate;            // Backup date
    struct {                        // Additional Finder information - 16 bytes
        int8_t opaque[16];
    } finderInfo;
    uint16_t clumpSize;             // File clump size (not used)
    HFSExtentRecord dataExtents[3]; // Data fork extent records
    HFSExtentRecord rsrcExtents[3]; // Resource fork extent records
    uint32_t reserved;              // Reserved - initialized as zero
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

