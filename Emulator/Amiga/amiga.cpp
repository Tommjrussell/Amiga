#include "amiga.h"

#include "util/endian.h"

am::Amiga::Amiga(ChipRamConfig chipRamConfig, std::vector<uint8_t> rom)
{
	m_chipRam.resize(size_t(chipRamConfig));

	if (rom.empty())
	{
		m_rom.resize(512*1024, 0xcc);
	}
	else
	{
		m_rom = std::move(rom);
	}
}

uint8_t am::Amiga::PeekByte(uint32_t addr) const
{
	auto [type, mem] = const_cast<Amiga*>(this)->GetMappedMemory(addr);
	if (mem)
	{
		return *mem;
	}
	return 0;
}

uint16_t am::Amiga::PeekWord(uint32_t addr) const
{
	auto [type, mem] = const_cast<Amiga*>(this)->GetMappedMemory(addr);
	if (mem)
	{
		uint16_t value;
		memcpy(&value, mem, 2);
		return SwapEndian(value);
	}
	return 0;
}

void am::Amiga::PokeByte(uint32_t addr, uint8_t value)
{
	auto [type, mem] = GetMappedMemory(addr);
	if (mem)
	{
		*mem = value;
	}
}

std::tuple<am::Mapped, uint8_t*> am::Amiga::GetMappedMemory(uint32_t addr)
{
	// ignore the most-significant byte. The 68000-based Amigas only have a 24-bit address bus.
	addr &= 0x00ff'ffff;

	if (m_romOverlayEnabled && addr < m_rom.size())
	{
		return { Mapped::Rom, m_rom.data() + addr };
	}

	if (addr < 0x20'000)
	{
		// Up to 2Mib of chip ram. If less than 2Mib is present,
		// the higher addresses mirror the lower.
		const uint32_t chipRamMask = uint32_t(m_chipRam.size()) - 1;
		return { Mapped::ChipRam, m_chipRam.data() + (addr & chipRamMask) };
	}

	if (addr < 0xa0'0000)
	{
		return { Mapped::AutoConfig, nullptr };
	}

	if (addr < 0xbf'0000)
	{
		return { Mapped::Reserved, nullptr };
	}

	if (addr < 0xc0'0000)
	{
		return { Mapped::Cia, nullptr };
	}

	if (addr < 0xe0'0000)
	{
		// Everything in the range c00000-e00000 can mirror the chip registers
		if ((addr - 0xc0'0000) < m_slowRam.size())
		{
			return { Mapped::SlowRam, m_slowRam.data() + (addr - 0xc0'0000) };
		}

		return { Mapped::ChipRegisters, nullptr };
	}

	if (addr < 0xe8'0000)
	{
		return { Mapped::Reserved, nullptr };
	}

	if (addr < 0xf0'0000)
	{
		return { Mapped::AutoConfig, nullptr };
	}

	if (addr < 0xf8'0000)
	{
		// range f00000-f80000 is where the extended rom of some Amiga models and derivatives
		// resides. We'll ignore that for now though
		return { Mapped::Reserved, nullptr };
	}

	// Rom
	const uint32_t romMask = uint32_t(m_rom.size()) - 1;
	return { Mapped::Rom, m_rom.data() + ((addr - 0xf8'0000) & romMask) };
}