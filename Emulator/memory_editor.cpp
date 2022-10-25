#include "memory_editor.h"

#include "amiga/amiga.h"

#include <imgui.h>
#include "3rd Party/imgui_memory_editor.h"

guru::MemoryEditor::MemoryEditor(am::Amiga* amiga)
	: m_amiga(amiga)
	, m_imMemEditor(std::make_unique<::MemoryEditor>())
{
	m_imMemEditor->ReadFn = [](const uint8_t* data, size_t addr) -> uint8_t
	{
		auto amiga = reinterpret_cast<const am::Amiga*>(data);
		return amiga->PeekByte(uint32_t(addr));
	};

	m_imMemEditor->WriteFn = [](uint8_t* data, size_t addr, uint8_t d)
	{
		auto amiga = reinterpret_cast<am::Amiga*>(data);
		amiga->PokeByte(uint32_t(addr), d);
	};
}

guru::MemoryEditor::~MemoryEditor()
{
}

bool guru::MemoryEditor::Draw()
{
	m_imMemEditor->DrawWindow("Memory Editor", reinterpret_cast<uint8_t*>(m_amiga), 0x1000000, 0);
	return m_imMemEditor->Open;
}

void guru::MemoryEditor::GotoAndHighlightMem(uint32_t addr, uint32_t size)
{
	m_imMemEditor->GotoAddrAndHighlight(addr, addr + size);
}