#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace am
{
	class Amiga;
}

namespace guru
{
	class Debugger;
	class MemoryEditor;

	class AmigaApp
	{
	public:
		explicit AmigaApp(const std::string& resDir, const std::string& romFile);
		~AmigaApp();

		bool IsRunning() const { return m_isRunning; }
		void SetRunning(bool running);

		bool Update();
		void Render();

		bool UseCrtEmulation() const { return m_useCrtEmulation; }

	private:
		std::vector<uint8_t> LoadRom(const std::string& romFile) const;

	private:
		bool m_isQuitting = false;
		bool m_useCrtEmulation = true;
		bool m_debuggerOpen = false;
		bool m_isRunning = false;
		bool m_isStarting = false;

		std::chrono::steady_clock::time_point m_last;

		std::unique_ptr<am::Amiga> m_amiga;

		std::unique_ptr<Debugger> m_debugger;
		std::unique_ptr<MemoryEditor> m_memoryEditor;
	};
}
