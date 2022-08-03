#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace guru
{
	bool IsArchive(const std::filesystem::path& path);
	std::vector<std::string> ListFilesFromZip(const std::filesystem::path& zipPath, std::string_view extension);
	bool LoadDiskImage(const std::filesystem::path& path, std::string_view archFile, std::vector<uint8_t>& image, std::string& name);
}