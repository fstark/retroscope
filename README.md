# vdiskcat
Scan vintage HFS disks

## Testing

disk.img contains a sample hfs filesystem

A C++ command-line tool that analyzes vintage Macintosh HFS disk images and extracts volume names from HFS partitions. The tool will then list all files present on the disk, recursively.

## Features

- Detects Apple Partition Map (APM) formatted disk images
- Processes individual HFS partitions within partitioned disks
- Handles raw HFS partition files
- Extracts and displays HFS volume names
- **Lists all files and directories recursively (when catalog can be parsed)**
- Supports both partitioned disk images and single partition files
- Parses HFS catalog B-tree structure to enumerate all files

## Usage

```bash
./vdiskcat <disk_image_file>
```

The tool will:
1. Check if the file is a partitioned disk with Apple Partition Map
2. If partitioned, scan all HFS partitions and display their volume names
3. If not partitioned, treat as a raw HFS partition and extract the volume name
4. **Parse the HFS catalog and list all files and directories recursively**

## Building

```bash
make
```

Or manually:
```bash
g++ -std=c++11 -Wall -Wextra -O2 -o vdiskcat vdiskcat.cpp
```

## Technical Details

The tool understands:
- Apple Partition Map (APM) structure
- HFS Master Directory Block format
- **HFS catalog B-tree structure for file enumeration**
- Big-endian byte order used by classic Macintosh systems
- Pascal strings used for volume names in HFS
- **HFS extent records for reading file data**

## Example Output

For a partitioned disk:
```
Analyzing disk image: system.img (1474560 bytes)
Detected partitioned disk image
Found Apple Partition Map with 3 entries

Partition 1: Apple (Type: Apple_partition_map)

Partition 2: MacOS (Type: Apple_HFS)
  Start: 512 bytes, Size: 1474048 bytes
HFS Volume: "System Disk"

Files and directories:
[DIR]  System Folder/
  [FILE] System (482304 bytes)
  [FILE] Finder (204800 bytes)
  [DIR]  Extensions/
    [FILE] AppleShare (45056 bytes)
    [FILE] Network Extension (32768 bytes)
[DIR]  Applications/
  [FILE] SimpleText (98304 bytes)
[FILE] ReadMe (4096 bytes)
```

For a raw partition:
```
Analyzing disk image: volume.hfs (800000 bytes)
Checking for raw HFS partition...
Found raw HFS partition
HFS Volume: "My Data"

Files and directories:
[DIR]  Documents/
  [FILE] Letter.txt (2048 bytes)
  [FILE] Spreadsheet.xls (15360 bytes)
[DIR]  Pictures/
  [FILE] Photo1.jpg (102400 bytes)
[FILE] Notes.txt (1024 bytes)
```
