#pragma once

#include <memory>

namespace ImGui
{
	class FileBrowser;
}

namespace am
{
	class Amiga;
}

namespace guru
{
	class DiskManager
	{
	public:
		explicit DiskManager(am::Amiga* amiga);
		~DiskManager();

		bool Draw();

	private:
		am::Amiga* m_amiga;
		std::unique_ptr<ImGui::FileBrowser> m_fileDialog;

		bool m_loadFailed[4] = {};
		int m_selectedDrive = 0;
	};
}