#include "rcf_file.h"

RcfFile::RcfFile(std::string file) : filePath(file)
{
	LoadHeader();
	LoadMetadata();
	LoadEntries();
}

std::vector<uint8_t> RcfFile::ReadFile(uint32_t from, uint32_t length)
{
	std::vector<uint8_t> buffer(length);
	std::ifstream file(filePath, std::ifstream::binary);
	file.seekg(from);
	file.read((char*)buffer.data(), length);

	file.close();

	return buffer;
}

void RcfFile::LoadHeader()
{
	std::vector<uint8_t> rawData = ReadFile(0, HEADER_LENGTH);

	header = RcfHeader();	
	uint8_t offset;

	//Name
	offset = EnumToValue(RcfHeaderOffsets::Name);
	for (uint8_t i = 0; i < 32; i++) {
		header.name.push_back(rawData[offset + i]);
	}

	//Version
	offset = EnumToValue(RcfHeaderOffsets::Version);
	for (uint8_t i = 0; i < 2; i++) {
		header.version[i] = rawData[EnumToValue(RcfHeaderOffsets::Version) + i];
	}

	//Endianess
	offset = EnumToValue(RcfHeaderOffsets::EndianFlag);
	header.big_endian = rawData[offset];

	//Is library valid?
	offset = EnumToValue(RcfHeaderOffsets::LibraryValid);
	header.libraryValid = rawData[offset];

	//Entries Offset
	offset = EnumToValue(RcfHeaderOffsets::EntryOffset);
	header.entryOffset = BytesToValue(4, offset, rawData.data());

	//Entries Length
	offset = EnumToValue(RcfHeaderOffsets::EntryLength);
	header.entryLength = BytesToValue(4, offset, rawData.data());

	//Filename Offset
	offset = EnumToValue(RcfHeaderOffsets::MetadataOffset);
	header.metadataOffset = BytesToValue(4, offset, rawData.data());

	//Filename Length
	offset = EnumToValue(RcfHeaderOffsets::MetadataLength);
	header.metadataLength = BytesToValue(4, offset, rawData.data());

	//Number of files
	offset = EnumToValue(RcfHeaderOffsets::NumberOfFiles);
	header.numberOfFiles = BytesToValue(4, offset, rawData.data());
}

void RcfFile::LoadEntries()
{
	if (header.numberOfFiles <= 0)
		return;

	std::vector<uint8_t> rawData = ReadFile(header.entryOffset, header.entryLength);

	RcfEntry entry;
	for (int i = 0; i < header.numberOfFiles; i++) {
		//We multiply by 12 because thats the size of an entry block: Hash (4 bytes) + Offset (4 bytes) + Length (4 bytes) = 12 bytes 
		entry = ReadEntry(12 * i, rawData);
		entryList.push_back(entry);
	}
}

RcfEntry RcfFile::ReadEntry(uint32_t baseOffset, std::vector<uint8_t>& rawData)
{
	RcfEntry entry = RcfEntry();
	uint8_t offset;

	//Hash
	offset = EnumToValue(RcfEntryOffsets::Hash);
	entry.hash = BytesToValue(4, baseOffset + offset, rawData.data());

	//Offset
	offset = EnumToValue(RcfEntryOffsets::Offset);
	entry.dataOffset = BytesToValue(4, baseOffset + offset, rawData.data());

	//Length
	offset = EnumToValue(RcfEntryOffsets::Length);
	entry.dataLength = BytesToValue(4, baseOffset + offset, rawData.data());

	//Data
	std::vector<uint8_t> data = ReadFile(entry.dataOffset, entry.dataLength);
	for (size_t i = 0; i < entry.dataLength; i++) {
		entry.data.push_back(data[i]);
	}

	//Metadata ref
	entry.metadata = FindMetadata(entry.hash);

	return entry;
}

void RcfFile::LoadMetadata()
{
	if (header.numberOfFiles <= 0)
		return;

	std::vector<uint8_t> rawData = ReadFile(header.metadataOffset, header.metadataLength);

	RcfMetadata metadata = ReadMetadata(EnumToValue(RcfMetadataOffsets::FolderPadding), rawData);
	metadataList.push_back(metadata);

	uint32_t offset = EnumToValue(RcfMetadataOffsets::FolderPadding) + EnumToValue(RcfMetadataOffsets::Filename) + metadata.filenameLength + 3;
	for (int i = 1; i < header.numberOfFiles; i++) {		
		metadata = ReadMetadata(offset, rawData);
		metadataList.push_back(metadata);
		offset += EnumToValue(RcfMetadataOffsets::Filename) + metadata.filenameLength + 3;
	}
}


