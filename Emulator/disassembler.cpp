#include "disassembler.h"

#include <iterator>
#include <string.h>

namespace
{
	struct OperationEncoding
	{
		uint16_t mask;
		uint16_t signature;
		const char* disassembly;
	};

	OperationEncoding encodingList[] =
	{
		{ 0b11111111'11111111,	0b00000000'00111100,	"ori        {imm:b}, CCR" },
		{ 0b11111111'11111111,	0b00000000'01111100,	"ori        {imm:w}, SR" },
		{ 0b11111111'00000000,	0b00000000'00000000,	"ori.{s}    {imm}, {ea}" },
		{ 0b11111111'11111111,	0b00000010'00111100,	"andi       {imm:b}, CCR" },
		{ 0b11111111'11111111,	0b00000010'01111100,	"andi       {imm:w}, SR" },
		{ 0b11111111'00000000,	0b00000010'00000000,	"andi.{s}   {imm}, {ea}" },
		{ 0b11111111'00000000,	0b00000100'00000000,	"subi.{s}   {imm}, {ea}" },
		{ 0b11111111'00000000,	0b00000110'00000000,	"addi.{s}   {imm}, {ea}" },
		{ 0b11111111'11111111,	0b00001010'00111100,	"eori       {imm:b}, CCR" },
		{ 0b11111111'11111111,	0b00001010'01111100,	"eori       {imm:w}, SR" },
		{ 0b11111111'00000000,	0b00001010'00000000,	"eori.{s}   {imm}, {ea}" },
		{ 0b11111111'00000000,	0b00001100'00000000,	"cmpi.{s}   {imm}, {ea}" },
		{ 0b11111111'11000000,	0b00001000'00000000,	"btst       {imm:b}, {ea}" },
		{ 0b11111111'11000000,	0b00001000'01000000,	"bchg       {imm:b}, {ea}" },
		{ 0b11111111'11000000,	0b00001000'10000000,	"bclr       {imm:b}, {ea}" },
		{ 0b11111111'11000000,	0b00001000'11000000,	"bset       {imm:b}, {ea}" },
		{ 0b11110001'10111000,	0b00000001'00001000,	"movep.{wl} ({immDisp16},A{reg}), D{REG}" },
		{ 0b11110001'10111000,	0b00000001'10001000,	"movep.{wl} D{REG}, ({immDisp16},A{reg})" },
		{ 0b11110001'11000000,	0b00000001'00000000,	"btst       D{REG}, {ea}" },
		{ 0b11110001'11000000,	0b00000001'01000000,	"bchg       D{REG}, {ea}" },
		{ 0b11110001'11000000,	0b00000001'10000000,	"bclr       D{REG}, {ea}" },
		{ 0b11110001'11000000,	0b00000001'11000000,	"bset       D{REG}, {ea}" },
		{ 0b11100001'11000000,	0b00100000'01000000,	"movea.{S}  {ea}, A{REG}" },
		{ 0b11000000'00000000,	0b00000000'00000000,	"move.{S}   {ea}, {EA}" },
		{ 0b11111111'11000000,	0b01000000'11000000,	"move       SR, {ea:w}" },
		{ 0b11111111'11000000,	0b01000100'11000000,	"move       {ea:b}, CCR" },
		{ 0b11111111'11000000,	0b01000110'11000000,	"move       {ea:w}, SR" },
		{ 0b11111111'00000000,	0b01000000'00000000,	"negx.{s}   {ea}" },
		{ 0b11111111'00000000,	0b01000010'00000000,	"clr.{s}    {ea}" },
		{ 0b11111111'00000000,	0b01000100'00000000,	"neg.{s}    {ea}" },
		{ 0b11111111'00000000,	0b01000110'00000000,	"not.{s}    {ea}" },
		{ 0b11111111'10111000,	0b01001000'10000000,	"ext.{wl}   D{reg}" },
		{ 0b11111111'11000000,	0b01001000'00000000,	"nbcd       {ea:b}" },
		{ 0b11111111'11111000,	0b01001000'01000000,	"swap       D{reg}" },
		{ 0b11111111'11000000,	0b01001000'01000000,	"pea        {ea:l}" },
		{ 0b11111111'11111111,	0b01001010'11111100,	"illegal" },
		{ 0b11111111'11000000,	0b01001010'11000000,	"tas		{ea:b}" },
		{ 0b11111111'00000000,	0b01001010'00000000,	"tst.{s}	{ea}" },
		{ 0b11111111'11110000,	0b01001110'01000000,	"trap		{v}" },
		{ 0b11111111'11111000,	0b01001110'01010000,	"link		A{reg}, {immDisp16}" },
		{ 0b11111111'11111000,	0b01001110'01011000,	"unlk		A{reg}" },
		{ 0b11111111'11111000,	0b01001110'01100000,	"move		A{reg}, USP" },
		{ 0b11111111'11111000,	0b01001110'01101000,	"move		USP, A{reg}" },
		{ 0b11111111'11111111,	0b01001110'01110000,	"reset" },
		{ 0b11111111'11111111,	0b01001110'01110001,	"nop" },
		{ 0b11111111'11111111,	0b01001110'01110010,	"stop		{imm:w}" },
		{ 0b11111111'11111111,	0b01001110'01110011,	"rte" },
		{ 0b11111111'11111111,	0b01001110'01110101,	"rts" },
		{ 0b11111111'11111111,	0b01001110'01110110,	"trapv" },
		{ 0b11111111'11111111,	0b01001110'01110111,	"rtr" },
		{ 0b11111111'11000000,	0b01001110'10000000,	"jsr		{ea}" },
		{ 0b11111111'11000000,	0b01001110'11000000,	"jmp		{ea}" },
		{ 0b11111111'10000000,	0b01001000'10000000,	"movem.{wl} {list}, {ea2}" },
		{ 0b11111111'10000000,	0b01001100'10000000,	"movem.{wl} {ea2}, {list}" },
		{ 0b11110001'11000000,	0b01000001'11000000,	"lea		{ea}, A{REG}" },
		{ 0b11110001'11000000,	0b01000001'10000000,	"chk		{ea}, D{REG}" },
		{ 0b11110000'11111000,	0b01010000'11001000,	"db{cc}		D{reg}, {braDisp}" },
		{ 0b11110000'11000000,	0b01010000'11000000,	"s{cc}		{ea}" },
		{ 0b11110001'00000000,	0b01010000'00000000,	"addq.{s}	{q}, {ea}" },
		{ 0b11110001'00000000,	0b01010001'00000000,	"subq.{s}	{q}, {ea}" },
		{ 0b11111111'00000000,	0b01100000'00000000,	"bra		{disp}" },
		{ 0b11111111'00000000,	0b01100001'00000000,	"bsr		{disp}" },
		{ 0b11110000'00000000,	0b01100000'00000000,	"b{cc}		{disp}" },
		{ 0b11110001'00000000,	0b01110000'00000000,	"moveq		{data}, D{REG}" },
		{ 0b11110001'11000000,	0b10000000'11000000,	"divu		{ea:w}, D{REG}" },
		{ 0b11110001'11000000,	0b10000001'11000000,	"divs		{ea:w}, D{REG}" },
		{ 0b11110001'11111000,	0b10000001'00000000,	"sbcd		D{reg}, D{REG}" },
		{ 0b11110001'11111000,	0b10000001'00001000,	"sbcd		-(A{reg}), -(A{REG})" },
		{ 0b11110001'00000000,	0b10000000'00000000,	"or.{s}		{ea}, D{REG}" },
		{ 0b11110001'00000000,	0b10000001'00000000,	"or.{s}		D{REG}, {ea}" },
		{ 0b11110000'11000000,	0b10010000'11000000,	"suba.{WL}	{ea}, A{REG}" },
		{ 0b11110001'00111000,	0b10010001'00000000,	"subx.{s}	D{reg}, D{REG}" },
		{ 0b11110001'00111000,	0b10010001'00001000,	"subx.{s}	-(A{reg}), -(A{REG})" },
		{ 0b11110001'00000000,	0b10010000'00000000,	"sub.{s}	{ea}, D{REG}" },
		{ 0b11110001'00000000,	0b10010001'00000000,	"sub.{s}	D{REG}, {ea}" },
		{ 0b11110000'11000000,	0b10110000'11000000,	"cmpa.{WL}	{ea}, A{REG}" },
		{ 0b11110001'00111000,	0b10110001'00001000,	"cmpm.{s}	(A{reg})+, (A{REG})+" },
		{ 0b11110001'00000000,	0b10110000'00000000,	"cmp.{s}	{ea}, D{REG}" },
		{ 0b11110001'00000000,	0b10110001'00000000,	"eor.{s}	D{REG}, {ea}" },
		{ 0b11110001'11000000,	0b11000000'11000000,	"mulu		{ea:w}, D{REG}" },
		{ 0b11110001'11000000,	0b11000001'11000000,	"muls		{ea:w}, D{REG}" },
		{ 0b11110001'11111000,	0b11000001'00000000,	"abcd		D{reg}, D{REG}" },
		{ 0b11110001'11111000,	0b11000001'00001000,	"abcd		-(A{reg}), -(A{REG})" },
		{ 0b11110001'11111000,	0b11000001'01000000,	"exg		D{REG}, D{reg}" },
		{ 0b11110001'11111000,	0b11000001'01001000,	"exg		A{REG}, A{reg}" },
		{ 0b11110001'11111000,	0b11000001'10001000,	"exg		D{REG}, A{reg}" },
		{ 0b11110001'00000000,	0b11000000'00000000,	"and.{s}	{ea}, D{REG}" },
		{ 0b11110001'00000000,	0b11000001'00000000,	"and.{s}	D{REG}, {ea}" },
		{ 0b11110000'11000000,	0b11010000'11000000,	"adda.{WL}  {ea}, A{REG}" },
		{ 0b11110001'00111000,	0b11010001'00000000,	"addx.{s}	D{reg}, D{REG}" },
		{ 0b11110001'00111000,	0b11010001'00001000,	"addx.{s}	-(A{reg}), -(A{REG})" },
		{ 0b11110001'00000000,	0b11010000'00000000,	"add.{s}	{ea}, D{REG}" },
		{ 0b11110001'00000000,	0b11010001'00000000,	"add.{s}	D{REG}, {ea}" },
		{ 0b11111110'11000000,	0b11100000'11000000,	"as{R}		{ea}" },
		{ 0b11111110'11000000,	0b11100010'11000000,	"ls{R}		{ea}" },
		{ 0b11111110'11000000,	0b11100100'11000000,	"rox{R}		{ea}" },
		{ 0b11111110'11000000,	0b11100110'11000000,	"ro{R}		{ea}" },
		{ 0b11110000'00111000,	0b11100000'00000000,	"as{R}.{s}	{q}, D{reg}" },
		{ 0b11110000'00111000,	0b11100000'00001000,	"ls{R}.{s}	{q}, D{reg}" },
		{ 0b11110000'00111000,	0b11100000'00010000,	"rox{R}.{s}	{q}, D{reg}" },
		{ 0b11110000'00111000,	0b11100000'00011000,	"ro{R}.{s}	{q}, D{reg}" },
		{ 0b11110000'00111000,	0b11100000'00100000,	"as{R}.{s}	D{REG}, D{reg}" },
		{ 0b11110000'00111000,	0b11100000'00101000,	"ls{R}.{s}	D{REG}, D{reg}" },
		{ 0b11110000'00111000,	0b11100000'00110000,	"rox{R}.{s}	D{REG}, D{reg}" },
		{ 0b11110000'00111000,	0b11100000'00111000,	"ro{R}.{s}	D{REG}, D{reg}" },
	};

