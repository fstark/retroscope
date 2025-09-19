#include "vdiskcat.h"

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

// block_t implementations
btree_header_node_t block_t::as_btree_node()
{
	return btree_header_node_t();
}

master_directory_block_t block_t::as_master_directory_block() 
{ 
	return master_directory_block_t(*this); 
}

// master_directory_block_t implementations
master_directory_block_t::master_directory_block_t(block_t &block) : block_(block)
{
	content = reinterpret_cast<HFSMasterDirectoryBlock *>(block_.data());
}

bool master_directory_block_t::isHFSVolume() const
{
	return be16toh_custom(content->drSigWord) == 0x4244;
}

std::string master_directory_block_t::getVolumeName() const
{
	return pascalToString(content->drVN);
}

uint32_t master_directory_block_t::allocationBlockSize() const
{	
	return be32toh_custom(content->drAlBlkSiz);
}

uint16_t master_directory_block_t::allocationBlockStart() const
{
	return be16toh_custom(content->drAlBlSt);
}

uint16_t master_directory_block_t::extendsExtendStart(int index) const
{
	if (index < 0 || index >= 3)
		throw std::out_of_range("Extent index out of range");
	return be16toh_custom(content->drXTExtRec[index][0]);
}

uint16_t master_directory_block_t::extendsExtendCount(int index) const
{
	if (index < 0 || index >= 3)
		throw std::out_of_range("Extent index out of range");
	return be16toh_custom(content->drXTExtRec[index][1]);
}

uint32_t master_directory_block_t::catalogExtendStart(int index) const
{
	if (index < 0 || index >= 3)
		throw std::out_of_range("Extent index out of range");
	return be16toh_custom(content->drCTExtRec[index][0]);
}

uint16_t master_directory_block_t::catalogExtendCount(int index) const
{
	if (index < 0 || index >= 3)
		throw std::out_of_range("Extent index out of range");
	return be16toh_custom(content->drCTExtRec[index][1]);
}

// file_t implementations
void file_t::add_extent(uint16_t start, uint16_t count)
{
	extents_.push_back({start, count});
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

btree_file_t file_t::as_btree_file() 
{ 
	return btree_file_t(*this); 
}

// btree_file_t implementations
btree_file_t::btree_file_t(file_t &file) : file_(file)
{
	auto start = file_.to_absolute_block(0);
	std::cout << "Btree file starts at absolute block: " << start+file_.partition().allocation_start() << std::endl;
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

void partition_t::dumpextentTree()
{
    std::cout << "\n\nDumping Catalog Extent Tree:\n";
    // for (size_t i = 0; i < ca.size(); i++) {
    // 	std::cout << "Extent " << i << ": Start Block = " << catalogExtents_[i].start
    // 	          << ", Block Count = " << catalogExtents_[i].count << std::endl;
    // }
}

void partition_t::readCatalogRoot(uint16_t rootNode)
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
    const uint8_t *recordOffsets = block.data() + allocationBlockSize_ - (numRecords + 2) * sizeof(uint16_t);

    for (int i = 0; i < numRecords; i++)
    {
        uint16_t offset = be16toh_custom(*reinterpret_cast<const uint16_t *>(recordOffsets + i * sizeof(uint16_t)));
        if (offset >= allocationBlockSize_)
        {
            std::cerr << "Invalid record offset\n";
            continue;
        }

        const uint8_t *record = block.data() + offset;

        // Read key length
        uint16_t keyLength = be16toh_custom(*reinterpret_cast<const uint16_t *>(record));
        if (keyLength + sizeof(uint16_t) > allocationBlockSize_ - offset)
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

void partition_t::readCatalogHeader(uint64_t catalogExtendStartBlock)
{
    std::cout << "\n\nReading Catalog B-tree header at block " << catalogExtendStartBlock << std::endl;

    auto block = readBlock(catalogExtendStartBlock);

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
        // uint16_t signature = be16toh_custom(mdb.drSigWord);
        // std::cout << "SIGNATURE: 0x" << std::hex << signature << std::dec << " (\"BD\")" << std::endl;

        // // Number of files in root dir
        // uint16_t filesInRoot = be16toh_custom(mdb.drNmFls);
        // std::cout << "# of files in root dir: " << filesInRoot << std::endl;

        // // Number of directories in root dir
        // uint16_t dirsInRoot = be16toh_custom(mdb.drNmRtDirs);
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
                extents_.add_extent(startBlock, blockCount);
            }
        }

		btree_file_t extents_btree = extents_.as_btree_file();

        // //	Catalog size (prob useless)
        // uint16_t catalogSize = be32toh_custom(mdb.drCTFlSize);
        // std::cout << "Catalog file size (drCTFlSize): " << catalogSize << " bytes" << std::endl;

        // // Catalog file extents (first node of catalog btree)
        std::cout << "drCTExtRec (catalog extents):\n";
        for (int i = 0; i < 3; i++)
        {
            auto startBlock = mdb.catalogExtendStart(i);
            auto blockCount = mdb.catalogExtendCount(i);
            if (blockCount)
            {
                std::cout << "  [" << i << "]=" << startBlock << "-" << blockCount << "\n";
                catalog_.add_extent(startBlock, blockCount);
            }
        }
        std::cout << std::endl;

        std::cout << "===================================" << std::endl;

		//	We now read the extend btree for all file extends information
		auto b = read(4);
		b.dump();
		
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