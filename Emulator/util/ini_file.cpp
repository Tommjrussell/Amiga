#include "ini_file.h"
#include "tokens.h"

#include <fstream>

std::tuple<util::IniLoadResult, util::Ini, util::IniErrors> util::LoadIniFile(const std::filesystem::path& iniFile)
{
	std::ifstream fs(iniFile.string());
	if (!fs)
	{
		return { IniLoadResult::FileNotFound, {}, {} };
	}

	Ini ini;
	IniErrors errors;
	Ini::Section* section = &ini.m_sections["DEFAULT"];

	int lineNum = 1;

	for (std::string line; getline(fs, line); lineNum++)
	{
		size_t off = 0;
		auto token = GetToken(line, off);

		if (token.type == Token::Type::Name)
		{
			auto name = std::move(token.str);

			token = GetToken(line, off);
			if (token.type != Token::Type::Equal)
			{
				errors.emplace_back(lineNum, "Expected key or section name");
				continue;
			}

			token = GetToken(line, off);

			Ini::Value value;

			if (token.type == Token::Type::Unsigned)
			{
				value = Ini::Value{ .type = Ini::Value::Unsigned, .unsignedValue = token.numUnsigned };
			}
			else if (token.type == Token::Type::Float)
			{
				value = Ini::Value{ .type = Ini::Value::Float, .fpValue = token.fpNum };
			}
			else if (token.type == Token::Type::String)
			{
				value = Ini::Value{ .type = Ini::Value::String, .string = token.str };
			}
			else
			{
				errors.emplace_back(lineNum, "Key has unexpected value type");
				continue;
			}

			(*section)[name] = std::move(value);
		}
		else if (token.type == Token::Type::OpenSquareBracket)
		{
			// [section_name]

			token = GetToken(line, off);
			if (token.type != Token::Type::Name)
			{
				errors.emplace_back(lineNum, "Invalid section name");
				continue;
			}

			std::string sectionName = std::move(token.str);

			token = GetToken(line, off);
			if (token.type != Token::Type::CloseSquareBracket)
			{
				errors.emplace_back(lineNum, "Section declaration not closed");
				continue;
			}

			section = &ini.m_sections[sectionName];
		}
		else if (token.type != Token::Type::End)
		{
			errors.emplace_back(lineNum, "Unexpected token.");
			continue;
		}
	}

	return
	{
		errors.empty() ? IniLoadResult::Ok : IniLoadResult::ParseErrors,
		std::move(ini),
		std::move(errors)
	};

}

namespace
{
	void WriteSectionContents(std::ofstream& fs, const util::Ini::Section& section)
	{
		using util::Ini;

		if (section.empty())
			return;

		for (auto& entry : section)
		{
			fs << entry.first << " = ";

			auto& value = entry.second;

			switch (value.type)
			{
			case Ini::Value::Unsigned:
				fs << value.unsignedValue;
				break;
			case Ini::Value::Signed:
				fs << value.signedValue;
				break;
			case Ini::Value::Float:
				fs << value.fpValue;
				break;
			case Ini::Value::String:
				fs << "\"" << value.string << "\"";
			}
			fs << "\n";
		}
		fs << "\n";
	}
}

bool util::SaveIniFile(const util::Ini& ini, const std::filesystem::path& iniFile)
{
	std::ofstream fs(iniFile.string());
	if (!fs)
	{
		return false;
	}

	const Ini::Section& section = const_cast<util::Ini&>(ini).m_sections["DEFAULT"];
	WriteSectionContents(fs, section);


	for (auto& section : ini.m_sections)
	{
		// skip sections with no content
		if (section.second.empty())
			continue;

		if (section.first != "DEFAULT")
		{
			fs << "[" << section.first << "]\n\n";
		}

		WriteSectionContents(fs, section.second);
	}

	return true;
}

const util::Ini::Section* util::GetSection(const util::Ini& ini, std::string_view sectionName)
{
	auto it = ini.m_sections.find(std::string(sectionName));
	if (it == std::end(ini.m_sections))
		return nullptr;

	return &(it->second);
}

namespace
{
	const util::Ini::Value* GetKey(const util::Ini::Section* section, std::string_view key)
	{
		if (!section)
			return nullptr;

		auto it = section->find(std::string(key));
		if (it == std::end(*section))
			return nullptr;

		return &(it->second);
	}
}

std::optional<std::string> util::GetStringKey(const util::Ini::Section* section, std::string_view key)
{
	auto value = GetKey(section, key);
	if (!value)
		return {};

	if (value->type != Ini::Value::String)
		return {};

	return value->string;
}

std::optional<bool> util::GetBoolKey(const util::Ini::Section* section, std::string_view key)
{
	auto value = GetKey(section, key);
	if (!value)
		return {};

	if (value->type == Ini::Value::Unsigned)
		return (value->unsignedValue != 0);

	if (value->type == Ini::Value::Signed)
		return (value->signedValue != 0);

	return {};
}

std::optional<uint64_t> util::GetUnsignedIntKey(const util::Ini::Section* section, std::string_view key)
{
	auto value = GetKey(section, key);
	if (!value)
		return {};

	if (value->type != Ini::Value::Unsigned)
		return {};

	return value->unsignedValue;
}

std::optional<double> util::GetFloatKey(const util::Ini::Section* section, std::string_view key)
{
	auto value = GetKey(section, key);
	if (!value)
		return {};

	if (value->type == Ini::Value::Unsigned)
		return double(value->unsignedValue);

	if (value->type == Ini::Value::Signed)
		return double(value->signedValue);

	if (value->type == Ini::Value::Float)
		return value->fpValue;

	return {};
}

std::optional<int64_t> util::GetIntKey(const util::Ini::Section* section, std::string_view key)
{
	auto value = GetKey(section, key);
	if (!value)
		return {};

	if (value->type != Ini::Value::Signed)
		return {};

	return value->signedValue;
}

void util::SetStringKey(Ini::Section& section, std::string_view key, std::string_view value)
{
	Ini::Value v = { .type = Ini::Value::String };
	v.string = value;
	section[std::string(key)] = v;
}

void util::SetBoolKey(Ini::Section& section, std::string_view key, bool value)
{
	Ini::Value v = { .type = Ini::Value::Unsigned };
	v.unsignedValue = value ? 1 : 0;
	section[std::string(key)] = v;
}

void util::SetUnsignedIntKey(Ini::Section& section, std::string_view key, uint64_t value)
{
	Ini::Value v = { .type = Ini::Value::Unsigned };
	v.unsignedValue = value;
	section[std::string(key)] = v;
}

void util::SetFloatKey(Ini::Section& section, std::string_view key, double value)
{
	Ini::Value v = { .type = Ini::Value::Float };
	v.fpValue = value;
	section[std::string(key)] = v;
}

void util::SetIntKey(Ini::Section& section, std::string_view key, int value)
{
	Ini::Value v = { .type = Ini::Value::Signed };
	v.signedValue = value;
	section[std::string(key)] = v;
}