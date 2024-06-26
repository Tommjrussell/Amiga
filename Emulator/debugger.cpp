#include "debugger.h"

#include "App.h"

#include "amiga/disassembler.h"
#include "amiga/amiga.h"
#include "amiga/68000.h"
#include "amiga/registers.h"
#include "amiga/symbols.h"

#include "util/strings.h"
#include "util/imgui_extras.h"

#include <imgui.h>
#include <sstream>

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

guru::Debugger::Debugger(guru::AmigaApp* app, am::Amiga* amiga, am::Symbols* symbols)
	: m_app(app)
	, m_amiga(amiga)
	, m_symbols(symbols)
{
	m_memory = std::make_unique<DebuggerMemoryInterface>(amiga);
	m_disassembler = std::make_unique<am::Disassembler>(m_memory.get());
	m_disassembler->SetSymbols(m_symbols);
}

guru::Debugger::~Debugger()
{
}

void guru::Debugger::Refresh()
{
	m_disassembly.clear();
}

bool guru::Debugger::Draw()
{
	const auto scale = ImGui::GetFrameHeightWithSpacing();

	bool open = true;
	ImGui::SetNextWindowSize(ImVec2(40 * scale, 16 * scale), ImGuiCond_FirstUseEver);
	bool expanded = ImGui::Begin("Debugger", &open);

	if (!(open && expanded))
	{
		ImGui::End();
		return open;
	}

	{
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
		ImGui::BeginChild("Child_Side", ImVec2(16 * scale, 0), true, 0);

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

		if (ImGui::CollapsingHeader("CIAs"))
		{
			DrawCIAs();
		}

		if (ImGui::CollapsingHeader("Disk Drives"))
		{
			DrawDiskDrives();
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::SameLine();

	// Assembly view

	ImGui::BeginChild("##AssemblyView", ImVec2(0, 0), false, 0);

	int flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;
	if (m_trackPc)
	{
		flags |= ImGuiInputTextFlags_ReadOnly;
	}

	ImGui::PushItemWidth(5 * scale);
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

	ImGui::SameLine();

	if (ImGui::Button("Reload Symbols"))
	{
		m_symbols->Load();
		m_disassembly.clear();
	}

	ImGui::SameLine();
	if (ImGui::Button("Copy Disassembly"))
	{
		std::stringstream ss;

		for (auto& d : m_disassembly)
		{
			auto sub = m_symbols->GetSub(d.addr);
			if (sub && sub->start == d.addr)
			{
				ss << sub->name << "\n";
			}
			ss << d.text << "\n";
		}
		ImGui::SetClipboardText(ss.str().c_str());
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
			bool recentre = false;
			if (m_disassembly.empty())
			{
				UpdateAssembly();
				recentre = m_trackPc;
			}

			auto cpu = m_amiga->GetCpu();
			auto pc = cpu->GetPC();

			int line = 0;
			char idBuff[8];

			for (auto& d : m_disassembly)
			{
				auto sub = m_symbols->GetSub(d.addr);
				if (sub && sub->start == d.addr)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0xEE, 0x44, 0, 255));
					ImGui::Text(sub->name.c_str());
					ImGui::PopStyleColor();
				}

				if (d.addr == pc)
				{
					if (recentre)
					{
						ImGui::SetScrollHereY(0.5f);
						recentre = false;
					}
				}

				// Colour gets progressively more yellow as number of recent visits increases
				static const ImU32 visitedColours[] =
				{
					IM_COL32(0, 255, 0, 255),
					IM_COL32(31, 255, 0, 255),
					IM_COL32(63, 255, 0, 255),
					IM_COL32(95, 255, 0, 255),
					IM_COL32(127, 255, 0, 255),
					IM_COL32(159, 255, 0, 255),
					IM_COL32(191, 255, 0, 255),
					IM_COL32(223, 255, 0, 255),
					IM_COL32(255, 255, 0, 255),
				};

				if (d.visited > 0)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, visitedColours[std::min(8, d.visited - 1)]);
				}

				ImGui::TextUnformatted(d.text.c_str(), d.text.c_str() + 10);
				ImGui::SameLine(0, 0);

				if (d.visited > 0)
				{
					ImGui::PopStyleColor();
				}

				sprintf_s(idBuff, "A%d", line);
				if (ImGui::BeginPopupContextItem(idBuff))
				{
					if (ImGui::MenuItem("Copy Addr"))
					{
						char clipboardText[12];
						sprintf_s(clipboardText, "0x%06x", d.addr);
						ImGui::SetClipboardText(clipboardText);
					}
					ImGui::EndPopup();
				}

				if (d.addr == pc)
				{
					ImGui::Selectable(d.text.c_str() + 10, true);
				}
				else
				{
					ImGui::TextUnformatted(d.text.c_str() + 10);
				}

				if (d.disassembledOpcode.source || d.disassembledOpcode.dest)
				{
					sprintf_s(idBuff, "O%d", line);
					if (ImGui::BeginPopupContextItem(idBuff))
					{
						char menuTxt[40];
						if (d.disassembledOpcode.source)
						{
							const uint32_t addr = CalculateEaAddress(d.disassembledOpcode.source.value(), d.addr + 2);
							sprintf_s(menuTxt, "Source : %s", AddressToString(addr).c_str());
							if (ImGui::MenuItem(menuTxt))
							{
								m_app->ShowMemInMemoryEditor(addr, d.disassembledOpcode.size);
							}
						}

						if (d.disassembledOpcode.dest)
						{
							const uint32_t addr = CalculateEaAddress(d.disassembledOpcode.dest.value(), d.addr + 2);

							sprintf_s(menuTxt, "Dest   : %s", AddressToString(addr).c_str());
							if (ImGui::MenuItem(menuTxt))
							{
								m_app->ShowMemInMemoryEditor(addr, d.disassembledOpcode.size);
							}
						}

						ImGui::EndPopup();
					}
				}

				++line;
			}
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::EndChild();


	ImGui::End();
	return open;
}


