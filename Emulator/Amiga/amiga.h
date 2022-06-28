#pragma once

#include "68000.h"
#include "screen_buffer.h"
#include "audio.h"
#include "mfm.h"

#include <stdint.h>
#include <tuple>
#include <vector>
#include <memory>
#include <string>

namespace am
{
	enum class Register : uint32_t;
	enum class Dma : uint16_t;

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

	struct CIA
	{
		uint8_t pra = 0;
		uint8_t prb = 0;
		uint8_t ddra = 0;
		uint8_t ddrb = 0;
		uint8_t sdr = 0;

		uint8_t irqData = 0;
		uint8_t irqMask = 0;
		bool intSignal = false;

		uint32_t tod = 0;			// time-of-day counter (TOD)
		uint32_t todLatched = 0;	// latched value of TOD. Counter continues to count while latched.
		uint32_t todAlarm = 0;

		bool todRunning = true;
		bool todWriteAlarm = false;
		bool todIsLatched = false;

		bool timerBCountsUnderflow = false;

		struct Timer
		{
			bool running = false;
			bool continuous = false;
			uint16_t value = 0;
			uint16_t latchedValue = 0;
			uint8_t controlRegister = 0;

			void SetLSB(uint8_t data);
			void SetMSB(uint8_t data);
			void ConfigTimerCIA(uint8_t data);
			bool Tick();

		} timer[2];
	};

	enum class CopperState : uint8_t
	{
		Stopped,
		Waiting,
		Read,
		Move,
		WaitSkip,
		Abort,
		WakeUp,
	};

	struct Copper
	{
		uint32_t pc = 0;
		uint32_t readAddr = 0;
		uint16_t ir1 = 0;
		uint16_t ir2 = 0;
		uint16_t verticalWaitPos = 0;
		uint16_t horizontalWaitPos = 0;
		uint16_t verticalMask = 0;
		uint16_t horizontalMask = 0;
		CopperState state = CopperState::Stopped;
		bool skipping = false;
		bool waitForBlitter = false;
	};

	struct Blitter
	{
		uint32_t ptr[4];
		int32_t modulo[4];
		uint16_t data[4];
		bool enabled[4];
		int lines;
		int wordsPerLine;
		uint16_t firstWordMask;
		uint16_t lastWordMask;
		uint8_t minterm;
	};

	struct FloppyDrive
	{
		bool selected = false;
		bool motorOn = false;
		bool stepSignal = false;
		bool diskChange = false;
		uint8_t currCylinder = 0;
		uint8_t side = 0;
	};

	struct FloppyDisk
	{
		std::string fileId;
		std::vector<uint8_t> data;
		DiskImage image;
	};

	struct DiskDma
	{
		uint32_t ptr = 0;
		uint32_t len = 0;
		uint16_t encodedSequenceCounter = 0;
		uint8_t  encodedSequenceBitOffset = 0;
		bool writing = false;
		bool inProgress = false;
		bool secondaryDmaEnabled = false;
		bool useWordSync = false;
	};

	struct Sprite
	{
		bool active = false;
		bool armed = false;
		bool attached = false;
		int8_t drawPos = 0;
		uint16_t horizontalStart = 0;
		int startLine = 0;
		int endLine = 0;
		uint32_t ptr = 0;
	};

	struct AudioChannel
	{
		uint32_t pointer = 0;
		uint8_t currentSample = 128; // 128 is the 'rest' level.
		uint8_t volume = 0;

		uint8_t state = 0b000;
		bool	dmaOn = false;
		bool	dmaReq = false;
		bool	intreq2 = false;
		uint16_t data = 0;
		uint16_t perCounter = 0;
		uint16_t holdingLatch = 0; // Next word of data from AUDxDAT
		uint16_t lenCounter = 0;
	};

	class Amiga : public cpu::IBus
	{
	public:
		explicit Amiga(ChipRamConfig chipRamConfig, std::vector<uint8_t> rom);

		void SetAudioPlayer(am::AudioPlayer* player)
		{
			m_audioPlayer = player;
		}

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

		const CIA* GetCIA(int num)
		{
			return &m_cia[num];
		}

		const Copper& GetCopper() const
		{
			return m_copper;
		}

		const FloppyDrive& GetFloppyDrive(int n) const
		{
			return m_floppyDrive[n];
		}

		void SetPC(uint32_t pc);

		void SetBreakpoint(uint32_t addr);
		void EnableBreakOnCopper();
		void ClearBreakpoint();
		void EnableBreakOnRegister(uint32_t regAddr);
		void DisableBreakOnRegister();
		void EnableBreakOnLine(uint32_t lineNum);

		void SetDataBreakpoint(uint32_t addr, uint32_t size);
		void DisableDataBreakpoint();
		bool DataBreakpointTriggered();

		bool isNTSC() const
		{
			return m_isNtsc;
		}

		int GetVPos() const
		{
			return m_vPos;
		}

		int GetFrameLength() const
		{
			return m_frameLength;
		}

		const std::string& GetDisk(int driveNum) const;
		bool SetDisk(int driveNum, const std::string& fileId, std::vector<uint8_t>&& diskImage);
		void EjectDisk(int driveNum);

		bool IsDiskInserted(int driveNum) const
		{
			return !m_floppyDisk[driveNum].fileId.empty();
		}

		void SetControllerButton(int controller, int button, bool pressed);
		void SetJoystickMove(int x, int y);
		void SetMouseMove(int x, int y);

		void QueueKeyPress(uint8_t keycode);

	public:
		virtual uint16_t ReadBusWord(uint32_t addr) override final;
		virtual void WriteBusWord(uint32_t addr, uint16_t value) override final;
		virtual uint8_t ReadBusByte(uint32_t addr) override final;
		virtual void WriteBusByte(uint32_t addr, uint8_t value) override final;

