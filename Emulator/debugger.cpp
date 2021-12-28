#include "debugger.h"

#include "App.h"

#include "amiga/disassembler.h"
#include "amiga/amiga.h"
#include "amiga/68000.h"

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


	bool open = true;
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	bool expanded = ImGui::Begin("Debugger", &open);

	if (!(open && expanded))
	{
		ImGui::End();
		return open;
	}

	auto regs = m_amiga->GetCpu()->GetRegisters();
	const bool running = m_app->IsRunning();

	{
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
		ImGui::BeginChild("Child_Side", ImVec2(300, 0), true, 0);

		// Registers
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
			m_instructions.clear();
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

		if (ActiveButton("Start", !running))
		{
			m_amiga->ClearBreakpoint();
			m_app->SetRunning(true);
			m_instructions.clear();
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
			m_instructions.clear();
		}


		ImGui::SameLine();
		if (ActiveButton("Step Over", !running))
		{
			// Use the disassembler to calculate the length of the next instruction
			m_disassembler->pc = regs.pc;
			m_disassembler->Disassemble(); // ignore disassembled string
			m_amiga->SetBreakpoint(m_disassembler->pc);
			m_app->SetRunning(true);
			m_instructions.clear();
		}

		ImGui::SameLine();
		if (ImGui::Button("Reset"))
		{
			m_amiga->ClearBreakpoint();
			m_amiga->Reset();
			m_instructions.clear();
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::SameLine();

	// Assembly view

	{
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
		ImGui::BeginChild("Child_Disassembly", ImVec2(0, 0), true, 0);

		if (m_app->IsRunning())
		{
			ImGui::Text("<< RUNNING >>");
		}
		else
		{
			if (m_instructions.empty())
			{
				UpdateAssembly();
			}

			for (auto&l : m_instructions)
			{
				ImGui::Text(l.c_str());
			}
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::End();
	return open;
}

void guru::Debugger::UpdateAssembly()
{
	auto cpu = m_amiga->GetCpu();
	auto pc = cpu->GetPC();

	auto[history, ptr] = cpu->GetOperationHistory();

	uint32_t earliestAddr = pc;

	// Look for the earliest recently previously-executed instruction that
	// is not more than 32 bytes earlier than the current pc

	for (int i = 0; i < history.size(); i++)
	{
		if (history[i] < earliestAddr && (history[i] + 32) >= pc)
		{
			earliestAddr = history[i];
		}
	}

	m_disassembler->pc = earliestAddr;

	for (int lines = 0; lines < 48; lines++)
	{
		std::string line = HexToString(m_disassembler->pc);
		line += (m_disassembler->pc == pc) ? " > " : " : ";
		line += m_disassembler->Disassemble();
		m_instructions.emplace_back(line);
	}
}