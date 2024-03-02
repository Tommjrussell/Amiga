#pragma once

#include "amiga/screen_buffer.h"
#include "amiga/audio.h"

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <filesystem>

namespace util
{
	enum class Key : int;
}

namespace am
{
	class Amiga;
	class Symbols;
}

namespace guru
{
	class Debugger;
	class CCDebugger;
	class VariableWatch;
	class MemoryEditor;
	class DiskManager;
	class LogViewer;
	class DiskActivity;

	struct JoystickState
	{
		int x;
		int y;
		uint32_t buttons;
	};

	class AmigaApp
	{
	public:
		explicit AmigaApp(const std::filesystem::path& programDir, const std::filesystem::path& configDir);
		~AmigaApp();

		void Shutdown();

		bool IsRunning() const { return m_isRunning; }
		void SetRunning(bool running);

		const am::ScreenBuffer* GetScreen() const;

		bool Update();
		void Render(int displayWidth, int displayHeight);

		bool UseCrtEmulation() const
		{
			return m_useCrtEmulation;
		}

		bool SetKey(int key, int action, int mods);
		void SetMouseButton(int button, int action, int mods);
		void SetMouseMove(double xMove, double yMove);

		void SetJoystickState(const JoystickState& state)
		{
			m_joystickState = state;
		}

		bool AmigaHasFocus() const
		{
			return m_inputMode == InputMode::EmulatorHasFocus;
		}

		bool SetDiskImage(int drive, std::string& pathToImage);

		void SetAudioPlayer(am::AudioPlayer* player);

		void SetSymbolsFile(const std::string& symbolsFile);

		void ShowMemInMemoryEditor(uint32_t addr, uint32_t size);

		const std::filesystem::path& GetProgramDir() const
		{
			return m_programDir;
		}

		static std::filesystem::path GetLocalAppDir();
		static std::filesystem::path GetOrCreateLocalAppDir();

	private:
		std::vector<uint8_t> LoadRom(const std::string& romFile) const;
		void ConvertAndSendKeyCode(util::Key key, bool down);

		void LoadSnapshot(const std::filesystem::path& file);
		void SaveSnapshot(const std::filesystem::path& file);

		void OpenMemoryEditor();

		void LoadSettings();
		void SaveSettings();

	private:
		std::filesystem::path m_programDir;
		std::filesystem::path m_configDir;

		bool m_isQuitting = false;
		bool m_useCrtEmulation = true;
		bool m_joystickEmulation = false;
		bool m_debuggerOpen = false;
		bool m_ccDebuggerOpen = false;
		bool m_variableWatchOpen = false;
		bool m_isRunning = false;
		bool m_isStarting = false;

		std::chrono::steady_clock::time_point m_last;

		std::unique_ptr<am::Amiga> m_amiga;

		std::unique_ptr<Debugger> m_debugger;
		std::unique_ptr<CCDebugger> m_ccDebugger;
		std::unique_ptr<VariableWatch> m_variableWatch;
		std::unique_ptr<MemoryEditor> m_memoryEditor;
		std::unique_ptr<DiskManager> m_diskManager;
		std::unique_ptr<DiskActivity> m_diskActivity;
		std::unique_ptr<LogViewer> m_logViewer;

		std::unique_ptr<am::Symbols> m_symbols;

		enum class InputMode
		{
			EmulatorHasFocus,
			GuiHasFocus,
		};

		InputMode m_inputMode = InputMode::GuiHasFocus;

		JoystickState m_joystickState = {};
		JoystickState m_oldJoystickState = {};
		JoystickState m_emulatedJoystickState = {};

		std::map<util::Key, uint8_t> m_keyMap;

		std::string m_romFile;
	};
}
