#include "amiga.h"

#include "registers.h"

#include "util/endian.h"
#include "util/platform.h"
#include "util/strings.h"
#include "util/stream.h"

#include <cassert>
#include <sstream>


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

am::Amiga::Amiga(ChipRamConfig chipRamConfig, util::Log* log)
	: m_lastScreen(new ScreenBuffer)
	, m_currentScreen(new ScreenBuffer)
	, m_log(log)
{
	m_chipRam.resize(size_t(chipRamConfig));
	m_rom.resize(512*1024, 0xcc);
	m_registers.resize(_countof(registerInfo), 0x0000);
	m_m68000 = std::make_unique<cpu::M68000>(this);
}

void am::Amiga::SetRom(std::span<const uint8_t> rom)
{
	memcpy(m_rom.data(), rom.data(), std::min(rom.size(), m_rom.size()));
	Reset();
}

void am::Amiga::SetPC(uint32_t pc)
{
	m_m68000->SetPC(pc);
}

void am::Amiga::SetBreakpoint(uint32_t addr)
{
	m_breakpoint = addr;
	if (addr == m_m68000->GetPC())
	{
		m_breakpointSetAfter = true;
	}
	m_breakpointEnabled = true;
}

void am::Amiga::ClearBreakpoint()
{
	m_breakpointEnabled = false;
	m_breakpointSetAfter = false;
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

void am::Amiga::SetDataBreakpoint(uint32_t addr, uint32_t size)
{
	m_breakAtAddressChanged = true;
	m_dataBreakpoint = addr;
	m_dataBreakpointSize = size;

	switch (size)
	{
	case 1:
		m_currentDataBreakpointData = PeekByte(addr);
		break;
	case 2:
		m_currentDataBreakpointData = PeekWord(addr);
		break;
	case 4:
		m_currentDataBreakpointData = (uint32_t(PeekWord(addr)) << 16) | PeekWord(addr + 2);
		break;
	};
}

void am::Amiga::DisableDataBreakpoint()
{
	m_breakAtAddressChanged = false;
}

bool am::Amiga::DataBreakpointTriggered()
{
	uint32_t value = 0;

	switch (m_dataBreakpointSize)
	{
	case 1:
		value = PeekByte(m_dataBreakpoint);
		break;
	case 2:
		value = PeekWord(m_dataBreakpoint);
		break;
	case 4:
		value = (uint32_t(PeekWord(m_dataBreakpoint)) << 16) | PeekWord(m_dataBreakpoint + 2);
		break;
	};

	return (value != m_currentDataBreakpointData);
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
		else if (type == Mapped::ChipRegisters)
		{
			if ((addr & 0x03f000) == 0x03f000)
			{
				// Not sure exactly how this should work. I'm guessing that the whole
				// register is written and the other byte of the word is just zeros.
				uint16_t wordValue = (addr & 1) ? uint16_t(value) : uint16_t(value) << 8;
				const auto regNum = addr & 0x000ffe;
				WriteRegister(regNum, wordValue);
			}
		}
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
			if (!m_breakAtNextInstruction && m_breakpointSetAfter)
			{
				m_breakpointSetAfter = false;
			}
			else
			{
				m_breakAtNextInstruction = false;
				m_running = false;
				return;
			}
		}

		if (m_breakAtAddressChanged)
		{
			if (DataBreakpointTriggered())
			{
				// Reset the breakpoint to record new value
				SetDataBreakpoint(m_dataBreakpoint, m_dataBreakpointSize);
				m_running = false;
				return;
			}
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

	// Update Audio
	for (int i = 0; i < 4; i++)
	{
		UpdateAudioChannel(i);
	}

	if (m_audioBufferCountdown == 0)
	{
		m_audioBufferCountdown = 100;

		for (int i = 0; i < 2; i++)
		{
			auto& audioBuffer = m_audioBuffer[i];
			auto pos = m_audioBufferPos * 2;
			auto channel = i * 2;

			auto GetSample = [this](int channel) -> uint8_t
			{
				auto& a = m_audio[channel];

				if (a.volume == 64)
				{
					return a.currentSample ^ 0x80;
				}
				else
				{
					return uint8_t((int32_t(int8_t(a.currentSample)) * int32_t(a.volume) * 4) / 256) ^ 0x80;
				}
			};

			audioBuffer[pos + 0] = GetSample(channel + 0);
			audioBuffer[pos + 1] = GetSample(channel + 1);
		}

		m_audioBufferPos++;
		if (m_audioBufferPos == kAudioBufferLength)
		{
			if (m_audioPlayer)
			{
				m_audioPlayer->AddAudioBuffer(&m_audioBuffer);
			}
			m_audioBufferPos = 0;
		}

	}
	m_audioBufferCountdown--;

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

		m_bpFetchState = BpFetchState::Off;
		if (m_vPos >= m_windowStartY && m_vPos < m_windowStopY)
		{
			m_bpFetchState = BpFetchState::Idle;
		}

		if (!m_bitplane.externalResync)
		{
			TickCIAtod(1);

			auto& vposr = Reg(Register::VPOSR);

			vhposr = (m_vPos & 0xff) << 8;
			vposr = m_vPos >> 8;
			const bool isLongFrame = (m_frameLength & 1) != 0;
			vposr |= (isLongFrame ? 0x8000 : 0);
			vposr |= uint16_t(m_agnusVersion);
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

	m_bpFetchState = BpFetchState::Off;
	m_fetchPos = 0;

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

	m_audioBufferCountdown = 0;
	m_audioBufferPos = 0;

	for (int i = 0; i < 4; i++)
	{
		m_audio[i] = {};
	}

	for (int i = 0; i < 2; i++)
	{
		m_audioBuffer[i].clear();
		m_audioBuffer[i].resize(kAudioBufferLength * 2);
	}

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
		--value;
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
		if (controller == 0)
		{
			m_rightMouseButtonDown = pressed;
			auto& potgor = Reg(Register::POTGOR);
			if ((potgor & 0x0800) == 0x0000)
			{
				if (m_rightMouseButtonDown)
				{
					potgor &= ~0x0400;
				}
				else
				{
					potgor |= 0x0400;
				}
			}
		}
		break;

	case 2: // middle mouse button / fire 3?
		// TODO
		break;

	default:
		break;
	}
}

void am::Amiga::SetMouseMove(int x, int y)
{
	auto& joy0dat = Reg(Register::JOY0DAT);

	auto vertCount = (joy0dat >> 8) & 0xff;
	auto horzCount = joy0dat & 0xff;

	horzCount += x;
	vertCount += y;

	joy0dat = uint16_t(vertCount << 8) | uint16_t(horzCount & 0xff);
}

void am::Amiga::SetJoystickMove(int x, int y)
{
	auto& joy1dat = Reg(Register::JOY1DAT);

	const bool left = (x >= 0);
	const bool right = (x <= 0);
	const bool up = (y >= 0);
	const bool down = (y <= 0);

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

	if (m_breakOnRegisterEnabled && regNum == m_breakAtRegister)
	{
		m_breakAtNextInstruction = true;
	}

	if (regInfo.type == RegType::Strobe)
	{
		// Reading to strobe a register (rather than writing) is rare
		// but is done by Bubble Bobble (at least)
		StrobeRegister(regNum);

		// Not sure what value is 'read' from a strobe register but for
		// now assume it is always 0.
		return 0x0000;
	}

	if (regInfo.type != RegType::ReadOnly)
	{
		return 0x0000;
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

	if (m_breakOnRegisterEnabled && regNum == m_breakAtRegister)
	{
		m_breakAtNextInstruction = true;
	}

	if (regInfo.type == RegType::Strobe)
	{
		StrobeRegister(regNum);
		return;
	}

	if (regInfo.type != RegType::WriteOnly)
		return;

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

		if (m_diskDma.inProgress && m_log->IsLogging(LogOptions::Disk))
		{
			std::stringstream ss;
			ss << "Disk DMA aborted"
				<< " cmd=" << util::HexToString(m_m68000->GetCurrentInstructionAddr())
				<< " : ptr=" << util::HexToString(m_diskDma.ptr)
				<< " remaining=" << util::HexToString(uint16_t(m_diskDma.len))
				<< " pos=" << m_diskDma.encodedSequenceCounter;

			m_log->AddMessage(m_totalCClocks, ss.str());
		}

		m_diskDma.secondaryDmaEnabled = (value & 0x8000) != 0;
		m_diskDma.writing = (value & 0x4000) != 0;
		m_diskDma.len = (value & 0x3fff);
		m_diskDma.inProgress = false;

		if (currentlyEnabled && m_diskDma.secondaryDmaEnabled)
		{
			StartDiskDMA();

			if (m_log->IsLogging(LogOptions::Disk))
			{
				std::stringstream ss;
				ss << "Disk DMA" << (m_diskDma.useWordSync ? " (sync)" : "")
					<< " cmd=" << util::HexToString(m_m68000->GetCurrentInstructionAddr())
					<< " : trk=" << int(m_floppyDrive[0].currCylinder) << "/" << int(m_floppyDrive[0].side)
					<< " to=" << util::HexToString(m_diskDma.ptr)
					<< " len=" << util::HexToString(uint16_t(m_diskDma.len))
					<< " start=" << m_diskDma.encodedSequenceCounter;

				m_log->AddMessage(m_totalCClocks, ss.str());
			}
		}
	}	break;

	case am::Register::SERPER:
		// Serial port curretly not emulated but silently accept
		// control values to this register.
		break;

	case am::Register::POTGO:
	{
		if ((value & 0xc000) == 0xc000)
		{
			Reg(am::Register::POTGOR) &= ~0xc000;
			Reg(am::Register::POTGOR) |= 0x4000;
		}

		if ((value & 0x3000) == 0x3000)
		{
			Reg(am::Register::POTGOR) &= ~0x3000;
			Reg(am::Register::POTGOR) |= 0x1000;
		}

		if ((value & 0x0c00) == 0x0c00)
		{
			Reg(am::Register::POTGOR) &= ~0x0c00;
			if (!m_rightMouseButtonDown)
			{
				Reg(am::Register::POTGOR) |= 0x0400;
			}
		}

		if ((value & 0x0300) == 0x0300)
		{
			Reg(am::Register::POTGOR) &= ~0x0300;
			Reg(am::Register::POTGOR) |= 0x0100;
		}
	} break;

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
	{
		auto dmaconr = UpdateFlagRegister(am::Register::DMACONR, value & 0x87ff);

		for (int i = 0; i < 4; i++)
		{
			const bool dmaOn = (dmaconr & (0x1 << i)) != 0;

			if (m_audio[i].dmaOn != dmaOn)
			{
				UpdateAudioChannelOnDmaChange(i, dmaOn);
			}
		}
	}	break;

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

	case am::Register::AUD0LCH:
	case am::Register::AUD1LCH:
	case am::Register::AUD2LCH:
	case am::Register::AUD3LCH:
	case am::Register::AUD0LCL:
	case am::Register::AUD1LCL:
	case am::Register::AUD2LCL:
	case am::Register::AUD3LCL:
	case am::Register::AUD0LEN:
	case am::Register::AUD1LEN:
	case am::Register::AUD2LEN:
	case am::Register::AUD3LEN:
	case am::Register::AUD0PER:
	case am::Register::AUD1PER:
	case am::Register::AUD2PER:
	case am::Register::AUD3PER:
		break;

	case am::Register::AUD0VOL:
	case am::Register::AUD1VOL:
	case am::Register::AUD2VOL:
	case am::Register::AUD3VOL:
	{
		const int channel = ((regNum & ~1) - int(am::Register::AUD0VOL)) / 16;
		const uint8_t volume = std::min(value & 0x007f, 0x40);
		m_audio[channel].volume = volume;
	}	break;

	case am::Register::AUD0DAT:
	case am::Register::AUD1DAT:
	case am::Register::AUD2DAT:
	case am::Register::AUD3DAT:
	{
		int channel = ((regNum & ~1) - int(am::Register::AUD0DAT)) / 16;
		UpdateAudioChannelOnData(channel, value);
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
		m_bitplane.ham = ((value & 0x8C00) == 0x0800);
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
		memset(m_playfieldBuffer[0].data() + m_pixelBufferLoadPtr, 0, 16);
		memset(m_playfieldBuffer[1].data() + m_pixelBufferLoadPtr, 0, 16);

		for (int i = 0; i < m_bitplane.numPlanesEnabled; i++)
		{
			auto bits = Reg(Register(int(Register::BPL1DAT) + (i * 2)));

			for (int j = 0; j < 16; j++)
			{
				m_playfieldBuffer[i & 1][m_pixelBufferLoadPtr + j] |= (bits & (32768u >> j)) != 0 ? (1 << i) : 0;
			}
		}

		m_pixelBufferReadPtr = (m_pixelBufferLoadPtr - (m_bitplane.hires ? 24 : 12)) & kPixelBufferMask;
		m_pixelBufferLoadPtr = (m_pixelBufferLoadPtr + 16) & kPixelBufferMask;

	}	break;

	case am::Register::BPL2DAT:
	case am::Register::BPL3DAT:
	case am::Register::BPL4DAT:
	case am::Register::BPL5DAT:
	case am::Register::BPL6DAT:
		break;

	case am::Register::SPR0PTH:
	case am::Register::SPR0PTL:
	case am::Register::SPR1PTH:
	case am::Register::SPR1PTL:
	case am::Register::SPR2PTH:
	case am::Register::SPR2PTL:
	case am::Register::SPR3PTH:
	case am::Register::SPR3PTL:
	case am::Register::SPR4PTH:
	case am::Register::SPR4PTL:
	case am::Register::SPR5PTH:
	case am::Register::SPR5PTL:
	case am::Register::SPR6PTH:
	case am::Register::SPR6PTL:
	case am::Register::SPR7PTH:
	case am::Register::SPR7PTL:
	{
		const auto spriteNum = (regNum - uint32_t(Register::SPR0PTH)) / 4;
		const auto highWord = (regNum & 0b010) == 0;
		SetLongPointerValue(m_sprite[spriteNum].ptr, highWord, value);
	}	break;


	case am::Register::SPR0POS:
	case am::Register::SPR0CTL:
	case am::Register::SPR0DATA:
	case am::Register::SPR0DATB:
	case am::Register::SPR1POS:
	case am::Register::SPR1CTL:
	case am::Register::SPR1DATA:
	case am::Register::SPR1DATB:
	case am::Register::SPR2POS:
	case am::Register::SPR2CTL:
	case am::Register::SPR2DATA:
	case am::Register::SPR2DATB:
	case am::Register::SPR3POS:
	case am::Register::SPR3CTL:
	case am::Register::SPR3DATA:
	case am::Register::SPR3DATB:
	case am::Register::SPR4POS:
	case am::Register::SPR4CTL:
	case am::Register::SPR4DATA:
	case am::Register::SPR4DATB:
	case am::Register::SPR5POS:
	case am::Register::SPR5CTL:
	case am::Register::SPR5DATA:
	case am::Register::SPR5DATB:
	case am::Register::SPR6POS:
	case am::Register::SPR6CTL:
	case am::Register::SPR6DATA:
	case am::Register::SPR6DATB:
	case am::Register::SPR7POS:
	case am::Register::SPR7CTL:
	case am::Register::SPR7DATA:
	case am::Register::SPR7DATB:
	{
		auto spriteNum = (regNum - uint32_t(am::Register::SPR0POS)) / 8;
		auto spriteReg = ((regNum - uint32_t(am::Register::SPR0POS)) / 2) % 4;
		auto& sprite = m_sprite[spriteNum];

		switch (spriteReg)
		{
		case 0: // SPRxPOS
		{
			sprite.startLine &= 0x0100;
			sprite.startLine |= (value & 0xff00) >> 8;
			sprite.horizontalStart &= 0x0001;
			sprite.horizontalStart |= (value & 0x00ff) << 1;
		}	break;

		case 1: // SPRxCTL
		{
			sprite.horizontalStart &= ~0x0001;
			sprite.horizontalStart |= value & 0x0001;
			sprite.startLine &= 0x00ff;
			sprite.startLine |= (value & 0x0004) << 6;
			sprite.endLine = (value & 0xff00) >> 8 | ((value & 0x0002) << 7);
			sprite.attached = (value & 0x0080) != 0;
			sprite.armed = false;
		}	break;

		case 2:	// SPRxDATA
		{
			sprite.armed = true;
		}	break;

		case 3: // SPRxDATB
			break;
		}
	}	break;

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

void am::Amiga::StrobeRegister(uint32_t regNum)
{
	switch (am::Register(regNum & ~1))
	{

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
		m_currentScreen->fill(0);
		return;
	}

	if (bufferLine < 0 || bufferLine >= (m_isNtsc ? 216 : 272))
		return;

	uint8_t values[4] = {};
	uint8_t pfMask[4] = {};

	int valueIndex = 0;

	if (line >= m_windowStartY && line < m_windowStopY)
	{
		auto startIndex = m_windowStartX - 0x79;
		auto endIndex = m_windowStopX - 0x79;

		if (m_bitplane.hires)
		{
			for (int x = 0; x < 4; x++)
			{
				int loresPixelPos = (xPos * 2 + x / 2);

				int delay0 = ((m_pixelFetchDelay + m_bitplane.playfieldDelay[0]) & 0xf) * 2;
				int delay1 = ((m_pixelFetchDelay + m_bitplane.playfieldDelay[1]) & 0xf) * 2;

				int playfieldPtr0 = (m_pixelBufferReadPtr + (x - delay0)) & kPixelBufferMask;
				int playfieldPtr1 = (m_pixelBufferReadPtr + (x - delay1)) & kPixelBufferMask;

				if (loresPixelPos >= 0 && loresPixelPos < 336)
				{
					int value = 0;

					if (loresPixelPos >= startIndex && loresPixelPos < endIndex)
					{
						if (m_bitplane.doublePlayfield)
						{
							uint8_t pfValue[2];

							pfValue[0] = m_playfieldBuffer[0][playfieldPtr0];
							pfValue[1] = m_playfieldBuffer[1][playfieldPtr1] >> 1;

							pfMask[valueIndex] |= (pfValue[0] != 0) ? 1 : 0;
							pfMask[valueIndex] |= (pfValue[1] != 0) ? 2 : 0;

							const int fromPlayfield = (pfValue[m_bitplane.playfieldPriority] != 0)
								? m_bitplane.playfieldPriority
								: pfValue[1 - m_bitplane.playfieldPriority] != 0
								? 1 - m_bitplane.playfieldPriority
								: 0;

							value = pfValue[fromPlayfield];
							value = ((value & 0b10000) >> 2) | ((value & 0b100) >> 1) | value & 1;
							value += (8 * fromPlayfield);
						}
						else
						{
							value = m_playfieldBuffer[0][playfieldPtr0] | m_playfieldBuffer[1][playfieldPtr1];
							pfMask[valueIndex] = (value != 0) ? 1 : 0;
						}
					}

					values[valueIndex++] = value;
				}
				m_playfieldBuffer[0][playfieldPtr0] = 0;
				m_playfieldBuffer[1][playfieldPtr1] = 0;
			}
			m_pixelBufferReadPtr += 4;
		}
		else
		{
			for (int x = 0; x < 2; x++)
			{
				int loresPixelPos = (xPos * 2 + x);

				int delay0 = ((m_pixelFetchDelay + m_bitplane.playfieldDelay[0]) & 0xf);
				int delay1 = ((m_pixelFetchDelay + m_bitplane.playfieldDelay[1]) & 0xf);

				int playfieldPtr0 = (m_pixelBufferReadPtr + (x - delay0)) & kPixelBufferMask;
				int playfieldPtr1 = (m_pixelBufferReadPtr + (x - delay1)) & kPixelBufferMask;

				if (loresPixelPos >= 0 && loresPixelPos < 336)
				{
					int value = 0;

					if (loresPixelPos >= startIndex && loresPixelPos < endIndex)
					{
						if (m_bitplane.doublePlayfield)
						{
							uint8_t pfValue[2];

							pfValue[0] = m_playfieldBuffer[0][playfieldPtr0];
							pfValue[1] = m_playfieldBuffer[1][playfieldPtr1] >> 1;

							pfMask[valueIndex] |= (pfValue[0] != 0) ? 1 : 0;
							pfMask[valueIndex] |= (pfValue[1] != 0) ? 2 : 0;

							const int fromPlayfield = (pfValue[m_bitplane.playfieldPriority] != 0)
								? m_bitplane.playfieldPriority
								: pfValue[1 - m_bitplane.playfieldPriority] != 0
								? 1 - m_bitplane.playfieldPriority
								: 0;

							value = pfValue[fromPlayfield];
							value = ((value & 0b10000) >> 2) | ((value & 0b100) >> 1) | value & 1;
							value += (8 * fromPlayfield);
						}
						else
						{
							value = m_playfieldBuffer[0][playfieldPtr0] | m_playfieldBuffer[1][playfieldPtr1];
							pfMask[valueIndex] = (value != 0) ? 1 : 0;
						}
					}

					pfMask[valueIndex + 1] = pfMask[valueIndex];
					values[valueIndex++] = value;
					values[valueIndex++] = value;

				}
				m_playfieldBuffer[0][playfieldPtr0] = 0;
				m_playfieldBuffer[1][playfieldPtr1] = 0;

			}
			m_pixelBufferReadPtr += 2;
		}

		uint8_t spriteValue[2] = {};
		uint8_t spriteNum[2] = { uint8_t(~0), uint8_t(~0) };

		for (int s = 0; s < 8; s++)
		{
			auto& sprite = m_sprite[s];
			if (!sprite.armed)
				continue;

			const bool attached = m_sprite[s|1].attached; // extract attached status from odd sprite num

			for (int x = 0; x < 2; x++)
			{
				const int loresPixelPos = (xPos * 2 + x);

				// Note: Sprites horizontal position is delayed by an extra pixel (hence 0x78 instead of 0x79 in next line)
				if (sprite.drawPos > 0 || (sprite.horizontalStart - 0x78) == loresPixelPos)
				{
					auto startIndex = m_windowStartX - 0x79;

					// has a higher priority sprite already been written to this pixel?
					if (loresPixelPos >= startIndex && s <= spriteNum[x])
					{
						const auto sprdata = Reg(Register(int(Register::SPR0DATA) + s * 8));
						const auto sprdatb = Reg(Register(int(Register::SPR0DATB) + s * 8));

						uint8_t value = 0;
						value = ((sprdata >> (15 - sprite.drawPos)) & 1) | (((sprdatb >> (15 - sprite.drawPos)) << 1) & 2);
						if (value != 0)
						{
							if (attached)
							{
								if ((s & 1) == 0) // even attached sprite
								{
									spriteValue[x] = 0x10 | value;
									spriteNum[x] = s + 1; // set to the num of the next (attached) sprite so it is not masked.
								}
								else // odd attached sprite
								{
									if (spriteNum[x] == s)
									{
										spriteValue[x] |= (value << 2);
									}
									else
									{
										spriteValue[x] = 0x10 | (value << 2);
										spriteNum[x] = s;
									}
								}
							}
							else
							{
								spriteValue[x] = 16 + (s / 2) * 4 + value;
								spriteNum[x] = s;
							}
						}
					}

					sprite.drawPos++;
					sprite.drawPos &= 0x0f;
				}
			}
		}

		auto index = bufferLine * kScreenBufferWidth + (xPos * 4);
		for (int i = 0; i < valueIndex; i++)
		{
			bool drawSprite = false;

			if (spriteValue[i / 2] != 0)
			{
				drawSprite = true;

				const int spriteGroup = spriteNum[i / 2] / 2;

				if (pfMask[i] & 1)
				{
					if (spriteGroup >= m_bitplane.playfieldSpritePri[0])
						drawSprite = false;
				}
				if (pfMask[i] & 2)
				{
					if (spriteGroup >= m_bitplane.playfieldSpritePri[1])
						drawSprite = false;
				}
			}

			ColourRef col;

			if (m_bitplane.ham)
			{
				const uint32_t sel = (values[i] >> 4) & 0b11;
				const uint32_t mod = values[i] & 0xf;

				auto& heldCol = m_bitplane.heldCol;

				switch (sel)
				{
				case 0b00: // new palette colour
					heldCol = m_palette[mod];
					break;

				case 0b01: // new blue channel value
					heldCol &= 0xff00ffff;
					heldCol |= mod << 16;
					heldCol |= mod << 20;
					break;

				case 0b10: // new red channel value
					heldCol &= 0xffffff00;
					heldCol |= mod;
					heldCol |= mod << 4;
					break;

				case 0b11: // new green channel value
					heldCol &= 0xffff00ff;
					heldCol |= mod << 8;
					heldCol |= mod << 12;
					break;
				}

				col = heldCol;
			}
			else
			{
				col = m_palette[values[i] & 0x3f];
			}

			if (drawSprite)
			{
				col = m_palette[spriteValue[i / 2]];
			}

			(*m_currentScreen.get())[index++] = col;
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

namespace
{
	// Lookup tables for appying inclusive and exclusive
	// fill algorithms to a nibble of data.

	uint8_t inFill[2][16] =
	{
		// without carry in
		{
			0b0'0000,	// 0000
			0b1'1111,	// 0001
			0b1'1110,	// 0010
			0b0'0011,	// 0011
			0b1'1100,	// 0100
			0b0'0111,	// 0101
			0b0'0110,	// 0110
			0b1'1111,	// 0111
			0b1'1000,	// 1000
			0b0'1111,	// 1001
			0b0'1110,	// 1010
			0b1'1011,	// 1011
			0b1'1100,	// 1100
			0b1'1111,	// 1101
			0b1'1110,	// 1110
			0b0'1111,	// 1111
		},
		// with carry in
		{
			0b1'1111,	// 0000
			0b0'0001,	// 0001
			0b0'0011,	// 0010
			0b1'1111,	// 0011
			0b0'0111,	// 0100
			0b1'1101,	// 0101
			0b1'1111,	// 0110
			0b0'0111,	// 0111
			0b0'1111,	// 1000
			0b1'1001,	// 1001
			0b1'1011,	// 1010
			0b0'1111,	// 1011
			0b1'1111,	// 1100
			0b0'1101,	// 1101
			0b0'1111,	// 1110
			0b1'1111,	// 1111
		}
	};

	uint8_t exFill[2][16] =
	{
		// without carry in
		{
			0b0'0000,	// 0000
			0b1'1111,	// 0001
			0b1'1110,	// 0010
			0b0'0001,	// 0011
			0b1'1100,	// 0100
			0b0'0011,	// 0101
			0b0'0010,	// 0110
			0b1'1101,	// 0111
			0b1'1000,	// 1000
			0b0'0111,	// 1001
			0b0'0110,	// 1010
			0b1'1001,	// 1011
			0b0'0100,	// 1100
			0b1'1011,	// 1101
			0b1'1010,	// 1110
			0b0'0101,	// 1111
		},
		// with carry in
		{
			0b1'1111,	// 0000
			0b0'0000,	// 0001
			0b0'0001,	// 0010
			0b1'1110,	// 0011
			0b0'0011,	// 0100
			0b1'1100,	// 0101
			0b1'1101,	// 0110
			0b0'0010,	// 0111
			0b0'0111,	// 1000
			0b1'1000,	// 1001
			0b1'1001,	// 1010
			0b0'0110,	// 1011
			0b1'1011,	// 1100
			0b0'0100,	// 1101
			0b0'0101,	// 1110
			0b1'1010,	// 1111
		}
	};

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

		const int fillMode = (con1 >> 3) & 0b11;
		auto* fillTable = (fillMode == 1) ? inFill : exFill;
		const int lineFillCarryIn = (con1 >> 2) & 0b1;

		uint32_t addTo = descendingMode ? -2 : 2;

		uint16_t aShiftIn = 0;
		uint16_t bShiftIn = 0;

		uint16_t res;
		uint32_t resAddr;
		bool resQueued = false;

		for (auto l = 0; l < m_blitter.lines; l++)
		{
			int carryIn = lineFillCarryIn;

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

				if (res != 0)
				{
					dmaconr &= ~0x2000;
				}

				if (fillMode != 0)
				{
					// Apply the area fill algorithm, a nibble at a time
					for (int s = 0; s < 16; s += 4)
					{
						auto fill = fillTable[carryIn][(res >> s) & 0x0f];
						res &= ~(0xf << s);
						res |= (fill & 0xf) << s;
						carryIn = (fill >> 4) & 1;
					}
				}

				if (m_blitter.enabled[3])
				{
					resQueued = true;
					resAddr = m_blitter.ptr[3] & 0xffff'fffe;
					m_blitter.ptr[3] += addTo;
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
			const int channel = (m_hPos - 0x0d) / 2;
			return DoAudioDMA(channel);
		}

		return false;
	}

	// Sprites can be wiped out by display DMA so check that first
	if (DmaEnabled(Dma::BPLEN) && m_bpFetchState == BpFetchState::Idle)
	{
		auto ddfstrt = Reg(Register::DDFSTRT) & 0b0000000011111100;
		ddfstrt = std::max(ddfstrt, 0x18);
		if (m_hPos == ddfstrt)
		{
			m_bpFetchState = BpFetchState::Fetching;
			m_fetchPos = 0;
			m_pixelFetchDelay = m_bitplane.hires ? 0 : ((m_hPos & 0b100) * 2);
		}
	}

	if (m_bpFetchState == BpFetchState::Fetching || m_bpFetchState == BpFetchState::Finishing)
	{
		static constexpr uint8_t kPlaneReadOrderLores[8] = { ~0, 3, 5, 1, ~0, 2, 4, 0 };
		static constexpr uint8_t kPlaneReadOrderHires[8] = {  3, 1, 2, 0,  3, 1, 2, 0 };

		int bp = m_bitplane.hires
			? kPlaneReadOrderHires[m_fetchPos]
			: kPlaneReadOrderLores[m_fetchPos];

		bool dmaUsed = false;

		if (bp < m_bitplane.numPlanesEnabled)
		{
			const auto bplData = ReadChipWord(m_bitplane.ptr[bp]);
			m_bitplane.ptr[bp] += 2;
			WriteRegister(uint32_t(Register::BPL1DAT) + (bp << 1), bplData);
			dmaUsed = true;
		}

		++m_fetchPos;
		if (m_fetchPos == 8)
		{
			m_fetchPos = 0;

			if (m_bpFetchState == BpFetchState::Finishing)
			{
				// Apply Modulos for the bitplane pointers.
				const auto bpl1mod = int16_t(Reg(Register::BPL1MOD) & 0xfffe);
				const auto bpl2mod = int16_t(Reg(Register::BPL2MOD) & 0xfffe);
				for (int i = 0; i < m_bitplane.numPlanesEnabled; i++)
				{
					m_bitplane.ptr[i] += (i & 1) ? bpl2mod : bpl1mod;
				}
				m_bpFetchState = BpFetchState::Idle;
			}
			else
			{
				auto ddfstop = Reg(Register::DDFSTOP) & 0b0000000011111100;
				ddfstop = std::min(ddfstop, 0xd8);

				if ((m_hPos + 1) >= ddfstop)
				{
					m_bpFetchState = BpFetchState::Finishing;
				}
			}
		}

		if (dmaUsed)
			return true;
	}

	const int spriteStart = m_isNtsc ? 20 : 25;

	if (m_vPos >= spriteStart && DmaEnabled(Dma::SPREN))
	{
		if (m_hPos < 0x34 && oddClock)
		{
			const int spriteNum = (m_hPos - 0x15) / 4;
			const int fetchNum = ((m_hPos - 0x15) / 2) & 1;

			auto& sprite = m_sprite[spriteNum];

			if (m_vPos == spriteStart || m_vPos == sprite.endLine)
			{
				// 1st fetch goes to SPRxPOS, 2nd fetch goes to SPRxCTL, which also disarms the sprite
				auto fetchedWord = ReadChipWord(sprite.ptr);
				sprite.ptr += 2;
				const uint32_t baseReg = (fetchNum == 0) ? uint32_t(Register::SPR0POS) : uint32_t(Register::SPR0CTL);
				WriteRegister(baseReg + spriteNum * 8, fetchedWord);
				sprite.active = false;		// Sprite DMA now goes inactive until start line is reached.
				return true;
			}

			if (m_vPos == sprite.startLine)
			{
				sprite.active = true;
			}

			if (sprite.active)
			{
				// 1st fetch goes to SPRxDATB, 2nd fetch goes to SPRxDATA, which also arms the sprite
				auto fetchedWord = ReadChipWord(sprite.ptr);
				sprite.ptr += 2;
				const uint32_t baseReg = (fetchNum == 0) ? uint32_t(Register::SPR0DATA) : uint32_t(Register::SPR0DATB);
				WriteRegister(baseReg + spriteNum * 8, fetchedWord);
				return true;
			}
		}
	}

	return false;
}

const std::string& am::Amiga::GetDiskName(int driveNum) const
{
	assert(driveNum >= 0 && driveNum < 4);
	return m_floppyDisk[driveNum].displayName;
}

const std::string& am::Amiga::GetDiskFilename(int driveNum) const
{
	assert(driveNum >= 0 && driveNum < 4);
	return m_floppyDisk[driveNum].fileLocation;
}

bool am::Amiga::SetDisk(int driveNum, const std::string& filename, const std::string& displayName, std::vector<uint8_t>&& data)
{
	assert(driveNum >= 0 && driveNum < 4);

	if (filename.empty() || data.empty())
		return false;

	auto& disk = m_floppyDisk[driveNum];

	disk.fileLocation = filename;
	disk.displayName = displayName;
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
		disk.displayName.clear();
		disk.fileLocation.clear();
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
			drive.diskChange = IsDiskInserted(m_driveSelected);

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

			if (m_log->IsLogging(LogOptions::Disk))
			{
				std::stringstream ss;
				ss << "Disk Stepped "
					<< " addr=" << util::HexToString(m_m68000->GetCurrentInstructionAddr())
					<< " cyl=" << int(m_floppyDrive[0].currCylinder)
					<< " side=" << int(m_floppyDrive[0].side);

				m_log->AddMessage(m_totalCClocks, ss.str());
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
			// id data (report as amiga drive (all 1s))
			// Note: line must be driven low for a '1'
			SetFlag(m_cia[0].pra, 0x20, false);
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
		uint16_t value = 0;

		// Disk DMA continues even when drive is unselected or motor is switched off (reads zeros).

		if (m_driveSelected > -1)
		{
			auto& drive = m_floppyDrive[m_driveSelected];
			if (drive.motorOn)
			{
				auto& disk = m_floppyDisk[m_driveSelected];
				auto& track = disk.image[drive.currCylinder][drive.side];

				value = track[m_diskDma.encodedSequenceCounter];
				value <<= 8;
				m_diskDma.encodedSequenceCounter = (m_diskDma.encodedSequenceCounter + 1) % track.size();
				value |= uint16_t(track[m_diskDma.encodedSequenceCounter]);
				m_diskDma.encodedSequenceCounter = (m_diskDma.encodedSequenceCounter + 1) % track.size();
			}
		}
		WriteChipWord(m_diskDma.ptr, value);
		m_diskDma.ptr += 2;
	}

	m_diskDma.len--;
	if (m_diskDma.len == 0)
	{
		if (m_log->IsLogging(LogOptions::Disk))
		{
			m_log->AddMessage(m_totalCClocks, "Disk DMA Finished.");
		}

		m_diskDma.inProgress = false;
		auto& intreqr = Reg(Register::INTREQR);
		intreqr |= 0x0002; // set disk block finished interupt

		DoInterruptRequest();
	}
}

bool am::Amiga::DoAudioDMA(int channel)
{
	auto& audio = m_audio[channel];

	if (!audio.dmaReq)
		return false;

	audio.dmaReq = false;

	const auto value = ReadChipWord(audio.pointer);
	audio.pointer += 2;

	WriteRegister(uint32_t(Register::AUD0DAT) + channel * 0x10, value);

	return true;
}

void am::Amiga::UpdateAudioChannel(int channel)
{
	auto& audio = m_audio[channel];

	switch (audio.state)
	{
	case 0b000: // idle
		break;

	case 0b001: // DMA on, waiting for AUDxDAT...
	case 0b101:
		break;

	case 0b010: // output high byte
	{
		if (audio.perCounter > 1)
		{
			audio.perCounter--;
			return;
		}

		audio.state = 0b011;
		audio.currentSample = audio.data & 0xff;
		audio.perCounter = Reg(am::Register(int(Register::AUD0PER) + channel * 0x10));
	}	break;

	case 0b011:
	{
		if (audio.perCounter > 1)
		{
			audio.perCounter--;
			return;
		}

		bool activeInt = (Reg(Register::INTREQR) & (0x0080 << channel)) != 0;
		if (audio.dmaOn || !activeInt)
		{
			audio.data = audio.holdingLatch;
			audio.currentSample = uint8_t(audio.data >> 8);
			audio.perCounter = Reg(am::Register(int(Register::AUD0PER) + channel * 0x10));
			audio.state = 0b010;
			if (audio.dmaOn)
			{
				audio.dmaReq = true;
			}
			if (!audio.dmaOn || audio.intreq2)
			{
				auto& intreqr = Reg(Register::INTREQR);
				intreqr |= (0x0080 << channel);
				DoInterruptRequest();
				audio.intreq2 = false;
			}
		}
		else
		{
			// go idle when DMA is off and interrupt has not been serviced
			audio.state = 0b000;
		}

	}	break;

	case 0b100:
	case 0b110:
	case 0b111:
		// unused (should not occur?)
		break;
	}
}

void am::Amiga::UpdateAudioChannelOnDmaChange(int channel, bool dmaOn)
{
	auto& audio = m_audio[channel];

	switch (audio.state)
	{
	case 0b000: // idle
		if (!dmaOn)
			DEBUGGER_BREAK(); // Shouldn't happen

		audio.state = 0b001;
		audio.dmaOn = true;
		audio.dmaReq = true;
		audio.lenCounter = Reg(am::Register(int(Register::AUD0LEN) + channel * 0x10));
		audio.perCounter = Reg(am::Register(int(Register::AUD0PER) + channel * 0x10));

		audio.pointer = 0;
		audio.pointer = uint32_t(Reg(am::Register(int(Register::AUD0LCH) + channel * 0x10))) << 16;
		audio.pointer |= Reg(am::Register(int(Register::AUD0LCL) + channel * 0x10));
		break;

	case 0b001:
	case 0b101:
	{
		if (dmaOn)
			DEBUGGER_BREAK(); // Shouldn't happen

		// If we turn off dma now, fall back to idle immediately
		audio.state = 0b000;
		audio.dmaOn = false;
	}	break;

	case 0b010:	// main cycle
	case 0b011:
		audio.dmaOn = dmaOn;
		break;

	case 0b100:
	case 0b110:
	case 0b111:
		// unused (should not occur?)
		break;
	}
}

void am::Amiga::UpdateAudioChannelOnData(int channel, uint16_t value)
{
	auto& audio = m_audio[channel];

	audio.holdingLatch = value;

	switch (audio.state)
	{

	case 0b000: // idle
	{
		bool activeInt = (Reg(Register::INTREQR) & (0x0080 << channel)) != 0;
		if (!audio.dmaOn && !activeInt)
		{
			audio.state = 0b010;
			audio.data = audio.holdingLatch;
			audio.perCounter = Reg(am::Register(int(Register::AUD0PER) + channel * 0x10));

			auto& intreqr = Reg(Register::INTREQR);
			intreqr |= (0x0080 << channel);
			DoInterruptRequest();
		}
	}	break;

	case 0b001:
	{
		assert(audio.dmaOn);
		if (audio.lenCounter > 1)
			audio.lenCounter--;

		auto& intreqr = Reg(Register::INTREQR);
		intreqr |= (0x0080 << channel);
		DoInterruptRequest();

		audio.state = 0b101;
		audio.dmaReq = true;
	}	break;

	case 0b101:
	{
		assert(audio.dmaOn);
		audio.perCounter = Reg(am::Register(int(Register::AUD0PER) + channel * 0x10));
		audio.data = audio.holdingLatch;
		audio.dmaReq = true;
		audio.state = 0b010;
	}	break;

	case 0b010:	// main cycle
	case 0b011:
	{
		if (audio.dmaOn)
		{
			if (audio.lenCounter > 1)
			{
				audio.lenCounter--;
			}
			else
			{
				audio.lenCounter = Reg(am::Register(int(Register::AUD0LEN) + channel * 0x10));
				audio.pointer = 0;
				audio.pointer = uint32_t(Reg(am::Register(int(Register::AUD0LCH) + channel * 0x10))) << 16;
				audio.pointer |= Reg(am::Register(int(Register::AUD0LCL) + channel * 0x10));
				audio.intreq2 = true;
			}
		}

	}	break;

	case 0b100:
	case 0b110:
	case 0b111:
		// unused (should not occur?)
		break;
	}
}

namespace
{
	constexpr char shapshotMagicValue[] = "GuRuAmi";
	constexpr int shapshotVersion = 0x01;
}

void am::Amiga::WriteSnapshot(std::ostream& os) const
{
	using util::Stream;

	Stream(os, shapshotMagicValue);
	Stream(os, shapshotVersion);

	const_cast<Amiga*>(this)->Stream(os);
}

bool am::Amiga::ReadSnapshot(std::istream& is)
{
	char magicValue[8];

	using util::Stream;

	Stream(is, magicValue);
	if (strncmp(magicValue, shapshotMagicValue, sizeof magicValue))
	{
		return false;
	}

	int version;
	Stream(is, version);

	if (version != shapshotVersion)
		return false;

	this->Stream(is);

	return true;
}

template <typename S>
void am::Amiga::Stream(S& s)
{
	m_m68000->Stream(s);

	using util::Stream;
	using util::StreamVector;

	Stream(s, m_romOverlayEnabled);
	Stream(s, m_isNtsc);
	Stream(s, m_running);
	Stream(s, m_rightMouseButtonDown);

	Stream(s, m_bitplane);

	Stream(s, m_vPos);
	Stream(s, m_hPos);
	Stream(s, m_lineLength);
	Stream(s, m_frameLength);

	Stream(s, m_bpFetchState);
	Stream(s, m_fetchPos);

	Stream(s, m_windowStartX);
	Stream(s, m_windowStopX);
	Stream(s, m_windowStartY);
	Stream(s, m_windowStopY);

	Stream(s, m_sharedBusRws);
	Stream(s, m_exclusiveBusRws);

	Stream(s, m_timerCountdown);

	Stream(s, m_totalCClocks);
	Stream(s, m_cpuBusyTimer);

	Stream(s, m_copper);

	Stream(s, m_blitter);

	Stream(s, m_blitterCountdown);

	Stream(s, m_cia[0]);
	Stream(s, m_cia[1]);

	Stream(s, m_palette);

	Stream(s, m_pixelBufferLoadPtr);
	Stream(s, m_pixelBufferReadPtr);
	Stream(s, m_pixelFetchDelay);

	for (int i = 0; i < 8; i++)
	{
		Stream(s, m_sprite[i]);
	}

	for (int i = 0; i < 4; i++)
	{
		Stream(s, m_floppyDrive[i]);
	}

	Stream(s, m_driveSelected);
	Stream(s, m_diskRotationCountdown);

	Stream(s, m_diskDma);

	Stream(s, m_keyQueue);
	Stream(s, m_keyQueueFront);
	Stream(s, m_keyQueueBack);
	Stream(s, m_keyCooldown);

	for (int i = 0; i < 4; i++)
	{
		Stream(s, m_audio[i]);
	}

	StreamVector(s, m_registers);
	StreamVector(s, m_chipRam);
	StreamVector(s, m_slowRam);
}
