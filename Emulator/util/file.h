#pragma once

#include <string>
#include <vector>
#include <exception>

namespace util
{
	/// Loads contents of a binary file into a vector.
	bool LoadBinaryFile(const std::string& filename, std::vector<uint8_t>& contents);
	bool SaveBinaryFile(const std::string& filename, const std::vector<uint8_t>& contents);
	bool LoadBinaryHeader(const std::string& filename, std::vector<uint8_t>& contents, size_t sizeOfHeader);

	bool LoadUtf8File(const std::string& filename, std::string& contents);
}