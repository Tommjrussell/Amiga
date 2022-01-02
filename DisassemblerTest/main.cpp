#include "../Emulator/amiga/disassembler.h"

#include <vector>
#include <iostream>
#include <string>
#include <initializer_list>
#include <stdint.h>

struct MachineCode : public am::IMemory
{
	MachineCode(std::initializer_list<uint16_t> code)
	{
		for (auto e : code)
		{
			memory.push_back(uint8_t(e >> 8));
			memory.push_back(uint8_t(e));
		}
	}

	virtual uint16_t GetWord(uint32_t addr) const override
	{
		if ((addr & 1) == 1)
		{
			_ASSERTE(false);
			return 0;
		}

		if (addr >= memory.size())
		{
			_ASSERTE(false);
			return 0;
		}

		uint16_t value = uint16_t(memory[addr] << 8);
		value |= uint16_t(memory[addr+1]);

		return value;
	}

	virtual uint8_t GetByte(uint32_t) const override
	{
		// Not needed by the disassembler so leave unimplemented
		_ASSERTE(false);
		return 0;
	}

	std::vector<uint8_t> memory;
};

int main()
{
	MachineCode m
	{
		0xe248,							// lsr.w $1,D0
		0x0cb8, 0x4845, 0x4c50, 0x0000, // cmpi.l $48454c50, (0000).w
		0x1029, 0x001f,		// move.b ($001f, A1), D0
		0x532e, 0x0126,		// subq.b $01, ($0126, A6)
		0x4880,				// ext.w D0
		0x4e75,				// rts
		0x6632,				// bne 50 -> $00000040
		0x4cd7, 0x55f0,
		0x4cd5, 0xaaaa,
		0x48d5, 0xaaaa,
		0x4895, 0xffff
	};

	am::Disassembler d(&m);

	while (d.pc < m.memory.size())
	{
		auto result = d.Disassemble();
		std::cout << result << "\n";
	}

	return 0;
}
