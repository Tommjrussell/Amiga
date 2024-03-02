#pragma once

#include <filesystem>
#include <string>
#include <map>
#include <tuple>
#include <optional>
#include <vector>

namespace util
{

	struct Ini
	{
		struct Value
		{
			enum Type
			{
				Unsigned,
				Signed,
				Float,
				String
			};

			Type type;
			union
			{
				uint64_t unsignedValue;
				int64_t signedValue;
				double fpValue;
			};
			std::string string;
		};

		using Section = std::map<std::string, Value>;
		std::map<std::string, Section> m_sections;
	};

	enum class IniLoadResult
	{
		Ok,
		FileNotFound,
		FileNotReadable,
		ParseErrors,
	};

	using IniErrors = std::vector<std::pair<int, std::string>>;

	std::tuple<IniLoadResult, Ini, IniErrors> LoadIniFile(const std::filesystem::path& iniFile);
	bool SaveIniFile(const Ini& ini, const std::filesystem::path& iniFile);

	const Ini::Section* GetSection(const Ini& ini, std::string_view sectionName);
	std::optional<std::string> GetStringKey(const Ini::Section* section, std::string_view key);
	std::optional<bool> GetBoolKey(const Ini::Section* section, std::string_view key);
	std::optional<uint64_t> GetUnsignedIntKey(const Ini::Section* section, std::string_view key);
	std::optional<double> GetFloatKey(const Ini::Section* section, std::string_view key);
	std::optional<int64_t> GetIntKey(const Ini::Section* section, std::string_view key);

	void SetStringKey(Ini::Section& section, std::string_view key, std::string_view value);
	void SetBoolKey(Ini::Section& section, std::string_view key, bool value);
	void SetUnsignedIntKey(Ini::Section& section, std::string_view key, uint64_t value);
	void SetFloatKey(Ini::Section& section, std::string_view key, double value);
	void SetIntKey(Ini::Section& section, std::string_view key, int value);

}