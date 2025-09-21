# Retroscope

Every disk, every app, one place.

## What is this?

A C++ command-line tool that analyzes vintage Macintosh HFS disk images and list content.

## Why?

I have thousands of disk images, containing various version of vintage (m68k) Mac Software. Findig software is difficult. Knowing if I have a specific version is difficult. Finding what software opens what type of file is difficult.

Retroscope has been created to become a tool that pull all the info from directories containing disk images in one place where it can be easily queried.

## Features

- Detects Apple Partition Map (APM) formatted disk images
- Processes individual HFS partitions within partitioned disks

## TODO

* Iso CD rom 'bin' images
* dc42 images
* MFS support

## How to use?

For now, it is a simple command line that lists the content of a single file (with one or several partitions on it)

```
$ make
$ ./retroscope sample.dsk
Analyzing disk image: sample.dsk (819200 bytes)
Checking for raw HFS partition...
Found raw HFS partition
Partition start: 0, size: 819200
=== HFS Master Directory Block ===
Volume Name = After Dark� Disk

=== FOLDER HIERARCHY ===
Folder: /
  Folder: After Dark� Disk
    File: After Dark (cdev/ADrk) DATA: 0 bytes RSRC: 78740 bytes
    File: Desktop (FNDR/ERIK) DATA: 0 bytes RSRC: 12083 bytes
    Folder: After Dark Files
      File: Berkeley Systems Logo (PICT/DAD2) DATA: 4700 bytes RSRC: 0 bytes
      File: Apple Logo (PICT/DAD2) DATA: 12266 bytes RSRC: 0 bytes
      File: After Dark Image (PICT/DAD2) DATA: 159406 bytes RSRC: 0 bytes
      File: Doodles (ADgm/ADrk) DATA: 0 bytes RSRC: 5003 bytes
      File: Clock (ADgm/ADrk) DATA: 0 bytes RSRC: 4149 bytes
      File: Can of Worms (ADgm/ADrk) DATA: 0 bytes RSRC: 5142 bytes
      File: Bouncing Ball (ADgm/ADrk) DATA: 0 bytes RSRC: 3963 bytes
...
```

Even in this crude form, it is somewhat useful (ie: ```./retroscope large_image.dsk | grep APPL/``` will give youall the apps on the disk).

## Testing

sample.dsk contains a sample hfs filesystem.

