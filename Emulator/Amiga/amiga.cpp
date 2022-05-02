#include "amiga.h"

#include "registers.h"

#include "util/endian.h"
#include "util/platform.h"

#include <cassert>


namespace
{
	// NTSC line length is actually 227.5 this is represented as an alternating lines of length of 227/228
	// PAL line length is a constant 227
	constexpr int kNTSC_longLineLength = 228;
	constexpr int kNTSC_shortLineLength = 227;
	constexpr int kPAL_lineLength = 227;

	constexpr int kNTSC_longFrameLines = 263;
	constexpr int kNTSC_shortFrameLines = 262;
	constexpr int kPAL_longFrameLines = 313;
	constexpr int kPAL_shortFrameLines = 312;

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
	: m_lastScreen(new ScreenBuffer)
	, m_currentScreen(new ScreenBuffer)
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

	m_m68000 = std::make_unique<cpu::M68000>(this);

	Reset();
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
	m_breakAtLine = false;
}

void am::Amiga::EnableBreakOnRegister(uint32_t regAddr)
{
	m_breakAtRegister = regAddr;
	m_breakOnRegisterEnabled = true;
}

void am::Amiga::EnableBreakOnCopper()
{
	m_breakAtNextCopperInstruction = true;
}

void am::Amiga::DisableBreakOnRegister()
{
	m_breakOnRegisterEnabled = false;
}

