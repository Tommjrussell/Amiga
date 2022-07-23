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
			End,
		};

		Type type;
		uint32_t num;
		std::string str;
	};

	Token GetToken(const std::string& input, size_t& off);
}
