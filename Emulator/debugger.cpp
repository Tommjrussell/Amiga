#include "debugger.h"

#include "disassembler.h"

#include <imgui.h>

guru::Memory::Memory(std::initializer_list<uint16_t> code)
{
	for (auto e : code)
	{
		m_memory.push_back(uint8_t(e >> 8));
		m_memory.push_back(uint8_t(e));
	}
}

uint16_t guru::Memory::GetWord(uint32_t addr) const
{
	if ((addr & 1) == 1)
	{
		_ASSERTE(false);
		return 0;
	}

	if (addr >= m_memory.size())
	{
		_ASSERTE(false);
		return 0;
	}

	uint16_t value = uint16_t(m_memory[addr] << 8);
	value |= uint16_t(m_memory[addr + 1]);

	return value;
}

uint8_t guru::Memory::GetByte(uint32_t) const
{
	return 0;
}


guru::Debugger::Debugger()
{
	m_memory = std::make_unique<Memory>(std::initializer_list<uint16_t>{
		0x1029, 0x001f,		// move.b ($001f, A1), D0
		0x532e, 0x0126,		// subq.b $01, ($0126, A6)
		0x4880,				// ext.w D0
		0x4e75,				// rts
		0x6632,				// bne 50 -> $00000040
		0x4cd7, 0x55f0,
		0x4cd5, 0xaaaa,
		0x48d5, 0xaaaa,
		0x4895, 0xffff});

	m_disassembler = std::make_unique<am::Disassembler>(m_memory.get());
}

guru::Debugger::~Debugger()
{
}

bool guru::Debugger::Draw()
{
	if (m_instructions.empty())
	{
		while (m_disassembler->pc < uint32_t(m_memory->GetSize()))
		{
			m_instructions.emplace_back(m_disassembler->Disassemble());
		}
	}

	bool open = true;
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	bool expanded = ImGui::Begin("Debugger", &open);

	if (open && expanded)
	{
		for (auto&l : m_instructions)
		{
			ImGui::Text(l.c_str());
		}
	}

	ImGui::End();
	return open;
}
