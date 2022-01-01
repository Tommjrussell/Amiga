#include "amiga.h"

#include "util/endian.h"

namespace
{
	bool IsSharedAccess(am::Mapped m)
	{
		return (uint32_t(m) & uint32_t(am::Mapped::Shared)) != 0;
	}
}

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

	m_m68000 = std::make_unique<cpu::M68000>(this);

	int delay;
	m_m68000->Reset(delay);
}

void am::Amiga::SetPC(uint32_t pc)
{
	m_m68000->SetPC(pc);
}

void am::Amiga::SetBreakpoint(uint32_t addr)
{
	m_breakpoint = addr;
	m_breakpointEnabled = true;
}

void am::Amiga::ClearBreakpoint()
{
	m_breakpointEnabled = false;
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

uint16_t am::Amiga::ReadBusWord(uint32_t addr)
{
	auto [type, mem] = GetMappedMemory(addr);
	if (IsSharedAccess(type))
	{
		m_sharedBusRws++;
	}
	else
	{
		m_exclusiveBusRws++;
	}

	if (mem)
	{
		uint16_t value;
		memcpy(&value, mem, 2);
		return SwapEndian(value);
	}
	else
	{
		// TODO : implement register/peripheral access
		return 0;
	}
}

void am::Amiga::WriteBusWord(uint32_t addr, uint16_t value)
{
	auto [type, mem] = GetMappedMemory(addr);
	if (IsSharedAccess(type))
	{
		m_sharedBusRws++;
	}
	else
	{
		m_exclusiveBusRws++;
	}

	if (mem && type != Mapped::Rom)
	{
		value = SwapEndian(value);
		memcpy(mem, &value, 2);
	}
	else
	{
		// TODO : implement register/peripheral access
	}
}

uint8_t am::Amiga::ReadBusByte(uint32_t addr)
{
	auto [type, mem] = GetMappedMemory(addr);
	if (IsSharedAccess(type))
	{
		m_sharedBusRws++;
	}
	else
	{
		m_exclusiveBusRws++;
	}

	if (mem)
	{
		return *mem;
	}
	else
	{
		if (type == Mapped::Cia)
		{
			if ((addr & 0xe03001) == 0xa02001) // CIAA
			{
				const auto port = (addr >> 8) & 0xf;
				return ReadCIA(0, port);
			}

			if ((addr & 0xe03001) == 0xa01000) // CIAB
			{
				const auto port = (addr >> 8) & 0xf;
				return ReadCIA(1, port);
			}
		}

		// TODO : implement register/peripheral access
		return 0;
	}
}

void am::Amiga::WriteBusByte(uint32_t addr, uint8_t value)
{
	auto [type, mem] = GetMappedMemory(addr);
	if (IsSharedAccess(type))
	{
		m_sharedBusRws++;
	}
	else
	{
		m_exclusiveBusRws++;
	}

	if (mem && type != Mapped::Rom)
	{
		*mem = value;
	}
	else
	{
		if (type == Mapped::Cia)
		{
			if ((addr & 0xe03001) == 0xa02001) // CIAA
			{
				auto port = (addr >> 8) & 0xf;
				WriteCIA(0, port, value);
			}
			else if ((addr & 0xe03001) == 0xa01000) // CIAB
			{
				auto port = (addr >> 8) & 0xf;
				WriteCIA(1, port, value);
			}
		}

		// TODO : implement register/peripheral access
	}
}

bool am::Amiga::ExecuteFor(uint64_t cclocks)
{
	const auto runTill = m_totalCClocks + cclocks;

	bool running = true;

	while (running && m_totalCClocks < runTill)
	{
		running = DoOneTick();
	}
	return running;
}

bool am::Amiga::ExecuteOneCpuInstruction()
{
	while (!CpuReady())
	{
		DoOneTick();
	}

	if (m_m68000->GetExecutionState() == cpu::ExecuteState::ReadyToDecode)
	{
		DoOneTick();
	}

	while (!(CpuReady() && m_m68000->GetExecutionState() != cpu::ExecuteState::ReadyToExecute))
	{
		DoOneTick();
	}

	return true;
}

bool am::Amiga::ExecuteToEndOfCpuInstruction()
{
	while (!CpuReady())
	{
		DoOneTick();
	}

	if (m_m68000->GetExecutionState() == cpu::ExecuteState::ReadyToExecute)
	{
		do
		{
			DoOneTick();
		}
		while (!CpuReady());
	}
	return true;
}

bool am::Amiga::DoOneTick()
{
	bool running = true;
	bool chipBusBusy = false;

	if (m_m68000->GetExecutionState() == cpu::ExecuteState::ReadyToDecode && CpuReady())
	{
		if (m_breakpointEnabled && m_m68000->GetPC() == m_breakpoint)
			return false;

		running = m_m68000->DecodeOneInstruction(m_cpuBusyTimer);
	}

	if (m_cpuBusyTimer == 0)
	{
		if (m_exclusiveBusRws > 0)
		{
			m_exclusiveBusRws--;
			m_cpuBusyTimer = 1;
		}
		else if (m_sharedBusRws > 0)
		{
			if (!chipBusBusy)
			{
				m_sharedBusRws--;
				m_cpuBusyTimer = 1;
				chipBusBusy = true;
			}
		}
	}
	else
	{
		m_cpuBusyTimer--;
	}

	if (m_m68000->GetExecutionState() == cpu::ExecuteState::ReadyToExecute && CpuReady())
	{
		running = m_m68000->ExecuteOneInstruction(m_cpuBusyTimer);
	}

	m_totalCClocks++;

	return running;
}

void am::Amiga::Reset()
{
	::memset(m_chipRam.data(), 0, m_chipRam.size());
	::memset(m_slowRam.data(), 0, m_slowRam.size());

	m_sharedBusRws = 0;
	m_exclusiveBusRws = 0;
	m_m68000->Reset(m_cpuBusyTimer);
	m_totalCClocks = 0;

	ResetCIA(0);
	ResetCIA(1);

	// Enable ROM overlay
	m_cia[0].pra |= 0x01;
	m_romOverlayEnabled = true;
}

void am::Amiga::ResetCIA(int num)
{
	auto& cia = m_cia[num];
	cia.pra = 0;
	cia.prb = 0;
	cia.ddra = 0;
	cia.ddrb = 0;
}

void am::Amiga::WriteCIA(int num, int port, uint8_t data)
{
	auto& cia = m_cia[num];

	switch (port)
	{
	case 0x0: // pra
	{
		cia.pra &= ~cia.ddra;
		cia.pra |= data & cia.ddra;

		if (num == 0)
		{
			// For convenience, mirror the state if the ROM overlay bit in this bool variable
			m_romOverlayEnabled = (cia.pra & 0x01) != 0;
		}

	}	break;

	case 0x1: // prb
	{
		cia.prb &= ~cia.ddrb;
		cia.prb |= data & cia.ddrb;
	}	break;

	case 0x2: // ddra
		cia.ddra = data;
		break;

	case 0x3: // ddrb
		cia.ddrb = data;
		break;

	default:
		// TODO : other registers
		break;

	}
}

uint8_t am::Amiga::ReadCIA(int num, int port)
{
	auto& cia = m_cia[num];

	switch (port)
	{
	case 0x0:
		return cia.pra;
	case 0x1:
		return cia.prb;
	case 0x2:
		return cia.ddra;
	case 0x3:
		return cia.ddrb;

	default:
		// TODO : other registers
		return 0;
	}
}