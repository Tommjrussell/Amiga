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

		void SetOptions(uint64_t logOptions)
		{
			m_logOptions = logOptions;
		}

		uint64_t GetOptions() const
		{
			return m_logOptions;
		}

		bool IsLogging(uint64_t options) const
		{
			return (m_logOptions & options) != 0;
		}

		const std::deque<std::pair<uint64_t, std::string>>& GetMessages() const;

	private:
		std::deque<std::pair<uint64_t, std::string>> m_messages;
		size_t m_maxSize = 0;
		uint64_t m_logOptions; // bitfield
	};
}