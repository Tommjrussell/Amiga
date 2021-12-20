#pragma once

// Utility functions related to strings

#include <numeric>
#include <string>

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