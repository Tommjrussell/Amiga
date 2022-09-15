#pragma once

namespace am
{
	class Amiga;
	class Symbols;
}

namespace guru
{

	class VariableWatch
	{
	public:
		explicit VariableWatch(/*guru::AmigaApp* app, */am::Amiga* amiga, am::Symbols* symbols);
		~VariableWatch();

		bool Draw();

	private:
		am::Amiga* m_amiga;
		am::Symbols* m_symbols;
	};
}