namespace
{
	uint32_t GetRegValue(const cpu::Registers& regs, am::Disasm::Reg reg, uint32_t opcodePc)
	{
		using namespace am::Disasm;

		if (reg == Reg::NoReg)
			return 0;

		if (reg == Reg::PC)
			return opcodePc;

		if (int(reg) >= int(Reg::A0) && int(reg) <= int(Reg::A7))
			return regs.a[int(reg) - int(Reg::A0)];

		if (int(reg) >= int(Reg::D0) && int(reg) <= int(Reg::D7))
			return regs.d[int(reg) - int(Reg::D0)];

		return 0; // unknown register
	}
}

std::string guru::Debugger::AddressToString(uint32_t addr) const
{
	char buffer[40];

	if (m_symbols)
	{
		auto var = m_symbols->GetVariable(addr);
		if (var)
		{
			if (var->addr == addr)
			{
				sprintf_s(buffer, "%s (%08x)", var->name.c_str(), addr);
			}
			else
			{
				sprintf_s(buffer, "%s+%01x (%08x)", var->name.c_str(), var->addr - addr, addr);
			}
			return std::string(buffer);
		}
	}

	sprintf_s(buffer, "%08x", addr);
	return std::string(buffer);
}

uint32_t guru::Debugger::CalculateEaAddress(const am::Disasm::EaMem& ea, uint32_t opcodePc) const
{
	auto& regs = m_amiga->GetCpu()->GetRegisters();

	uint32_t addr = GetRegValue(regs, ea.baseReg, opcodePc);
	addr += ea.displacement;

	auto indexValue = int32_t(GetRegValue(regs, ea.indexReg, opcodePc));
	if (ea.indexSize == 2)
		indexValue = int32_t(int16_t(indexValue));

	indexValue *= ea.indexScale;
	addr += indexValue;
	return addr;
}

