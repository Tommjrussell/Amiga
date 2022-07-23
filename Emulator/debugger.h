#pragma once

#include "amiga/disassembler.h"

#include <memory>
#include <initializer_list>
#include <vector>

namespace am
{
	class Amiga;
	class Symbols;
}

namespace guru
{
	class AmigaApp;
	class DebuggerMemoryInterface;

	class Debugger
	{
	public:
		explicit Debugger(guru::AmigaApp* app, am::Amiga* amiga);
		~Debugger();

		bool Draw();
		void OnStartRunning();
		void SetSymbolsFile(const std::string& symbolsFile);

	private:

		void DrawCpuRegisters();
		void DrawControls();
		void DrawSystemInterrupts();
		void DrawDMA();
		void DrawCIAs();
		void DrawDiskDrives();

		void UpdateAssembly();

	private:

		std::unique_ptr<DebuggerMemoryInterface> m_memory;
		std::unique_ptr<am::Disassembler> m_disassembler;
		std::unique_ptr<am::Symbols> m_symbols;

		struct DisassemblyLine
		{
			uint32_t addr;
			int visited;
			std::string text;
		};

		std::vector<DisassemblyLine> m_disassembly;
		std::string m_symbolsFile;

		guru::AmigaApp* m_app;
		am::Amiga* m_amiga;
		uint32_t m_breakpoint = 0;

		bool m_trackPc = true;
		uint32_t m_disassemblyStart = 0;

		bool m_breakOnRegister = false;
		bool m_dataBpEnabled = false;
		uint32_t m_regBreakpoint = -1;
		uint32_t m_dataBp = 0;
		int m_dataBpSize = 0;

	};
}