void am::Amiga::EnableBreakOnLine(uint32_t lineNum)
{
	m_breakAtLine = true;
	m_breakAtLineNum = lineNum;
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
		else if (type == Mapped::ChipRegisters)
		{
			if ((addr & 0x03f000) == 0x03f000)
			{
				const auto regNum = addr & 0x000fff;
				uint16_t wordread = ReadRegister(regNum);
				if (addr & 0x1)
				{
					// Return least significant byte
					return uint8_t(wordread & 0xff);
				}
				else
				{
					// Return most significant byte
					return uint8_t(wordread >> 8);
				}
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

uint16_t am::Amiga::ReadChipWord(uint32_t addr) const
{
	const uint32_t chipRamMask = uint32_t(m_chipRam.size()) - 1;
	uint16_t value;
	memcpy(&value, &m_chipRam[addr & chipRamMask], 2);
	return SwapEndian(value);
}

void am::Amiga::WriteChipWord(uint32_t addr, uint16_t value)
{
	const uint32_t chipRamMask = uint32_t(m_chipRam.size()) - 1;
	value = SwapEndian(value);
	memcpy(&m_chipRam[addr & chipRamMask], &value, 2);
}

bool am::Amiga::ExecuteFor(uint64_t cclocks)
{
	const auto runTill = m_totalCClocks + cclocks;

	m_running = true;

	while (m_running && m_totalCClocks < runTill)
	{
		DoOneTick();
	}
	return m_running;
}

bool am::Amiga::ExecuteOneCpuInstruction()
{
	while (!CpuReady())
	{
		DoOneTick();
	}

	m_breakAtNextInstruction = false;
	while (m_m68000->GetExecutionState() == cpu::ExecuteState::ReadyToDecode)
	{
		DoOneTick();
	}

	while (!CpuReady() || m_m68000->GetExecutionState() == cpu::ExecuteState::ReadyToExecute)
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

void am::Amiga::DoOneTick()
{
	bool chipBusBusy = DoScanlineDma();

	const bool evenClock = (m_hPos & 1) == 0;
	if (evenClock)
	{
		DoCopper(chipBusBusy);
	}

	if (m_m68000->GetExecutionState() == cpu::ExecuteState::ReadyToDecode && CpuReady())
	{
		if (m_breakAtNextInstruction || (m_breakpointEnabled && m_m68000->GetPC() == m_breakpoint))
		{
			m_breakAtNextInstruction = false;
			m_running = false;
			return;
		}

		if (!m_m68000->DecodeOneInstruction(m_cpuBusyTimer))
		{
			m_running = false;
		}
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
		if (!m_m68000->ExecuteOneInstruction(m_cpuBusyTimer))
		{
			m_running = false;
		}
	}

	UpdateScreen();

	if (m_blitterCountdown > 0)
	{
		--m_blitterCountdown;
		if (m_blitterCountdown == 0)
		{
			auto& dmaconr = Reg(Register::DMACONR);
			dmaconr &= ~0x4000; // Clear BBUSY flag
			// Signal blitter interrupt
			WriteRegister(uint32_t(am::Register::INTREQ), 0x8040);
		}
	}

	m_timerCountdown--;
	if (m_timerCountdown == 0)
	{
		// timers update at 1/10 CPU frequency
		TickCIATimers();
		m_timerCountdown = 5;
	}

	// Update Keyboard
	if (m_keyCooldown > 0)
	{
		m_keyCooldown--;
	}
	else if (m_keyQueueBack != m_keyQueueFront)
	{
		TransmitKeyCode();
	}

	// Update Floppy
	if (m_driveSelected != -1 && IsDiskInserted(m_driveSelected))
	{
		if (m_floppyDrive[m_driveSelected].motorOn)
		{
			m_diskRotationCountdown--;
			if (m_diskRotationCountdown == 0)
			{
				// Each revolution of the disk sets the CIAB flag signal
				SetCIAInterrupt(m_cia[1], 0x10);
				m_diskRotationCountdown = 700000;
			}
		}
	}

	m_totalCClocks++;
	m_hPos++;

	auto& vhposr = Reg(Register::VHPOSR);

	if (m_hPos == m_lineLength)
	{
		m_hPos = 0;

		if (m_isNtsc)
		{
			m_lineLength ^= 0b111; // Flip the last three bits to alternate between line lengths of 227/228
		}
		else
		{
			m_lineLength = kPAL_lineLength;
		}

		if (DmaEnabled(Dma::BPLEN) && m_vPos >= m_windowStartY && m_vPos < m_windowStopY)
		{
			// Apply Modulos for the bitplane pointers.
			// TODO : figure out the exact conditions for these
			auto bpl1mod = int16_t(Reg(Register::BPL1MOD) & 0xfffe);
			auto bpl2mod = int16_t(Reg(Register::BPL2MOD) & 0xfffe);
			for (int i = 0; i < m_bitplane.numPlanesEnabled; i++)
			{
				m_bitplane.ptr[i] += (i & 1) ? bpl2mod : bpl1mod;
			}
		}

		m_vPos++;
		if (m_vPos == m_frameLength)
		{
			if (!m_bitplane.externalResync)
			{
				TickCIAtod(0);

				// Send the V-Blank interrupt
				WriteRegister(uint32_t(am::Register::INTREQ), 0x8020);

				// Reset the copper
				WriteRegister(uint32_t(am::Register::COPJMP1), 0);
			}

			m_vPos = 0;
			if (m_bitplane.interlaced)
			{
				m_frameLength ^= 0b1; // Flip the last bit to alternate between frames of 262/263(NTSC) or 312/313(PAL) lines
			}
			else
			{
				// TODO : frame length can apparently be forced by modifying the LOF bit in the VPOSW register
				m_frameLength |= 0b1; // In non-interlaced mode, make sure we are always on a long frame (odd number of lines)
			}
		}

		if (!m_bitplane.externalResync)
		{
			TickCIAtod(1);

			auto& vposr = Reg(Register::VPOSR);

			vhposr = (m_vPos & 0xff) << 8;
			vposr = m_vPos >> 8;
			const bool isLongFrame = (m_frameLength & 1) != 0;
			vposr |= (isLongFrame ? 0x8000 : 0);
		}

		if (m_breakAtLine && m_vPos == m_breakAtLineNum)
		{
			m_breakAtLine = false;
			m_running = false;
		}
	}

	if (!m_bitplane.externalResync)
	{
		vhposr &= 0xff00;
		vhposr |= m_hPos;
	}
}

void am::Amiga::Reset()
{
	::memset(m_registers.data(), 0, m_registers.size() * sizeof(uint16_t));

	::memset(m_chipRam.data(), 0, m_chipRam.size());
	::memset(m_slowRam.data(), 0, m_slowRam.size());

	m_palette.fill(ColourRef(0));

	m_bitplane = {};

	m_vPos = 0;
	m_hPos = 0;
	m_lineLength = m_isNtsc ? kNTSC_shortLineLength : kPAL_lineLength;
	m_frameLength = m_isNtsc ? kNTSC_longFrameLines : kPAL_longFrameLines;

	m_windowStartX = 0;
	m_windowStopX = 0;
	m_windowStartY = 0;
	m_windowStopY = 0;

	m_copper = {};

	m_blitter = {};
	m_blitterCountdown = 0;

	m_cia[0] = {};
	m_cia[1] = {};

	m_timerCountdown = 3; // random init value

	// Enable ROM overlay (bit 0)
	// Start with CHNG flag (bit 2) low (disk not present)
	// All other bits held high (inactive)
	m_cia[0].pra |= 0b1111'1011;
	m_romOverlayEnabled = true;

	m_sharedBusRws = 0;
	m_exclusiveBusRws = 0;
	m_totalCClocks = 0;

	m_currentScreen->fill(0);
	m_lastScreen->fill(0);

	m_pixelBufferLoadPtr = 0;
	m_pixelBufferReadPtr = 0;

	for (int i = 0; i < 4; i++)
	{
		m_floppyDrive[i] = {};
	}
	m_driveSelected = -1;
	m_diskRotationCountdown = 0;

	m_diskDma = {};

	m_m68000->Reset(m_cpuBusyTimer);

	m_keyQueueFront = 0;
	m_keyQueueBack = 0;
	m_keyCooldown = 0;
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

		if (num == 1)
		{
			ProcessDriveCommands(data);
		}
	}	break;

	case 0x2: // ddra
		cia.ddra = data;
		break;

	case 0x3: // ddrb
		cia.ddrb = data;
		break;

	case 0x4: // timer A LSB
	{
		cia.timer[0].SetLSB(data);
	}	break;

	case 0x5: // timer A MSB
	{
		cia.timer[0].SetMSB(data);
	}	break;

	case 0x6: // timer B LSB
	{
		cia.timer[1].SetLSB(data);
	}	break;

	case 0x7: // timer B MSB
	{
		cia.timer[1].SetMSB(data);
	}	break;

	case 0x8: // tod LSB
	{
		if (cia.todWriteAlarm)
		{
			cia.todAlarm &= 0x00ffff00;
			cia.todAlarm |= uint32_t(data);
		}
		else
		{
			cia.tod &= 0x00ffff00;
			cia.tod |= uint32_t(data);
			cia.todRunning = true;
		}
	}	break;

	case 0x9: // tod middle byte
	{
		if (cia.todWriteAlarm)
		{
			cia.todAlarm &= 0x00ff00ff;
			cia.todAlarm |= (uint32_t(data) << 8);
		}
		else
		{
			cia.todRunning = false;
			cia.tod &= 0x00ff00ff;
			cia.tod |= (uint32_t(data) << 8);
		}
	}	break;

	case 0xa: // tod MSB
	{
		if (cia.todWriteAlarm)
		{
			cia.todAlarm &= 0x0000ffff;
			cia.todAlarm |= (uint32_t(data) << 16);
		}
		else
		{
			cia.todRunning = false;
			cia.tod &= 0x0000ffff;
			cia.tod |= (uint32_t(data) << 16);
		}
	}	break;

	case 0xd:
	{
		if ((data & 0x80) != 0)
		{
			cia.irqMask |= (data & 0x7f);
		}
		else
		{
			cia.irqMask &= ~data;
		}
		// TODO : Should updating the mask fire existing interrupts? (HRM implies no)
	}	break;

	case 0xe: // Control Register A
	{
		cia.timer[0].ConfigTimerCIA(data);
	}	break;

	case 0xf: // Control Register B
	{
		cia.timer[1].ConfigTimerCIA(data);
		cia.todWriteAlarm = (cia.timer[1].controlRegister & 0x80) != 0;
		cia.timerBCountsUnderflow = (cia.timer[1].controlRegister & 0x40) != 0;
	}	break;

	default:
		// TODO : other registers
		break;

	}
}
void am::CIA::Timer::ConfigTimerCIA(uint8_t data)
{
	controlRegister = data;

	running = (controlRegister & 0x01) != 0;
	continuous = (controlRegister & 0x08) == 0;

	if ((controlRegister & 0x10) != 0) // force load strobe bit
	{
		controlRegister &= ~0x10;
		value = latchedValue;
	}
}

void am::CIA::Timer::SetLSB(uint8_t data)
{
	latchedValue &= 0xff00;
	latchedValue |= uint16_t(data);
}

void am::CIA::Timer::SetMSB(uint8_t data)
{
	latchedValue &= 0x00ff;
	latchedValue |= uint16_t(data) << 8;
	if (!running && !continuous)
	{
		value = latchedValue;
		controlRegister |= 0x01;
		running = true;
	}
}

bool am::CIA::Timer::Tick()
{
	if (running)
	{
		if (value == 0)
		{
			value = latchedValue;
			if (!continuous)
			{
				running = false;
				controlRegister &= ~0x01;
			}

			return true;
		}
		else
		{
			value--;
		}
	}
	return false;
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

	case 0x4: // timer A LSB
		return uint8_t(cia.timer[0].value & 0xff);

	case 0x5: // timer A MSB
		return uint8_t((cia.timer[0].value >> 8) & 0xff);

	case 0x6: // timer B LSB
		return uint8_t(cia.timer[1].value & 0xff);

	case 0x7: // timer B MSB
		return uint8_t((cia.timer[1].value >> 8) & 0xff);

	case 0x8: // tod LSB
	{
		uint8_t value;
		if (cia.todIsLatched)
		{
			value = uint8_t(cia.todLatched & 0xff);
			cia.todIsLatched = false;
		}
		else
		{
			value = uint8_t(cia.tod & 0xff);
		}
		return value;
	}

	case 0x9: // tod middle byte
	{
		if (cia.todIsLatched)
		{
			return uint8_t((cia.todLatched & 0x0000ff00) >> 8);
		}
		else
		{
			return uint8_t((cia.tod & 0x0000ff00) >> 8);
		}
	}

	case 0xa: // tod MSB
	{
		if (!cia.todIsLatched)
		{
			cia.todIsLatched = true;
			cia.todLatched = cia.tod;
		}
		return uint8_t((cia.todLatched & 0x00ff0000) >> 16);
	}

	case 0xc:
	{
		// serial data register (keyboard input on CIAA)
		return cia.sdr;
	}

	case 0xd:
	{
		uint8_t irqs = cia.irqData;
		cia.irqData = 0;
		cia.intSignal = false;
		DoInterruptRequest();
		return irqs;
	}

	case 0xe:
		return cia.timer[0].controlRegister;

	case 0xf:
		return cia.timer[1].controlRegister;

	default:
		// TODO : other registers
		return 0;
	}
}

void am::Amiga::SetCIAInterrupt(CIA& cia, uint8_t bit)
{
	cia.irqData |= bit;
	if ((cia.irqMask & bit) != 0)
	{
		cia.irqData |= 0x80;
		cia.intSignal = true;
		DoInterruptRequest();
	}
}

void am::Amiga::TickCIAtod(int num)
{
	auto& cia = m_cia[num];

	if (!cia.todRunning)
		return;

	cia.tod++;
	cia.tod &= 0x00ffffff; // only 24 bits available

	if (cia.tod == cia.todAlarm)
	{
		SetCIAInterrupt(cia, 0x04);
	}
}

void am::Amiga::TickCIATimers()
{
	for (int i = 0; i < 2; i++)
	{
		bool tickTimerB = !m_cia[i].timerBCountsUnderflow;

		if (m_cia[i].timer[0].Tick())
		{
			tickTimerB = true;
			SetCIAInterrupt(m_cia[i], 0x01);
		}

		if (tickTimerB && m_cia[i].timer[1].Tick())
		{
			SetCIAInterrupt(m_cia[i], 0x02);
		}
	}
}

void am::Amiga::SetControllerButton(int controller, int button, bool pressed)
{
	switch (button)
	{

	case 0: // left mouse button / fire 1
	{
		uint8_t bit = 1u << ((controller == 0) ? 6 : 7);

		if (pressed)
		{
			m_cia[0].pra &= ~bit;
		}
		else
		{
			m_cia[0].pra |= bit;
		}
	}	break;

	case 1: // right mouse button / fire 2
		// TODO
		break;

	case 2: // middle mouse button / fire 3?
		// TODO
		break;

	default:
		break;
	}
}

void am::Amiga::SetJoystickMove(int x, int y)
{
	auto& joy1dat = Reg(Register::JOY1DAT);

	const bool left = (x != -1);
	const bool right = (x != 1);
	const bool up = (y != -1);
	const bool down = (y != 1);

	joy1dat = 0;

	if (!right)
		joy1dat |= 0x0002;
	if (right ^ down)
		joy1dat |= 0x0001;

	if (!left)
		joy1dat |= 0x0200;
	if (left ^ up)
		joy1dat |= 0x0100;
}

void am::Amiga::QueueKeyPress(uint8_t keycode)
{
	if ((m_keyQueueBack + 1) % kKeyQueueSize == m_keyQueueFront)
		return; // key buffer full

	m_keyQueue[m_keyQueueBack] = ~((keycode << 1) | ((keycode & 0x80) >> 7));
	m_keyQueueBack = (m_keyQueueBack + 1) % kKeyQueueSize;
}

void am::Amiga::TransmitKeyCode()
{
	m_cia[0].sdr = m_keyQueue[m_keyQueueFront];
	m_keyQueueFront = (m_keyQueueFront + 1) % kKeyQueueSize;
	SetCIAInterrupt(m_cia[0], 0x08);
	m_keyCooldown = 1715;
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

	case am::Register::DSKPTH:
	case am::Register::DSKPTL:
		SetLongPointerValue(m_diskDma.ptr, (regIndex & 1) == 0, value);
		break;

	case am::Register::DSKLEN:
	{
		// Writing to this register will start the DMA if enabled bit is already set.
		const bool currentlyEnabled = m_diskDma.secondaryDmaEnabled;

		m_diskDma.secondaryDmaEnabled = (value & 0x8000) != 0;
		m_diskDma.writing = (value & 0x4000) != 0;
		m_diskDma.len = (value & 0x3fff);
		m_diskDma.inProgress = false;

		if (currentlyEnabled && m_diskDma.secondaryDmaEnabled)
		{
			StartDiskDMA();
		}
	}	break;

	case am::Register::SERPER:
		// Serial port curretly not emulated but silently accept
		// control values to this register.
		break;

	case am::Register::BLTCON0:
	case am::Register::BLTCON1:
	case am::Register::BLTAFWM:
	case am::Register::BLTALWM:
		break;

	case am::Register::BLTCPTH:
	case am::Register::BLTCPTL:
	case am::Register::BLTBPTH:
	case am::Register::BLTBPTL:
	case am::Register::BLTAPTH:
	case am::Register::BLTAPTL:
	case am::Register::BLTDPTH:
	case am::Register::BLTDPTL:
	{
		auto ptr = (regNum - uint32_t(Register::BLTCPTH)) / 4;
		SetLongPointerValue(m_blitter.ptr[ptr], (regIndex & 1) == 0, value);
	}	break;

	case am::Register::BLTSIZE:
		DoInstantBlitter();
		break;

	case am::Register::BLTCMOD:
	case am::Register::BLTBMOD:
	case am::Register::BLTAMOD:
	case am::Register::BLTDMOD:
	case am::Register::BLTCDAT:
	case am::Register::BLTBDAT:
	case am::Register::BLTADAT:
		break;

	case am::Register::DSKSYNC:
		break;

	case am::Register::COP1LCH:
	case am::Register::COP1LCL:
	case am::Register::COP2LCH:
	case am::Register::COP2LCL:
		break;

	case am::Register::COPJMP1:
	{
		m_copper.pc = uint32_t(Reg(Register::COP1LCH) & 0b00000000'00011111) << 16
			| Reg(Register::COP1LCL);
		if (m_copper.state == CopperState::Stopped || m_copper.state == CopperState::Waiting)
		{
			m_copper.state = CopperState::Read;
			if (m_breakAtNextCopperInstruction)
			{
				m_breakAtNextCopperInstruction = false;
				m_running = false;
			}
		}
		else if (m_copper.state == CopperState::WaitSkip)
		{
			m_copper.state = CopperState::Abort;
		}
	}	break;

	case am::Register::COPJMP2:
	{
		m_copper.pc = uint32_t(Reg(Register::COP2LCH) & 0b00000000'00011111) << 16
			| Reg(Register::COP2LCL);
		if (m_copper.state == CopperState::Stopped || m_copper.state == CopperState::Waiting)
		{
			m_copper.state = CopperState::Read;
			if (m_breakAtNextCopperInstruction)
			{
				m_breakAtNextCopperInstruction = false;
				m_running = false;
			}
		}
		else if (m_copper.state == CopperState::WaitSkip)
		{
			m_copper.state = CopperState::Abort;
		}
	}	break;

	case am::Register::DIWSTRT:
		m_windowStartX = value & 0x00ff;
		m_windowStartY = (value & 0xff00) >> 8;
		break;

	case am::Register::DIWSTOP:
		m_windowStopX = (value & 0x00ff) | 0x0100;
		m_windowStopY = (value & 0xff00) >> 8;
		m_windowStopY |= (((~m_windowStopY) & 0x80u) << 1u);
		break;

	case am::Register::DDFSTRT:
	case am::Register::DDFSTOP:
		break;

	case am::Register::DMACON:
		UpdateFlagRegister(am::Register::DMACONR, value & 0x87ff);
		break;

	case am::Register::INTENA:
		UpdateFlagRegister(am::Register::INTENAR, value);
		DoInterruptRequest();
		break;

	case am::Register::INTREQ:
		UpdateFlagRegister(am::Register::INTREQR, value);
		DoInterruptRequest();
		break;

	case am::Register::ADKCON:
	{
		auto adkconr = UpdateFlagRegister(Register::ADKCONR, value);
		m_diskDma.useWordSync = (adkconr & 0x0400) != 0;
	}	break;

	case am::Register::BPL1PTH:
	case am::Register::BPL1PTL:
	case am::Register::BPL2PTH:
	case am::Register::BPL2PTL:
	case am::Register::BPL3PTH:
	case am::Register::BPL3PTL:
	case am::Register::BPL4PTH:
	case am::Register::BPL4PTL:
	case am::Register::BPL5PTH:
	case am::Register::BPL5PTL:
	case am::Register::BPL6PTH:
	case am::Register::BPL6PTL:
	{
		const auto planeIdx = (regNum - uint32_t(Register::BPL1PTH)) / 4;
		const auto highWord = (regNum & 0b010) == 0;
		SetLongPointerValue(m_bitplane.ptr[planeIdx], highWord, value);
	}	break;

	case am::Register::BPLCON0:
		m_bitplane.hires = ((value & 0x8000) != 0);
		m_bitplane.numPlanesEnabled = (value & 0x7000) >> 12;
		m_bitplane.ham = ((value & 0x0800) != 0);
		m_bitplane.doublePlayfield = (value & 0x0400) != 0;
		m_bitplane.compositeColourEnabled = ((value & 0x0200) != 0);
		m_bitplane.genlockAudioEnabled = ((value & 0x0100) != 0);
		m_bitplane.lightPenEnabled = ((value & 0x0008) != 0);
		m_bitplane.interlaced = ((value & 0x0004) != 0);
		m_bitplane.externalResync = (value & 0x0002) != 0;
		break;

	case am::Register::BPLCON1:
		m_bitplane.playfieldDelay[0] = (value & 0x000f) >> 0;
		m_bitplane.playfieldDelay[1] = (value & 0x00f0) >> 4;
		break;

	case am::Register::BPLCON2:
		m_bitplane.playfieldPriority = (value >> 6) & 1;
		m_bitplane.playfieldSpritePri[0] = value & 7;
		m_bitplane.playfieldSpritePri[1] = (value >> 3) & 7;
		break;

	/// ECS only register - leave unimplemented for now
	//case am::Register::BPLCON3:
	//	break;

	case am::Register::BPL1MOD:
	case am::Register::BPL2MOD:
		break;

	case am::Register::BPL1DAT:
	{
		memset(m_pixelBuffer.data() + m_pixelBufferLoadPtr, 0, 16);

		for (int i = 0; i < m_bitplane.numPlanesEnabled; i++)
		{
			auto bits = Reg(Register(int(Register::BPL1DAT) + (i * 2)));

			for (int j = 0; j < 16; j++)
			{
				m_pixelBuffer[m_pixelBufferLoadPtr + j] |= (bits & (32768u >> j)) != 0 ? (1 << i) : 0;
			}
		}

		m_pixelBufferReadPtr = (m_pixelBufferLoadPtr - (m_bitplane.hires ? 24 : 12)) & kPixelBufferMask; // TODO: apply horizontal delay
		m_pixelBufferLoadPtr = (m_pixelBufferLoadPtr + 16) & kPixelBufferMask;

	}	break;

	case am::Register::BPL2DAT:
	case am::Register::BPL3DAT:
	case am::Register::BPL4DAT:
	case am::Register::BPL5DAT:
	case am::Register::BPL6DAT:
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

		// Reduce brightness by 50%
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

void am::Amiga::DoInterruptRequest()
{
	// The interrupt request or mask registers have potentially been altered.
	// So see if we need to inform the CPU of an interrupt request (and at which level).

	// Also check if either CIA chip is signalling an interrupt

	auto& intreqr = Reg(Register::INTREQR);
	auto& intenar = Reg(Register::INTENAR);

	if (m_cia[0].intSignal) // CIAA
	{
		intreqr |= 0x0008;
	}
	else
	{
		intreqr &= ~0x0008;
	}

	if (m_cia[1].intSignal) // CIAB
	{
		intreqr |= 0x2000;
	}
	else
	{
		intreqr &= ~0x2000;
	}

	if ((intenar & 0x4000) == 0)
	{
		// master interrupt enabled bit not set so no interrupts.
		m_m68000->SetInterruptControl(0);
		return;
	}

	const uint16_t interrupts = intreqr & intenar;

	// Bits in 'interrupts' are the currently outstanding enabled interrupts.
	// Find the highest level one and set to that.
	int msb = 0;
	auto temp = interrupts;
	while(temp)
	{
		temp >>= 1;
		msb++;
	}

	static const uint8_t systemToProcessorIntLevel[15] =
	{
		0, 1, 1, 1, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6
	};

	m_m68000->SetInterruptControl(systemToProcessorIntLevel[msb]);
}

bool am::Amiga::DmaEnabled(am::Dma dmaChannel) const
{
	const auto dmaconr = Reg(Register::DMACONR);
	const uint16_t enabled = uint16_t(dmaChannel) | uint16_t(Dma::DMAEN);

	return (dmaconr & enabled) == enabled;
}

void am::Amiga::DoCopper(bool& chipBusBusy)
{
	switch (m_copper.state)
	{
	case CopperState::Stopped:
		break;

	case CopperState::Waiting:
	{
		const auto verticalComparePosition = uint16_t(m_vPos) & m_copper.verticalMask;
		const auto horizontalComparePosition = uint16_t(m_hPos) & m_copper.horizontalMask;

		if (verticalComparePosition > (m_copper.verticalWaitPos & m_copper.verticalMask) ||
			(verticalComparePosition == (m_copper.verticalWaitPos & m_copper.verticalMask) && horizontalComparePosition >= (m_copper.horizontalWaitPos & m_copper.horizontalMask)))
		{
			// TODO : check blitter finished also if required.
			m_copper.state = CopperState::WakeUp;
		}

	}	break;

	case CopperState::Read:
	{
		if (DmaEnabled(Dma::COPEN) && !chipBusBusy)
		{
			m_copper.readAddr = m_copper.pc;
			m_copper.pc += 4;
			m_copper.ir1 = ReadChipWord(m_copper.readAddr);
			chipBusBusy = true;
			m_copper.readAddr += 2;

			if ((m_copper.ir1 & 1) == 0) // Move
			{
				m_copper.state = CopperState::Move;
			}
			else
			{
				m_copper.state = CopperState::WaitSkip;
			}

		}
	}	break;

	case CopperState::Move:
	{
		if (!chipBusBusy)
		{
			m_copper.ir2 = ReadChipWord(m_copper.readAddr);
			chipBusBusy = true;

			bool skipping = m_copper.skipping;
			m_copper.skipping = false;

			const auto copcon = Reg(Register::COPCON);
			const bool danger = (copcon & 0x02) != 0; // bit 01 is 'danger' bit

			const auto reg = m_copper.ir1 & 0x01ff;

			if (reg < 0x40 || (!danger && reg < 0x80))
			{
				// Based on information gleaned from forums,
				// attempting to move to a 'banned' register
				// stops the copper (even if skipping).
				m_copper.state = CopperState::Stopped;
			}
			else
			{
				if (!skipping)
				{
					WriteRegister(reg, m_copper.ir2);
				}
				m_copper.state = CopperState::Read;
			}

			if (m_breakAtNextCopperInstruction)
			{
				m_breakAtNextCopperInstruction = false;
				m_running = false;

			}
		}
	}	break;


	case CopperState::WaitSkip:
	{
		if (!chipBusBusy)
		{
			m_copper.ir2 = ReadChipWord(m_copper.readAddr);
			chipBusBusy = true;
			m_copper.skipping = false;

			const auto verticalWaitPos = uint16_t(m_copper.ir1 >> 8);
			const auto horizontalWaitPos = uint16_t(m_copper.ir1 & 0x00fe);

			const auto verticalMask = uint16_t(((m_copper.ir2 >> 8) & 0x7f) | 0x80);
			const auto horizontalMask = uint16_t(m_copper.ir2 & 0x00fe);

			if ((m_copper.ir2 & 1) == 0) // Wait
			{
				m_copper.waitForBlitter = (m_copper.ir2 & 0x8000) == 0;
				m_copper.verticalMask = verticalMask;
				m_copper.verticalWaitPos = verticalWaitPos;
				m_copper.horizontalMask = horizontalMask;
				m_copper.horizontalWaitPos = horizontalWaitPos;
				m_copper.state = CopperState::Waiting;
			}
			else // Skip
			{
				const auto verticalComparePos = uint16_t(m_vPos) & verticalMask;
				const auto horizontalComparePos = uint16_t(m_hPos) & horizontalMask;

				if (verticalComparePos < (verticalWaitPos & verticalMask) ||
					(verticalComparePos == (verticalWaitPos & verticalMask) && horizontalComparePos < (horizontalWaitPos & horizontalMask)))
				{
					m_copper.skipping = true;
				}
				m_copper.state = CopperState::Read;

				if (m_breakAtNextCopperInstruction)
				{
					m_breakAtNextCopperInstruction = false;
					m_running = false;
				}
			}
		}
	}	break;

	case CopperState::Abort:
	{
		if (!chipBusBusy)
		{
			// Happens when we're half way through executing a wait/skip instruction and an external source (CPU) strobes the
			// COPJMP* register. We must complete the fetching but not perform the wait/skip.

			m_copper.ir2 = ReadChipWord(m_copper.readAddr);
			chipBusBusy = true;
			m_copper.skipping = false;
			m_copper.state = CopperState::Read;
		}
	}	break;

	case CopperState::WakeUp:
	{
		m_copper.state = CopperState::Read;

		if (m_breakAtNextCopperInstruction)
		{
			m_breakAtNextCopperInstruction = false;
			m_running = false;
		}
	}	break;

	}
}

void am::Amiga::UpdateScreen()
{
	// early return for non-displayable lines
	if (m_vPos < 36 || m_vPos >= (m_isNtsc ? 253 : 309))
		return;

	// Messy logic to convert the beam counter to the actual screen position.
	// The screen position lags behind the beam counter and may therefore be
	// still on the previous line.

	int line = m_vPos;
	int xPos = m_hPos - 0x41;

	auto bufferLine = line - 36;

	if (xPos < -0x1c)
	{
		xPos += kPAL_lineLength;
		bufferLine--;
		line--;
	}
	else if (xPos == 0 && bufferLine == (m_isNtsc ? 216 : 272))
	{
		// Swap the screens immediately once the bottom line is finished.
		std::swap(m_currentScreen, m_lastScreen);
		return;
	}

	if (bufferLine < 0 || bufferLine >= (m_isNtsc ? 216 : 272))
		return;

	if (line >= m_windowStartY && line < m_windowStopY)
	{
		auto startIndex = m_windowStartX - 0x79;
		auto endIndex = m_windowStopX - 0x79;

		auto index = bufferLine * kScreenBufferWidth + (xPos * 4);

		if (m_bitplane.hires)
		{
			for (int x = 0; x < 4; x++)
			{
				int loresPixelPos = (xPos * 2 + x / 2);

				if (loresPixelPos >= 0 && loresPixelPos < 336)
				{
					if (loresPixelPos >= startIndex && loresPixelPos < endIndex)
					{
						auto value = m_pixelBuffer[(m_pixelBufferReadPtr + x) & kPixelBufferMask];

						(*m_currentScreen.get())[index++] = m_palette[value];
					}
					else
					{
						(*m_currentScreen.get())[index++] = m_palette[0];
					}
				}
				m_pixelBuffer[(m_pixelBufferReadPtr + x) & kPixelBufferMask] = 0;
			}
			m_pixelBufferReadPtr += 4;
		}
		else
		{
			for (int x = 0; x < 2; x++)
			{
				int loresPixelPos = (xPos * 2 + x);

				if (loresPixelPos >= 0 && loresPixelPos < 336)
				{
					if (loresPixelPos >= startIndex && loresPixelPos < endIndex)
					{
						auto value = m_pixelBuffer[(m_pixelBufferReadPtr + x) & kPixelBufferMask];

						(*m_currentScreen.get())[index++] = m_palette[value];
						(*m_currentScreen.get())[index++] = m_palette[value];
					}
					else
					{
						(*m_currentScreen.get())[index++] = m_palette[0];
						(*m_currentScreen.get())[index++] = m_palette[0];
					}
				}
				m_pixelBuffer[(m_pixelBufferReadPtr + x) & kPixelBufferMask] = 0;

			}
			m_pixelBufferReadPtr += 2;
		}
	}
	else
	{
		if (xPos >= 0 && xPos < 0xa8)
		{
			for (int x = 0; x < 4; x++)
			{
				auto index = bufferLine * kScreenBufferWidth + (xPos * 4);
				(*m_currentScreen.get())[index + x] = m_palette[0];
			}
		}
	}
}

namespace
{
	uint16_t DoBlitterFunction(uint8_t minterm, uint16_t a, uint16_t b, uint16_t c)
	{
		//////////////////////
		//	A	B	C		?
		//	0	0	0		1
		//	0	0	1		2
		//	0	1	0		4
		//	0	1	1		8
		//	1	0	0		16
		//	1	0	1		32
		//	1	1	0		64
		//	1	1	1		128

		uint16_t val = 0;

		if (minterm & 0x01)
		{
			val |= ~(a|b|c);
		}
		if (minterm & 0x02)
		{
			val |= ~(a|b) & c;
		}
		if (minterm & 0x04)
		{
			val |= ~(a|c) & b;
		}
		if (minterm & 0x08)
		{
			val |= b & c & ~a;
		}
		if (minterm & 0x10)
		{
			val |= a & ~(b|c);
		}
		if (minterm & 0x20)
		{
			val |= a & c & ~b;
		}
		if (minterm & 0x40)
		{
			val |= a & b & ~c;
		}
		if (minterm & 0x80)
		{
			val |= a & b & c;
		}

		return val;
	}
}

void am::Amiga::DoInstantBlitter()
{
	const auto con0 = Reg(Register::BLTCON0);
	const auto con1 = Reg(Register::BLTCON1);
	m_blitter.minterm = uint8_t(con0 & 0xff);

	const auto bltsize = Reg(Register::BLTSIZE);
	m_blitter.wordsPerLine = bltsize & 0x3f;
	if (m_blitter.wordsPerLine == 0)
		m_blitter.wordsPerLine = 0x40;

	m_blitter.lines = bltsize >> 6;
	if (m_blitter.lines == 0)
		m_blitter.lines = 0x400;

	m_blitter.modulo[0] = int16_t(Reg(Register::BLTCMOD) & 0xfffe);
	m_blitter.modulo[1] = int16_t(Reg(Register::BLTBMOD) & 0xfffe);
	m_blitter.modulo[2] = int16_t(Reg(Register::BLTAMOD) & 0xfffe);
	m_blitter.modulo[3] = int16_t(Reg(Register::BLTDMOD) & 0xfffe);

	m_blitter.data[0] = Reg(Register::BLTCDAT);
	m_blitter.data[1] = Reg(Register::BLTBDAT);
	m_blitter.data[2] = Reg(Register::BLTADAT);
	m_blitter.data[3] = Reg(Register::BLTDDAT);

	auto aShift = uint16_t((con0 >> 12) & 0x000f);
	auto bShift = uint16_t((con1 >> 12) & 0x000f);

	m_blitter.enabled[0] = (con0 & 0x0200) != 0;
	m_blitter.enabled[1] = (con0 & 0x0400) != 0;
	m_blitter.enabled[2] = (con0 & 0x0800) != 0;
	m_blitter.enabled[3] = (con0 & 0x0100) != 0;

	int blitClks = 0;

	auto& dmaconr = Reg(Register::DMACONR);

	dmaconr |= 0x2000; // Initialise BZERO to true

	if ((con1 & 1) == 0) // normal mode
	{
		bool descendingMode = (con1 & 0x02) != 0;

		if (descendingMode)
		{
			m_blitter.modulo[0] = -m_blitter.modulo[0];
			m_blitter.modulo[1] = -m_blitter.modulo[1];
			m_blitter.modulo[2] = -m_blitter.modulo[2];
			m_blitter.modulo[3] = -m_blitter.modulo[3];
		}

		m_blitter.firstWordMask = Reg(Register::BLTAFWM);
		m_blitter.lastWordMask = Reg(Register::BLTALWM);

		auto exclusiveFill = (con1 & 0x0010) != 0;
		auto inclusiveFill = (con1 & 0x0008) != 0;

		uint32_t addTo = descendingMode ? -2 : 2;

		uint16_t aShiftIn = 0;
		uint16_t bShiftIn = 0;

		uint16_t res;
		uint32_t resAddr;
		bool resQueued = false;

		for (auto l = 0; l < m_blitter.lines; l++)
		{
			for (auto w = 0; w < m_blitter.wordsPerLine; w++)
			{
				for (auto c = 0; c < 3; c++)
				{
					if (m_blitter.enabled[c])
					{
						m_blitter.data[c] = ReadChipWord(m_blitter.ptr[c] & 0xffff'fffe);
						++blitClks;
						m_blitter.ptr[c] += addTo;
					}
				}

				if (resQueued)
				{
					WriteChipWord(resAddr, res);
					++blitClks;
					resQueued = false;
				}

				uint16_t aData = m_blitter.data[2];

				if (w == 0)
				{
					aData &= m_blitter.firstWordMask;
				}
				if (w == m_blitter.wordsPerLine - 1)
				{
					aData &= m_blitter.lastWordMask;
				}

				uint16_t savedA = aData;

				if (descendingMode)
				{
					aData = ((uint32_t(aData) << 16 | aShiftIn) >> (16 - aShift)) & 0xffff;
				}
				else
				{
					aData = (((uint32_t(aShiftIn) << 16) | aData) >> aShift) & 0xffff;
				}
				aShiftIn = savedA;

				uint16_t bData = m_blitter.data[1];
				uint16_t savedB = bData;

				if (descendingMode)
				{
					bData = ((uint32_t(bData) << 16 | bShiftIn) >> (16 - bShift)) & 0xffff;
				}
				else
				{
					bData = (((uint32_t(bShiftIn) << 16) | bData) >> bShift) & 0xffff;
				}
				bShiftIn = savedB;

				res = DoBlitterFunction(m_blitter.minterm, aData, bData, m_blitter.data[0]);
				if (m_blitter.enabled[3])
				{
					resQueued = true;
					resAddr = m_blitter.ptr[3] & 0xffff'fffe;
					m_blitter.ptr[3] += addTo;
				}

				if (res != 0)
				{
					dmaconr &= ~0x2000;
				}
			}

			for (auto c = 0; c < 4; c++)
			{
				if (m_blitter.enabled[c])
				{
					m_blitter.ptr[c] += m_blitter.modulo[c];
				}
			}
		}

		if (resQueued)
		{
			WriteChipWord(resAddr, res);
			++blitClks;
		}
	}
	else // line mode
	{
		enum Dir { LEFT = 1, RIGHT = 2, UP = 4, DOWN = 8 };

		Dir maj_step[] = { Dir::DOWN,	Dir::UP,	Dir::DOWN,	Dir::UP,	Dir::RIGHT, Dir::LEFT,	Dir::RIGHT,	Dir::LEFT };
		Dir min_step[] = { Dir::RIGHT,	Dir::RIGHT, Dir::LEFT,	Dir::LEFT,	Dir::DOWN,	Dir::DOWN,	Dir::UP,	Dir::UP };

		int inc_majmin = m_blitter.modulo[2];
		int inc_maj = m_blitter.modulo[1];
		int acc = int16_t(m_blitter.ptr[2]);
		int octantCode = (con1 >> 2) & 7;
		int count = m_blitter.lines;

		bool dot_on_row = false;
		bool onedot = (con1 & 2) != 0;

		uint32_t aShiftIn = 0;
		uint32_t bShiftIn = uint32_t(m_blitter.data[1]) << 16;

		while (count)
		{
			if (m_blitter.enabled[0])
			{
				m_blitter.data[0] = ReadChipWord(m_blitter.ptr[0] & 0xffff'fffe);
				++blitClks;
			}

			if (!(onedot && dot_on_row))
			{
				uint16_t aData = m_blitter.data[2];
				uint32_t savedA = uint32_t(aData) << 16;

				aData = ((aShiftIn | aData) >> aShift) & 0xffff;
				aShiftIn = savedA;

				uint16_t bData = m_blitter.data[1];
				uint32_t savedB = uint32_t(bData) << 16;

				bData = ((bShiftIn | bData) >> bShift) & 0xffff;
				bShiftIn = savedB;

				auto res = DoBlitterFunction(m_blitter.minterm, aData, bData, m_blitter.data[0]);
				WriteChipWord(m_blitter.ptr[3] & 0xffff'fffe, res);
				++blitClks;

				dot_on_row = true;
			}

			int step = 0;

			if (acc < 0)
			{
				step = maj_step[octantCode];
				acc += inc_maj;
			}
			else
			{
				step = maj_step[octantCode] | min_step[octantCode];
				acc += inc_majmin;
			}

			if ((step & Dir::LEFT) != 0)
			{
				aShift = (aShift - 1) & 0xf;
				if (aShift == 0xf)
				{
					m_blitter.ptr[0] -= 2;
					m_blitter.ptr[3] -= 2;
				}
			}
			else if ((step & Dir::RIGHT) != 0)
			{
				aShift = (aShift + 1) & 0xf;
				if (aShift == 0)
				{
					m_blitter.ptr[0] += 2;
					m_blitter.ptr[3] += 2;
				}
			}

			if ((step & Dir::UP) != 0)
			{
				m_blitter.ptr[0] -= m_blitter.modulo[0];
				m_blitter.ptr[3] -= m_blitter.modulo[0];
				dot_on_row = false;
			}
			else if ((step & Dir::DOWN) != 0)
			{
				m_blitter.ptr[0] += m_blitter.modulo[0];
				m_blitter.ptr[3] += m_blitter.modulo[0];
				dot_on_row = false;
			}

			count--;
		}
	}

	if (blitClks > 0)
	{
		dmaconr |= 0x4000; // set BBUSY bit
		m_blitterCountdown = blitClks;
	}
}

bool am::Amiga::DoScanlineDma()
{
	const bool oddClock = (m_hPos & 1) != 0;

	if (m_hPos == m_lineLength - 1)
	{
		// There are 4 memory refresh cycles. the first appears at cycle -1
		// i.e. the last cycle of the previous line.
		return true;
	}

	if (m_hPos < 0x14)
	{
		// This range spans the memory refresh, disk DMA and Audio DMA
		// which cannot be overridden

		if (!oddClock)
			return false;

		if (m_hPos < 0x6)
			return true; // Memory refresh cycle

		if (m_hPos < 0xc)
		{
			if (DmaEnabled(Dma::DSKEN) && m_diskDma.inProgress)
			{
				DoDiskDMA();
				return true;
			}
		}
		else
		{
			// TODO: Audio DMA
		}

		return false;
	}

	// Sprites and display DMA only active on lines within the display window
	if (m_vPos < m_windowStartY || m_vPos >= m_windowStopY)
		return false;

	// Sprites can be wiped out by display DMA so check that first
	if (DmaEnabled(Dma::BPLEN))
	{
		auto ddfstrt = Reg(Register::DDFSTRT) & 0b0000000011111100;
		auto ddfstop = Reg(Register::DDFSTOP) & 0b0000000011111100;

		ddfstrt = std::max(ddfstrt, 0x18);
		ddfstop = std::min(ddfstop, 0xd8);

		const auto fetchEnd = ddfstop + (m_bitplane.hires ? 0xc : 0x8);

		if (m_hPos >= ddfstrt && m_hPos < fetchEnd)
		{
			// we are in the fetch zone

			static constexpr uint8_t kPlaneReadOrderLores[8] = { ~0, 3, 5, 1, ~0, 2, 4, 0 };
			static constexpr uint8_t kPlaneReadOrderHires[4] = { 3, 1, 2, 0 };

			int bp = m_bitplane.hires
				? kPlaneReadOrderHires[m_hPos & 0x03]
				: kPlaneReadOrderLores[m_hPos & 0x07];

			if (bp < m_bitplane.numPlanesEnabled)
			{
				const auto bplData = ReadChipWord(m_bitplane.ptr[bp]);
				m_bitplane.ptr[bp] += 2;
				WriteRegister(uint32_t(Register::BPL1DAT) + (bp << 1), bplData);
				return true;
			}
		}
	}

	return false;
}

const std::string& am::Amiga::GetDisk(int driveNum) const
{
	assert(driveNum >= 0 && driveNum < 4);
	return m_floppyDisk[driveNum].fileId;
}

bool am::Amiga::SetDisk(int driveNum, const std::string& fileId, std::vector<uint8_t>&& data)
{
	assert(driveNum >= 0 && driveNum < 4);

	if (fileId.empty() || data.empty())
		return false;

	auto& disk = m_floppyDisk[driveNum];

	disk.fileId = fileId;
	disk.data = std::move(data);

	EncodeDiskImage(disk.data, disk.image);

	return true;
}

void am::Amiga::EjectDisk(int driveNum)
{
	assert(driveNum >= 0 && driveNum < 4);

	if (IsDiskInserted(driveNum))
	{
		auto& disk = m_floppyDisk[driveNum];
		disk.fileId.clear();
		disk.data.clear();
		disk.image.clear();

		UpdateFloppyDriveFlags();
	}
}

void am::Amiga::ProcessDriveCommands(uint8_t data)
{
	bool driveSelected = false;

	// Cannot find documentation about what happens when more than one drive is selected at once. I'm assuming all well-behaved
	// software will not do this. For now I will assume that the lowest-numbered enabled drive is the selected one.

	bool step = (data & 0x01) == 0;
	bool directionInwards = (data & 0x02) == 0;
	bool side = (data & 0x04) == 0;

	for (int i = 0; i < 4; i++)
	{
		auto& drive = m_floppyDrive[i];

		if ((data & (0x1 << (i + 3))) == 0)
		{
			if (driveSelected)
			{
				// selected multiple drives? how to handle this?
				_ASSERTE(false);
			}
			else
			{
				m_driveSelected = i;
				driveSelected = true;
			}

			if (!drive.selected)
			{
				// If selection has just changed

				drive.motorOn = (data & 0x80) == 0; // latched on select (active low)

				if (drive.motorOn)
				{
					m_diskRotationCountdown = 700000;
				}

				// Set step signal to off for this drive so that it can be registered if set at same time as selection.
				drive.stepSignal = false;
			}

			drive.selected = true;
			// TODO : what else needs to be set?
		}
		else
		{
			drive.selected = false;
		}
	}

	if (!driveSelected)
	{
		m_driveSelected = -1; // no drive is selected
	}
	else
	{
		auto& drive = m_floppyDrive[m_driveSelected];

		if (step && !drive.stepSignal)
		{
			// step signal has gone low - do a step

			// Update the DSKCHANGE flag - low when no disk is in drive.
			if (IsDiskInserted(m_driveSelected))
			{
				drive.diskChange = true;
			}

			if (directionInwards)
			{
				if ((drive.currCylinder + 1) < kCylindersPerDisk)
				{
					drive.currCylinder++;
				}
			}
			else
			{
				if (drive.currCylinder > 0)
					drive.currCylinder--;
			}
		}

		drive.side = side ? 1 : 0;

		drive.stepSignal = step;
	}

	UpdateFloppyDriveFlags();
}

void am::Amiga::UpdateFloppyDriveFlags()
{
	auto SetFlag = [](uint8_t& flags, uint8_t bits, bool set)
	{
		if (set)
		{
			flags |= bits;
		}
		else
		{
			flags &= ~bits;
		}
	};

	if (m_driveSelected == -1)
	{
		// set flags for no drive selected. (hold all drive flags high)
		m_cia[0].pra |= 0b0011'1100;
	}
	else
	{
		auto& drive = m_floppyDrive[m_driveSelected];

		SetFlag(m_cia[0].pra, 0x04, drive.diskChange); // DSKCHANGE
		SetFlag(m_cia[0].pra, 0x08, false); // DSKPROT (currently all disks appear write-protected)
		SetFlag(m_cia[0].pra, 0x10, drive.currCylinder != 0); // DSKTRACK0
		if (drive.motorOn)
		{
			SetFlag(m_cia[0].pra, 0x20, !IsDiskInserted(m_driveSelected)); // DSKRDY
		}
		else
		{
			SetFlag(m_cia[0].pra, 0x20, true); // id data (report as amiga drive (all 1s))
		}
	}
}

void am::Amiga::StartDiskDMA()
{
	// TODO : work out what happens when DMA is disabled during transfer.

	m_diskDma.inProgress = true;

	if (!m_diskDma.useWordSync)
		return;

	auto wordSync = Reg(Register::DSKSYNC);
	if (wordSync == 0x4489)
	{
		// This is the most likely sync word as it is part of the MFM sector sync pattern.
		// We therefore will hard-code the response to this, which is to skip the first three words.

		const auto currentSector = m_diskDma.encodedSequenceCounter / kMfmSectorSize;
		const auto currentSectorOffset = m_diskDma.encodedSequenceCounter % kMfmSectorSize;

		uint16_t nextSector = currentSector;

		if (currentSector >= kSectorsPerTrack)
		{
			nextSector = 0;
		}
		else if (currentSectorOffset >= 6)
		{
			nextSector = ((currentSector + 1) % kSectorsPerTrack);
		}

		m_diskDma.encodedSequenceCounter = nextSector * kMfmSectorSize + 6;
	}
	else
	{
		// TODO: How to respond to other Sync Words. Do we care?
		DEBUGGER_BREAK();
	}
}

void am::Amiga::DoDiskDMA()
{
	if (m_diskDma.writing)
	{
		// TODO : implement disk writing
		DEBUGGER_BREAK();
	}
	else
	{
		auto& drive = m_floppyDrive[m_driveSelected];
		auto& disk = m_floppyDisk[m_driveSelected];
		auto& track = disk.image[drive.currCylinder][drive.side];

		uint16_t value = track[m_diskDma.encodedSequenceCounter];
		value <<= 8;
		m_diskDma.encodedSequenceCounter = (m_diskDma.encodedSequenceCounter + 1) % track.size();
		value |= uint16_t(track[m_diskDma.encodedSequenceCounter]);
		m_diskDma.encodedSequenceCounter = (m_diskDma.encodedSequenceCounter + 1) % track.size();

		WriteChipWord(m_diskDma.ptr, value);
		m_diskDma.ptr += 2;
	}

	m_diskDma.len--;
	if (m_diskDma.len == 0)
	{
		m_diskDma.inProgress = false;
		auto& intreqr = Reg(Register::INTREQR);
		intreqr |= 0x0002; // set disk block finished interupt

		DoInterruptRequest();
	}
}