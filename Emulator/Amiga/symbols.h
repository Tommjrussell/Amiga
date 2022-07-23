#pragma once

#include <string>
#include <vector>
#include <stdint.h>

namespace am
{

	struct Subroutine
	{
		uint32_t start;
		uint32_t end;
		std::string name;
	};

	struct SymLoadResults
	{
		bool readError;
		int numErrors;
		int errorLine;
		int numLoaded;
	};

	class Symbols
	{
	public:
		SymLoadResults Load(const std::string& file);

		void AddSubroutine(Subroutine&& sub);

		const Subroutine* GetSub(uint32_t addr) const;

	private:
		std::vector<Subroutine> m_subroutines;
	};

}