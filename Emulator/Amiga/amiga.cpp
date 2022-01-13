#include "amiga.h"

#include "registers.h"

#include "util/endian.h"

#include <cassert>


namespace
{

	enum class RegType : uint8_t
	{
		Reserved,
		Strobe,
		ReadOnly,
		WriteOnly,
		DmaReadOnly,
		DmaWriteOnly,
	};

	enum class Chipset : uint8_t
	{
		OCS,
		ECS,
		AGA
	};

	struct RegisterInfo
	{
		uint16_t addr;
		RegType type;
		Chipset minChipset;
	};

	const RegisterInfo registerInfo[] =
	{
		{ uint16_t(am::Register::BLTDDAT), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::DMACONR), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::VPOSR), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::VHPOSR), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::DSKDATR), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::JOY0DAT), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::JOY1DAT), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::CLXDAT), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::ADKCONR), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::POT0DAT), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::POT1DAT), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::POTGOR), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::SERDATR), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::DSKBYTR), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::INTENAR), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::INTREQR), RegType::ReadOnly, Chipset::OCS },
		{ uint16_t(am::Register::DSKPTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::DSKPTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::DSKLEN), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::DSKDAT), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::REFPTR), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::VPOSW), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::VHPOSW), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COPCON), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SERDAT), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SERPER), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::POTGO), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::JOYTEST), RegType::WriteOnly, Chipset::OCS },

		{ uint16_t(am::Register::STREQU), RegType::Strobe, Chipset::OCS },
		{ uint16_t(am::Register::STRVBL), RegType::Strobe, Chipset::OCS },
		{ uint16_t(am::Register::STRHOR), RegType::Strobe, Chipset::OCS },
		{ uint16_t(am::Register::STRLONG), RegType::Strobe, Chipset::OCS },

		{ uint16_t(am::Register::BLTCON0), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTCON1), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTAFWM), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTALWM), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTCPTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTCPTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTBPTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTBPTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTAPTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTAPTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTDPTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTDPTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTSIZE), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTCON0L), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::BLTSIZV), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTSIZH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTCMOD), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTBMOD), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTAMOD), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTDMOD), RegType::WriteOnly, Chipset::OCS },

		{ 0x000, RegType::Reserved, Chipset::OCS }, // 068
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 06A
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 06C
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 06E

		{ uint16_t(am::Register::BLTCDAT), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTBDAT), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BLTADAT), RegType::WriteOnly, Chipset::OCS },

		{ 0x000, RegType::Reserved, Chipset::OCS }, // 076

		{ uint16_t(am::Register::SPRHDAT), RegType::WriteOnly, Chipset::ECS },

		{ 0x000, RegType::Reserved, Chipset::OCS }, // 07A

		{ uint16_t(am::Register::DENISEID), RegType::ReadOnly, Chipset::ECS },

		{ uint16_t(am::Register::DSKSYNC), RegType::WriteOnly, Chipset::OCS },

		{ uint16_t(am::Register::COP1LCH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COP1LCL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COP2LCH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COP2LCL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COPJMP1), RegType::Strobe, Chipset::OCS },
		{ uint16_t(am::Register::COPJMP2), RegType::Strobe, Chipset::OCS },
		{ uint16_t(am::Register::COPINS), RegType::WriteOnly, Chipset::OCS },

		{ uint16_t(am::Register::DIWSTRT), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::DIWSTOP), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::DDFSTRT), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::DDFSTOP), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::DMACON), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::CLXCON), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::INTENA), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::INTREQ), RegType::WriteOnly, Chipset::OCS },

		{ uint16_t(am::Register::ADKCON), RegType::WriteOnly, Chipset::OCS },

		{ uint16_t(am::Register::AUD0LCH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD0LCL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD0LEN), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD0PER), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD0VOL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD0DAT), RegType::WriteOnly, Chipset::OCS },
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0AC
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0AE
		{ uint16_t(am::Register::AUD1LCH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD1LCL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD1LEN), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD1PER), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD1VOL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD1DAT), RegType::WriteOnly, Chipset::OCS },
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0BC
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0BE
		{ uint16_t(am::Register::AUD2LCH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD2LCL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD2LEN), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD2PER), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD2VOL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD2DAT), RegType::WriteOnly, Chipset::OCS },
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0CC
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0CE
		{ uint16_t(am::Register::AUD3LCH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD3LCL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD3LEN), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD3PER), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD3VOL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::AUD3DAT), RegType::WriteOnly, Chipset::OCS },
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0DC
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0DE

		{ uint16_t(am::Register::BPL1PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL1PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL2PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL2PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL3PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL3PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL4PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL4PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL5PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL5PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL6PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL6PTL), RegType::WriteOnly, Chipset::OCS },

		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0F8
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0FA
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0FC
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 0FE

		{ uint16_t(am::Register::BPLCON0), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPLCON1), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPLCON2), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPLCON3), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL1MOD), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL2MOD), RegType::WriteOnly, Chipset::OCS },

		{ 0x000, RegType::Reserved, Chipset::OCS }, // 10C
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 10E

		{ uint16_t(am::Register::BPL1DAT), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL2DAT), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL3DAT), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL4DAT), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL5DAT), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::BPL6DAT), RegType::WriteOnly, Chipset::OCS },

		{ 0x000, RegType::Reserved, Chipset::OCS }, // 11C
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 11E

		{ uint16_t(am::Register::SPR0PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR0PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR1PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR1PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR2PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR2PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR3PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR3PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR4PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR4PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR5PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR5PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR6PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR6PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR7PTH), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR7PTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR0POS), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR0CTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR0DATA), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR0DATB), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR1POS), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR1CTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR1DATA), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR1DATB), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR2POS), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR2CTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR2DATA), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR2DATB), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR3POS), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR3CTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR3DATA), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR3DATB), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR4POS), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR4CTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR4DATA), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR4DATB), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR5POS), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR5CTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR5DATA), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR5DATB), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR6POS), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR6CTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR6DATA), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR6DATB), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR7POS), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR7CTL), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR7DATA), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::SPR7DATB), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR00), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR01), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR02), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR03), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR04), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR05), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR06), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR07), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR08), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR09), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR10), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR11), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR12), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR13), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR14), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR15), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR16), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR17), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR18), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR19), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR20), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR21), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR22), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR23), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR24), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR25), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR26), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR27), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR28), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR29), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR30), RegType::WriteOnly, Chipset::OCS },
		{ uint16_t(am::Register::COLOR31), RegType::WriteOnly, Chipset::OCS },

		{ uint16_t(am::Register::HTOTAL), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::HSSTOP), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::HBSTRT), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::HBSTOP), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::VTOTAL), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::VSSTOP), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::VBSTRT), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::VBSTOP), RegType::WriteOnly, Chipset::ECS },

		{ 0x000, RegType::Reserved, Chipset::OCS }, // 1D0
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 1D2
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 1D4
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 1D6
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 1D8
		{ 0x000, RegType::Reserved, Chipset::OCS }, // 1DA

		{ uint16_t(am::Register::BEAMCON0), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::HSSTRT), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::VSSTRT), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::HCENTER), RegType::WriteOnly, Chipset::ECS },
		{ uint16_t(am::Register::DIWHIGH), RegType::WriteOnly, Chipset::ECS },
	};


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

	m_registers.resize(_countof(registerInfo), 0x0000);

	m_palette.fill(ColourRef(0));

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

