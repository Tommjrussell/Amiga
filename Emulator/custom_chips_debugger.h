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
		void DrawColourPalette();
		void DrawBitplaneControl();

	private:
		guru::AmigaApp* m_app;
		am::Amiga* m_amiga;
	};
}