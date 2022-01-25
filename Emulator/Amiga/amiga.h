#pragma once

#include "68000.h"
#include "screen_buffer.h"

#include <stdint.h>
#include <tuple>
#include <vector>
#include <memory>

namespace am
{
	enum class Register : uint32_t;

	enum class ChipRamConfig : uint32_t
	{
		ChipRam256k = 256 * 1024,
		ChipRam512k = 512 * 1024,
		ChipRam1Mib = 1024 * 1024,
		ChipRam2Mib = 2048 * 1024,
	};

	enum class Mapped : uint32_t
	{
		Reserved = 0x00,

		// Attributes
		Memory = 0x01,
		ChipBus = 0x02,
		ReadOnly = 0x04,
		DmaVisible = 0x08,

		// Peripherals
		Cia = 0x10,
		RealTimeClock = 0x20,
		ChipRegisters = 0x40,
		AutoConfig = 0x80,

		// Mapped Types

		ChipRam = Memory | ChipBus | DmaVisible,
		SlowRam = Memory | ChipBus,
		FastRam = Memory,
		Rom = Memory | ReadOnly,
		Peripheral = 0xf0,

		Shared = ChipBus | ChipRegisters,
	};

	class Amiga : public cpu::IBus
	{
	public:
		explicit Amiga(ChipRamConfig chipRamConfig, std::vector<uint8_t> rom);

		uint8_t PeekByte(uint32_t addr) const;
		uint16_t PeekWord(uint32_t addr) const;
		void PokeByte(uint32_t addr, uint8_t value);

		uint16_t PeekRegister(am::Register r) const;

		bool ExecuteFor(uint64_t cclocks);
		bool ExecuteOneCpuInstruction();
		bool ExecuteToEndOfCpuInstruction();

		void Reset();

		const ScreenBuffer* GetScreen() const
		{
			return m_lastScreen.get();
		}

		const cpu::M68000* GetCpu() const
		{
			return m_m68000.get();
		}

		void SetPC(uint32_t pc);

		void SetBreakpoint(uint32_t addr);
		void ClearBreakpoint();
		void EnableBreakOnRegister(uint32_t regAddr);
		void DisableBreakOnRegister();

		bool isNTSC() const { return m_isNtsc; }

	public:
		virtual uint16_t ReadBusWord(uint32_t addr) override final;
		virtual void WriteBusWord(uint32_t addr, uint16_t value) override final;
		virtual uint8_t ReadBusByte(uint32_t addr) override final;
		virtual void WriteBusByte(uint32_t addr, uint8_t value) override final;

	private:
		std::tuple<Mapped, uint8_t*> GetMappedMemory(uint32_t addr);

		bool CpuReady() const
		{
			return m_cpuBusyTimer == 0 && m_exclusiveBusRws == 0 && m_sharedBusRws == 0;
		}

		bool DoOneTick();

		void UpdateScreen();

		void ResetCIA(int num);
		void WriteCIA(int num, int port, uint8_t data);
		uint8_t ReadCIA(int num, int port);

		uint16_t ReadRegister(uint32_t regNum);
		void WriteRegister(uint32_t regNum, uint16_t value);

		uint16_t& Reg(am::Register r)
		{
			return m_registers[uint32_t(r) / 2];
		}

		uint16_t UpdateFlagRegister(am::Register r, uint16_t value);

	private:

		struct CIA
		{
			uint8_t pra;
			uint8_t prb;
			uint8_t ddra;
			uint8_t ddrb;
		};

	private:
		std::vector<uint8_t> m_rom;
		std::vector<uint8_t> m_chipRam;
		std::vector<uint8_t> m_slowRam;

		bool m_romOverlayEnabled = false;
		bool m_isNtsc = false;

		bool m_breakpointEnabled = false;
		bool m_breakOnRegisterEnabled = false;
		bool m_breakAtNextInstruction = false;

		struct BitPlaneControl
		{
			bool hires = false;
			bool ham = false;
			bool doublePlayfield = false;
			bool compositeColourEnabled = false;
			bool genlockAudioEnabled = false;
			bool lightPenEnabled = false;
			bool interlaced = false;
			bool externalResync = false;
			uint8_t numPlanesEnabled = 0;
			uint8_t playfieldPriority = 0;
			uint8_t playfieldDelay[2] = {};
			uint8_t playfieldSpritePri[2] = {};
		} m_bitplane;

		int m_vPos = 0;
		int m_hPos = 0;
		int m_lineLength = 0;
		int m_frameLength = 0;

		uint32_t m_breakpoint  = 0;
		uint32_t m_breakAtRegister = 0;

		uint32_t m_sharedBusRws = 0;
		uint32_t m_exclusiveBusRws = 0;

		uint64_t m_totalCClocks = 0;
		int m_cpuBusyTimer = 0;

		std::vector<uint16_t> m_registers;

		std::unique_ptr<cpu::M68000> m_m68000;

		CIA m_cia[2];

		std::array<ColourRef, 64> m_palette;

		std::unique_ptr<ScreenBuffer> m_currentScreen;
		std::unique_ptr<ScreenBuffer> m_lastScreen;
	};

}