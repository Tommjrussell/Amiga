#pragma once

#include <deque>
#include <string>
#include <stdint.h>

namespace util
{
	class Log
	{
	public:
		explicit Log(size_t maxSize);
		void AddMessage(uint64_t time, std::string message);
		void Clear();

		const std::deque<std::pair<uint64_t, std::string>>& GetMessages() const;

	private:
		std::deque<std::pair<uint64_t, std::string>> m_messages;
		size_t m_maxSize = 0;
	};
}