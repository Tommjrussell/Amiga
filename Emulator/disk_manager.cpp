#include "disk_manager.h"

#include "disk_image.h"

#include "amiga/amiga.h"

#include "util/imgui_extras.h"

#include <imgui.h>
#include "3rd Party/imfilebrowser.h"

#include <vector>
#include <string>

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
		std::vector<uint8_t> image;
		std::string name;

		bool ok = LoadDiskImage(m_fileDialog->GetSelected(), image, name);

		m_loadFailed[m_selectedDrive] = !ok;

		if (ok)
		{
			m_loadFailed[m_selectedDrive] = !m_amiga->SetDisk(m_selectedDrive, name, std::move(image));
		}

		m_fileDialog->ClearSelected();
	}

	return true;
}
