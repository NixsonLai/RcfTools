#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <cxxopts.hpp>

#include "rcf_file.h"


std::vector<uint8_t> ReadFile(std::string path)
{
	std::ifstream file(path, std::ifstream::binary);
	std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
	file.close();

	return buffer;
}

void Unpack(std::string path)
{
	std::cout << "..." << std::endl;
	RcfFile rcf = RcfFile(path);
	rcf.Deserialize();
}

void Pack(std::string filename)
{
	std::cout << "..." << std::endl;
	RcfFile rcf = RcfFile();
	rcf.Serialize(filename);
}


int main(int argc, char* argv[]) 
{
	#ifdef _DEBUG
		std::cout << "Press enter to start" << std::endl;
		std::cin.get();
	#endif

	cxxopts::Options options("RcfTools", "Prototype's .rcf file packer and unpacker.");
	options.add_options()
		("u,unpack", "Unpacks a rcf file", cxxopts::value<std::string>())
		("p,pack", "Packs files into a rcf file", cxxopts::value<std::string>())
		("h,help", "Prints help");
	
	auto result = options.parse(argc, argv);
	if (argc > 2) {
		if (result.count("unpack") > 0) {
			std::string file = result["unpack"].as<std::string>();
			Unpack(file);
		}
		else if (result.count("pack")) {
			std::string args = result["pack"].as<std::string>();
			Pack(args);
		}
		else {
			std::cout << "No valid arguments were given." << std::endl;
		}
	}
	else if (argc == 2) {
		std::string path(argv[1]);
		size_t pos = path.find(".rcf");
		if (pos != std::string::npos)
			Unpack(path);
		else if (result.count("help"))
			std::cout << options.help() << std::endl;
		else
			std::cout << "No valid arguments were given." << std::endl;
	}
	else {
		std::cout << "No arguments were given." << std::endl;
	}

	return 0;
}

