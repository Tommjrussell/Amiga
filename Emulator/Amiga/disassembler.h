#pragma once

#include <string>
#include <optional>
#include <stdint.h>

namespace am
{
	class Symbols;

	class IMemory
	{
	public:
		virtual uint16_t GetWord(uint32_t) const = 0;
		virtual uint8_t GetByte(uint32_t) const = 0;
	};

	namespace Disasm
	{
		enum class Reg : uint32_t
		{
			NoReg,
			PC,
			A0,
			A1,
			A2,
			A3,
			A4,
			A5,
			A6,
			A7,
			D0,
			D1,
			D2,
			D3,
			D4,
			D5,
			D6,
			D7,
		};

		struct EaMem
		{
			Reg baseReg = Reg::NoReg;
			uint32_t displacement = 0;	// Sign-extended displacement
			Reg indexReg = Reg::NoReg;
			int8_t indexScale = 1;		// Scale factor of index (1,2,4,8) 68000 is hardwired to 1
			int8_t indexSize = 0;		// Size of index in register (only 2 or 4 valid)
		};

		struct Opcode
		{
			std::string text;
			int size = 0;
			std::optional<EaMem> source;
			std::optional<EaMem> dest;
		};
	}

	class Disassembler
	{
	public:
		explicit Disassembler(IMemory* memoryInterface) : m_memory(memoryInterface) {}

		Disasm::Opcode Disassemble();

		void SetSymbols(Symbols* symbols)
		{
			m_symbols = symbols;
		}

	private:
		uint32_t GetImmediateValue(int size);
		std::optional<Disasm::EaMem> WriteEffectiveAddress(int mode, int reg, int size, char*& buffptr, int& charsLeft, bool isJumpTarget);
		void WriteRegisterList(uint16_t mask, bool isReversed, char*& buffptr, int& charsLeft);

	public:
		uint32_t pc = 0;

	private:

		IMemory* m_memory;
		Symbols* m_symbols = nullptr;
	};

}