RcfMetadata RcfFile::ReadMetadata(uint32_t baseOffset, std::vector<uint8_t>& rawData)
{

	RcfMetadata metadata = RcfMetadata();
	uint8_t offset;

	//Date
	offset = EnumToValue(RcfMetadataOffsets::Date);
	metadata.date = (time_t)BytesToValue(4, baseOffset + offset, rawData.data());

	//Padding
	offset = EnumToValue(RcfMetadataOffsets::Padding);
	for (uint8_t i = 0; i < 4; i++) {
		metadata.padding[i] = rawData[baseOffset + offset + i];
	}

	//Filename length
	offset = EnumToValue(RcfMetadataOffsets::FilenameLength);
	metadata.filenameLength = BytesToValue(4, baseOffset + offset, rawData.data());

	//Filename
	offset = EnumToValue(RcfMetadataOffsets::Filename);
	for (uint8_t i = 0; i < metadata.filenameLength; i++) {
		metadata.filename.push_back(rawData[baseOffset + offset + i]);
	}

	return metadata;
}

RcfMetadata* RcfFile::FindMetadata(uint32_t hash)
{
	for (size_t i = 0; i < metadataList.size(); i++) {
		RcfMetadata* p = &metadataList[i];
		uint32_t hashedStr = HashString(p->filename.c_str());
		if (hashedStr == hash)
			return p;
	}

	return nullptr;
}

void RcfFile::Extract(size_t entryIndex, std::string path)
{
	RcfEntry* entry = &entryList[entryIndex];
	RcfMetadata* metadata = entry->metadata;
	
	path += metadata->filename;
	std::vector<std::string> paths = SplitStringPath(path, "\\");
	size_t pos = path.find(paths.back());
	std::string dest = path.substr(0, pos);

	std::filesystem::create_directories(dest);

	std::ofstream os(path, std::ios::out | std::ios::binary);
	for (size_t i = 0; i < entry->dataLength; i++) {
		os.write((char*)&entry->data[i], sizeof(uint8_t));
	}
	os.close();

	if (!os.good()) {
		std::cout << "Error occurred at writing time!" << std::endl;
	}
}

void RcfFile::Deserialize()
{
	std::cout << "Unpacking..." << std::endl;
	for (size_t i = 0; i < entryList.size(); i++) {
		Extract(i, ".\\extracted\\");
	}

	std::cout << "Unpacking done! " << entryList.size() << " files has been unnpacked." << std::endl;
}

void RcfFile::MakeHeader(std::vector<uint8_t>& data)
{
	//Name
	std::string name = "ATG CORE CEMENT LIBRARY";
	for (size_t i = 0; i < 32; i++) {
		if (name.length() > i)
			data.push_back(name[i]);
		else
			data.push_back(0);
	}

	data.push_back(2);	//Major version
	data.push_back(1);	//Minor version
	data.push_back(0);	//Endianess (Little)
	data.push_back(1);	//idk but it crashs if it is not 1.

	//Entry offset. Always is 3C.
	std::vector<uint8_t> arr = ValueToArray(4, 0x3C);
	data.insert(std::end(data), std::begin(arr), std::end(arr));

	//Entry length. 0 for now
	arr = ValueToArray(4, 0x0);
	data.insert(std::end(data), std::begin(arr), std::end(arr));

	//Metadata offset. 0 for now
	data.insert(std::end(data), std::begin(arr), std::end(arr));

	//Metadata length. 0 for now
	data.insert(std::end(data), std::begin(arr), std::end(arr));

	//Null padding. Always 0
	data.insert(std::end(data), std::begin(arr), std::end(arr));

	//Number of files. 0 for now
	data.insert(std::end(data), std::begin(arr), std::end(arr));
}

void RcfFile::MakeEntry(std::vector<std::string> path)
{
	if (useSlash)
		path[1] = "\\" + path[1];

	//Metadata
	RcfMetadata metadata = RcfMetadata();
	metadata.date = (time_t)std::time(nullptr);
	metadata.filenameLength = path[1].length();
	metadata.filename = path[1];
	metadataList.push_back(metadata);

	//File
	std::ifstream stream(path[0], std::ifstream::binary);
	std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
	stream.close();

	//Entry
	RcfEntry entry = RcfEntry();
	entry.hash = HashString(path[1].c_str());
	entry.dataLength = buffer.size();
	entry.dataOffset = 0;
	entry.data = buffer;
	entry.metadata = &metadataList.back();
	entryList.push_back(entry);

}

size_t RcfFile::PackEntry(size_t index, std::vector<uint8_t>& data)
{
	std::vector<uint8_t> arr = ValueToArray(4, entryList[index].hash);
	for (size_t i = 0; i < arr.size(); i++)
		data.push_back(arr[i]);

	for (size_t i = 0; i < arr.size(); i++)
		data.push_back(0);

	arr = ValueToArray(4, entryList[index].dataLength);
	for (size_t i = 0; i < arr.size(); i++)
		data.push_back(arr[i]);

	return 0;
}