void guru::Debugger::UpdateAssembly()
{
	auto cpu = m_amiga->GetCpu();
	auto pc = cpu->GetPC();

	auto [history, ptr] = cpu->GetOperationHistory();

	if (m_trackPc)
	{
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

	uint32_t disassemblyStop = 0;

	const am::Subroutine* currentSub = nullptr;
	const am::Subroutine* nextSub = nullptr;

	if (m_symbols)
	{
		auto sub = m_symbols->GetSub(m_trackPc ? pc : m_disassemblyStart);
		if (sub)
		{
			if (sub->start < m_disassemblyStart)
			{
				m_disassemblyStart = sub->start;
			}
			disassemblyStop = sub->end;
		}

		currentSub = m_symbols->GetSub(m_disassemblyStart);
		nextSub = m_symbols->NextSub(m_disassemblyStart);
	}

	m_disassembler->pc = m_disassemblyStart;

	bool snapToPc = (m_disassemblyStart > pc);

	for (int lines = 0; ; lines++)
	{
		if (lines >= 48)
		{
			if (disassemblyStop == 0)
				break;

			if (m_disassembler->pc > disassemblyStop)
				break;
		}

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

		uint32_t addr = m_disassembler->pc;

		auto disasm = m_disassembler->Disassemble();
		if (nextSub && m_disassembler->pc >= nextSub->start)
		{
			currentSub = nextSub;
			nextSub = m_symbols->NextSub(m_disassembler->pc);
			if (m_disassembler->pc > currentSub->start)
			{
				m_disassembler->pc = currentSub->start;
				disasm = { "???" };
			}
		}

		int visited = 0;
		for (auto ad : *history)
		{
			if (ad == addr)
				visited++;
		}

		std::string line = util::HexToString(addr);
		line += onPc ? " > " : " : ";
		line += disasm.text;
		m_disassembly.emplace_back(DisassemblyLine{addr, visited, std::move(disasm), line});
	}
}

void guru::Debugger::DrawCpuRegisters()
{
	auto regs = m_amiga->GetCpu()->GetRegisters();

	const auto scale = ImGui::GetFrameHeightWithSpacing();

	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.4f * scale);

	ImGui::BeginChild("Child_Registers", ImVec2(0, 9.6 * scale), true, 0);

	ImGui::Columns(3, NULL, false);

	ImGui::PushItemWidth(3 * scale);
	for (int i = 0; i < 8; i++)
	{
		char buff[8];
		sprintf_s(buff, "a%d", i);
		ImGui::InputInt(buff, (int*)&regs.a[i], 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Goto Address"))
			{
				m_app->ShowMemInMemoryEditor(regs.a[i], 0);
			}

			ImGui::EndPopup();
		}
	}
	ImGui::PopItemWidth();

	ImGui::NextColumn();

	ImGui::PushItemWidth(3 * scale);
	for (int i = 0; i < 8; i++)
	{
		char buff[8];
		sprintf_s(buff, "d%d", i);
		ImGui::InputInt(buff, (int*)&regs.d[i], 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	}
	ImGui::PopItemWidth();

	ImGui::NextColumn();

	ImGui::PushItemWidth(3 * scale);
	const int pcDislayFlags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;

	if (ImGui::InputInt("pc", (int*)&regs.pc, 0, 0, pcDislayFlags))
	{
		m_amiga->SetPC(regs.pc);
		m_disassembly.clear();
	}
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Goto Address"))
		{
			m_app->ShowMemInMemoryEditor(regs.pc, 0);
		}

		ImGui::EndPopup();
	}

	ImGui::InputInt(m_amiga->GetCpu()->InSupervisorMode() ? "usp" : "ssp",
		(int*)&regs.altA7, 0, 0,
		ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Goto Address"))
		{
			m_app->ShowMemInMemoryEditor(regs.altA7, 0);
		}

		ImGui::EndPopup();
	}
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
	ImGui::PushItemWidth(2 * scale);
	int intMask = (regs.status & 0x0700) >> 8;
	ImGui::InputInt("Int Mask", &intMask, 0, 0, ImGuiInputTextFlags_ReadOnly);
	ImGui::PopItemWidth();

	ImGui::NextColumn();
	bool trace = (regs.status & 0x8000) != 0;
	ImGui::Checkbox("Trace", &trace);


	ImGui::EndChild();
	ImGui::PopStyleVar(2);
}

