#pragma once

#include <memory>
#include <vector>
#include <string>
#include <filesystem>

namespace ImGui
{
	class FileBrowser;
}

namespace guru
{
	class AmigaApp;

	class DiskManager
	{
	public:
		explicit DiskManager(guru::AmigaApp* app);
		~DiskManager();

		bool Draw();

	private:
		guru::AmigaApp* m_app;
		std::unique_ptr<ImGui::FileBrowser> m_fileDialog;

		bool m_loadFailed[4] = {};
		int m_selectedDrive = 0;
		std::filesystem::path m_selectedFile;
		std::vector<std::string> m_archiveFiles;
	};
}