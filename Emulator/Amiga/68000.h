#pragma once

#include <stdint.h>
#include <array>

namespace cpu
{
	constexpr size_t kNumOpcodeEntries = 94;

	enum class ExecuteState : int
	{
		ReadyToDecode,
		ReadyToExecute,
		Stopped
	};

	struct Registers
	{
		uint32_t a[8];
		uint32_t d[8];
		uint32_t altA7;
		uint32_t pc;
		uint16_t status;
	};

	constexpr uint16_t Extend = 0b10000;
	constexpr uint16_t Negative = 0b01000;
	constexpr uint16_t Zero = 0b00100;
	constexpr uint16_t Overflow = 0b00010;
	constexpr uint16_t Carry = 0b00001;

	namespace EA
	{
		enum Flags : uint16_t
		{
			DataRegister = 0x0001, // Dn
			AddrRegister = 0x0002, // An
			Address = 0x0004, // (An)
			AddressWithPostInc = 0x0008, // (An)+
			AddressWithPreDec = 0x0010, // -(An)
			AddressWithDisplacement = 0x0020, // (d16,An)
			AddressWithIndex = 0x0040, // (d8,An,Xn)
			AbsoluteShort = 0x0080, // (xxx).w
			AbsoluteLong = 0x0100, // (xxx).l
			PcWithDisplacement = 0x0200, // (d16,PC)
			PcWithIndex = 0x0400, // (d8,PC,Xn)
			Immediate = 0x0800, // #imm

			// The following are named combinations of the previous flags
			IncDec = AddressWithPostInc | AddressWithPreDec,
			AddressRelative = AddressWithDisplacement | AddressWithIndex,
			PcRelative = PcWithDisplacement | PcWithIndex,
			Absolute = AbsoluteShort | AbsoluteLong,

			MemoryAlterable = Address | IncDec | AddressRelative | Absolute,
			DataAlterable = MemoryAlterable | DataRegister,
			Alterable = MemoryAlterable | DataRegister | AddrRegister,
			DataAddressing = DataAlterable | Immediate | PcRelative,
			ControlAlterable = Address | AddressRelative | Absolute,
			ControlAddressing = ControlAlterable | PcRelative,
			All = DataAddressing | AddrRegister,
		};
	}

	class IBus
	{
	public:
		virtual uint16_t ReadBusWord(uint32_t addr) = 0;
		virtual void WriteBusWord(uint32_t addr, uint16_t value) = 0;
		virtual uint8_t ReadBusByte(uint32_t addr) = 0;
		virtual void WriteBusByte(uint32_t addr, uint8_t value) = 0;
	};

	class M68000
	{
	public:
		explicit M68000(IBus* bus);

		void Reset(int& delay);

		const Registers& GetRegisters() const
		{
			return m_regs;
		}

		void SetPC(uint32_t pc)
		{
			m_regs.pc = pc;
		}

		uint32_t GetPC() const
		{
			return m_regs.pc;
		}

		bool InSupervisorMode() const
		{
			return (m_regs.status & 0x2000) != 0;
		}

		ExecuteState GetExecutionState() const
		{
			return m_executeState;
		}

		bool DecodeOneInstruction(int& delay);
		bool ExecuteOneInstruction(int& delay);

		std::pair<const std::array<uint32_t, 32>&, uint32_t> GetOperationHistory() const
		{
			return std::make_pair(m_operationHistory, m_operationHistoryPtr);
		}

	private:

		bool UnimplementOpcode(int& delay);

		bool Opcode_lea(int& delay);
		bool Opcode_move(int& delay);
		bool Opcode_subq(int& delay);
		bool Opcode_bcc(int& delay);
		bool Opcode_cmpa(int& delay);
		bool Opcode_cmpi(int& delay);

	private:

		bool EvaluateCondition(int condition) const;

		bool StartInternalException(uint8_t vectorNum);
		uint16_t FetchNextOperationWord();

		uint32_t ReadBusLong(uint32_t addr);
		void WriteBusLong(uint32_t addr, uint32_t value);

		uint32_t ReadBus(uint32_t addr, int size);
		void WriteBus(uint32_t addr, int size, uint32_t value);

		void SetStatusRegister(uint16_t value);
		void SetFlag(uint16_t flag, bool condition);

		enum class EffectiveAddressType : uint32_t
		{
			DataRegister,
			AddressRegister,
			MemoryAlterable,
			MemorySourceOnly,
			Immediate,
			Bad,
		};

		struct EA
		{
			EffectiveAddressType type;
			uint32_t addrIdx;
			uint32_t mode;
			uint32_t xn;
		};

		EA DecodeEffectiveAddress(uint32_t mode, uint32_t xn, int size, int& delay);
		bool GetEaValue(const EA& ea, int size, uint64_t& value);
		bool SetEaValue(const EA& ea, int size, uint64_t value);

	private:
		IBus* m_bus;
		Registers m_regs = { 0 };

		ExecuteState m_executeState = ExecuteState::ReadyToDecode;
		uint32_t m_operationAddr;
		uint32_t m_currentInstructionIndex = 0;
		uint32_t m_immediateValue = 0;
		uint16_t m_operation;
		uint8_t m_opcodeSize = 0;

		EA m_ea[2];

		std::array<uint32_t, 32> m_operationHistory;
		uint32_t m_operationHistoryPtr;

		typedef bool (cpu::M68000::* OpcodeInstruction)(int&);
		static OpcodeInstruction OpcodeFunction[kNumOpcodeEntries];

	};

}