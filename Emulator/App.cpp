#include "App.h"

#include "amiga/amiga.h"
#include "debugger.h"
#include "custom_chips_debugger.h"
#include "memory_editor.h"

#include "util/file.h"

#include <imgui.h>
#include <memory>
#include <filesystem>

namespace
{
	constexpr int PAL_CClockFreq = 3546895;
	constexpr int NTSC_CClockFreq = 3579545;
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