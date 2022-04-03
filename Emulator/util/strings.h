#pragma once

// Utility functions related to strings

#include <numeric>
#include <string>
#include <algorithm>

namespace util
{
	template <typename T>
	std::string HexToString(T value)
	{
		static_assert(std::numeric_limits<T>::is_integer, "Convert to integer type before calling HexToString");
		static const char digits[] = "0123456789ABCDEF";

		constexpr size_t numDigits = sizeof(T) * 2;
		char out[numDigits + 1] = { '\0' };

		for (size_t i = 0; i < numDigits; i++)
		{
			const auto v = (value >> (numDigits - i - 1) * 4) & 0x0f;
			out[i] = digits[v];
		}

		return out;
	}

	inline std::string ToUpper(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), ::toupper);
		return s;
	}

	inline std::string ToLower(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return s;
	}

	inline bool BeginsWith(const std::string& s, const std::string& start)
	{
		return (0 == s.find(start));
	}

	inline bool EndsWith(const std::string& s, const std::string& end)
	{
		if (end.length() > s.length())
			return false;

		const auto off = s.length() - end.length();

		auto res = s.find(end);

		return (off == res);
	}
}