	const char* const condition[16] =
	{
		"t ", "f ", "hi", "ls",
		"cc", "cs", "ne", "eq",
		"vc", "vs", "pl", "mi",
		"ge", "lt", "gt", "le",
	};

	enum class PieceType
	{
		end,
		text,
		code,
		operator_done,
	};

	enum class CodeType
	{
		size,
		size_for_move,
		size_word_or_long_low,
		size_word_or_long_high,
		ea,
		ea_skip_word,
		ea_move_destination,
		condition,
		register_low,
		register_high,
		data_short,
		data_long,
		vector,
		displacement,
		displacement_optional,
		displacement_data,
		immediate,
		register_list,

		CodeType_length
	};

	const char* const codeStrings[int(CodeType::CodeType_length)] =
	{
		"s",			// size,
		"S",			// size_for_move,
		"wl",			// size_word_or_long_low,
		"WL",			// size_word_or_long_high,
		"ea",			// ea,
		"ea2",			// ea_skip_word,
		"EA",			// ea_move_destination,
		"cc",			// condition,
		"reg",			// register_low,
		"REG",			// register_high,
		"q",			// data_short,
		"data",			// data_long,
		"v",			// vector,
		"braDisp",		// displacement,
		"disp",			// displacement_optional,
		"immDisp16",	// displacement_data,
		"imm",			// immediate
		"list",			// register_list
	};

