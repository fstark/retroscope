#pragma once

// MFS (Macintosh File System) structures
#pragma pack(push, 1)

//	This is the master directory block for MFS, at block 2 of an MFS volume (512 bytes block size)
struct MFSMasterDirectoryBlock
{
	uint16_t drSigWord;        // 0x00: Volume signature (0xD2D7 for MFS)
	uint32_t drCrDate;         // 0x02: Volume creation date
	uint32_t drLsBkUp;         // 0x06: Date of last backup
	uint16_t drAtrb;           // 0x0A: Volume attributes
	uint16_t drNmFls;          // 0x0C: Number of files in directory
	uint16_t drDirSt;          // 0x0E: First block of directory
	uint16_t drBlLen;          // 0x10: Length of directory in blocks
	uint16_t drNmAlBlks;       // 0x12: Number of allocation blocks
	uint32_t drAlBlkSiz;       // 0x14: Size of allocation blocks
	uint32_t drClpSiz;         // 0x18: Default clump size
	uint16_t drAlBlSt;         // 0x1C: First allocation block
	uint32_t drNxtFNum;        // 0x1E: Next unused file number
	uint16_t drFreeBks;        // 0x22: Number of unused allocation blocks
	uint8_t drVN[28];          // 0x24: Volume name (Pascal string)
};

// MFS Directory Entry (50 bytes total)
struct MFSDirectoryEntry {
    int8_t     deFlags;       /* file flags (bit 7 = used, bit 0 = locked) */
    int8_t     deVers;        /* version, usually 0 */
    int32_t    deType;        /* 4-char file type */
    int32_t    deCreator;     /* 4-char creator */
    int16_t    deFndrFlags;   /* Finder flags */
    int32_t    deIconPos;     /* Finder icon position / info */
    int16_t    deFolder;      /* pseudo-folder ID (Finder use) */
    int32_t    deFileNum;     /* unique file ID */
    int16_t    deDataABlk;    /* first alloc block of data fork */
    int32_t    deDataLen;     /* logical EOF of data fork */
    int32_t    deDataAlloc;   /* allocated bytes for data fork */
    int16_t    deRsrcABlk;    /* first alloc block of resource fork */
    int32_t    deRsrcLen;     /* logical EOF of resource fork */
    int32_t    deRsrcAlloc;   /* allocated bytes for resource fork */
    int32_t    deCrDate;      /* creation date (secs since 1904) */
    int32_t    deMdDate;      /* modification date */
    uint8_t    deNameLen;     /* Pascal length of filename (0–63) */
    /* char deName[deNameLen];  <-- variable-length Pascal string follows */
    /* padding to even boundary */
};

// MFS Volume attributes
const uint16_t kMFSVolumeHardwareLockBit = 7;      // Volume is locked by hardware
const uint16_t kMFSVolumeSoftwareLockBit = 15;     // Volume is locked by software

// MFS File flags
const uint8_t kMFSFileLockedBit = 0;               // File is locked

#pragma pack(pop)