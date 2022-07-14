#include "log.h"

util::Log::Log(size_t maxSize)
	: m_maxSize(maxSize)
{
}

void util::Log::AddMessage(uint64_t time, std::string message)
{
	if (m_messages.size() == m_maxSize)
	{
		m_messages.pop_front();
	}
	m_messages.push_back(std::make_pair(time, std::move(message)));
}

void util::Log::Clear()
{
	m_messages.clear();
}

const std::deque<std::pair<uint64_t, std::string>>& util::Log::GetMessages() const
{
	return m_messages;
}