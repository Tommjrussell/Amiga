#pragma once

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

	private:
		guru::AmigaApp* m_app;
		am::Amiga* m_amiga;

		bool m_beamValuesInHex = false;
	};
}