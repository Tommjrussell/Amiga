#pragma once

#include <stdint.h>
#include <tuple>
#include <vector>

namespace am
{
	enum class ChipRamConfig : uint32_t
	{
		ChipRam256k = 256 * 1024,
		ChipRam512k = 512 * 1024,
		ChipRam1Mib = 1024 * 1024,
		ChipRam2Mib = 2048 * 1024,
	};

	enum class Mapped
	{
		// Attributes
		Memory = 0x01,
		ChipBus = 0x02,
		ReadOnly = 0x04,
		DmaVisible = 0x08,

		// Peripherals
		Cia = 0x10,
		RealTimeClock = 0x20,
		ChipRegisters = 0x30,
		AutoConfig = 0x40,

		// Mapped Types
		Reserved = 0x00,
		ChipRam = Memory | ChipBus | DmaVisible,
		SlowRam = Memory | ChipBus,
		FastRam = Memory,
		Rom = Memory | ReadOnly,
		Peripheral = 0xf0,
	};

	class Amiga
	{
	public:
		explicit Amiga(ChipRamConfig chipRamConfig, std::vector<uint8_t> rom);

		uint8_t PeekByte(uint32_t addr) const;
		uint16_t PeekWord(uint32_t addr) const;
		void PokeByte(uint32_t addr, uint8_t value);

	private:
		std::tuple<Mapped, uint8_t*> GetMappedMemory(uint32_t addr);

	private:
		std::vector<uint8_t> m_rom;
		std::vector<uint8_t> m_chipRam;
		std::vector<uint8_t> m_slowRam;

		bool m_romOverlayEnabled = true;
	};

}