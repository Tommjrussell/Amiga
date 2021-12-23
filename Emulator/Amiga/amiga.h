#pragma once

#include "68000.h"

#include <stdint.h>
#include <tuple>
#include <vector>
#include <memory>

namespace am
{
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

		bool ExecuteFor(uint64_t cclocks);
		bool ExecuteOneCpuInstruction();
		bool ExecuteToEndOfCpuInstruction();

		const cpu::M68000* GetCpu() const
		{
			return m_m68000.get();
		}

		void SetPC(uint32_t pc);

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

	private:
		std::vector<uint8_t> m_rom;
		std::vector<uint8_t> m_chipRam;
		std::vector<uint8_t> m_slowRam;

		bool m_romOverlayEnabled = true;

		uint32_t m_sharedBusRws = 0;
		uint32_t m_exclusiveBusRws = 0;

		uint64_t m_totalCClocks = 0;
		int m_cpuBusyTimer = 0;

		std::unique_ptr<cpu::M68000> m_m68000;
	};

}