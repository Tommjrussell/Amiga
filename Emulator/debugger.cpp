#include "debugger.h"

#include "App.h"

#include "amiga/disassembler.h"
#include "amiga/amiga.h"
#include "amiga/68000.h"
#include "amiga/registers.h"

#include "util/strings.h"

#include <imgui.h>

namespace guru
{
	class DebuggerMemoryInterface : public am::IMemory
	{
	public:
		explicit DebuggerMemoryInterface(am::Amiga* amiga)
			: m_amiga(amiga)
		{
		}

		uint16_t GetWord(uint32_t addr) const override
		{
			return m_amiga->PeekWord(addr);
		}

		uint8_t GetByte(uint32_t addr) const override
		{
			return m_amiga->PeekByte(addr);
		}

		private:
			am::Amiga* m_amiga;
	};
}

guru::Debugger::Debugger(guru::AmigaApp* app, am::Amiga* amiga)
	: m_app(app)
	, m_amiga(amiga)
{
	m_memory = std::make_unique<DebuggerMemoryInterface>(amiga);
	m_disassembler = std::make_unique<am::Disassembler>(m_memory.get());
}

guru::Debugger::~Debugger()
{
}

bool guru::Debugger::Draw()
{
	bool open = true;
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	bool expanded = ImGui::Begin("Debugger", &open);

	if (!(open && expanded))
	{
		ImGui::End();
		return open;
	}

	{
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
		ImGui::BeginChild("Child_Side", ImVec2(300, 0), true, 0);

		DrawCpuRegisters();
		DrawControls();

		if (ImGui::CollapsingHeader("Interrupts"))
		{
			DrawSystemInterrupts();
		}

		if (ImGui::CollapsingHeader("DMA"))
		{
			DrawDMA();
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::SameLine();

	// Assembly view

	ImGui::BeginChild("", ImVec2(0, 0), false, 0);

	int flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;
	if (m_trackPc)
	{
		flags |= ImGuiInputTextFlags_ReadOnly;
	}

	ImGui::PushItemWidth(64);
	if (ImGui::InputInt("Disassemble from addr", (int*)&m_disassemblyStart, 0, 0, flags))
	{
		m_disassembly.clear();
	}
	ImGui::PopItemWidth();

	ImGui::SameLine();

	if (ImGui::Checkbox("Track PC", &m_trackPc))
	{
		m_disassembly.clear();
	}

	{
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
		ImGui::BeginChild("Child_Disassembly", ImVec2(0, 0), true, 0);

		if (m_app->IsRunning())
		{
			ImGui::Text("<< RUNNING >>");
		}
		else
		{
			if (m_disassembly.empty())
			{
				UpdateAssembly();
			}

			auto cpu = m_amiga->GetCpu();
			auto pc = cpu->GetPC();

			for (auto& d : m_disassembly)
			{
				if (d.addr == pc)
				{
					ImGui::Selectable(d.text.c_str(), true);
				}
				else
				{
					ImGui::Text(d.text.c_str());
				}
			}
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::EndChild();


	ImGui::End();
	return open;
}

void guru::Debugger::UpdateAssembly()
{
	auto cpu = m_amiga->GetCpu();
	auto pc = cpu->GetPC();

	if (m_trackPc)
	{
		auto[history, ptr] = cpu->GetOperationHistory();

		uint32_t earliestAddr = pc;

		// Look for the earliest recently previously-executed instruction that
		// is not more than 32 bytes earlier than the current pc

		for (int i = 0; i < history->size(); i++)
		{
			if ((*history)[i] < earliestAddr && ((*history)[i] + 32) >= pc)
			{
				earliestAddr = (*history)[i];
			}
		}

		m_disassemblyStart = earliestAddr;
	}

	m_disassembler->pc = m_disassemblyStart;

	bool snapToPc = (m_disassemblyStart > pc);

	for (int lines = 0; lines < 48; lines++)
	{
		bool onPc = (m_disassembler->pc == pc);
		if (onPc && !snapToPc)
		{
			snapToPc = true;
		}
		else if (!snapToPc && m_disassembler->pc > pc)
		{
			// Nasty cludge. Force the disassembler to the PC if we've skipped
			// over it. Usually caused by the presence of padding or data bytes
			// between an earlier visited instruction and the PC.
			// (TODO : improve this)
			m_disassembler->pc = pc;
			snapToPc = onPc = true;
		}

		const uint32_t addr = m_disassembler->pc;
		std::string line = HexToString(addr);
		line += onPc ? " > " : " : ";
		line += m_disassembler->Disassemble();
		m_disassembly.emplace_back(DisassemblyLine{addr, line});
	}
}

void guru::Debugger::DrawCpuRegisters()
{
	auto regs = m_amiga->GetCpu()->GetRegisters();

	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("Child_Registers", ImVec2(0, 220.0f), true, 0);

	ImGui::Columns(3, NULL, false);

	ImGui::PushItemWidth(64);
	for (int i = 0; i < 8; i++)
	{
		char buff[8];
		sprintf_s(buff, "a%d", i);
		ImGui::InputInt(buff, (int*)&regs.a[i], 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	}
	ImGui::PopItemWidth();

	ImGui::NextColumn();

	ImGui::PushItemWidth(64);
	for (int i = 0; i < 8; i++)
	{
		char buff[8];
		sprintf_s(buff, "d%d", i);
		ImGui::InputInt(buff, (int*)&regs.d[i], 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	}
	ImGui::PopItemWidth();

	ImGui::NextColumn();

	ImGui::PushItemWidth(64);
	const int pcDislayFlags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;

	if (ImGui::InputInt("pc", (int*)&regs.pc, 0, 0, pcDislayFlags))
	{
		m_amiga->SetPC(regs.pc);
		m_disassembly.clear();
	}

	ImGui::InputInt(m_amiga->GetCpu()->InSupervisorMode() ? "usp" : "ssp",
		(int*)&regs.altA7, 0, 0,
		ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::PopItemWidth();

	bool extendFlag = (regs.status & 0x10) != 0;
	bool negativeFlag = (regs.status & 0x08) != 0;
	bool zeroFlag = (regs.status & 0x04) != 0;
	bool overflowFlag = (regs.status & 0x02) != 0;
	bool carryFlag = (regs.status & 0x01) != 0;

	ImGui::Checkbox("eXtend", &extendFlag);
	ImGui::Checkbox("Negative", &negativeFlag);
	ImGui::Checkbox("Zero", &zeroFlag);
	ImGui::Checkbox("oVerflow", &overflowFlag);
	ImGui::Checkbox("Carry", &carryFlag);

	ImGui::NextColumn();
	bool supervisorMode = m_amiga->GetCpu()->InSupervisorMode();
	ImGui::Checkbox("Super", &supervisorMode);

	ImGui::NextColumn();
	ImGui::PushItemWidth(16);
	int intMask = (regs.status & 0x0700) >> 8;
	ImGui::InputInt("Int Mask", &intMask, 0, 0, ImGuiInputTextFlags_ReadOnly);
	ImGui::PopItemWidth();

	ImGui::EndChild();
	ImGui::PopStyleVar(2);
}

void guru::Debugger::DrawControls()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("Controls", ImVec2(0, 104), true, 0);

	const bool running = m_app->IsRunning();
	auto regs = m_amiga->GetCpu()->GetRegisters();

	auto ActiveButton = [](const char* name, bool active, const ImVec2& size = ImVec2(0, 0)) -> bool
	{
		if (!active)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 128, 128, 255));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(128, 128, 128, 255));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(128, 128, 128, 255));
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
		}

		auto ret = ImGui::Button(name, size);

		if (!active)
		{
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(3);
		}

		return active && ret;
	};

	if (ActiveButton("Start", !running))
	{
		m_amiga->ClearBreakpoint();
		m_app->SetRunning(true);
		m_disassembly.clear();
	}

	ImGui::SameLine();
	if (ActiveButton("Stop", running))
	{
		m_app->SetRunning(false);
		m_amiga->ClearBreakpoint();
	}

	ImGui::SameLine();
	if (ActiveButton("Step", !running))
	{
		m_amiga->ClearBreakpoint();
		m_amiga->ExecuteOneCpuInstruction();
		m_disassembly.clear();
	}


	ImGui::SameLine();
	if (ActiveButton("Step Over", !running))
	{
		// Use the disassembler to calculate the length of the next instruction
		m_disassembler->pc = regs.pc;
		m_disassembler->Disassemble(); // ignore disassembled string
		m_amiga->SetBreakpoint(m_disassembler->pc);
		m_app->SetRunning(true);
		m_disassembly.clear();
	}

	ImGui::SameLine();
	if (ImGui::Button("Reset"))
	{
		m_amiga->ClearBreakpoint();
		m_amiga->Reset();
		m_disassembly.clear();
	}

	if (ActiveButton("Run Until :", !running))
	{
		m_amiga->SetBreakpoint(m_breakpoint);
		m_app->SetRunning(true);
		m_disassembly.clear();
	}

	ImGui::SameLine();
	ImGui::PushItemWidth(64);
	ImGui::InputInt("", (int*)&m_breakpoint, 0, 0, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::PopItemWidth();

	if (ImGui::Checkbox("Break on unimplemented register", &m_breakOnUnimplementedRegister))
	{
		if (m_breakOnUnimplementedRegister)
		{
			m_amiga->EnableBreakOnRegister(0xffff'ffff);
		}
		else
		{
			m_amiga->DisableBreakOnRegister();
		}
	}

	if (ImGui::Button("PC History"))
		ImGui::OpenPopup("pc_history_popup");

	if (ImGui::BeginPopup("pc_history_popup"))
	{
		auto cpu = m_amiga->GetCpu();
		auto[history, ptr] = cpu->GetOperationHistory();

		for (int i = 0; i < history->size(); i++)
		{
			if (ptr == 0)
			{
				ptr = history->size();
			}
			ptr--;

			if ((*history)[ptr] == 0xffffffff)
				break;

			ImGui::Text("%08x", (*history)[ptr]);

		}
		ImGui::EndPopup();
	}

	ImGui::EndChild();
	ImGui::PopStyleVar(2);
}

void guru::Debugger::DrawSystemInterrupts()
{
	static const char* ints[14] =
	{
		"EXTE", "DSYN", "RBF", "AUD3", "AUD2", "AUD1", "AUD0", "BLIT", "VRTB", "COPR", "PORT", "SOFT", "DBLK", "TBE"
	};
	static const char* desc[14] =
	{
		"EXTERN\nExternal interrupt\nBIT#:13\nLEVEL:6",
		"DSKSYN\nDisk sync register ( DSKSYNC )\nmatches disk data\nBIT#:12\nLEVEL:5",
		"RBF\nSerial port receive buffer full\nBIT#:11\nLEVEL:5",
		"AUD3\nAudio channel 3 block finished\nBIT#:10\nLEVEL:4",
		"AUD2\nAudio channel 2 block finished\nBIT#:9\nLEVEL:4",
		"AUD1\nAudio channel 1 block finished\nBIT#:8\nLEVEL:4",
		"AUD0\nAudio channel 0 block finished\nBIT#:7\nLEVEL:4",
		"BLIT\nBlitter finished\nBIT#:6\nLEVEL:3",
		"VERTB\nStart of vertical blank\nBIT#:5\nLEVEL:3",
		"COPER\nCopper\nBIT#:4\nLEVEL:3",
		"PORTS\nI/O ports and timers\nBIT#:3\nLEVEL:2",
		"SOFT\nReserved for software-initiated\ninterrupt\nBIT#:2\nLEVEL:1",
		"DSKBLK\nDisk block finished\nBIT#:1\nLEVEL:1",
		"TBE\nSerial port transmit buffer empty\nBIT#:0\nLEVEL:1",
	};

	const uint16_t intenar = m_amiga->PeekRegister(am::Register::INTENAR);
	const uint16_t intreqr = m_amiga->PeekRegister(am::Register::INTREQR);

	bool masterEnabled = (intenar & (1 << 14)) != 0;
	ImGui::Checkbox("INTEN", &masterEnabled);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Master interrupt\n(enable only, no request)");

	ImGui::Columns(7, NULL, false);

	for (int row = 0; row < 2; row++)
	{
		int rowStart = row * 7;
		int rowEnd = rowStart + 7;

		for (int i = rowStart; i < rowEnd; i++)
		{
			ImGui::Text(ints[i]);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(desc[i]);

			bool enabled = (intenar & (1 << (13 - i))) != 0;
			bool set     = (intreqr & (1 << (13 - i))) != 0;

			ImGui::Checkbox("", &enabled);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(enabled ? "Enabled" : "Disabled");
			ImGui::Checkbox("", &set);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(set ? (enabled ? "Set" : "Set but disabled") : "Not set");
			ImGui::NextColumn();
		}
	}
	ImGui::Columns(1);
}

void guru::Debugger::DrawDMA()
{
	static const char* bits[15] =
	{
		"BBUSY", "BZERO", nullptr, nullptr, "BLTPRI", "DMAEN", "BPLEN", "COPEN", "BLTEN", "SPREN", "DSKEN", "AUD3EN", "AUD2EN", "AUD1EN", "AUD0EN"
	};

	static const char* desc[15] =
	{
		"BBUSY\nBlitter busy status bit (read only)",
		"BZERO\nBlitter logic  zero status bit\n(read only).",
		nullptr,
		nullptr,
		"BLTPRI\nBlitter DMA priority\n"
			"(over CPU micro) (also called\n"
			"\"blitter nasty\") (disables /BLS\n"
			"pin, preventing micro from\n"
			"stealing any bus cycles while\n"
			"blitter DMA is running).",
		"DMAEN\nEnable all DMA",
		"BPLEN\nBitplane DMA enable",
		"COPEN\nCopper DMA enable",
		"BLTEN\nBlitter DMA enable",
		"SPREN\nSprite DMA enable",
		"DSKEN\nDisk DMA enable",
		"AUD3EN\nAudio channel 3 DMA enable",
		"AUD2EN\nAudio channel 2 DMA enable",
		"AUD1EN\nAudio channel 1 DMA enable",
		"AUD0EN\nAudio channel 0 DMA enable",
	};

	const uint16_t dmaconr = m_amiga->PeekRegister(am::Register::DMACONR);

	ImGui::Columns(5, NULL, false);

	for (int row = 0; row < 3; row++)
	{
		int rowStart = row * 5;
		int rowEnd = rowStart + 5;

		for (int i = rowStart; i < rowEnd; i++)
		{
			if (bits[i] != nullptr)
			{
				ImGui::Text(bits[i]);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip(desc[i]);

				bool set = (dmaconr & (1 << (14 - i))) != 0;

				ImGui::Checkbox("", &set);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip(desc[i]);
			}

			ImGui::NextColumn();

		}
	}
	ImGui::Columns(1);

}