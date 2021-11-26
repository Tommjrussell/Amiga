#pragma once

#include "disassembler.h"

#include <memory>
#include <initializer_list>
#include <vector>

namespace guru
{

	class Memory : public am::IMemory
	{
	public:
		explicit Memory(std::initializer_list<uint16_t> code);

		virtual uint16_t GetWord(uint32_t addr) const override;
		virtual uint8_t GetByte(uint32_t) const override;

		size_t GetSize() const
		{
			return m_memory.size();
		}

	private:
		std::vector<uint8_t> m_memory;
	};

	class Debugger
	{
	public:
		explicit Debugger();
		~Debugger();

		bool Draw();

	private:

		std::unique_ptr<Memory> m_memory;
		std::unique_ptr<am::Disassembler> m_disassembler;

		std::vector<std::string> m_instructions;
	};
}