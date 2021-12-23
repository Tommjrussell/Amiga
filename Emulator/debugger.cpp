#include "debugger.h"

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

guru::Debugger::Debugger(am::Amiga* amiga)
	: m_amiga(amiga)
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

	auto regs = m_amiga->GetCpu()->GetRegisters();

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

		if (ImGui::Button("Step"))
		{
			m_amiga->ExecuteOneCpuInstruction();
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

		if (m_instructions.empty())
		{
			auto pc = m_amiga->GetCpu()->GetPC();
			m_disassembler->pc = pc;
			auto end = pc + 128;

			while (m_disassembler->pc < end)
			{
				std::string line = HexToString(m_disassembler->pc);
				line += (m_disassembler->pc == pc) ? " > " : " : ";
				line += m_disassembler->Disassemble();
				m_instructions.emplace_back(line);
			}
		}

		for (auto&l : m_instructions)
		{
			ImGui::Text(l.c_str());
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::End();
	return open;
}
