#pragma once

namespace am
{
	class Amiga;
}

namespace util
{
	class Log;
}

namespace guru
{

	class LogViewer
	{
	public:
		explicit LogViewer(util::Log* log);

		bool Draw();

	private:
		util::Log* m_log;
	};

}