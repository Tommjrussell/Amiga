#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace guru
{
	bool LoadDiskImage(const std::filesystem::path& path, std::string_view archFile, std::vector<uint8_t>& image, std::string& name);
}