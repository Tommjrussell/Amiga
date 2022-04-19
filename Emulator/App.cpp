#include "App.h"

#include "amiga/amiga.h"
#include "debugger.h"
#include "custom_chips_debugger.h"
#include "memory_editor.h"
#include "disk_manager.h"

#include "util/file.h"
#include "util/key_codes.h"

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

	using util::Key;

	struct AmigaKeyMap
	{
		Key key;
		uint8_t amigaKeycode;
	};

	const AmigaKeyMap kStandardKeyMapping[] =
	{
		{ Key::KEY_SPACE,         0x40 },
		{ Key::KEY_APOSTROPHE,    0x2a },
		{ Key::KEY_COMMA,         0x38 },
		{ Key::KEY_MINUS,         0x0b },
		{ Key::KEY_PERIOD,        0x39 },
		{ Key::KEY_SLASH,         0x3a },
		{ Key::KEY_0,             0x0a },
		{ Key::KEY_1,             0x01 },
		{ Key::KEY_2,             0x02 },
		{ Key::KEY_3,             0x03 },
		{ Key::KEY_4,             0x04 },
		{ Key::KEY_5,             0x05 },
		{ Key::KEY_6,             0x06 },
		{ Key::KEY_7,             0x07 },
		{ Key::KEY_8,             0x08 },
		{ Key::KEY_9,             0x09 },
		{ Key::KEY_SEMICOLON,     0x29 },
		{ Key::KEY_EQUAL,         0x0c },
		{ Key::KEY_A,             0x20 },
		{ Key::KEY_B,             0x35 },
		{ Key::KEY_C,             0x33 },
		{ Key::KEY_D,             0x22 },
		{ Key::KEY_E,             0x12 },
		{ Key::KEY_F,             0x23 },
		{ Key::KEY_G,             0x24 },
		{ Key::KEY_H,             0x25 },
		{ Key::KEY_I,             0x17 },
		{ Key::KEY_J,             0x26 },
		{ Key::KEY_K,             0x27 },
		{ Key::KEY_L,             0x28 },
		{ Key::KEY_M,             0x37 },
		{ Key::KEY_N,             0x36 },
		{ Key::KEY_O,             0x18 },
		{ Key::KEY_P,             0x19 },
		{ Key::KEY_Q,             0x10 },
		{ Key::KEY_R,             0x13 },
		{ Key::KEY_S,             0x21 },
		{ Key::KEY_T,             0x14 },
		{ Key::KEY_U,             0x16 },
		{ Key::KEY_V,             0x34 },
		{ Key::KEY_W,             0x11 },
		{ Key::KEY_X,             0x32 },
		{ Key::KEY_Y,             0x15 },
		{ Key::KEY_Z,             0x31 },
		{ Key::KEY_LEFT_BRACKET,  0x1a },
		{ Key::KEY_BACKSLASH,     0x0d },
		{ Key::KEY_RIGHT_BRACKET, 0x1b },
		{ Key::KEY_GRAVE_ACCENT,  0x00 },

		/* Function keys */
		{ Key::KEY_ESCAPE,        0x45 },
		{ Key::KEY_ENTER,         0x44 },
		{ Key::KEY_TAB,           0x42 },
		{ Key::KEY_BACKSPACE,     0x41 },
		{ Key::KEY_DELETE,        0x46 },
		{ Key::KEY_RIGHT,         0x4e },
		{ Key::KEY_LEFT,          0x4f },
		{ Key::KEY_DOWN,          0x4d },
		{ Key::KEY_UP,            0x4c },
		{ Key::KEY_HOME,          0x5f },  // Note: Home key mapped to Amiga's 'Help' key
		{ Key::KEY_CAPS_LOCK,     0x62 },
		{ Key::KEY_F1,            0x50 },
		{ Key::KEY_F2,            0x51 },
		{ Key::KEY_F3,            0x52 },
		{ Key::KEY_F4,            0x53 },
		{ Key::KEY_F5,            0x54 },
		{ Key::KEY_F6,            0x55 },
		{ Key::KEY_F7,            0x56 },
		{ Key::KEY_F8,            0x57 },
		{ Key::KEY_F9,            0x58 },
		{ Key::KEY_F10,           0x59 },
		{ Key::KEY_KP_0,          0x0f },
		{ Key::KEY_KP_1,          0x1d },
		{ Key::KEY_KP_2,          0x1e },
		{ Key::KEY_KP_3,          0x1f },
		{ Key::KEY_KP_4,          0x2d },
		{ Key::KEY_KP_5,          0x2e },
		{ Key::KEY_KP_6,          0x2f },
		{ Key::KEY_KP_7,          0x3d },
		{ Key::KEY_KP_8,          0x3e },
		{ Key::KEY_KP_9,          0x3f },
		{ Key::KEY_KP_DECIMAL,    0x3c },
		{ Key::KEY_KP_DIVIDE,     0x5c },
		{ Key::KEY_KP_MULTIPLY,   0x5d },
		{ Key::KEY_KP_SUBTRACT,   0x4a },
		{ Key::KEY_KP_ADD,        0x5e },
		{ Key::KEY_KP_ENTER,      0x43 },
		{ Key::KEY_LEFT_SHIFT,    0x60 },
		{ Key::KEY_LEFT_CONTROL,  0x63 },
		{ Key::KEY_LEFT_ALT,      0x64 },
		{ Key::KEY_LEFT_SUPER,    0x66 },
		{ Key::KEY_RIGHT_SHIFT,   0x61 },
		{ Key::KEY_RIGHT_CONTROL, 0x63 },
		{ Key::KEY_RIGHT_ALT,     0x64 },
		{ Key::KEY_RIGHT_SUPER,   0x67 },
	};

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

	for (int i = 0; i < _countof(kStandardKeyMapping); i++)
	{
		m_keyMap[kStandardKeyMapping[i].key] = kStandardKeyMapping[i].amigaKeycode;
	}
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
	using util::Key;

	if (action == GLFW_PRESS && (mods & GLFW_MOD_SHIFT) != 0 && Key(key) == Key::KEY_F11)
	{
		// shift + F11 toggles between sending input to the Amiga or to the emulator GUI
		m_inputMode = (m_inputMode == InputMode::GuiHasFocus) ? InputMode::EmulatorHasFocus : InputMode::GuiHasFocus;
		return;
	}

	if (m_inputMode == InputMode::GuiHasFocus)
	{
		if (action == GLFW_PRESS && Key(key) == Key::KEY_F5)
		{
			SetRunning(!m_isRunning);
			return;
		}
	}
	else
	{
		if (action == GLFW_PRESS || action == GLFW_RELEASE)
		{
			ConvertAndSendKeyCode(Key(key), action == GLFW_PRESS);
		}
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

void guru::AmigaApp::ConvertAndSendKeyCode(util::Key key, bool down)
{
	if (!m_isRunning && down == true)
		return; // do not queue new key presses while the emulator is paused

	auto it = m_keyMap.find(key);
	if (it == m_keyMap.end())
		return;

	m_amiga->QueueKeyPress(it->second + (down ? 0 : 0x80));
}