	enum Sizes
	{
		Byte,
		Word,
		Long,

		Sizes_length
	};

	const char sizeCodes[3] = { 'b', 'w', 'l' };

	constexpr int kOperandStartColumn = 10;

	struct Piece
	{
		PieceType type;
		const char* textStart;
		int textLength;
		CodeType code;
		int size;
	};

	class Parser
	{
	public:
		Parser(const char* string)
			: str(string)
			, curr(string)
		{
		}

		Piece NextPiece()
		{
			Piece p;

			const char c = *curr;

			if (c == '\0')
			{
				p.type = PieceType::end;
			}
			else if (c == '{')
			{
				// This is a parameter format code
				p.type = PieceType::code;
				p.code = CodeType::CodeType_length;
				p.size = -1;

				const char* start = curr + 1;
				const char* end = ::strchr(start, '}');
				_ASSERTE(end);

				const char* sizeSpecifierDivider = ::strchr(start, ':');
				const char* codeEnd = sizeSpecifierDivider ? sizeSpecifierDivider : end;

				auto len = codeEnd - start;

				for (int i = 0; i < std::size(codeStrings); i++)
				{
					if (strncmp(start, codeStrings[i], len) == 0)
					{
						p.code = CodeType(i);
						break;
					}
				}

				_ASSERTE(p.code != CodeType::CodeType_length);

				if (sizeSpecifierDivider)
				{
					const char sizeSpecifier = sizeSpecifierDivider[1];
					for (int i = 0; i < std::size(sizeCodes); i++)
					{
						if (sizeSpecifier == sizeCodes[i])
						{
							p.size = i;
							break;
						}
					}

					_ASSERTE(p.size != -1);
				}

				curr = end + 1;
			}
			else if (inOperator && ::iswspace(c))
			{
				p.type = PieceType::operator_done;
				while (::iswspace(*curr))
					++curr;

				inOperator = false;
			}
			else
			{
				p.type = PieceType::text;
				const char* start = curr;
				p.textStart = start;

				const char* nextCodeStart = ::strchr(start, '{');
				const char* end = nextCodeStart;

				const char* nextWhitespace = nullptr;
				if (inOperator)
				{
					nextWhitespace = start + ::strcspn(start, " \t");
					if ((!end || nextWhitespace < end))
						end = nextWhitespace;
				}
				else if (!end)
				{
					end = ::strchr(start, '\0');
				}

				p.textLength = int(end - start);

				curr = end;
			}
			return p;
		}


