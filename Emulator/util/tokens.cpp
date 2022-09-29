#include "tokens.h"
#include <ctype.h>

namespace
{
	int HexCharToInt(char c)
	{
		if (c >= '0' && c <= '9')
			return c - '0';

		if (c >= 'a' && c <= 'f')
			return 10 + c - 'a';

		if (c >= 'A' && c <= 'F')
			return 10 + c - 'A';

		return 0;
	}

	util::Token GetName(const std::string& input, size_t& off)
	{
		using namespace util;

		const size_t end = input.length();
		Token t = { Token::Type::Name };
		size_t startOff = off;
		++off;

		while (off < end)
		{
			char c = input[off];

			if (!::isalnum(c) && c != '_')
				break;

			++off;
		}

		t.str = input.substr(startOff, off - startOff);
		return t;
	}

	util::Token GetString(const std::string& input, size_t& off)
	{
		using namespace util;

		const size_t end = input.length();
		Token t = { Token::Type::String };
		++off;
		size_t startOff = off;

		while (off < end)
		{
			char c = input[off++];

			if (c == '\"')
			{
				t.str = input.substr(startOff, (off - 1) - startOff);
				return t;
			}
		}

		t.type = Token::Type::Bad;
		return t;
	}

	util::Token GetNumber(const std::string& input, size_t& off)
	{
		using namespace util;

		const size_t end = input.length();
		Token t = { Token::Type::Int };

		bool isHex = false;

		if (input[off] == '0' && (off + 2) < end)
		{
			if (input[off + 1] == 'x' || input[off + 1] == 'X')
			{
				isHex = true;
				off += 2;
			}
		}

		uint32_t value = 0;

		while (off < end)
		{
			char c = input[off];

			if (isHex)
			{
				if (!isxdigit(c))
					break;

				value <<= 4;
				value |= HexCharToInt(c);
			}
			else
			{
				if (!isdigit(c))
					break;

				value *= 10;
				value += (c - '0');
			}

			++off;
		}

		t.num = value;
		return t;
	}
}

util::Token util::GetToken(const std::string& input, size_t& off)
{
	const size_t end = input.length();

	while (off < end)
	{
		char c = input[off];

		if (::isblank(c))
		{
			++off;
			continue;
		}

		if (c == ';')
		{
			// rest of string is comment
			break;
		}
		if (isalpha(c) || c == '_')
		{
			return GetName(input, off);
		}
		else if (c == '\"')
		{
			return GetString(input, off);
		}
		else if (isdigit(c))
		{
			return GetNumber(input, off);
		}
	}

	return { Token::Type::End };
}