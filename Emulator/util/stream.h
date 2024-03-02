#pragma once

#include <istream>
#include <ostream>
#include <vector>

namespace util
{

	template <typename T>
	void Stream(std::ostream& os, const T& v)
	{
		os.write(reinterpret_cast<const char*>(&v), sizeof(T));
	}

	template <typename T>
	void Stream(std::istream& is, T& v)
	{
		is.read(reinterpret_cast<char*>(&v), sizeof(T));
	}

	template <typename T>
	void StreamVector(std::ostream& os, const std::vector<T>& v)
	{
		const size_t size = v.size();
		os.write(reinterpret_cast<const char*>(&size), sizeof(size));
		os.write(reinterpret_cast<const char*>(v.data()), size * sizeof(T));
	}

	template <typename T>
	void StreamVector(std::istream& is, std::vector<T>& v)
	{
		size_t len = 0;
		is.read(reinterpret_cast<char*>(&len), sizeof(len));
		v.resize(len);
		is.read(reinterpret_cast<char*>(v.data()), len * sizeof(T));
	}

	inline void StreamString(std::ostream& os, std::string_view str)
	{
		const size_t len = str.length();
		os.write(reinterpret_cast<const char*>(&len), sizeof(len));
		os.write(reinterpret_cast<const char*>(str.data()), len);
	}

	inline void StreamString(std::istream& is, std::string& str)
	{
		size_t len = 0;
		std::vector<char> chars;
		is.read(reinterpret_cast<char*>(&len), sizeof(len));
		chars.resize(len + 1, '\0');
		is.read(reinterpret_cast<char*>(chars.data()), len);
		str = chars.data();
	}

}