#pragma once

#include <memory>

namespace am
{
	class Amiga;
}

struct MemoryEditor;

namespace guru
{

	class MemoryEditor
	{
	public:
		explicit MemoryEditor(am::Amiga* amiga);
		~MemoryEditor();

		void GotoAndHighlightMem(uint32_t addr, uint32_t size);

		bool Draw();

	private:
		am::Amiga* m_amiga;
		std::unique_ptr<::MemoryEditor> m_imMemEditor;
	};

}