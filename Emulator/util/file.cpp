#include "file.h"

#include <fstream>
#include <string>
#include <sstream>

// Load the first 'sizeOfHeader' bytes of a file (to check for file type etc..)
bool util::LoadBinaryHeader(const std::string& filename, std::vector<uint8_t>& contents, size_t sizeOfHeader)
{
	std::ifstream ifile(filename, std::ios::binary | std::ios::ate);
	if (!ifile.is_open())
		return false;

	std::streamsize size = ifile.tellg();
	ifile.seekg(0, std::ios::beg);

	if (size < sizeOfHeader)
		return false;

	contents.resize(sizeOfHeader);

	ifile.read((char*)contents.data(), contents.size());
	return true;
}

bool util::LoadBinaryFile(const std::string& filename, std::vector<uint8_t>& contents)
{
	std::ifstream ifile(filename.data(), std::ios::binary | std::ios::ate);
	if (!ifile.is_open())
		return false;

	std::streamsize size = ifile.tellg();
	ifile.seekg(0, std::ios::beg);
	contents.resize(size);
	ifile.read((char*)contents.data(), contents.size());
	return true;
}

bool util::SaveBinaryFile(const std::string& filename, const std::vector<uint8_t>& contents)
{
	std::ofstream ofile(filename.data(), std::ios::out | std::ios::binary);
	if (!ofile.is_open())
		return false;

	ofile.write(reinterpret_cast<const char*>(contents.data()), contents.size());
	return true;
}

bool util::LoadUtf8File(const std::string& filename, std::string& contents)
{
	std::ifstream filestream(filename.data(), std::ios::in);

	if (!filestream.is_open())
		return false;

	std::stringstream buffer;
	buffer << filestream.rdbuf();

	contents = buffer.str();
	return true;
}
