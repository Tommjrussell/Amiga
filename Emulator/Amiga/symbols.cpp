#include "symbols.h"

#include "util/tokens.h"
#include "util/strings.h"

#include <algorithm>
#include <fstream>

void am::Symbols::SetSymbolsFile(const std::string& symbolsFile)
{
	m_symbolsFile = symbolsFile;
}

am::SymLoadResults am::Symbols::Load()
{
	m_subroutines.clear();
	m_variables.clear();

	std::ifstream ifs(m_symbolsFile);
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

		if (t.type != Token::Type::Unsigned)
		{
			++res.numErrors;
			if (res.errorLine == -1)
			{
				res.errorLine = lineNum;
			}
			continue;
		}

		uint32_t addr = uint32_t(t.numUnsigned);

		t = util::GetToken(line, off);

		if (t.type != Token::Type::Int && t.type != Token::Type::Name)
		{
			++res.numErrors;
			if (res.errorLine == -1)
			{
				res.errorLine = lineNum;
			}
			continue;
		}

		if (t.type == Token::Type::Name)
		{
			int size = 0;

			auto typeStr = util::ToUpper(t.str);

			if (typeStr == "LONG")
				size = 4;
			else if (typeStr == "WORD")
				size = 2;
			else if (typeStr == "BYTE")
				size = 1;

			if (size == 0)
			{
				++res.numErrors;
				if (res.errorLine == -1)
				{
					res.errorLine = lineNum;
				}
				continue;
			}

			t = util::GetToken(line, off);

			if (t.type != Token::Type::String)
			{
				++res.numErrors;
				if (res.errorLine == -1)
				{
					res.errorLine = lineNum;
				}
				continue;
			}

			Variable var;
			var.addr = addr;
			var.size = size;
			var.name = t.str;

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

			AddVariable(std::move(var));
			res.numLoaded++;
		}
		else
		{
			Subroutine sub;
			sub.start = addr;
			sub.end = uint32_t(t.numUnsigned);

			t = util::GetToken(line, off);

			if (t.type != Token::Type::String)
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
	}

	std::sort(std::begin(m_subroutines), std::end(m_subroutines),
		[](const Subroutine& a, const Subroutine& b)
		{
			return a.start < b.start;
		}
	);

	std::sort(std::begin(m_variables), std::end(m_variables),
		[](const Variable& a, const Variable& b)
		{
			return a.addr < b.addr;
		}
	);

	return res;
}

void am::Symbols::AddSubroutine(Subroutine&& sub)
{
	m_subroutines.emplace_back(std::move(sub));
}

void am::Symbols::AddVariable(Variable&& var)
{
	m_variables.emplace_back(std::move(var));
}

const am::Variable* am::Symbols::GetVariable(uint32_t addr) const
{
	if (m_variables.empty())
		return nullptr;

	const auto begin = std::begin(m_variables);
	const auto end = std::end(m_variables);

	auto it = std::lower_bound(begin, end, addr,
		[](const Variable& var, uint32_t addr)
		{
			return var.addr < addr;
		}
	);

	if (it == end)
		return nullptr;

	if (it->addr != addr)
		return nullptr;

	return &(*it);
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

const am::Subroutine* am::Symbols::NextSub(uint32_t addr) const
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

	if (it == end)
		return nullptr;

	return &(*it);
}