size_t RcfFile::PackMetadata(size_t index, std::vector<uint8_t>& data)
{
	size_t size = 0;

	size += 4;
	std::vector<uint8_t> arr = ValueToArray(4, metadataList[index].date);
	for (size_t i = 0; i < arr.size(); i++)
		data.push_back(arr[i]);

	size += 4;
	for (size_t i = 0; i <4; i++)
		data.push_back(metadataList[index].padding[i]);

	size += 4;
	for (size_t i = 0; i < 4; i++)
		data.push_back(metadataList[index].empty[i]);

	size += 4;
	arr = ValueToArray(4, metadataList[index].filenameLength + 1);
	for (size_t i = 0; i < arr.size(); i++)
		data.push_back(arr[i]);

	size += metadataList[index].filename.length() + 1;
	for (size_t i = 0; i < metadataList[index].filename.length(); i++)
		data.push_back(metadataList[index].filename[i]);
	data.push_back(0);

	//Inbetween item pad
	size += 3;
	for (size_t i = 0; i < 3; i++)
		data.push_back(0);

	return size;
}

bool CompareByHash(RcfEntry entry1, RcfEntry entry2)
{
	return (entry1.hash < entry2.hash);
}

void RcfFile::Serialize(std::string filename, bool slashit)
{
	useSlash = slashit;
	std::cout << "Packing..." << std::endl;

	std::vector<uint8_t> binaryFile{};
	MakeHeader(binaryFile);

	std::string path = std::filesystem::current_path().string() + "\\" + TOPACK_FOLDER_MAME;
	
	size_t numOfFiles = 0;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
		if (std::filesystem::is_regular_file(entry)) {
			std::vector<std::string> paths{};
			paths.push_back(entry.path().string());

			std::string shortStr = entry.path().string();
			shortStr.erase(0, path.length() + 1);
			paths.push_back(shortStr);

			++numOfFiles;
			MakeEntry(paths);
		}
	}

	std::sort(entryList.begin(), entryList.end(), CompareByHash);

	//Set the number of files
	std::vector<uint8_t> arr = ValueToArray(4, numOfFiles);
	for (size_t i = 0; i < 4; i++)
		binaryFile[EnumToValue(RcfHeaderOffsets::NumberOfFiles) + i] = arr[i];

	//Pack entries
	for (size_t i = 0; i < entryList.size(); i++) {
		PackEntry(i, binaryFile);
	}

	//Adds padding
	while(binaryFile.size() % 2048)
		binaryFile.push_back(0);

	arr = ValueToArray(4, binaryFile.size());
	for (size_t i = 0; i < arr.size(); i++)
		binaryFile[EnumToValue(RcfHeaderOffsets::MetadataOffset) + i] = arr[i];

	arr = { 0, 8, 0, 0 };
	for (size_t i = 0; i < 4; i++)
		binaryFile.push_back(arr[i]);

	arr = { 0, 0, 0, 0 };
	for (size_t i = 0; i < 4; i++)
		binaryFile.push_back(arr[i]);

	//Pack metadata

	size_t size = 8;
	for (size_t i = 0; i < metadataList.size(); i++) {
		size += PackMetadata(i, binaryFile);
	}

	arr = ValueToArray(4, size);
	for (size_t i = 0; i < arr.size(); i++)
		binaryFile[EnumToValue(RcfHeaderOffsets::MetadataLength) + i] = arr[i];

	//Adds padding
	while (binaryFile.size() % 2048)
		binaryFile.push_back(0);

	//Pack files
	arr = ValueToArray(4, entryList.size() * 12);
	for (size_t i = 0; i < 4; i++)
		binaryFile[EnumToValue(RcfHeaderOffsets::EntryLength) + i] = arr[i];

	for (size_t i = 0; i < entryList.size(); i++) {
		size_t a = 0x3C + 4 + (12 * i);

		//Sew the reference
		arr = ValueToArray(4, binaryFile.size());
		for (size_t j = 0; j < arr.size(); j++) {
			binaryFile[a + j] = arr[j];
		}

		//Write the file data
		for (size_t j = 0; j < entryList[i].data.size(); j++) {
			binaryFile.push_back(entryList[i].data[j]);
		}

		//Add the padding
		if(i < entryList.size() - 1)
			while (binaryFile.size() % 2048)
				binaryFile.push_back(0);
	}

	std::ofstream os(filename, std::ios::out | std::ios::binary);
	os.write(reinterpret_cast<const char*>(&binaryFile[0]), binaryFile.size() * sizeof(uint8_t));


	std::cout << "Packing done! " << numOfFiles << " files has been packed." << std::endl;
}


