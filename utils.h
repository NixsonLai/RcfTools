#pragma once
#include <vector>
#include <bitset>

template<typename T>
auto EnumToValue(T value)
{
	return static_cast<typename std::underlying_type<T>::type>(value);
}

inline uint32_t ArrayToValue(uint16_t length, uint8_t* buffer)
{
	if (length <= 0)
		return 0;

	uint32_t total = buffer[0];
	for (int i = 1; i < length; i++) {
		int a = static_cast<uint32_t>(buffer[i]) << (8 * i);
		total = total | a;
	}

	return total;
}


inline std::vector<uint8_t> ValueToArray(uint8_t length, uint32_t value)
{
	std::vector<uint8_t> buffer{};
	if (length <= 0)
		return {};

	buffer.push_back(value & 0xFF);
	for (size_t i = 1; i < 4; i++) {
		int a = (value >> (8 * i)) & 0xFF;
		buffer.push_back(a);
	}

	return buffer;
}

inline uint32_t HashString(const char* string)
{
	uint32_t res;
	for (res = 0; *string; string++) {
		if (res == 0 && *string == '\\')
			continue;

		uint32_t c = (uint32_t)(*string < 'a') ? (*string + 'a' - 'A') : (*string);
		res = (res << 5) - res + c;
	}

	return res;
}


inline uint32_t BytesToValue(size_t length, uint32_t offset, uint8_t* rawData)
{
	std::vector<uint8_t> placeholder;
	for (uint8_t i = 0; i < length; i++) {
		placeholder.push_back(rawData[offset + i]);
	}
	
	return ArrayToValue(4, placeholder.data());
}

inline std::vector<std::string> SplitStringPath(std::string path, std::string delimiter)
{
	std::vector<std::string> paths{};

	size_t pos = 0;
	while ((pos = path.find(delimiter)) != std::string::npos) {
		std::string token = path.substr(0, pos);
		paths.push_back(token);
		path.erase(0, pos + delimiter.length());
	}

	paths.push_back(path);

	return paths;
}

