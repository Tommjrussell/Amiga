#include "debugger.h"

#include "amiga/disassembler.h"
#include "amiga/amiga.h"

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

	if (open && expanded)
	{
		ImGui::BeginChild("pc", ImVec2(0, 36), true);

		ImGui::PushItemWidth(64);
		if (ImGui::InputInt("pc", (int*)&m_pc, 0, 0, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
		{
			m_instructions.clear();
		}
		ImGui::PopItemWidth();

		ImGui::EndChild();

		if (m_instructions.empty())
		{
			m_disassembler->pc = m_pc;
			auto end = m_pc + 128;

			while (m_disassembler->pc < end)
			{
				std::string line = HexToString(m_disassembler->pc);
				line += " : ";
				line += m_disassembler->Disassemble();
				m_instructions.emplace_back(line);
			}
		}

		for (auto&l : m_instructions)
		{
			ImGui::Text(l.c_str());
		}
	}

	ImGui::End();
	return open;
}
