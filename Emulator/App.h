#pragma once

#include "amiga/screen_buffer.h"

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
	class CCDebugger;
	class MemoryEditor;
	class DiskManager;

	struct JoystickState
	{
		int x;
		int y;
		bool button[12];
	};

	class AmigaApp
	{
	public:
		explicit AmigaApp(const std::string& resDir, const std::string& romFile);
		~AmigaApp();

		bool IsRunning() const { return m_isRunning; }
		void SetRunning(bool running);

		const am::ScreenBuffer* GetScreen() const;

		bool Update();
		void Render();

		bool UseCrtEmulation() const
		{
			return m_useCrtEmulation;
		}

		void SetKey(int key, int action, int mods);
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

	private:
		std::vector<uint8_t> LoadRom(const std::string& romFile) const;

	private:
		bool m_isQuitting = false;
		bool m_useCrtEmulation = true;
		bool m_debuggerOpen = false;
		bool m_ccDebuggerOpen = false;
		bool m_isRunning = false;
		bool m_isStarting = false;

		std::chrono::steady_clock::time_point m_last;

		std::unique_ptr<am::Amiga> m_amiga;

		std::unique_ptr<Debugger> m_debugger;
		std::unique_ptr<CCDebugger> m_ccDebugger;
		std::unique_ptr<MemoryEditor> m_memoryEditor;
		std::unique_ptr<DiskManager> m_diskManager;

		enum class InputMode
		{
			EmulatorHasFocus,
			GuiHasFocus,
		};

		InputMode m_inputMode = InputMode::GuiHasFocus;

		JoystickState m_joystickState = {};
		JoystickState m_oldJoystickState = {};
	};
}
