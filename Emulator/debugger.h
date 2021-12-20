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
	class DebuggerMemoryInterface;

	class Debugger
	{
	public:
		explicit Debugger(am::Amiga* amiga);
		~Debugger();

		bool Draw();

	private:

		std::unique_ptr<DebuggerMemoryInterface> m_memory;
		std::unique_ptr<am::Disassembler> m_disassembler;

		std::vector<std::string> m_instructions;
		uint32_t m_pc = 0;
	};
}