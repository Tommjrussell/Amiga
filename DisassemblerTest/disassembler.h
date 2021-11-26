#pragma once

#include <string>
#include <stdint.h>

namespace am
{
	class Memory
	{
	public:
		virtual uint16_t GetWord(uint32_t) const = 0;
		virtual uint8_t GetByte(uint32_t) const = 0;
	};

	class Disassembler
	{
	public:
		explicit Disassembler(Memory* memoryInterface) : m_memory(memoryInterface) {}

		std::string Disassemble();

	private:
		uint32_t GetImmediateValue(int size);
		void WriteEffectiveAddress(int mode, int reg, int size, char*& buffptr, int& charsLeft);
		void WriteRegisterList(uint16_t mask, bool isReversed, char*& buffptr, int& charsLeft);

	public:
		uint32_t pc = 0;

	private:

		Memory* m_memory;
	};

}