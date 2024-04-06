#include "App.h"

#include "amiga/amiga.h"
#include "Amiga/symbols.h"
#include "debugger.h"
#include "custom_chips_debugger.h"
#include "variable_watch.h"
#include "memory_editor.h"
#include "disk_manager.h"
#include "disk_image.h"
#include "disk_activity.h"
#include "log_viewer.h"

#include "util/file.h"
#include "util/key_codes.h"
#include "util/strings.h"
#include "util/stream.h"
#include "util/platform.h"
#include "util/ini_file.h"
#include "util/imgui_extras.h"

#include "3rd Party/imfilebrowser.h"

#include <imgui.h>
#include <memory>
#include <fstream>
#include <filesystem>

namespace
{
	const guru::FrontEndSettings kDefaultFrontEndSettings;

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

	constexpr char cloantoRomSig[] = "AMIROMTYPE1";
	constexpr size_t cloantoRomSigLen = _countof(cloantoRomSig) - 1;

	std::pair<bool, std::string> CheckRom(std::string_view romFile)
	{
		if (romFile.empty())
		{
			return { false, "No Rom File selected." };
		}

		std::filesystem::path file(romFile);
		const auto displayFilename = file.filename().string();

		if (!std::filesystem::exists(file))
		{
			return { false, "Rom file does not exist." };
		}

		auto size = std::filesystem::file_size(file);

		if (size == 524'288)
		{
			// ok, assume unencrypted rom.
			return { true, "File OK, unencrypted." };
		}

		if (size == 524'299)
		{
			std::vector<uint8_t> header;
			if (!util::LoadBinaryHeader(std::string(romFile), header, cloantoRomSigLen))
				return { false, "Rom file unaccessible." };

			if (memcmp(header.data(), cloantoRomSig, cloantoRomSigLen) != 0)
				return { false, "Rom file has unrecognised header." };

			auto romkeyFile = file.parent_path() / "rom.key";
			if (!std::filesystem::exists(romkeyFile))
				return { false, "Rom encrypted but no rom.key file found." };

			return { true, "File OK, encrypted." };
		}

		// Reject rom of any other size.
		return { false, "Rom file was unexpected size (expected 512Kib)" };
	}

	std::vector<uint8_t> LoadRom(const std::string& romFile)
	{
		std::filesystem::path file(romFile);
		const auto displayFilename = file.filename().string();

		std::vector<uint8_t> rom;
		if (!util::LoadBinaryFile(romFile, rom))
		{
			printf("ERROR: rom '%s' not loaded.\n", displayFilename.c_str());
			return rom;
		}

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

		if (!util::LoadBinaryFile(romkeyFile.string(), romkey))
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

	class SettingsWindow : public guru::Dialog
	{
	public:
		explicit SettingsWindow(guru::AppSettings& appSettings, guru::FrontEndSettings& feSettings)
			: m_appSettings(&appSettings)
			, m_feSettings(&feSettings)
		{
			strncpy_s(m_adfDirBuffer, m_appSettings->adfDir.data(), sizeof(m_adfDirBuffer));

			auto [romGood, why] = CheckRom(m_appSettings->romFile);
			m_romFileStatusText = why;
		}

		bool Draw() override
		{
			using util::ActiveButton;

			bool open = true;
			ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
			bool expanded = ImGui::Begin("Settings", &open);

			if (!(open && expanded))
			{
				ImGui::End();
				return open;
			}

			if (ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None))
			{
				if (ImGui::BeginTabItem("Display"))
				{
					ImGui::Checkbox("Use Crt Shader", &m_feSettings->useCrtEmulation);

					ImGui::SliderFloat("Horizontal Warp", &m_feSettings->crtWarpX, 0, .3f);
					ImGui::SliderFloat("Vertical Warp", &m_feSettings->crtWarpY, 0, .3f);

					if (ImGui::Button("Default Warp"))
					{
						m_feSettings->crtWarpX = kDefaultFrontEndSettings.crtWarpX;
						m_feSettings->crtWarpY = kDefaultFrontEndSettings.crtWarpY;
					}
					ImGui::SliderFloat("Brightness Adjust", &m_feSettings->brightnessAdjust, 0.1f, 2.0f);

					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Dirs"))
				{
					if (ActiveButton("Apply", m_adfDirModified))
					{
						m_appSettings->adfDir = m_adfDirBuffer;
						m_adfDirModified = false;
					}
					ImGui::SameLine();
					if (ImGui::InputText("ADF directory", m_adfDirBuffer, sizeof(m_adfDirBuffer)))
					{
						m_adfDirModified = true;
					}

					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("System"))
				{
					if (ImGui::Button("Select..."))
					{
						if (!m_fileDialog)
						{
							m_fileDialog = std::make_unique<ImGui::FileBrowser>();
							m_fileDialog->SetTitle("Select Rom Image");
							m_fileDialog->SetTypeFilters({ ".rom" });

						}
						auto romPath = std::filesystem::path(m_appSettings->romFile);
						if (romPath.has_parent_path())
						{
							m_fileDialog->SetPwd(romPath.parent_path());
						}
						m_fileDialog->Open();
					}
					ImGui::SameLine();
					ImGui::InputText("##RomFile", const_cast<char*>(m_appSettings->romFile.c_str()), m_appSettings->romFile.length(), ImGuiInputTextFlags_ReadOnly);
					ImGui::SameLine();
					ImGui::Text(m_romFileStatusText.c_str());
				}
				ImGui::EndTabBar();
			}

			ImGui::End();

			if (m_fileDialog)
			{
				m_fileDialog->Display();

				if (m_fileDialog->HasSelected())
				{
					m_appSettings->romFile = m_fileDialog->GetSelected().string();
					m_fileDialog->ClearSelected();
					m_fileDialog.reset();
					auto [romGood, why] = CheckRom(m_appSettings->romFile);
					m_romFileStatusText = why;
				}
			}

			return open;
		}

	private:
		guru::AppSettings* m_appSettings;
		guru::FrontEndSettings* m_feSettings;
		std::unique_ptr<ImGui::FileBrowser> m_fileDialog;
		char m_adfDirBuffer[256] = { '\0' };
		std::string m_romFileStatusText;
		bool m_adfDirModified = false;

	};
}


guru::AmigaApp::AmigaApp(const std::filesystem::path& programDir, const std::filesystem::path& configDir)
	: m_programDir(programDir)
	, m_configDir(configDir)
{
	LoadSettings();

	m_amiga = std::make_unique<am::Amiga>(am::ChipRamConfig::ChipRam1Mib, &m_log);

	if (!m_settings.romFile.empty())
	{
		const auto rom = LoadRom(m_settings.romFile);
		m_amiga->SetRom(rom);
	}

	m_symbols = std::make_unique<am::Symbols>();

	m_debugger = std::make_unique<Debugger>(this, m_amiga.get(), m_symbols.get());
	m_ccDebugger = std::make_unique<CCDebugger>(this, m_amiga.get());
	m_variableWatch = std::make_unique<VariableWatch>(m_amiga.get(), m_symbols.get());
	m_diskActivity = std::make_unique<DiskActivity>(m_amiga.get());

	for (int i = 0; i < _countof(kStandardKeyMapping); i++)
	{
		m_keyMap[kStandardKeyMapping[i].key] = kStandardKeyMapping[i].amigaKeycode;
	}
}

bool guru::AmigaApp::Setup()
{
	auto fontFile = m_programDir / "fonts" / "ProggyVector-Regular.ttf";

	ImGuiIO& io = ImGui::GetIO();
	m_lowDpiFont = io.Fonts->AddFontDefault();
	m_highDpiFont = io.Fonts->AddFontFromFileTTF(fontFile.generic_string().c_str(), 22);

	return true;
}

guru::AmigaApp::~AmigaApp()
{
}

void guru::AmigaApp::SetAudioPlayer(am::AudioPlayer* player)
{
	m_amiga->SetAudioPlayer(player);
}

void guru::AmigaApp::SetRunning(bool running)
{
	if (m_isRunning == running)
		return;

	m_isStarting = running;
	m_isRunning = running;

	if (running)
	{
		m_debugger->Refresh();
	}
}

void guru::AmigaApp::Reset()
{
	if (!m_settings.romFile.empty())
	{
		const auto rom = LoadRom(m_settings.romFile);
		m_amiga->SetRom(rom);
	}
	else
	{
		m_amiga->Reset();
	}
}

bool guru::AmigaApp::SetDiskImage(int drive, std::string& pathToImage)
{
	auto[file, archFile] = util::SplitOn(pathToImage, "::");

	std::filesystem::path path(file);

	std::vector<uint8_t> image;
	std::string name;

	bool ok = LoadDiskImage(path, archFile, image, name);

	if (ok)
	{
		ok = m_amiga->SetDisk(drive, pathToImage, name, std::move(image));
	}

	return ok;
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
		m_joystickState.buttons |= m_emulatedJoystickState.buttons;

		const auto buttonsDiff = m_joystickState.buttons ^ m_oldJoystickState.buttons;

		for (int b = 0; b < 3; b++)
		{
			if (buttonsDiff & (1 << b))
			{
				m_amiga->SetControllerButton(1, b, m_joystickState.buttons & (1 << b));
			}
		}
		true + false;
		m_amiga->SetJoystickMove(m_joystickState.x + m_emulatedJoystickState.x, m_joystickState.y + m_emulatedJoystickState.y);

		m_oldJoystickState = m_joystickState;

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

void guru::AmigaApp::Render(int displayWidth, int displayHeight)
{
	ImGui::PushFont(m_feSettings.highDPI ? m_highDpiFont : m_lowDpiFont);

	m_diskActivity->Draw(displayWidth, displayHeight);

	if (m_inputMode == InputMode::EmulatorHasFocus)
		return;

	//ImGui::ShowDemoWindow();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Main"))
		{
			if (ImGui::MenuItem("Debugger", "F1"))
			{
				m_debuggerOpen = true;
			}

			if (ImGui::MenuItem("Custom Chips Debugger", ""))
			{
				m_ccDebuggerOpen = true;
			}

			if (ImGui::MenuItem("Variable Watch", ""))
			{
				m_variableWatchOpen = true;
			}

			if (ImGui::MenuItem("Memory Viewer/Editor", ""))
			{
				OpenMemoryEditor();
			}

			if (ImGui::MenuItem("Disk Manager", ""))
			{
				if (!m_diskManager)
				{
					m_diskManager = std::make_unique<DiskManager>(this);
				}
			}

			if (ImGui::MenuItem("Log Viewer", ""))
			{
				if (!m_logViewer)
				{
					m_logViewer = std::make_unique<LogViewer>(&m_log);
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Quick Load Snapshot ", ""))
			{
				auto path = GetLocalAppDir();
				path /= "snap.bin";
				LoadSnapshot(path);
			}

			if (ImGui::MenuItem("Quick Save Snapshot ", ""))
			{
				auto path = GetOrCreateLocalAppDir();
				path /= "snap.bin";
				SaveSnapshot(path);
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
			if (ImGui::MenuItem("Settings", ""))
			{
				m_displayOptionsWindows = std::make_unique<SettingsWindow>(m_settings, m_feSettings);
			}

			ImGui::MenuItem("Joystick Emulation", "", &m_settings.joystickEmulation);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Arrow keys + ctrl simulate joystick input");

			if (ImGui::MenuItem("Fullscreen/Windowed", "", m_feSettings.fullScreen))
			{
				m_feSettings.fullScreen = !m_feSettings.fullScreen;
			}

			if (ImGui::MenuItem("Toggle High DPI", "", m_feSettings.highDPI))
			{
				m_feSettings.highDPI = !m_feSettings.highDPI;
			}

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

	if (m_variableWatchOpen)
	{
		m_variableWatchOpen = m_variableWatch->Draw();
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

	if (m_displayOptionsWindows)
	{
		if (!m_displayOptionsWindows->Draw())
		{
			m_displayOptionsWindows.reset();
		}
	}

	if (m_logViewer)
	{
		if (!m_logViewer->Draw())
		{
			m_logViewer.reset();
		}
	}

	ImGui::PopFont();

};

const am::ScreenBuffer* guru::AmigaApp::GetScreen() const
{
	return m_amiga->GetScreen();
}

bool guru::AmigaApp::SetKey(int key, int action, int mods)
{
	using util::Key;

	if (action == GLFW_PRESS && (mods & GLFW_MOD_SHIFT) != 0 && Key(key) == Key::KEY_F11)
	{
		// shift + F11 toggles between sending input to the Amiga or to the emulator GUI
		m_inputMode = (m_inputMode == InputMode::GuiHasFocus) ? InputMode::EmulatorHasFocus : InputMode::GuiHasFocus;
		return true;
	}

	if (m_inputMode == InputMode::GuiHasFocus)
	{
		if (Key(key) == Key::KEY_F1)
		{
			if (action == GLFW_PRESS)
			{
				m_debuggerOpen = !m_debuggerOpen;
			}
			return true;
		}
		else if (Key(key) == Key::KEY_F5)
		{
			if (action == GLFW_PRESS)
			{
				SetRunning(!m_isRunning);
			}
			return true;
		}
	}
	else
	{
		if (action == GLFW_PRESS || action == GLFW_RELEASE)
		{
			if (m_settings.joystickEmulation)
			{
				switch (Key(key))
				{
				case Key::KEY_RIGHT:
					m_emulatedJoystickState.x = (action == GLFW_PRESS) ? 1 : 0;
					return true;
				case Key::KEY_LEFT:
					m_emulatedJoystickState.x = (action == GLFW_PRESS) ? -1 : 0;
					return true;
				case Key::KEY_DOWN:
					m_emulatedJoystickState.y = (action == GLFW_PRESS) ? 1 : 0;
					return true;
				case Key::KEY_UP:
					m_emulatedJoystickState.y = (action == GLFW_PRESS) ? -1 : 0;
					return true;
				case Key::KEY_RIGHT_CONTROL:
					m_emulatedJoystickState.buttons = (action == GLFW_PRESS) ? 1 : 0;
					return true;
				}
			}

			ConvertAndSendKeyCode(Key(key), action == GLFW_PRESS);
		}
		return true;
	}

	return false; // not handled
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
	m_amiga->SetMouseMove(int(std::floor(xMove / 2)), int(std::floor(yMove / 2)));
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

void guru::AmigaApp::SetSymbolsFile(const std::string& symbolsFile)
{
	m_symbols->SetSymbolsFile(symbolsFile);
	m_symbols->Load();
}

std::filesystem::path guru::AmigaApp::GetLocalAppDir()
{
	auto appPath = util::GetLocalDataDirectory();
	appPath /= "Guru Amiga";
	return appPath;
}

std::filesystem::path guru::AmigaApp::GetOrCreateLocalAppDir()
{
	auto appPath = GetLocalAppDir();
	std::filesystem::create_directory(appPath);
	return appPath;
}

void guru::AmigaApp::LoadSnapshot(const std::filesystem::path& file)
{
	std::ifstream ifile(file, std::ios::binary);
	if (!ifile.is_open())
		return;

	if (!m_amiga->ReadSnapshot(ifile))
		return;

	for (int i = 0; i < 4; i++)
	{
		std::string fileLocation;
		util::StreamString(ifile, fileLocation);

		if (fileLocation != m_amiga->GetDiskFilename(i))
		{
			SetDiskImage(i, fileLocation);
		}
	}

	m_debugger->Refresh();
}

void guru::AmigaApp::SaveSnapshot(const std::filesystem::path& file)
{
	std::ofstream ofile(file, std::ios::out | std::ios::binary);
	if (!ofile.is_open())
		return;

	m_amiga->WriteSnapshot(ofile);

	for (int i = 0; i < 4; i++)
	{
		util::StreamString(ofile, m_amiga->GetDiskFilename(i));
	}

}

void guru::AmigaApp::OpenMemoryEditor()
{
	if (!m_memoryEditor)
	{
		m_memoryEditor = std::make_unique<MemoryEditor>(m_amiga.get());
	}

	ImGui::SetWindowFocus("Memory Editor");
}

void guru::AmigaApp::ShowMemInMemoryEditor(uint32_t addr, uint32_t size)
{
	OpenMemoryEditor();
	m_memoryEditor->GotoAndHighlightMem(addr, size);
}

void guru::AmigaApp::LoadSettings()
{
	auto appDir = GetOrCreateLocalAppDir();
	auto [res, ini, inierrors] = util::LoadIniFile(appDir / "guru.ini");

	if (res != util::IniLoadResult::Ok)
		return;

	{
		auto systemSection = GetSection(ini, "System");
		if (auto romFile = GetStringKey(systemSection, "rom"))
		{
			m_settings.romFile = romFile.value();
		}
	}

	{
		auto uiSection = GetSection(ini, "UI");
		if (auto useHighDpiFont = GetBoolKey(uiSection, "useHighDPIFont"))
		{
			m_feSettings.highDPI = useHighDpiFont.value();
		}
	}

	{
		auto displaySection = GetSection(ini, "Display");
		if (auto useCrt = GetBoolKey(displaySection, "useCrt"))
		{
			m_feSettings.useCrtEmulation = useCrt.value();
		}

		if (auto crtWarpX = GetFloatKey(displaySection, "crtWarpX"))
		{
			m_feSettings.crtWarpX = float(crtWarpX.value());
		}

		if (auto crtWarpY = GetFloatKey(displaySection, "crtWarpY"))
		{
			m_feSettings.crtWarpY = float(crtWarpY.value());
		}

		if (auto brightnessAdjust = GetFloatKey(displaySection, "brightnessAdjust"))
		{
			m_feSettings.brightnessAdjust = float(brightnessAdjust.value());
		}
	}

	{
		auto section = GetSection(ini, "Directories");
		if (auto adfDir = GetStringKey(section, "adfDirectories"))
		{
			m_settings.adfDir = adfDir.value();
		}
	}

	{
		auto inputSection = GetSection(ini, "Input");
		if (auto joystickEmulation = GetBoolKey(inputSection, "joystickEmulation"))
		{
			m_settings.joystickEmulation = joystickEmulation.value();
		}
	}

}

void guru::AmigaApp::SaveSettings()
{
	util::Ini ini;
	{
		auto& systemSection = ini.m_sections["System"];
		SetStringKey(systemSection, "rom", m_settings.romFile);
	}

	{
		auto& uiSection = ini.m_sections["UI"];
		SetBoolKey(uiSection, "useHighDPIFont", m_feSettings.highDPI);
	}

	{
		auto& displaySection = ini.m_sections["Display"];
		SetBoolKey(displaySection, "useCrt", m_feSettings.useCrtEmulation);
		SetFloatKey(displaySection, "crtWarpX", m_feSettings.crtWarpX);
		SetFloatKey(displaySection, "crtWarpY", m_feSettings.crtWarpY);
		SetFloatKey(displaySection, "brightnessAdjust", m_feSettings.brightnessAdjust);
	}
	{
		auto& directoriesSection = ini.m_sections["Directories"];
		SetStringKey(directoriesSection, "adfDirectories", m_settings.adfDir);
	}

	{
		auto& inputSection = ini.m_sections["Input"];
		SetBoolKey(inputSection, "joystickEmulation", m_settings.joystickEmulation);
	}

	auto appDir = GetOrCreateLocalAppDir();
	util::SaveIniFile(ini, appDir / "guru.ini");
}

void guru::AmigaApp::Shutdown()
{
	SaveSettings();
}