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

	struct Variable
	{
		uint32_t addr;
		uint8_t size;
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
		void SetSymbolsFile(const std::string& symbolsFile);

		SymLoadResults Load();

		void AddSubroutine(Subroutine&& sub);
		void AddVariable(Variable&& var);

		const Variable* GetVariable(uint32_t addr) const;

		const Subroutine* GetSub(uint32_t addr) const;
		const Subroutine* NextSub(uint32_t addr) const;

		const std::vector<Variable>& GetVariables() const
		{
			return m_variables;
		}

	private:
		std::vector<Subroutine> m_subroutines;
		std::vector<Variable> m_variables;

		std::string m_symbolsFile;
	};

}