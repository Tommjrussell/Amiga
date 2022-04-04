#include "disk_manager.h"


#include "util/strings.h"
#include "util/file.h"
#include "util/imgui_extras.h"
#include "util/scope_guard.h"

#include "amiga/amiga.h"

#include <imgui.h>
#include "3rd Party/imfilebrowser.h"

#include "minizip/unzip.h"

guru::DiskManager::DiskManager(am::Amiga* amiga)
	: m_amiga(amiga)
{
	m_fileDialog = std::make_unique<ImGui::FileBrowser>();

	m_fileDialog->SetTitle("Select Disk Image");
	m_fileDialog->SetTypeFilters({ ".adf", ".adz", ".ADF", ".zip", ".ZIP" });
}

guru::DiskManager::~DiskManager()
{
}

bool guru::DiskManager::Draw()
{
	ImGui::SetNextWindowSize(ImVec2(660, 256), ImGuiCond_FirstUseEver);

	bool open = true;
	bool expanded = ImGui::Begin("DiskManager", &open);

	if (!(open && expanded))
	{
		ImGui::End();
		return open;
	}

	static const char* driveNameLabels[4] = { "DF0: (internal)", "DF1:", "DF2:", "DF3:" };

	for (int i = 0; i < 4; i++)
	{
		auto& diskName = m_amiga->GetDisk(i);

		ImGui::BeginChild(driveNameLabels[i], ImVec2(640, 60), true, 0);
		ImGui::Text(driveNameLabels[i]);

		if (util::ActiveButton("Eject", !diskName.empty()))
		{
			m_amiga->EjectDisk(i);
		}

		ImGui::SameLine();

		if (util::ActiveButton("Select...", true))
		{
			m_fileDialog->Open();
			m_selectedDrive = i;
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(480);
		char buffer[512];
		if (diskName.empty())
		{
			strncpy_s(buffer, "<empty>", 8);
		}
		else if (m_loadFailed[i])
		{
			strncpy_s(buffer, "<Load Error!>", 15);
		}
		else
		{
			strncpy_s(buffer, diskName.c_str(), diskName.length() + 1);
		}

		char driveFileId[10];
		sprintf_s(driveFileId, "##DRIVE%02d", i);
		ImGui::InputText(driveFileId, buffer, 256, ImGuiInputTextFlags_ReadOnly);
		ImGui::PopItemWidth();

		ImGui::EndChild();
	}

	ImGui::End();

	m_fileDialog->Display();

	if (m_fileDialog->HasSelected())
	{
		auto path = m_fileDialog->GetSelected();
		auto name = path.filename().generic_u8string();

		std::vector<uint8_t> image;

		bool isGood = false;

		if (util::ToUpper(path.extension().string()) == ".ZIP")
		{
			std::string zippedFile;
			isGood = LoadZippedImage(path.string(), image, zippedFile);

			name += "::";
			name += zippedFile;
		}
		else
		{
			isGood = util::LoadBinaryFile(path.string(), image);
		}

		m_loadFailed[m_selectedDrive] = !isGood;

		if (isGood)
		{
			m_loadFailed[m_selectedDrive] = !m_amiga->SetDisk(m_selectedDrive, name, std::move(image));
		}

		m_fileDialog->ClearSelected();
	}

	return true;
}

bool guru::DiskManager::LoadZippedImage(const std::string& zipFilename, std::vector<uint8_t>& image, std::string& adfName)
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
		if (util::EndsWith(util::ToUpper(file), ".ADF"))
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