void guru::Debugger::DrawControls()
{
	using util::ActiveButton;

	const auto scale = ImGui::GetFrameHeightWithSpacing();
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.4f * scale);
	ImGui::BeginChild("Controls", ImVec2(0, 6.6*scale), true, 0);

	const bool running = m_app->IsRunning();
	auto regs = m_amiga->GetCpu()->GetRegisters();

	if (ActiveButton("Start", !running))
	{
		m_amiga->ClearBreakpoint();
		m_app->SetRunning(true);
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
	}

	ImGui::SameLine();
	if (ImGui::Button("Reset"))
	{
		m_amiga->ClearBreakpoint();
		m_app->Reset();
		m_disassembly.clear();
	}

	if (ActiveButton("Run Until :", !running))
	{
		m_amiga->SetBreakpoint(m_breakpoint);
		m_app->SetRunning(true);
	}

	ImGui::SameLine();
	ImGui::PushItemWidth(3 * scale);
	ImGui::InputInt("##breakpoint", (int*)&m_breakpoint, 0, 0, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::PopItemWidth();

	if (ImGui::Checkbox("Break on register", &m_breakOnRegister))
	{
		if (m_breakOnRegister)
		{
			m_amiga->EnableBreakOnRegister(m_regBreakpoint);
		}
		else
		{
			m_amiga->DisableBreakOnRegister();
		}
	}
	ImGui::SameLine();

	ImGui::PushItemWidth(3 * scale);
	if (ImGui::InputInt("##registerBreakpoint", (int*)&m_regBreakpoint, 0, 0, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
	{
		if (m_breakOnRegister)
		{
			m_amiga->EnableBreakOnRegister(m_regBreakpoint);
		}
	}
	ImGui::PopItemWidth();

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
				ptr = uint32_t(history->size());
			}
			ptr--;

			if ((*history)[ptr] == 0xffffffff)
				break;

			char oldAddr[64];
			sprintf_s(oldAddr, "%08x##history%d", (*history)[ptr], ptr);
			if (ImGui::Selectable(oldAddr, false))
			{
				m_trackPc = 0;
				m_disassemblyStart = (*history)[ptr];
				m_disassembly.clear();
			}
		}
		ImGui::EndPopup();
	}

	static const char* dataBpSizeItems[] = { "byte", "word", "long" };

	if (ImGui::Checkbox("Enable Data Breakpoint", &m_dataBpEnabled))
	{
		if (m_dataBpEnabled)
		{
			m_amiga->SetDataBreakpoint(m_dataBp, 1 << m_dataBpSize);
		}
		else
		{
			m_amiga->DisableDataBreakpoint();
		}
	}
	ImGui::PushItemWidth(3 * scale);
	if (ImGui::InputInt("##DataBreakpoint", (int*)&m_dataBp, 0, 0, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
	{
		m_dataBp &= ~((1 << m_dataBpSize) - 1);
		if (m_dataBpEnabled)
		{
			m_amiga->SetDataBreakpoint(m_dataBp, 1 << m_dataBpSize);
		}
	}
	ImGui::SameLine();
	if (ImGui::Combo("##DataBreakpointSize", &m_dataBpSize, dataBpSizeItems, IM_ARRAYSIZE(dataBpSizeItems)))
	{
		m_dataBp &= ~((1 << m_dataBpSize) - 1);
		if (m_dataBpEnabled)
		{
			m_amiga->SetDataBreakpoint(m_dataBp, 1 << m_dataBpSize);
		}
	}
	ImGui::PopItemWidth();

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

			ImGui::Checkbox("##", &enabled);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(enabled ? "Enabled" : "Disabled");
			ImGui::Checkbox("##", &set);
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

				ImGui::Checkbox("##", &set);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip(desc[i]);
			}

			ImGui::NextColumn();

		}
	}
	ImGui::Columns(1);

}