	private:

		uint16_t ReadChipWord(uint32_t addr) const;
		void WriteChipWord(uint32_t addr, uint16_t value);

		std::tuple<Mapped, uint8_t*> GetMappedMemory(uint32_t addr);

		bool CpuReady() const
		{
			return m_cpuBusyTimer == 0 && m_exclusiveBusRws == 0 && m_sharedBusRws == 0;
		}

		void DoOneTick();

		void UpdateScreen();

		void DoCopper(bool& chipBusBusy);
		bool DoScanlineDma();
		void DoInstantBlitter();

		void WriteCIA(int num, int port, uint8_t data);
		uint8_t ReadCIA(int num, int port);
		void TickCIAtod(int num);

		void TickCIATimers();
		void SetCIAInterrupt(CIA& cia, uint8_t bit);

		uint16_t ReadRegister(uint32_t regNum);
		void WriteRegister(uint32_t regNum, uint16_t value);

		void DoInterruptRequest();

		uint16_t& Reg(am::Register r)
		{
			return m_registers[uint32_t(r) / 2];
		}

		uint16_t Reg(am::Register r) const
		{
			return m_registers[uint32_t(r) / 2];
		}

		void SetLongPointerValue(uint32_t& ptr, bool IsHighWord, uint16_t value)
		{
			if (IsHighWord)
			{
				// TODO : mask top bits differently depending on OCS/ECS
				ptr &= 0x0000ffff;
				ptr |= uint32_t(value & 0x001f) << 16;
			}
			else
			{
				ptr &= 0xffff0000;
				ptr |= value & 0xfffe;
			}
		}

		uint16_t UpdateFlagRegister(am::Register r, uint16_t value);

		bool DmaEnabled(am::Dma dmaChannel) const;

		void ProcessDriveCommands(uint8_t data);
		void UpdateFloppyDriveFlags();
		void StartDiskDMA();
		void DoDiskDMA();

		void TransmitKeyCode();

		bool DoAudioDMA(int channel);
		void UpdateAudioChannel(int channel);
		void UpdateAudioChannelOnDmaChange(int channel, bool dmaOn);
		void UpdateAudioChannelOnData(int channel, uint16_t value);

	private:
		std::vector<uint8_t> m_rom;
		std::vector<uint8_t> m_chipRam;
		std::vector<uint8_t> m_slowRam;

		bool m_romOverlayEnabled = false;
		bool m_isNtsc = false;

		bool m_breakpointEnabled = false;
		bool m_breakOnRegisterEnabled = false;
		bool m_breakAtNextInstruction = false;
		bool m_breakAtNextCopperInstruction = false;
		bool m_breakAtLine = false;
		bool m_breakAtAddressChanged = false;
		bool m_running = true;

		bool m_rightMouseButtonDown = false;

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
			uint32_t ptr[6] = {};
		} m_bitplane;

		int m_vPos = 0;
		int m_hPos = 0;
		int m_lineLength = 0;
		int m_frameLength = 0;

		enum class BpFetchState
		{
			Off,		// No fetching on this line.
			Idle,		// waiting to reach DDFSTRT
			Fetching,	// between DDFSTRT and DDFSTOP
			Finishing,	// DDFSTOP reached, do last fetch and apply BPLxMOD
		};

		BpFetchState m_bpFetchState = BpFetchState::Off;
		int m_fetchPos = 0;

		int m_windowStartX = 0;
		int m_windowStopX = 0;
		int m_windowStartY = 0;
		int m_windowStopY = 0;

		uint32_t m_breakpoint  = 0;
		uint32_t m_breakAtRegister = 0;
		uint32_t m_breakAtLineNum = 0;
		uint32_t m_dataBreakpoint = 0;
		uint32_t m_currentDataBreakpointData = 0;
		uint32_t m_dataBreakpointSize = 0;

		uint32_t m_sharedBusRws = 0;
		uint32_t m_exclusiveBusRws = 0;

		int m_timerCountdown = 0;

		uint64_t m_totalCClocks = 0;
		int m_cpuBusyTimer = 0;

		Copper m_copper = {};

		Blitter m_blitter = {};

		int m_blitterCountdown = 0;

		std::vector<uint16_t> m_registers;

		std::unique_ptr<cpu::M68000> m_m68000;

		CIA m_cia[2];

		std::array<ColourRef, 64> m_palette;

		constexpr static int kPixelBufferSize = 64;
		constexpr static int kPixelBufferMask = kPixelBufferSize - 1;

		using PlayfieldBuffer = std::array<uint8_t, kPixelBufferSize>;

		std::array<PlayfieldBuffer, 2> m_playfieldBuffer;
		int m_pixelBufferLoadPtr = 0;
		int m_pixelBufferReadPtr = 0;
		int m_pixelFetchDelay = 0;

		std::unique_ptr<ScreenBuffer> m_currentScreen;
		std::unique_ptr<ScreenBuffer> m_lastScreen;

		Sprite m_sprite[8];

		FloppyDisk m_floppyDisk[4];
		FloppyDrive m_floppyDrive[4];
		int m_driveSelected;
		int m_diskRotationCountdown; // Countdown to next full disk rotation.

		DiskDma m_diskDma;

		// Keyboard
		static constexpr int kKeyQueueSize = 32;
		std::array<uint8_t, kKeyQueueSize> m_keyQueue;
		int m_keyQueueFront;
		int m_keyQueueBack;
		int m_keyCooldown;

		// Audio
		am::AudioPlayer* m_audioPlayer = nullptr;
		uint64_t m_audioBufferPos = 0;
		int m_audioBufferCountdown = 0;
		AudioChannel m_audio[4];
		AudioBuffer m_audioBuffer;
	};

}