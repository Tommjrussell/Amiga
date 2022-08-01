#include "disk_image.h"

#include "util/file.h"
#include "util/scope_guard.h"
#include "util/strings.h"

#include "minizip/unzip.h"

namespace
{
	bool LoadZippedImage(const std::string& zipFilename, std::string_view archFile, std::vector<uint8_t>& image, std::string& adfName)
	{
		unzFile zipfile = unzOpen(zipFilename.c_str());
		if (!zipfile)
			return false;

		auto zipguard = util::make_scope_guard([&]()
			{
				unzClose(zipfile);
			});

		if (unzGoToFirstFile(zipfile) != UNZ_OK)
			return false;

		char filename[512];

		for (;;)
		{
			unz_file_info file_info;
			if (unzGetCurrentFileInfo(
				zipfile,
				&file_info,
				filename,
				_countof(filename),
				NULL, 0, NULL, 0) != UNZ_OK)
			{
				return false;
			}

			std::string file = filename;

			bool match = false;

			if (archFile.empty())
			{
				match = util::EndsWith(util::ToUpper(file), ".ADF");
			}
			else
			{
				match = (archFile == file);
			}

			if (match)
			{
				if (unzOpenCurrentFile(zipfile) != UNZ_OK)
					return false;

				auto fileguard = util::make_scope_guard([&]()
					{
						unzCloseCurrentFile(zipfile);
					});

				image.resize(file_info.uncompressed_size);

				const auto numBytes = unzReadCurrentFile(zipfile, image.data(), file_info.uncompressed_size);

				if (numBytes != file_info.uncompressed_size)
					return false;

				adfName = file;
				break;
			}

			if (unzGoToNextFile(zipfile) != UNZ_OK)
				break;
		}

		return true;
	}

} // namespace

bool guru::LoadDiskImage(const std::filesystem::path& path, std::string_view archFile, std::vector<uint8_t>& image, std::string& name)
{
	name = path.filename().generic_u8string();

	bool isGood = false;

	if (util::ToUpper(path.extension().string()) == ".ZIP")
	{
		std::string zippedFile;
		isGood = LoadZippedImage(path.string(), archFile, image, zippedFile);

		name += "::";
		name += zippedFile;
	}
	else
	{
		isGood = util::LoadBinaryFile(path.string(), image);
	}

	return isGood;
}