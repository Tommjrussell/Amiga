#include "disk_manager.h"

#include "disk_image.h"
#include "App.h"

#include "amiga/amiga.h"

#include "util/imgui_extras.h"

#include <imgui.h>
#include "3rd Party/imfilebrowser.h"

guru::DiskManager::DiskManager(guru::AmigaApp* app)
	: m_app(app)
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
	const auto scale = ImGui::GetFrameHeightWithSpacing();

	ImGui::SetNextWindowSize(ImVec2(40 * scale, 15 * scale), ImGuiCond_FirstUseEver);

	bool open = true;
	bool expanded = ImGui::Begin("DiskManager", &open);

	if (!(open && expanded))
	{
		ImGui::End();
		return open;
	}

	auto amiga = m_app->GetAmiga();

	static const char* driveNameLabels[4] = { "DF0: (internal)", "DF1:", "DF2:", "DF3:" };

	for (int i = 0; i < 4; i++)
	{
		auto& diskName = amiga->GetDiskName(i);

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.4f * scale);

		ImGui::BeginChild(driveNameLabels[i], ImVec2(0, 2.3 * scale), true, 0);
		ImGui::Text(driveNameLabels[i]);

		if (util::ActiveButton("Eject", !diskName.empty()))
		{
			amiga->EjectDisk(i);
		}

		ImGui::SameLine();

		if (util::ActiveButton("Select...", true))
		{
			auto& settings = m_app->GetAppSettings();
			std::filesystem::path adfPath(settings.adfDir);
			if (std::filesystem::exists(adfPath))
			{
				m_fileDialog->SetPwd(adfPath);
			}
			m_fileDialog->Open();

			m_selectedDrive = i;
		}

		ImGui::SameLine();

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
		ImGui::PushItemWidth(-1);
		ImGui::InputText(driveFileId, buffer, 256, ImGuiInputTextFlags_ReadOnly);
		ImGui::PopItemWidth();

		ImGui::EndChild();
		ImGui::PopStyleVar(2);
	}

	ImGui::End();

	m_fileDialog->Display();

	auto LoadDiskFile = [this, amiga](const std::filesystem::path& file, std::string_view archFile)
	{
		std::vector<uint8_t> image;
		std::string name;
		bool ok = LoadDiskImage(file, archFile, image, name);
		m_loadFailed[m_selectedDrive] = !ok;

		if (ok)
		{
			m_loadFailed[m_selectedDrive] = !amiga->SetDisk(m_selectedDrive, file.generic_string(), name, std::move(image));
		}
	};

	if (m_fileDialog->HasSelected())
	{
		m_selectedFile = m_fileDialog->GetSelected();

		if (IsArchive(m_selectedFile))
		{
			m_archiveFiles = ListFilesFromZip(m_selectedFile, ".ADF");
			if (m_archiveFiles.empty())
			{
				// No ADFs in the archive.
				m_loadFailed[m_selectedDrive] = true;
			}
			else if (m_archiveFiles.size() == 1)
			{
				LoadDiskFile(m_selectedFile, m_archiveFiles.front());
			}
			else
			{
				ImGui::OpenPopup("Select File...");
			}
		}
		else
		{
			LoadDiskFile(m_selectedFile, {});
		}

		m_fileDialog->ClearSelected();
	}

	if (ImGui::BeginPopupModal("Select File...", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Choose file from archive:");
		ImGui::PushItemWidth(20 * scale);
		if (ImGui::BeginListBox("##file selection"))
		{
			for (auto& file : m_archiveFiles)
			{
				bool selected = false;
				if (ImGui::Selectable(file.c_str(), &selected))
				{
					LoadDiskFile(m_selectedFile, file);
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndListBox();
		}
		ImGui::PopItemWidth();

		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();

		ImGui::EndPopup();
	}

	return true;
}
