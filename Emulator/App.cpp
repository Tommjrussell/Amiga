#include "App.h"

#include "amiga/amiga.h"
#include "debugger.h"
#include "custom_chips_debugger.h"
#include "memory_editor.h"
#include "disk_manager.h"

#include "util/file.h"

#include <imgui.h>
#include <memory>
#include <filesystem>

namespace
{
	constexpr int PAL_CClockFreq = 3546895;
	constexpr int NTSC_CClockFreq = 3579545;

	// Replicate some GLFW input constants here...
	constexpr int GLFW_RELEASE = 0;
	constexpr int GLFW_PRESS = 1;
	constexpr int GLFW_REPEAT = 2;

	constexpr int GLFW_MOD_SHIFT = 0x0001;
	constexpr int GLFW_MOD_CONTROL = 0x0002;
	constexpr int GLFW_MOD_ALT = 0x0004;
	constexpr int GLFW_MOD_SUPER = 0x0008;
	constexpr int GLFW_MOD_CAPS_LOCK = 0x0010;
	constexpr int GLFW_MOD_NUM_LOCK = 0x0020;

	constexpr int GLFW_KEY_F1 = 290;
	constexpr int GLFW_KEY_F2 = 291;
	constexpr int GLFW_KEY_F3 = 292;
	constexpr int GLFW_KEY_F4 = 293;
	constexpr int GLFW_KEY_F5 = 294;
	constexpr int GLFW_KEY_F6 = 295;
	constexpr int GLFW_KEY_F7 = 296;
	constexpr int GLFW_KEY_F8 = 297;
	constexpr int GLFW_KEY_F9 = 298;
	constexpr int GLFW_KEY_F10 = 299;
	constexpr int GLFW_KEY_F11 = 300;
	constexpr int GLFW_KEY_F12 = 301;
}

guru::AmigaApp::AmigaApp(const std::string& resDir, const std::string& romFile)
{
	std::vector<uint8_t> rom;

	if (!romFile.empty())
	{
		rom = std::move(LoadRom(romFile));
	}
	m_amiga = std::make_unique<am::Amiga>(am::ChipRamConfig::ChipRam1Mib, std::move(rom));

	m_debugger = std::make_unique<Debugger>(this, m_amiga.get());
	m_ccDebugger = std::make_unique<CCDebugger>(this, m_amiga.get());
}

guru::AmigaApp::~AmigaApp()
{
}

void guru::AmigaApp::SetRunning(bool running)
{
	if (m_isRunning == running)
		return;

	m_isStarting = running;
	m_isRunning = running;

	if (running)
	{
		m_debugger->OnStartRunning();
	}
}

bool guru::AmigaApp::Update()
{
	if (m_isStarting)
	{
		m_last = std::chrono::high_resolution_clock::now();
		m_isStarting = false;
	}
	else if (m_isRunning)
	{
		for (int b = 0; b < 3; b++)
		{
			if (m_joystickState.button[b] != m_oldJoystickState.button[b])
			{
				m_amiga->SetControllerButton(1, b, m_joystickState.button[b]);
			}
		}

		m_amiga->SetJoystickMove(m_joystickState.x, m_joystickState.y);

		m_joystickState = m_oldJoystickState;

		auto now = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> diff = now - m_last;
		m_last = now;

		if (diff < std::chrono::duration < double>(0.5))
		{
			const int cclks = int(std::round((diff * PAL_CClockFreq).count()));
			m_isRunning = m_amiga->ExecuteFor(cclks);
		}
	}

	return !m_isQuitting;
}

