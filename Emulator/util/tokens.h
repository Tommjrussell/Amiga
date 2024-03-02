#pragma once

#include <string>
#include <stdint.h>

namespace util
{

	struct Token
	{
		enum class Type
		{
			Bad,
			Name,
			String,
			Int,
			Unsigned,
			Float,

			// Symbols
			Equal,
			OpenSquareBracket,
			CloseSquareBracket,

			End,
		};

		Type type;
		uint64_t numUnsigned;
		int64_t numSigned;
		double fpNum;
		std::string str;
	};

	Token GetToken(const std::string& input, size_t& off);
}