void guru::Debugger::DrawCIAs()
{
	const auto scale = ImGui::GetFrameHeightWithSpacing();

	static const char label[2] = { 'A', 'B' };
	static const char* timerChildLabel[2][2] = {{ "CIAAtimerA", "CIAAtimerB" }, { "CIABtimerA", "CIABtimerB" }};
	static const char* todLabel[2] = { "ciaa-tod", "ciab-tod" };

	const am::CIA* cia[2];

	cia[0] = m_amiga->GetCIA(0);
	cia[1] = m_amiga->GetCIA(1);

	ImGui::Columns(2, NULL, false);

	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.4f * scale);

	for (int i = 0; i < 2; i++)
	{
		ImGui::Text("CIA-%c", label[i]);

		for (int j = 0; j < 2; j++)
		{
			ImGui::BeginChild(timerChildLabel[i][j], ImVec2(0, 2.9 * scale), true, 0);
			ImGui::Text("Timer %c ", label[j]);
			if (cia[i]->timer[j].continuous)
			{
				ImGui::SameLine();
				ImGui::Text("(c)");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("continuous");
			}
			if (cia[i]->timer[j].running)
			{
				ImGui::SameLine();
				ImGui::Text("(r)");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("running");
			}

			ImGui::Text("Value   : %04x", cia[i]->timer[j].value);
			ImGui::Text("Latched : %04x", cia[i]->timer[j].latchedValue);
			ImGui::EndChild();

		}

		ImGui::BeginChild(todLabel[i], ImVec2(0, 3.7 * scale), true, 0);
		ImGui::Text("TOD ");
		if (cia[i]->todRunning)
		{
			ImGui::SameLine();
			ImGui::Text("(r)");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Running");
		}
		if (cia[i]->todWriteAlarm)
		{
			ImGui::SameLine();
			ImGui::Text("(a)");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Writing Alarm Value");
		}
		if (cia[i]->todIsLatched)
		{
			ImGui::SameLine();
			ImGui::Text("(l)");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Value Latched");
		}

		ImGui::Text("Value   : %06x", cia[i]->tod);
		ImGui::Text("Latched : %06x", cia[i]->todLatched);
		ImGui::Text("Alarm   : %06x", cia[i]->todAlarm);
		ImGui::EndChild();

		ImGui::NextColumn();
	}
	ImGui::PopStyleVar(2);
	ImGui::Columns(1);
}

void guru::Debugger::DrawDiskDrives()
{
	ImGui::Columns(2, NULL, false);

	const auto scale = ImGui::GetFrameHeightWithSpacing();
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.4f * scale);

	auto DrawDriveDetails = [&](int n)
	{
		char id[20];
		sprintf_s(id, "DF%d", n);
		ImGui::BeginChild(id, ImVec2(0, 3.7 * scale), true, 0);

		auto& drive = m_amiga->GetFloppyDrive(n);

		ImGui::Text("%s %s", id, drive.selected ? "(selected)" : "");
		ImGui::Text(drive.motorOn ? "Motor ON" : "Motor OFF");
		ImGui::Text("Side     : %d", drive.side);
		ImGui::Text("Cylinder : %d", drive.currCylinder);

		ImGui::EndChild();
	};

	DrawDriveDetails(0);
	DrawDriveDetails(2);
	ImGui::NextColumn();
	DrawDriveDetails(1);
	DrawDriveDetails(3);

	ImGui::PopStyleVar(2);
	ImGui::Columns(1);

}