#pragma once

#include "amiga/disassembler.h"

#include <memory>
#include <initializer_list>
#include <vector>

namespace am
{
	class Amiga;
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

	private:

		void DrawCpuRegisters();
		void DrawControls();
		void DrawSystemInterrupts();

		void UpdateAssembly();

	private:

		std::unique_ptr<DebuggerMemoryInterface> m_memory;
		std::unique_ptr<am::Disassembler> m_disassembler;

		std::vector<std::string> m_instructions;

		guru::AmigaApp* m_app;
		am::Amiga* m_amiga;
		uint32_t m_breakpoint = 0;
	};
}