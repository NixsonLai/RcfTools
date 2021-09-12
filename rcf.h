#pragma once
#include <cstdint>
#include <vector>
#include <string>

#define HEADER_LENGTH 60

enum class RcfHeaderOffsets {
	Name = 0x0,
	Version = 0x20,
	EndianFlag = 0x22,
	LibraryValid = 0x23,
	EntryOffset = 0x24,
	EntryLength = 0x28,
	MetadataOffset = 0x2C,
	MetadataLength = 0x30,
	EmptySpace = 0x34,
	NumberOfFiles = 0x38,
};

struct RcfHeader {
	std::string name;
	uint8_t version[2];
	bool big_endian;
	bool libraryValid;
	uint32_t entryOffset;
	uint32_t entryLength;
	uint32_t metadataOffset;
	uint32_t metadataLength;
	uint8_t empty[4] = { 0, 0, 0, 0 };
	uint32_t numberOfFiles;
};

enum class RcfMetadataOffsets {
	FolderPadding = 0x8,
	Date = 0x0,
	Padding = 0x4,
	FilenameLength = 0xC,
	Filename = 0x10
};

struct RcfMetadata {
	time_t date;
	uint8_t padding[4] = { 0, 8, 0, 0 };;
	uint8_t empty[4] = { 0, 0, 0, 0 };
	uint32_t filenameLength;
	std::string filename;
};

enum class RcfEntryOffsets {
	Hash = 0x0,
	Offset = 0x4,
	Length = 0x8
};

struct RcfEntry {
	uint32_t hash;
	uint32_t dataOffset;
	uint32_t dataLength;
	std::vector<uint8_t> data;
	RcfMetadata* metadata;
};