	private:
		const char* str;
		const char* curr;
		bool inOperator = true;
	};

	void WriteImmediate(uint32_t value, int size, char*& buffptr, int& charsLeft)
	{
		if (size == 0)
		{
			charsLeft -= sprintf_s(buffptr, charsLeft, "$%02x", value);
			buffptr += 3;
		}
		else if (size == 1)
		{
			charsLeft -= sprintf_s(buffptr, charsLeft, "$%04x", value);
			buffptr += 5;
		}
		else if (size == 2)
		{
			charsLeft -= sprintf_s(buffptr, charsLeft, "$%08x", value);
			buffptr += 9;
		}
	}


}

std::string am::Disassembler::Disassemble()
{
	_ASSERTE(m_memory);

	const auto instruction = m_memory->GetWord(pc);
	pc += 2;
	const auto savedPc = pc;

	OperationEncoding* encoding = nullptr;

	for (auto& e : encodingList)
	{
		if ((instruction & e.mask) == e.signature)
		{
			encoding = &e;
			break;
		}
	}

	if (encoding == nullptr)
	{
		return "???";
	}

	Parser parser(encoding->disassembly);

	char buffer[128];
	char* buffptr = buffer;
	int charsLeft = int(std::size(buffer));
	int opcodeSize = 0;

	while (charsLeft > 0)
	{
		Piece p = parser.NextPiece();

		switch (p.type)
		{

		case PieceType::end:
		{
			buffer[std::size(buffer)-1] = '\0';
			return std::string(buffer);
		}

		case PieceType::text:
		{
			auto numChars = std::min(charsLeft, p.textLength);
			strncpy_s(buffptr, charsLeft, p.textStart, numChars);
			buffptr += numChars;
			charsLeft -= numChars;
		}	break;

		case PieceType::operator_done:
		{
			char* operandStart = buffer + kOperandStartColumn;

			if (buffptr >= operandStart)
			{
				*buffptr++ = ' ';
				charsLeft--;
			}
			else
			{
				while (buffptr < operandStart)
				{
					*buffptr++ = ' ';
					charsLeft--;
				}
			}
		}	break;

		case PieceType::code:
		{
			switch(p.code)
			{

			case CodeType::size:
			{
				opcodeSize = (instruction & 0b00000000'11000000) >> 6;
				if (opcodeSize == 0b11)
					return "<illegal size>";

				*buffptr++ = sizeCodes[opcodeSize];
				charsLeft--;
			}	break;

			case CodeType::size_for_move:
			{
				const auto moveSize = (instruction & 0b00110000'00000000) >> 12;
				opcodeSize = (moveSize == 0b01) ? 0b00 : (moveSize == 0b11) ? 0b01 : (moveSize == 0b10) ? 0b10 : 0b11;
				if (opcodeSize == 0b11)
					return "<illegal size>";

				*buffptr++ = sizeCodes[opcodeSize];
				charsLeft--;
			}	break;

			case CodeType::size_word_or_long_low:
			{
				opcodeSize = ((instruction & 0b00000000'01000000) >> 6) + 1;
				*buffptr++ = sizeCodes[opcodeSize];
				charsLeft--;
			}	break;

			case CodeType::size_word_or_long_high:
			{
				opcodeSize = ((instruction & 0b00000001'00000000) >> 8) + 1;
				*buffptr++ = sizeCodes[opcodeSize];
				charsLeft--;
			}	break;

			case CodeType::ea_skip_word:	// same as 'ea' but skips a word first
				pc += 2;
				[[fallthrough]];

			case CodeType::ea:
			{
				const auto mode = (instruction & 0b00000000'00111000) >> 3;
				const auto reg  = (instruction & 0b00000000'00000111);
				const auto size = (p.size != -1) ? p.size : opcodeSize;
				WriteEffectiveAddress(mode, reg, size, buffptr, charsLeft);
			}	break;

			case CodeType::ea_move_destination:
			{
				const auto mode = (instruction & 0b00000001'11000000) >> 6;
				const auto reg  = (instruction & 0b00001110'00000000) >> 9;
				const auto size = (p.size != -1) ? p.size : opcodeSize;
				WriteEffectiveAddress(mode, reg, size, buffptr, charsLeft);
			}	break;

			case CodeType::condition:
			{
				const int cc = (instruction & 0b00001111'00000000) >> 8;
				const auto count = sprintf_s(buffptr, charsLeft, "%s", condition[cc]);
				buffptr += count;
				charsLeft -= count;
			}	break;

			case CodeType::register_low:
			{
				const auto reg = (instruction & 0b00000000'00000111);
				const auto count = sprintf_s(buffptr, charsLeft, "%d", reg);
				buffptr += count;
				charsLeft -= count;
			}	break;

			case CodeType::register_high:
			{
				const auto reg = (instruction & 0b00001110'00000000) >> 9;
				const auto count = sprintf_s(buffptr, charsLeft, "%d", reg);
				buffptr += count;
				charsLeft -= count;
			}	break;

			case CodeType::data_short:
			{
				uint32_t quickValue = (instruction & 0b00001110'00000000) >> 9;
				quickValue = (quickValue == 0) ? 8 : quickValue;
				WriteImmediate(quickValue, 0, buffptr, charsLeft);
			}	break;

			case CodeType::data_long:
			{
				const int32_t data = int8_t(instruction & 0b00000000'11111111);
				WriteImmediate(uint32_t(data), 2, buffptr, charsLeft);
			}	break;

			case CodeType::vector:
			{
				const int vector = (instruction & 0b00000000'00001111);
				const auto count = sprintf_s(buffptr, charsLeft, "%d", vector);
				buffptr += count;
				charsLeft -= count;
			}	break;

			case CodeType::displacement:
			{
				const int16_t displacement = int16_t(m_memory->GetWord(pc));
				const uint32_t target = pc + displacement;
				const auto count = sprintf_s(buffptr, charsLeft, "%hi -> $%08x", displacement, target);
				buffptr += count;
				charsLeft -= count;
			}	break;

			case CodeType::displacement_optional:
			{
				int16_t displacement = int8_t(instruction & 0b00000000'11111111);
				uint32_t target = pc;
				if (displacement == 0)
				{
					displacement = int16_t(m_memory->GetWord(pc));
					pc += 2;
				}
				target += displacement;
				const auto count = sprintf_s(buffptr, charsLeft, "%hi -> $%08x", displacement, target);
				buffptr += count;
				charsLeft -= count;
			}	break;

			case CodeType::displacement_data:
			{
				const auto displacement = int16_t(GetImmediateValue(1));
				const auto count = sprintf_s(buffptr, charsLeft, "%hi", displacement);
				buffptr += count;
				charsLeft -= count;
			}	break;

			case CodeType::immediate:
			{
				const auto size = (p.size != -1) ? p.size : opcodeSize;
				const auto value = GetImmediateValue(size);
				WriteImmediate(value, size, buffptr, charsLeft);
			}	break;

			case CodeType::register_list:
			{
				// Note : register list is always read from the saved PC after the initial instruction word.
				const auto regMask = m_memory->GetWord(savedPc);
				bool reversed = false;
				if (!(instruction & 0b00000100'00000000) && ((instruction & 0b00000000'00111000) == 0b00000000'00100000))
				{
					reversed = true;
				}
				WriteRegisterList(regMask, reversed, buffptr, charsLeft);
			}	break;

			}

		}	break;

		}
	}

	buffer[std::size(buffer) - 1] = '\0';
	return std::string(buffer);
}

uint32_t am::Disassembler::GetImmediateValue(int size)
{
	uint32_t imm;

	switch (size)
	{
	case 0: // b
		imm = m_memory->GetWord(pc) & 0x00ff;
		pc += 2;
		break;

	case 1: // w
		imm = m_memory->GetWord(pc);
		pc += 2;
		break;

	case 2: // l
		imm = uint32_t(m_memory->GetWord(pc)) << 16;
		pc += 2;
		imm |= m_memory->GetWord(pc);
		pc += 2;
		break;
	}
	return imm;
}

void am::Disassembler::WriteEffectiveAddress(int mode, int reg, int size, char*& buffptr, int& charsLeft)
{
	int count = 0;

	switch (mode)
	{
	case 0b000: // Dn
		count = sprintf_s(buffptr, charsLeft, "D%d", reg);
		break;

	case 0b001: // An
		count = sprintf_s(buffptr, charsLeft, "A%d", reg);
		break;

	case 0b010: // (An)
		count = sprintf_s(buffptr, charsLeft, "(A%d)", reg);
		break;

	case 0b011: // (An)+
		count = sprintf_s(buffptr, charsLeft, "(A%d)+", reg);
		break;

	case 0b100: // -(An)
		count = sprintf_s(buffptr, charsLeft, "-(A%d)", reg);
		break;

	case 0b101: // (d16,An)
	{
		const uint32_t displacement = m_memory->GetWord(pc);
		pc += 2;
		count = sprintf_s(buffptr, charsLeft, "($%04x, A%d)", displacement, reg);
		break;
	}

	case 0b110: // (d8,An,Xn.SIZE)
	{
		const auto briefExtensionWord = m_memory->GetWord(pc);
		pc += 2;
		const auto displacement = uint32_t(briefExtensionWord & 0x00ff);
		const auto exWordReg = (briefExtensionWord & 0b01110000'00000000) >> 12;
		const bool isAddressReg = (briefExtensionWord & 0b10000000'00000000) != 0;
		const bool isLong = (briefExtensionWord & 0b00001000'00000000) != 0;

		count = sprintf_s(buffptr, charsLeft, "($%02x, A%d, %c%d.%c)",
			displacement,
			reg,
			(isAddressReg ? 'A' : 'D'),
			exWordReg,
			(isLong ? 'l' : 'w'));

	}	break;

	case 0b111:
	{
		// For the rest of the modes, 'reg' is an extended mode selector
		switch (reg)
		{
		case 0b000: // (xxx).W
		{
			uint32_t value = m_memory->GetWord(pc);
			pc += 2;
			count = sprintf_s(buffptr, charsLeft, "($%04x).w", value);
		}	break;

		case 0b001: // (xxx).L
		{
			uint32_t value = uint32_t(m_memory->GetWord(pc)) << 16;
			pc += 2;
			value |= m_memory->GetWord(pc);
			pc += 2;
			count = sprintf_s(buffptr, charsLeft, "($%08x).l", value);
		}	break;

		case 0b010: // (d16, PC)
		{
			const auto displacement = int16_t(m_memory->GetWord(pc));
			uint32_t value = pc + displacement;
			count = sprintf_s(buffptr, charsLeft, "($%04x, PC){$%08x}", uint16_t(displacement), value);
			pc += 2;
		}	break;

		case 0b011: // (d8, PC, Xn)
		{
			const auto briefExtensionWord = m_memory->GetWord(pc);
			pc += 2;
			const auto displacement = uint32_t(briefExtensionWord & 0x00ff);
			const auto exWordReg = (briefExtensionWord & 0b01110000'00000000) >> 12;
			const bool isAddressReg = (briefExtensionWord & 0b10000000'00000000) != 0;
			const bool isLong = (briefExtensionWord & 0b00001000'00000000) != 0;

			count = sprintf_s(buffptr, charsLeft, "($%02x, PC, %c%d.%c)",
				displacement,
				(isAddressReg ? 'A' : 'D'),
				exWordReg,
				(isLong ? 'l' : 'w'));

		}	break;

		case 0b100: // #imm
		{
			const auto imm = GetImmediateValue(size);
			WriteImmediate(imm, size, buffptr, charsLeft);
		}	break;

		default:
			break;
		}

	}	break;

	default:
		break;
	}

	buffptr += count;
	charsLeft -= count;
}

void am::Disassembler::WriteRegisterList(uint16_t mask, bool isReversed, char*& buffptr, int& charsLeft)
{
	if (isReversed)
	{
		mask = ((mask & 0xff00) >> 8) | ((mask & 0x00ff) << 8);
		mask = ((mask & 0xf0f0) >> 4) | ((mask & 0x0f0f) << 4);
		mask = ((mask & 0xcccc) >> 2) | ((mask & 0x3333) << 2);
		mask = ((mask & 0xaaaa) >> 1) | ((mask & 0x5555) << 1);
	}

	bool firstRange = true;

	auto WriteRegisterRange = [&](char r, int first, int last)
	{
		int count = 0;

		if (!firstRange)
		{
			if (charsLeft)
			{
				*buffptr++ = '/';
				charsLeft--;
			}
		}
		else
		{
			firstRange = false;
		}

		if (first == last)
		{
			count = sprintf_s(buffptr, charsLeft, "%c%d", r, first);
		}
		else
		{
			count = sprintf_s(buffptr, charsLeft, "%c%d-%c%d", r, first, r, last);
		}
		buffptr += count;
		charsLeft -= count;
	};

	static constexpr char regLetters[] = { 'D', 'A' };
	uint16_t currentBitMask = 0x0001;

	for(int g = 0; g < 2; g++)
	{
		int runStart = -1;
		for (int i = 0; i < 8; i++)
		{
			if (mask & currentBitMask)
			{
				if (runStart == -1)
				{
					runStart = i;
				}
			}
			else if (runStart != -1)
			{
				WriteRegisterRange(regLetters[g], runStart, i - 1);
				runStart = -1;
			}

			currentBitMask <<= 1;
		}

		if (runStart != -1)
		{
			WriteRegisterRange(regLetters[g], runStart, 7);
		}
	}
}