void am::Amiga::EnableBreakOnRegister(uint32_t regAddr)
{
	m_breakAtRegister = regAddr;
	m_breakOnRegisterEnabled = true;
}

void am::Amiga::DisableBreakOnRegister()
{
	m_breakOnRegisterEnabled = false;
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

	if (addr < 0x20'0000)
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
		if (type == Mapped::ChipRegisters)
		{
			if ((addr & 0x03f000) == 0x03f000)
			{
				const auto regNum = addr & 0x000fff;
				return ReadRegister(regNum);
			}
		}

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
		if (type == Mapped::ChipRegisters)
		{
			if ((addr & 0x03f000) == 0x03f000)
			{
				const auto regNum = addr & 0x000fff;
				WriteRegister(regNum, value);
			}
		}
		else if (type == Mapped::Cia)
		{
			// TODO : implement CIA access
		}
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
		m_breakAtNextInstruction = false;
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
		if (m_breakAtNextInstruction || (m_breakpointEnabled && m_m68000->GetPC() == m_breakpoint))
		{
			m_breakAtNextInstruction = false;
			return false;
		}


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
	::memset(m_registers.data(), 0, m_registers.size() * sizeof(uint16_t));

	::memset(m_chipRam.data(), 0, m_chipRam.size());
	::memset(m_slowRam.data(), 0, m_slowRam.size());

	ResetCIA(0);
	ResetCIA(1);

	// Enable ROM overlay
	m_cia[0].pra |= 0x01;
	m_romOverlayEnabled = true;

	m_sharedBusRws = 0;
	m_exclusiveBusRws = 0;
	m_totalCClocks = 0;

	m_m68000->Reset(m_cpuBusyTimer);
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

uint16_t am::Amiga::PeekRegister(am::Register r) const
{
	const auto index = uint32_t(r) / 2;
	if (index >= m_registers.size())
		return 0;

	return m_registers[index];
}

uint16_t am::Amiga::UpdateFlagRegister(am::Register r, uint16_t value)
{
	auto& reg = Reg(r);
	if ((value & 0x8000) != 0)
	{
		reg |= (value & 0x7fff);
	}
	else
	{
		reg &= ~(value & 0x7fff);
	}

	return reg;
}

uint16_t am::Amiga::ReadRegister(uint32_t regNum)
{
	auto regIndex = regNum / 2;
	if (regIndex >= _countof(registerInfo))
		return 0x0000;

	const auto& regInfo = registerInfo[regIndex];
	assert(regInfo.addr == (regNum & ~1));

	if (regInfo.type != RegType::ReadOnly)
	{
		return 0x0000;
	}

	if (m_breakOnRegisterEnabled && regNum == m_breakAtRegister)
	{
		m_breakAtNextInstruction = true;
	}

	const auto value = m_registers[regIndex];
	return value;
}

void am::Amiga::WriteRegister(uint32_t regNum, uint16_t value)
{
	auto regIndex = regNum / 2;
	if (regIndex >= _countof(registerInfo))
		return;

	const auto& regInfo = registerInfo[regIndex];
	assert(regInfo.addr == (regNum & ~1));

	if (regInfo.type != RegType::WriteOnly && regInfo.type != RegType::Strobe)
		return;

	if (m_breakOnRegisterEnabled && regNum == m_breakAtRegister)
	{
		m_breakAtNextInstruction = true;
	}

	m_registers[regIndex] = value;

	switch (am::Register(regNum & ~1))
	{

	case am::Register::DMACON:
		UpdateFlagRegister(Register::DMACONR, value & 0x87ff);
		break;

	case am::Register::INTENA:
		UpdateFlagRegister(am::Register::INTENAR, value);
		break;

	case am::Register::INTREQ:
		UpdateFlagRegister(am::Register::INTREQR, value);
		break;

	case am::Register::COLOR00:
	case am::Register::COLOR01:
	case am::Register::COLOR02:
	case am::Register::COLOR03:
	case am::Register::COLOR04:
	case am::Register::COLOR05:
	case am::Register::COLOR06:
	case am::Register::COLOR07:
	case am::Register::COLOR08:
	case am::Register::COLOR09:
	case am::Register::COLOR10:
	case am::Register::COLOR11:
	case am::Register::COLOR12:
	case am::Register::COLOR13:
	case am::Register::COLOR14:
	case am::Register::COLOR15:
	case am::Register::COLOR16:
	case am::Register::COLOR17:
	case am::Register::COLOR18:
	case am::Register::COLOR19:
	case am::Register::COLOR20:
	case am::Register::COLOR21:
	case am::Register::COLOR22:
	case am::Register::COLOR23:
	case am::Register::COLOR24:
	case am::Register::COLOR25:
	case am::Register::COLOR26:
	case am::Register::COLOR27:
	case am::Register::COLOR28:
	case am::Register::COLOR29:
	case am::Register::COLOR30:
	case am::Register::COLOR31:
	{
		m_registers[regIndex] &= 0x0fff;
		uint32_t r = (value & 0x0f00) >> 8;
		uint32_t g = (value & 0x00f0) >> 4;
		uint32_t b = (value & 0x000f) >> 0;
		const auto colourIndex = (regNum - uint32_t(Register::COLOR00)) / 2;
		auto rr = r | (r << 4);
		auto gg = g | (g << 4);
		auto bb = b | (b << 4);
		m_palette[colourIndex] = MakeColourRef(rr, gg, bb);

		// Halve the brightness
		rr = (rr >> 1) & 0xf7;
		gg = (gg >> 1) & 0xf7;
		bb = (bb >> 1) & 0xf7;
		m_palette[colourIndex + 32] = MakeColourRef(rr, gg, bb);
	}
	break;

	// TODO : Implement Register behaviour

	default:
		if (m_breakOnRegisterEnabled && m_breakAtRegister == 0xffff'ffff)
		{
			// Special value for breaking at unimplemented registers
			m_breakAtNextInstruction = true;
		}
		break;
	}
}