void guru::AmigaApp::Render()
{
	if (m_inputMode == InputMode::EmulatorHasFocus)
		return;

	//ImGui::ShowDemoWindow();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Main"))
		{
			if (ImGui::MenuItem("Debugger", ""))
			{
				m_debuggerOpen = true;
			}

			if (ImGui::MenuItem("Custom Chips Debugger", ""))
			{
				m_ccDebuggerOpen = true;
			}

			if (ImGui::MenuItem("Memory Viewer/Editor", ""))
			{
				if (!m_memoryEditor)
				{
					m_memoryEditor = std::make_unique<MemoryEditor>(m_amiga.get());
				}
			}

			if (ImGui::MenuItem("Disk Manager", ""))
			{
				if (!m_diskManager)
				{
					m_diskManager = std::make_unique<DiskManager>(m_amiga.get());
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Quit", "ALT+F4"))
			{
				m_isQuitting = true;
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Options"))
		{
			ImGui::MenuItem("CRT Emulation", "", &m_useCrtEmulation);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (m_debuggerOpen)
	{
		m_debuggerOpen = m_debugger->Draw();
	}

	if (m_ccDebuggerOpen)
	{
		m_ccDebuggerOpen = m_ccDebugger->Draw();
	}

	if (m_memoryEditor)
	{
		if (!m_memoryEditor->Draw())
		{
			m_memoryEditor.reset();
		}
	}

	if (m_diskManager)
	{
		if (!m_diskManager->Draw())
		{
			m_diskManager.reset();
		}
	}

};

std::vector<uint8_t> guru::AmigaApp::LoadRom(const std::string& romFile) const
{
	std::filesystem::path file(romFile);
	const auto displayFilename = file.filename().u8string();

	std::vector<uint8_t> rom;
	if (!util::LoadBinaryFile(romFile, rom))
	{
		printf("ERROR: rom '%s' not loaded.\n", displayFilename.c_str());
		return rom;
	}

	static const char cloantoRomSig[] = "AMIROMTYPE1";
	static const size_t cloantoRomSigLen = _countof(cloantoRomSig) - 1;

	if (rom.size() < cloantoRomSigLen
		|| memcmp(rom.data(), cloantoRomSig, cloantoRomSigLen) != 0)
	{
		printf("INFO : Rom file '%s' is unencrypted\n", displayFilename.c_str());
		printf("INFO : Rom size %llu bytes.\n", rom.size());
		return rom;
	}

	// It's a scrambled Cloanto rom file. Locate rom.key file to decrypt

	auto romkeyFile = file.parent_path() / "rom.key";
	std::vector<uint8_t> romkey;

	if (!util::LoadBinaryFile(romkeyFile.u8string(), romkey))
	{
		printf("ERROR: rom '%s' was encypted but rom.key file not available (must be in same directory).\n", displayFilename.c_str());
		return rom;
	}

	if (romkey.empty())
	{
		printf("ERROR: rom '%s' was encypted but rom.key file failed to load or was empty.\n", displayFilename.c_str());
	}

	size_t writePtr = 0;
	size_t readPtr = cloantoRomSigLen;
	size_t keyPtr = 0;

	while (readPtr < rom.size())
	{
		rom[writePtr++] = rom[readPtr++] ^ romkey[keyPtr++];
		if (keyPtr == romkey.size())
			keyPtr = 0;
	}

	rom.resize(writePtr);

	printf("INFO : Rom file '%s' successfully decrypted.\n", displayFilename.c_str());
	printf("INFO : Rom size %llu bytes.\n", rom.size());
	return rom;
}

const am::ScreenBuffer* guru::AmigaApp::GetScreen() const
{
	return m_amiga->GetScreen();
}

void guru::AmigaApp::SetKey(int key, int action, int mods)
{
	if (action == GLFW_PRESS && (mods & GLFW_MOD_SHIFT) != 0 && key == GLFW_KEY_F11)
	{
		// shift + F11 toggles between sending input to the Amiga or to the emulator GUI
		m_inputMode = (m_inputMode == InputMode::GuiHasFocus) ? InputMode::EmulatorHasFocus : InputMode::GuiHasFocus;
		return;
	}

	if (m_inputMode == InputMode::GuiHasFocus)
	{
		if (action == GLFW_PRESS && key == GLFW_KEY_F5)
		{
			SetRunning(!m_isRunning);
			return;
		}
	}
	else
	{
		// TODO : pass keyboard input to emulator
	}
}

void guru::AmigaApp::SetMouseButton(int button, int action, int mods)
{
	// Unlike the key callback, this is only called in emulator mode. Feed mouse buttons to emulator

	if (!(action == GLFW_PRESS || action == GLFW_RELEASE))
		return;

	if (button < 0 || button > 2)
		return;

	m_amiga->SetControllerButton(0, button, (action == GLFW_PRESS));
}

void guru::AmigaApp::SetMouseMove(double xMove, double yMove)
{
	// TODO
}
