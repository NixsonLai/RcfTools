#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <algorithm>

#include "rcf.h"
#include "utils.h"


#define TOPACK_FOLDER_MAME "Pack"

class RcfFile
{
	std::string filePath;
	bool useSlash;

	void LoadHeader();
	void LoadEntries();
	void LoadMetadata();

	std::vector<uint8_t> ReadFile(uint32_t from, uint32_t length);
	RcfEntry ReadEntry(uint32_t baseOffset, std::vector<uint8_t>& rawData);
	RcfMetadata ReadMetadata(uint32_t offset, std::vector<uint8_t>& rawData);
	RcfMetadata* FindMetadata(uint32_t hash);
	void Extract(size_t entryIndex, std::string path);
	void MakeHeader(std::vector<uint8_t>& data);
	void MakeEntry(std::vector<std::string> path);
	size_t PackEntry(size_t index, std::vector<uint8_t>& data);
	size_t PackMetadata(size_t index, std::vector<uint8_t>& data);

	public:
		//Constructors
		RcfFile() {}
		RcfFile(std::string file);
		~RcfFile() {}

		//Variables
		RcfHeader header;
		std::vector<RcfEntry> entryList{};
		std::vector<RcfMetadata> metadataList{};

		//Methods
		void Deserialize();
		void Serialize(std::string filename, bool slashit = false);
};

