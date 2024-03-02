#include "tokens.h"
#include <ctype.h>
#include <charconv>

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

		bool negative = false;

		if (input[off] == '-')
		{
			off++;
			negative = true;
		}
		else if (input[off] == '+')
		{
			off++;
		}

		if ((end - off) > 2 && input[off] == '0' && ::toupper(input[off + 1]) == 'X')
		{
			uint64_t value = 0;

			off += 2;
			while (off < end)
			{
				char c = input[off];

				if (!isxdigit(c))
					break;

				value <<= 4;
				value |= HexCharToInt(c);

				++off;
			}

			Token t;
			if (negative)
			{
				t.type = Token::Type::Int;
				t.numSigned = -int64_t(value);
			}
			else
			{
				t.type = Token::Type::Unsigned;
				t.numUnsigned = value;
			}
			return t;
		}

		auto startOff = off;

		enum NumState
		{
			PreDecimal,
			PostDecimal,
			End,
		};

		NumState state = PreDecimal;
		bool isDecimal = false;

		while (state != End && off < end)
		{
			char c = input[off++];

			switch (state)
			{
			case PreDecimal:

				if (c == '.')
				{
					state = PostDecimal;
					isDecimal = true;
					break;
				}

				if (::isdigit(c))
					break;


				state = End;
				break;

			case PostDecimal:
				if (::isdigit(c))
					break;

				state = End;
				break;
			}
		}

		if (isDecimal)
		{
			double value = 0.0;
			std::from_chars(input.data() + startOff, input.data() + off, value);
			Token t = { .type = Token::Type::Float };
			t.fpNum = value;
			return t;
		}
		else
		{
			uint64_t value = 0;
			std::from_chars(input.data() + startOff, input.data() + off, value);

			Token t;
			if (negative)
			{
				t.type = Token::Type::Int;
				t.numSigned = -int64_t(value);
			}
			else
			{
				t.type = Token::Type::Unsigned;
				t.numUnsigned = value;
			}
			return t;
		}
	}
}

util::Token util::GetToken(const std::string& input, size_t& off)
{
	const size_t end = input.length();

	char c = '\0';

	for (; off < end; ++off)
	{
		c = input[off];

		if (!::isblank(c))
		{
			break;
		}
	}

	if (off == end || c == ';')
		return { Token::Type::End };

	if (isalpha(c) || c == '_')
	{
		return GetName(input, off);
	}

	if (c == '\"')
	{
		return GetString(input, off);
	}

	if (isdigit(c) || c == '+' || c == '-' || c == '.')
	{
		return GetNumber(input, off);
	}

	if (c == '=')
	{
		++off;
		return { Token::Type::Equal };
	}

	if (c == '[')
	{
		++off;
		return { Token::Type::OpenSquareBracket };
	}

	if (c == ']')
	{
		++off;
		return { Token::Type::CloseSquareBracket };
	}

	return { Token::Type::Bad };

}