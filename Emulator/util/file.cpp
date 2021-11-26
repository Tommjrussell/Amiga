#include "file.h"

#include <fstream>
#include <string>
#include <sstream>

bool util::LoadBinaryFile(const std::string& filename, std::vector<uint8_t>& contents)
{
	std::ifstream ifile(filename, std::ios::binary | std::ios::ate);
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
	std::ofstream ofile(filename, std::ios::out | std::ios::binary);
	if (!ofile.is_open())
		return false;

	ofile.write(reinterpret_cast<const char*>(contents.data()), contents.size());
	return true;
}

bool util::LoadUtf8File(const std::string& filename, std::string& contents)
{
	std::ifstream filestream(filename, std::ios::in);

	if (!filestream.is_open())
		return false;

	std::stringstream buffer;
	buffer << filestream.rdbuf();

	contents = buffer.str();
	return true;
}
