#pragma once

#include <cstdint>

// Macintosh Resource Fork structures
// Based on Apple's resource fork format specification

#pragma pack(push, 1)

// Resource header structure (as stored in resource fork)
typedef struct ResourceHeader {
    uint32_t dataOffset;  // BE: offset to start of resource data area
    uint32_t mapOffset;   // BE: offset to start of resource map
    uint32_t dataLength;  // BE: length of resource data area (bytes)
    uint32_t mapLength;   // BE: length of resource map (bytes)
} ResourceHeader;

// Top-level structure for the Resource Map area
typedef struct ResourceMap {
	ResourceHeader copyOfHeader; // Copy of the ResourceHeader
	
    uint32_t nextResourceMap;        // Handle to next resource map (usually 0)
    int16_t  fileRefNum;             // File reference number
    uint16_t attributes;             // Resource file attributes (bit flags)
    uint16_t typeListOffset;         // Offset to type list (from start of map)
    uint16_t nameListOffset;         // Offset to name list (from start of map)
    // ... followed by type list and name list areas
} ResourceMap;

// One entry in the Type List
typedef struct ResourceType {
    uint32_t type;          // 4-char code, e.g. 'PICT'
    uint16_t numResources;  // number of resources of this type minus 1
    uint16_t refListOffset; // offset (from start of type list) to first reference
} ResourceType;

// One entry in a Reference List (for a specific type)
typedef struct ResourceReference {
    int16_t  id;            // Resource ID
    uint16_t nameOffset;    // Offset from start of name list (or 0xFFFF if none)
    uint8_t  attributes;    // Resource attributes
    uint8_t  dataOffset[3]; // Offset (from start of resource data) to data
    uint32_t handle;        // Reserved / not used in files
} ResourceReference;

#pragma pack(pop)

/*
File structure:

+-------------------+
| Resource Header   |  — 16 bytes
+-------------------+
| Resource Data     |
+-------------------+
| Resource Map      |
+-------------------+


Resource Map:
 ├─ Copy of Header (16 bytes)
 ├─ Handle to next resource map (4 bytes, usually 0)
 ├─ File reference number (2 bytes)
 ├─ Resource file attributes (2 bytes)
 ├─ Offset to type list (4 bytes)
 ├─ Offset to name list (4 bytes)
 ├─ Type List
 │   ├─ Type Count - 1 (2 bytes)
 │   ├─ Type Entries (12 bytes each)
 │   │   ├─ Type (4 bytes, e.g. 'ICN#')
 │   │   ├─ Resource Count - 1 (2 bytes)
 │   │   ├─ Offset to Reference List (2 bytes)
 │   ├─ Reference Lists
 │       ├─ One entry per resource of this type:
 │           ├─ Resource ID (2 bytes)
 │           ├─ Offset to Name (2 bytes, or 0xFFFF for none)
 │           ├─ Attributes (1 byte)
 │           ├─ Offset to Data (3 bytes, relative to start of data area)
 │           ├─ Handle (4 bytes, unused in files)
 └─ Name List
     ├─ Pascal-style strings of resource names

All integers are big-endian.

typeListOffset and nameListOffset are relative to the start of the map, not absolute file offsets.

numResources and the type count in the type list are “minus one” values (e.g., if 3 resources, field = 2).

The dataOffset field in ResourceReference is 3 bytes, stored big-endian, relative to the start of the resource data area.

Resource names in the name list are Pascal-style strings (1 length byte followed by text bytes).
*/
