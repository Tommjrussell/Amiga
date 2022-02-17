#pragma once

#include <string>

namespace am
{
	class Amiga;
}

namespace guru
{
	class AmigaApp;

	class CCDebugger
	{
	public:
		explicit CCDebugger(guru::AmigaApp* app, am::Amiga* amiga);
		~CCDebugger();

		bool Draw();

	private:
		void DrawBeamInfo();
		void DrawColourPalette();
		void DrawBitplaneControl();
		void DrawCopper();

		std::string DisassembleCopperInstruction(uint32_t addr) const;

	private:
		guru::AmigaApp* m_app;
		am::Amiga* m_amiga;

		int m_disassembleMode = 0;
		uint32_t m_disassembleAddr = 0;
		bool m_beamValuesInHex = false;
	};
}