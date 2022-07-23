#include "symbols.h"

#include "util/tokens.h"

#include <algorithm>
#include <fstream>

am::SymLoadResults am::Symbols::Load(const std::string& file)
{
	m_subroutines.clear();

	std::ifstream ifs(file);
	if (!ifs.is_open())
		return SymLoadResults{ false };


	SymLoadResults res{ true };
	res.errorLine = -1;
	std::string line;

	int errors = 0;

	for (int lineNum = 0; !ifs.eof(); lineNum++)
	{
		std::getline(ifs, line);

		if (line.empty())
			continue;

		size_t off = 0;

		using util::Token;

		auto t = util::GetToken(line, off);

		if (t.type == Token::Type::End)
			continue;

		Subroutine sub;

		if (t.type != Token::Type::Int)
		{
			++res.numErrors;
			if (res.errorLine == -1)
			{
				res.errorLine = lineNum;
			}
			continue;
		}

		sub.start = t.num;

		t = util::GetToken(line, off);

		if (t.type != Token::Type::Int)
		{
			++res.numErrors;
			if (res.errorLine == -1)
			{
				res.errorLine = lineNum;
			}
			continue;
		}

		sub.end = t.num;

		t = util::GetToken(line, off);

		if (t.type != Token::Type::Name)
		{
			++res.numErrors;
			if (res.errorLine == -1)
			{
				res.errorLine = lineNum;
			}
			continue;
		}

		sub.name = std::move(t.str);

		t = util::GetToken(line, off);

		if (t.type != Token::Type::End)
		{
			++res.numErrors;
			if (res.errorLine == -1)
			{
				res.errorLine = lineNum;
			}
			continue;
		}

		AddSubroutine(std::move(sub));
		res.numLoaded++;
	}

	std::sort(std::begin(m_subroutines), std::end(m_subroutines),
		[](const Subroutine& a, const Subroutine& b)
		{
			return a.start < b.start;
		}
	);

	return res;
}

void am::Symbols::AddSubroutine(Subroutine&& sub)
{
	m_subroutines.emplace_back(std::move(sub));
}

const am::Subroutine* am::Symbols::GetSub(uint32_t addr) const
{
	if (m_subroutines.empty())
		return nullptr;

	const auto begin = std::begin(m_subroutines);
	const auto end = std::end(m_subroutines);

	auto it = std::upper_bound(begin, end, addr,
		[](uint32_t addr, const Subroutine& sub)
		{
			return addr < sub.start;
		}
	);

	if (it == begin)
		return nullptr;

	it--;

	if (addr >= it->end)
		return nullptr;

	return &(*it);
}