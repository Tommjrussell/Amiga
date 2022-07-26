#pragma once

#include <array>

namespace am
{
	class Amiga;
}

namespace guru
{

	class DiskActivity
	{
	public:
		explicit DiskActivity(am::Amiga* amiga);
		void Draw(int displayWidth, int displayHeight);

	private:
		am::Amiga* m_amiga;
		std::array<int, 4> idleTimeout;
	};

}