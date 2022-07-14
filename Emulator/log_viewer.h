#pragma once

namespace am
{
	class Amiga;
}

namespace guru
{

	class LogViewer
	{
	public:
		explicit LogViewer(am::Amiga* amiga);

		bool Draw();

	private:
		am::Amiga* m_amiga;
	};

}