#include "68000.h"

#include "util/stream.h"

#include <functional>
#include <utility>

using namespace cpu;

namespace
{
	int gOpcodeGroups[16] = {};

	struct InstructionCode
	{
		uint16_t mask;
		uint16_t signature;
	};

	const InstructionCode kEncodingList[kNumOpcodeEntries] =
	{
		{ 0b11111111'11111111,	0b00000000'00111100 }, //	ori     {imm:b}, CCR
		{ 0b11111111'11111111,	0b00000000'01111100 }, //	ori     {imm:w}, SR
		{ 0b11111111'00000000,	0b00000000'00000000 }, //	ori.{s}   {imm}, {ea}
		{ 0b11111111'11111111,	0b00000010'00111100 }, //	andi    {imm:b}, CCR
		{ 0b11111111'11111111,	0b00000010'01111100 }, //	andi    {imm:w}, SR
		{ 0b11111111'00000000,	0b00000010'00000000 }, //	andi.{s}  {imm}, {ea}
		{ 0b11111111'00000000,	0b00000100'00000000 }, //	subi.{s}  {imm}, {ea}
		{ 0b11111111'00000000,	0b00000110'00000000 }, //	addi.{s}  {imm}, {ea}
		{ 0b11111111'11111111,	0b00001010'00111100 }, //	eori    {imm:b}, CCR
		{ 0b11111111'11111111,	0b00001010'01111100 }, //	eori    {imm:w}, SR
		{ 0b11111111'00000000,	0b00001010'00000000 }, //	eori.{s}  {imm}, {ea}
		{ 0b11111111'00000000,	0b00001100'00000000 }, //	cmpi.{s}  {imm}, {ea}
		{ 0b11111111'11000000,	0b00001000'00000000 }, //	btst    {imm:b}, {ea}
		{ 0b11111111'00000000,	0b00001000'00000000 }, //	b(chg/clr/set)    {imm:b}, {ea}
		{ 0b11110001'10111000,	0b00000001'00001000 }, //	movep.{wl} ({immDis16}, A{reg}), D{REG}
		{ 0b11110001'10111000,	0b00000001'10001000 }, //	movep.{wl} D{REG}, ({immDis16}, A{reg})
		{ 0b11110001'11000000,	0b00000001'00000000 }, //	btst    D{REG}, {ea}
		{ 0b11110001'00000000,	0b00000001'00000000 }, //	b(chg/clr/set)    D{REG}, {ea}

		{ 0b11110000'00000000,	0b00010000'00000000 }, //	move.b
		{ 0b11110000'00000000,	0b00100000'00000000 }, //	movea/move.l
		{ 0b11110000'00000000,	0b00110000'00000000 }, //	movea/move.w

		{ 0b11111111'11000000,	0b01000000'11000000 }, //	move    SR, {ea:w}
		{ 0b11111111'11000000,	0b01000100'11000000 }, //	move    {ea:b}, CCR
		{ 0b11111111'11000000,	0b01000110'11000000 }, //	move    {ea:w}, SR
		{ 0b11111111'00000000,	0b01000000'00000000 }, //	negx.{s}  {ea}
		{ 0b11111111'00000000,	0b01000010'00000000 }, //	clr.{s}   {ea}
		{ 0b11111111'00000000,	0b01000100'00000000 }, //	neg.{s}   {ea}
		{ 0b11111111'00000000,	0b01000110'00000000 }, //	not.{s}   {ea}
		{ 0b11111111'10111000,	0b01001000'10000000 }, //	ext.{wl}   D{reg}
		{ 0b11111111'11000000,	0b01001000'00000000 }, //	nbcd    {ea:b}
		{ 0b11111111'11111000,	0b01001000'01000000 }, //	swap    D{reg}
		{ 0b11111111'11000000,	0b01001000'01000000 }, //	pea     {ea:l}
		{ 0b11111111'11000000,	0b01001010'11000000 }, //	tas     {ea:b}
		{ 0b11111111'00000000,	0b01001010'00000000 }, //	tst.{s}   {ea}
		{ 0b11111111'11110000,	0b01001110'01000000 }, //	trap    {v}
		{ 0b11111111'11111000,	0b01001110'01010000 }, //	link    A{reg}, {immDis16}
		{ 0b11111111'11111000,	0b01001110'01011000 }, //	unlk    A{reg}
		{ 0b11111111'11110000,	0b01001110'01100000 }, //	move    A{reg}, USP
		{ 0b11111111'11111111,	0b01001110'01110000 }, //	reset
		{ 0b11111111'11111111,	0b01001110'01110001 }, //	nop
		{ 0b11111111'11111111,	0b01001110'01110010 }, //	stop
		{ 0b11111111'11111111,	0b01001110'01110011 }, //	rte
		{ 0b11111111'11111111,	0b01001110'01110101 }, //	rts
		{ 0b11111111'11111111,	0b01001110'01110110 }, //	trapv
		{ 0b11111111'11111111,	0b01001110'01110111 }, //	rtr
		{ 0b11111111'11000000,	0b01001110'10000000 }, //	jsr     {ea}
		{ 0b11111111'11000000,	0b01001110'11000000 }, //	jmp     {ea}
		{ 0b11111111'10000000,	0b01001000'10000000 }, //	movem.{wl} {list}, {ea}
		{ 0b11111111'10000000,	0b01001100'10000000 }, //	movem.{wl} {ea}, {list}
		{ 0b11110001'11000000,	0b01000001'11000000 }, //	lea     {ea}, A{REG}
		{ 0b11110001'11000000,	0b01000001'10000000 }, //	chk     {ea}, D{REG}

		{ 0b11110000'11111000,	0b01010000'11001000 }, //	db{cc}    D{reg}, {braDisp}
		{ 0b11110000'11000000,	0b01010000'11000000 }, //	s{cc}     {ea}
		{ 0b11110001'00000000,	0b01010000'00000000 }, //	addq.{s}  {q}, {ea}
		{ 0b11110001'00000000,	0b01010001'00000000 }, //	subq.{s}  {q}, {ea}

		{ 0b11111111'00000000,	0b01100001'00000000 }, //	bsr     {disp}
		{ 0b11110000'00000000,	0b01100000'00000000 }, //	b{cc}     {disp}

		{ 0b11110001'00000000,	0b01110000'00000000 }, //	moveq   {data}, D{REG}

		{ 0b11110001'11000000,	0b10000000'11000000 }, //	divu    {ea:w}, D{REG}
		{ 0b11110001'11000000,	0b10000001'11000000 }, //	divs    {ea:w}, D{REG}
		{ 0b11110001'11110000,	0b10000001'00000000 }, //	sbcd    {m}
		{ 0b11110001'00000000,	0b10000000'00000000 }, //	or.{s}    {ea}, D{REG}
		{ 0b11110001'00000000,	0b10000001'00000000 }, //	or.{s}    D{REG}, {ea}

		{ 0b11110000'11000000,	0b10010000'11000000 }, //	suba.{WL}  {ea}, A{REG}
		{ 0b11110001'00110000,	0b10010001'00000000 }, //	subx.{s}  {m}
		{ 0b11110001'00000000,	0b10010000'00000000 }, //	sub.{s}   {ea}, D{REG}
		{ 0b11110001'00000000,	0b10010001'00000000 }, //	sub.{s}   D{REG}, {ea}

		{ 0b11110000'11000000,	0b10110000'11000000 }, //	cmpa.{WL}  {ea}, A{REG}
		{ 0b11110001'00000000,	0b10110000'00000000 }, //	cmp.{s}   {ea}, D{REG}
		{ 0b11110001'00111000,	0b10110001'00001000 }, //	cmpm.{s}  A{reg}+, A{REG}+
		{ 0b11110001'00000000,	0b10110001'00000000 }, //	eor.{s}   D{REG}, {ea}

		{ 0b11110001'11000000,	0b11000000'11000000 }, //	mulu    {ea}, D{REG}
		{ 0b11110001'11000000,	0b11000001'11000000 }, //	muls    {ea}, D{REG}
		{ 0b11110001'11110000,	0b11000001'00000000 }, //	abcd    {m}
		{ 0b11110001'00110000,	0b11000001'00000000 }, //	exg     reg, reg
		{ 0b11110001'00000000,	0b11000000'00000000 }, //	and.{s}   {ea}, D{REG}
		{ 0b11110001'00000000,	0b11000001'00000000 }, //	and.{s}   D{REG}, {ea}

		{ 0b11110000'11000000,	0b11010000'11000000 }, //	adda.{WL}  {ea}, A{REG}
		{ 0b11110001'00110000,	0b11010001'00000000 }, //	addx.{s}  {m}
		{ 0b11110001'00000000,	0b11010000'00000000 }, //	add.{s}   {ea}, D{REG}
		{ 0b11110001'00000000,	0b11010001'00000000 }, //	add.{s}   D{REG}, {ea}

		{ 0b11111110'11000000,	0b11100000'11000000 }, //	as{R}     {ea}
		{ 0b11111110'11000000,	0b11100010'11000000 }, //	ls{R}     {ea}
		{ 0b11111110'11000000,	0b11100100'11000000 }, //	rox{R}    {ea}
		{ 0b11111110'11000000,	0b11100110'11000000 }, //	ro{R}     {ea}
		{ 0b11110000'00111000,	0b11100000'00000000 }, //	as{R}.{s}   {q}, D{reg}
		{ 0b11110000'00111000,	0b11100000'00001000 }, //	ls{R}.{s}   {q}, D{reg}
		{ 0b11110000'00111000,	0b11100000'00010000 }, //	rox{R}.{s}  {q}, D{reg}
		{ 0b11110000'00111000,	0b11100000'00011000 }, //	ro{R}.{s}   {q}, D{reg}
		{ 0b11110000'00111000,	0b11100000'00100000 }, //	as{R}.{s}   D{REG}, D{reg}
		{ 0b11110000'00111000,	0b11100000'00101000 }, //	ls{R}.{s}   D{REG}, D{reg}
		{ 0b11110000'00111000,	0b11100000'00110000 }, //	rox{R}.{s}  D{REG}, D{reg}
		{ 0b11110000'00111000,	0b11100000'00111000 }, //	ro{R}.{s}   D{REG}, D{reg}
	};

	constexpr uint8_t kImmediateDecodeMask = 0x03;
	constexpr uint8_t kImmediateNone = 0x00;
	constexpr uint8_t kImmediateUseSize = 0x01;
	constexpr uint8_t kImmediateWord = 0x02;

	constexpr uint8_t kEffectiveAddress1 = 0x04;
	constexpr uint8_t kEffectiveAddress2 = 0x08;

	constexpr uint8_t kSizeMask = 0x70;
	constexpr uint8_t kSizeUnspecified = 0x00; // unsized opcode.
	constexpr uint8_t kSizeVariableNormal = 0x10; // 2-bit (B/W/L) at bits #7-8
	constexpr uint8_t kSizeVariableSmallLow = 0x20; // 1-bit (W/L) at bit #7
	constexpr uint8_t kSizeVariableSmall = 0x30; // 1-bit (W/L) at bit #9
	constexpr uint8_t kSizeFixedByte = 0x40;
	constexpr uint8_t kSizeFixedWord = 0x50;
	constexpr uint8_t kSizeFixedLong = 0x60;

	constexpr uint8_t kSupervisor = 0x80;

	constexpr uint16_t kSizeMaskNormal = 0x00c0;
	constexpr uint16_t kSizeMaskSmallLow = 0x0040;
	constexpr uint16_t kSizeMaskSmall = 0x0100;


	struct Decoding
	{
		uint8_t code;
		uint16_t eaMask;
	};

	const Decoding kDecoding[kNumOpcodeEntries] =
	{
		{ kSizeVariableNormal | kImmediateUseSize, 0 },															//	ori     {imm:b}, CCR
		{ kSizeVariableNormal | kImmediateUseSize | kSupervisor, 0 },											//	ori     {imm:w}, SR
		{ kSizeVariableNormal | kImmediateUseSize | kEffectiveAddress1, EA::DataAlterable },					//	ori.{s}   {imm}, {ea}
		{ kSizeVariableNormal | kImmediateUseSize, 0 },															//	andi    {imm:b}, CCR
		{ kSizeVariableNormal | kImmediateUseSize | kSupervisor, 0 },											//	andi    {imm:w}, SR
		{ kSizeVariableNormal | kImmediateUseSize | kEffectiveAddress1, EA::DataAlterable },					//	andi.{s}  {imm}, {ea}
		{ kSizeVariableNormal | kImmediateUseSize | kEffectiveAddress1, EA::DataAlterable },					//	subi.{s}  {imm}, {ea}
		{ kSizeVariableNormal | kImmediateUseSize | kEffectiveAddress1, EA::DataAlterable },					//	addi.{s}  {imm}, {ea}
		{ kSizeVariableNormal | kImmediateUseSize, 0 },															//	eori    {imm:b}, CCR/SR
		{ kSizeVariableNormal | kImmediateUseSize | kSupervisor, 0 },											//	eori    {imm:w}, SR
		{ kSizeVariableNormal | kImmediateUseSize | kEffectiveAddress1, EA::DataAlterable },					//	eori.{s}  {imm}, {ea}
		{ kSizeVariableNormal | kImmediateUseSize | kEffectiveAddress1, EA::DataAlterable | EA::PcRelative },	//	cmpi.{s}  {imm}, {ea}
		{ kSizeFixedByte | kImmediateWord | kEffectiveAddress1, EA::DataAlterable | EA::PcRelative },			//	btst    {imm:b}, {ea}
		{ kSizeFixedByte | kImmediateWord | kEffectiveAddress1, EA::DataAlterable },							//	b(chg/clr/set)    {imm:b}, {ea}
		{ kImmediateWord, 0 },																					//	movep.{wl} ({immDis16}, A{reg}), D{REG}
		{ kImmediateWord, 0 },																					//	movep.{wl} D{REG}, ({immDis16}, A{reg})
		{ kSizeFixedByte | kEffectiveAddress1, EA::DataAddressing },											//	btst    D{REG}, {ea}
		{ kSizeFixedByte | kEffectiveAddress1, EA::DataAlterable },												//	b(chg/clr/set)    D{REG}, {ea}
		{ kEffectiveAddress1 | kEffectiveAddress2 | kSizeFixedByte, EA::All },									//	movea/move.b
		{ kEffectiveAddress1 | kEffectiveAddress2 | kSizeFixedLong, EA::All },									//	movea/move.l
		{ kEffectiveAddress1 | kEffectiveAddress2 | kSizeFixedWord, EA::All },									//	movea/move.w
		{ kSizeFixedWord | kEffectiveAddress1, EA::DataAlterable },												//	move    SR, {ea:w}
		{ kSizeFixedWord | kEffectiveAddress1, EA::DataAddressing },											//	move    {ea:b}, CCR (NOTE : this is a word size operation despite what the opcode sheet says (upper byte ignored though))
		{ kSizeFixedWord | kEffectiveAddress1 | kSupervisor, EA::DataAddressing },								//	move    {ea:w}, SR
		{ kSizeVariableNormal | kEffectiveAddress1, EA::DataAlterable },										//	negx.{s}  {ea}
		{ kSizeVariableNormal | kEffectiveAddress1, EA::DataAlterable },										//	clr.{s}   {ea}
		{ kSizeVariableNormal | kEffectiveAddress1, EA::DataAlterable },										//	neg.{s}   {ea}
		{ kSizeVariableNormal | kEffectiveAddress1, EA::DataAlterable },										//	not.{s}   {ea}
		{ kSizeVariableSmallLow, 0 },																			//	ext.{wl}   D{reg}
		{ kSizeFixedByte | kEffectiveAddress1, EA::DataAlterable },												//	nbcd    {ea:b}
		{ 0, 0 },																								//	swap    D{reg}
		{ kEffectiveAddress1, EA::ControlAddressing },															//	pea     {ea:l}
		{ kEffectiveAddress1, EA::DataAlterable },																//	tas     {ea:b}
		{ kSizeVariableNormal | kEffectiveAddress1, EA::Alterable | EA::Immediate },							//	tst.{s}   {ea}
		{ 0, 0 },																								//	trap    {v}
		{ kImmediateWord, 0 },																					//	link    A{reg}, {immDis16}
		{ 0, 0 },																								//	unlk    A{reg}
		{ kSupervisor, 0 },																						//	move    A{reg}, USP
		{ 0, 0 },																								//	reset
		{ 0, 0 },																								//	nop
		{ kImmediateWord | kSupervisor, 0 },																	//	stop
		{ kSupervisor, 0 },																						//	rte
		{ 0, 0 },																								//	rts
		{ 0, 0 },																								//	trapv
		{ 0, 0 },																								//	rtr
		{ kEffectiveAddress1, EA::ControlAddressing },															//	jsr     {ea}
		{ kEffectiveAddress1, EA::ControlAddressing },															//	jmp     {ea}
		{ kImmediateWord | kSizeVariableSmallLow | kEffectiveAddress1, EA::ControlAlterable | EA::AddressWithPreDec },		//	movem.{wl} {list}, {ea}
		{ kImmediateWord | kSizeVariableSmallLow | kEffectiveAddress1, EA::ControlAddressing | EA::AddressWithPostInc },	//	movem.{wl} {ea}, {list}
		{ kEffectiveAddress1, EA::ControlAddressing },															//	lea     {ea}, A{REG}
		{ kSizeFixedWord | kEffectiveAddress1, EA::DataAddressing },											//	chk     {ea}, D{REG}
		{ kImmediateWord, 0 },																					//	db{cc}    D{reg}, {braDisp}
		{ kSizeFixedByte | kEffectiveAddress1, EA::DataAlterable },												//	s{cc}     {ea}
		{ kSizeVariableNormal | kEffectiveAddress1, EA::Alterable },											//	addq.{s}  {q}, {ea}
		{ kSizeVariableNormal | kEffectiveAddress1, EA::Alterable },											//	subq.{s}  {q}, {ea}
		{ 0, 0 },																								//	bsr     {disp}
		{ 0, 0 },																								//	b{cc}     {disp}
		{ 0, 0 },																								//	moveq   {data}, D{REG}
		{ kSizeFixedWord | kEffectiveAddress1, EA::DataAddressing },											//	divu    {ea:w}, D{REG}
		{ kSizeFixedWord | kEffectiveAddress1, EA::DataAddressing },											//	divs    {ea:w}, D{REG}
		{ 0, 0 },																								//	sbcd    {m}
		{ kEffectiveAddress1 | kSizeVariableNormal, EA::DataAddressing },										//	or.{s}    {ea}, D{REG}
		{ kEffectiveAddress1 | kSizeVariableNormal, EA::MemoryAlterable },										//	or.{s}    D{REG}, {ea}
		{ kEffectiveAddress1 | kSizeVariableSmall, EA::All },													//	suba.{WL}  {ea}, A{REG}
		{ kSizeVariableNormal, 0 },																				//	subx.{s}  {m}
		{ kEffectiveAddress1 | kSizeVariableNormal, EA::All },													//	sub.{s}   {ea}, D{REG}
		{ kEffectiveAddress1 | kSizeVariableNormal, EA::MemoryAlterable },										//	sub.{s}   D{REG}, {ea}
		{ kEffectiveAddress1 | kSizeVariableSmall, EA::All },													//	cmpa.{WL}  {ea}, A{REG}
		{ kEffectiveAddress1 | kSizeVariableNormal, EA::All },													//	cmp.{s}   {ea}, D{REG}
		{ kSizeVariableNormal, 0 },																				//	cmpm.{s}  A{reg}+, A{REG}+
		{ kEffectiveAddress1 | kSizeVariableNormal, EA::DataAlterable },										//	eor.{s}   D{REG}, {ea}
		{ kSizeFixedWord | kEffectiveAddress1, EA::DataAddressing },											//	mulu    {ea}, D{REG}
		{ kSizeFixedWord | kEffectiveAddress1, EA::DataAddressing },											//	muls    {ea}, D{REG}
		{ 0, 0 },																								//	abcd    {m}
		{ 0, 0 },																								//	exg     reg, reg
		{ kEffectiveAddress1 | kSizeVariableNormal, EA::DataAddressing },										//	and.{s}   {ea}, D{REG}
		{ kEffectiveAddress1 | kSizeVariableNormal, EA::MemoryAlterable },										//	and.{s}   D{REG}, {ea}
		{ kEffectiveAddress1 | kSizeVariableSmall, EA::All },													//	adda.{WL}  {ea}, A{REG}
		{ kSizeVariableNormal, 0 },																				//	addx.{s}  {m}
		{ kEffectiveAddress1 | kSizeVariableNormal, EA::All },													//	add.{s}   {ea}, D{REG}
		{ kEffectiveAddress1 | kSizeVariableNormal, EA::MemoryAlterable },										//	add.{s}   D{REG}, {ea}
		{ kSizeFixedWord | kEffectiveAddress1, EA::MemoryAlterable },											//	as{R}     {ea}
		{ kSizeFixedWord | kEffectiveAddress1, EA::MemoryAlterable },											//	ls{R}     {ea}
		{ kSizeFixedWord | kEffectiveAddress1, EA::MemoryAlterable },											//	rox{R}    {ea}
		{ kSizeFixedWord | kEffectiveAddress1, EA::MemoryAlterable },											//	ro{R}     {ea}
		{ kSizeVariableNormal, 0 },																				//	as{R}.{s}   {q}, D{reg}
		{ kSizeVariableNormal, 0 },																				//	ls{R}.{s}   {q}, D{reg}
		{ kSizeVariableNormal, 0 },																				//	rox{R}.{s}  {q}, D{reg}
		{ kSizeVariableNormal, 0 },																				//	ro{R}.{s}   {q}, D{reg}
		{ kSizeVariableNormal, 0 },																				//	as{R}.{s}   D{REG}, D{reg}
		{ kSizeVariableNormal, 0 },																				//	ls{R}.{s}   D{REG}, D{reg}
		{ kSizeVariableNormal, 0 },																				//	rox{R}.{s}  D{REG}, D{reg}
		{ kSizeVariableNormal, 0 },																				//	ro{R}.{s}   D{REG}, D{reg}
	};

	bool BuildOpcodeGroupsTable()
	{
		const uint16_t groupMask = 0xf000;

		for (int i = 0; i < 16; i++)
		{
			uint16_t toMatch = uint16_t(i << 12);
			int j = 0;
			for (; j < int(kNumOpcodeEntries); j++)
			{
				if ((kEncodingList[j].signature & groupMask) == toMatch)
				{
					gOpcodeGroups[i] = j;
					break;
				}
			}
		}
		return true;
	}

	constexpr uint64_t SignExtend64(uint16_t value)
	{
		return uint64_t((int64_t(value) << 48) >> 48);
	}

	constexpr uint32_t SignExtend(uint16_t value)
	{
		return int32_t(int16_t(value));
	}

	uint32_t GetReg(const uint32_t& r, int size)
	{
		switch (size)
		{
		case 1:
			return (r & 0x000000ff);
		case 2:
			return (r & 0x0000ffff);
		case 4:
		default:
			return r;
		}
	}

	void SetReg(uint32_t& r, int size, uint32_t value)
	{
		switch (size)
		{
		case 1:
			r &= 0xffffff00;
			r |= value & 0x000000ff;
			break;

		case 2:
			r &= 0xffff0000;
			r |= value & 0x0000ffff;
			break;

		case 4:
		default:
			r = value;
		}
	}

}

M68000::OpcodeInstruction M68000::OpcodeFunction[kNumOpcodeEntries] =
{
	&M68000::Opcode_bitwise_to_status,	//	ori     {imm:b}, CCR
	&M68000::Opcode_bitwise_to_status,	//	ori     {imm:w}, SR
	&M68000::Opcode_bitwise_immediate,	//	ori.{s}   {imm}, {ea}
	&M68000::Opcode_bitwise_to_status,	//	andi    {imm:b}, CCR
	&M68000::Opcode_bitwise_to_status,	//	andi    {imm:w}, SR
	&M68000::Opcode_bitwise_immediate,	//	andi.{s}  {imm}, {ea}
	&M68000::Opcode_subi,				//	subi.{s}  {imm}, {ea}
	&M68000::Opcode_addi,				//	addi.{s}  {imm}, {ea}
	&M68000::Opcode_bitwise_to_status,	//	eori    {imm:b}, CCR
	&M68000::Opcode_bitwise_to_status,	//	eori    {imm:w}, SR
	&M68000::Opcode_bitwise_immediate,	//	eori.{s}  {imm}, {ea}
	&M68000::Opcode_cmpi,				//	cmpi.{s}  {imm}, {ea}
	&M68000::Opcode_bitop,				//	btst    {imm:b}, {ea}
	&M68000::Opcode_bitop,				//	b(chg/clr/set)    {imm:b}, {ea}
	&M68000::UnimplementOpcode,			//	movep.{wl} ({immDis16}, A{reg}), D{REG}
	&M68000::UnimplementOpcode,			//	movep.{wl} D{REG}, ({immDis16}, A{reg})
	&M68000::Opcode_bitop,				//	btst    D{REG}, {ea}
	&M68000::Opcode_bitop,				//	b(chg/clr/set)    D{REG}, {ea}
	&M68000::Opcode_move,				//	movea/move.b
	&M68000::Opcode_move,				//	movea/move.l
	&M68000::Opcode_move,				//	movea/move.w
	&M68000::Opcode_move_from_sr,		//	move    SR, {ea:w}
	&M68000::Opcode_move_to_ccr,		//	move    {ea:b}, CCR
	&M68000::Opcode_move_to_sr,			//	move    {ea:w}, SR
	&M68000::UnimplementOpcode,			//	negx.{s}  {ea}
	&M68000::Opcode_clr,				//	clr.{s}   {ea}
	&M68000::Opcode_neg,				//	neg.{s}   {ea}
	&M68000::Opcode_not,				//	not.{s}   {ea}
	&M68000::Opcode_ext,				//	ext.{wl}   D{reg}
	&M68000::UnimplementOpcode,			//	nbcd    {ea:b}
	&M68000::Opcode_swap,				//	swap    D{reg}
	&M68000::Opcode_pea,				//	pea     {ea:l}
	&M68000::UnimplementOpcode,			//	tas     {ea:b}
	&M68000::Opcode_tst,				//	tst.{s}   {ea}
	&M68000::Opcode_trap,				//	trap    {v}
	&M68000::Opcode_link,				//	link    A{reg}, {immDis16}
	&M68000::Opcode_unlk,				//	unlk    A{reg}
	&M68000::Opcode_move_usp,			//	move    A{reg}, USP
	&M68000::UnimplementOpcode,			//	reset
	&M68000::Opcode_nop,				//	nop
	&M68000::Opcode_stop,				//	stop
	&M68000::Opcode_rte,				//	rte
	&M68000::Opcode_rts,				//	rts
	&M68000::Opcode_trapv,				//	trapv
	&M68000::Opcode_rtr,				//	rtr
	&M68000::Opcode_jsr,				//	jsr     {ea}
	&M68000::Opcode_jmp,				//	jmp     {ea}
	&M68000::Opcode_movem,				//	movem.{wl} {list}, {ea}
	&M68000::Opcode_movem,				//	movem.{wl} {ea}, {list}
	&M68000::Opcode_lea,				//	lea     {ea}, A{REG}
	&M68000::Opcode_chk,				//	chk     {ea}, D{REG}
	&M68000::Opcode_dbcc,				//	db{cc}    D{reg}, {braDisp}
	&M68000::Opcode_scc,				//	s{cc}     {ea}
	&M68000::Opcode_addq,				//	addq.{s}  {q}, {ea}
	&M68000::Opcode_subq,				//	subq.{s}  {q}, {ea}
	&M68000::Opcode_bsr,				//	bsr     {disp}
	&M68000::Opcode_bcc,				//	b{cc}     {disp}
	&M68000::Opcode_moveq,				//	moveq   {data}, D{REG}
	&M68000::Opcode_divu,				//	divu    {ea:w}, D{REG}
	&M68000::Opcode_divs,				//	divs    {ea:w}, D{REG}
	&M68000::Opcode_sbcd,				//	sbcd    {m}
	&M68000::Opcode_bitwise,			//	or.{s}    {ea}, D{REG}
	&M68000::Opcode_bitwise,			//	or.{s}    D{REG}, {ea}
	&M68000::Opcode_suba,				//	suba.{WL}  {ea}, A{REG}
	&M68000::Opcode_subx,				//	subx.{s}  {m}
	&M68000::Opcode_sub,				//	sub.{s}   {ea}, D{REG}
	&M68000::Opcode_sub,				//	sub.{s}   D{REG}, {ea}
	&M68000::Opcode_cmpa,				//	cmpa.{WL}  {ea}, A{REG}
	&M68000::Opcode_cmp,				//	cmp.{s}   {ea}, D{REG}
	&M68000::Opcode_cmpm,				//	cmpm.{s}  A{reg}+, A{REG}+
	&M68000::Opcode_bitwise,			//	eor.{s}   D{REG}, {ea}
	&M68000::Opcode_mulu,				//	mulu    {ea}, D{REG}
	&M68000::Opcode_muls,				//	muls    {ea}, D{REG}
	&M68000::Opcode_abcd,				//	abcd    {m}
	&M68000::Opcode_exg,				//	exg     reg, reg
	&M68000::Opcode_bitwise,			//	and.{s}   {ea}, D{REG}
	&M68000::Opcode_bitwise,			//	and.{s}   D{REG}, {ea}
	&M68000::Opcode_adda,				//	adda.{WL}  {ea}, A{REG}
	&M68000::Opcode_addx,				//	addx.{s}  {m}
	&M68000::Opcode_add,				//	add.{s}   {ea}, D{REG}
	&M68000::Opcode_add,				//	add.{s}   D{REG}, {ea}
	&M68000::Opcode_shift_ea,			//	as{R}     {ea}
	&M68000::Opcode_shift_ea,			//	ls{R}     {ea}
	&M68000::Opcode_shift_ea,			//	rox{R}    {ea}
	&M68000::Opcode_shift_ea,			//	ro{R}     {ea}
	&M68000::Opcode_shift_reg,			//	as{R}.{s}   {q}, D{reg}
	&M68000::Opcode_shift_reg,			//	ls{R}.{s}   {q}, D{reg}
	&M68000::Opcode_shift_reg,			//	rox{R}.{s}  {q}, D{reg}
	&M68000::Opcode_shift_reg,			//	ro{R}.{s}   {q}, D{reg}
	&M68000::Opcode_shift_reg,			//	as{R}.{s}   D{REG}, D{reg}
	&M68000::Opcode_shift_reg,			//	ls{R}.{s}   D{REG}, D{reg}
	&M68000::Opcode_shift_reg,			//	rox{R}.{s}  D{REG}, D{reg}
	&M68000::Opcode_shift_reg,			//	ro{R}.{s}   D{REG}, D{reg}
};


M68000::M68000(IBus* bus)
	: m_bus(bus)
{
	// TODO: Can this be done at compile time?
	BuildOpcodeGroupsTable();
}

void M68000::Reset(int& delay)
{
	m_interruptControl = 0;

	m_operationHistory.fill(-1);
	m_operationHistoryPtr = 0;

	m_executeState = ExecuteState::ReadyToDecode;
	memset(&m_regs, 0, sizeof(Registers));
	m_regs.a[7] = m_bus->ReadBusWord(0x0000);
	m_regs.pc = ReadBusLong(0x0004);
	m_regs.status = 0b00100111'00000000;
	delay = 62;
}

bool M68000::InterruptActive() const
{
	return (m_interruptControl > ((m_regs.status >> 8) & 0x7)) || m_interruptControl == 7;
}

void M68000::SetInterruptControl(int intLevel)
{
	m_interruptControl = intLevel;
	if (m_executeState == ExecuteState::Stopped && InterruptActive())
	{
		m_executeState = ExecuteState::ReadyToDecode;
	}
}

bool M68000::DecodeOneInstruction(int& delay)
{
	if (InterruptActive())
	{
		StartInternalException(24 + m_interruptControl);

		delay += 12;

		m_regs.status &= 0b11111000'11111111;
		m_regs.status |= (m_interruptControl & 0x7) << 8;
		return true;
	}

	// returns false on illegal instruction

	m_executeState = ExecuteState::ReadyToExecute;

	m_operationAddr = m_regs.pc;

	m_operationHistory[m_operationHistoryPtr] = m_regs.pc;
	m_operationHistoryPtr = (m_operationHistoryPtr + 1) % m_operationHistory.size();

	m_operation = FetchNextOperationWord();

	m_currentInstructionIndex = kNumOpcodeEntries; // represents illegal opcode

	size_t instIndex = gOpcodeGroups[(m_operation & 0xf000) >> 12];
	for (; instIndex < kNumOpcodeEntries; instIndex++)
	{
		if ((m_operation & kEncodingList[instIndex].mask) == kEncodingList[instIndex].signature)
		{
			m_currentInstructionIndex = uint32_t(instIndex);
			break;
		}
	}

	if (instIndex >= kNumOpcodeEntries)
	{
		// illegal opcode so no further decoding required.
		m_regs.pc = m_operationAddr;
		return true;
	}

	const auto decodeCode = kDecoding[instIndex].code;

	if (!InSupervisorMode() && ((decodeCode & kSupervisor) != 0))
	{
		// Privileged instruction
		m_regs.pc = m_operationAddr;
		m_currentInstructionIndex = ~0;
		return true;
	}

	const auto sizeType = decodeCode & kSizeMask;

	switch (sizeType)
	{
	case kSizeUnspecified:
		m_opcodeSize = 0;
		break;

	case kSizeVariableNormal:
	{
		const auto size = (m_operation & kSizeMaskNormal) >> 6;
		m_opcodeSize = (size == 0b00) ? 1
			: (size == 0b01) ? 2
			: (size == 0b10) ? 4
			: 0;

		if (m_opcodeSize == 0)
		{
			m_regs.pc = m_operationAddr;
			m_currentInstructionIndex = kNumOpcodeEntries; // used to indicate illegal instruction
			return false;
		}
	}	break;

	case kSizeVariableSmallLow:
	{
		m_opcodeSize = ((m_operation & kSizeMaskSmallLow) != 0) ? 4 : 2;
	}	break;

	case kSizeVariableSmall:
	{
		m_opcodeSize = ((m_operation & kSizeMaskSmall) != 0) ? 4 : 2;
	}	break;

	case kSizeFixedByte:
		m_opcodeSize = 1;
		break;

	case kSizeFixedWord:
		m_opcodeSize = 2;
		break;

	case kSizeFixedLong:
		m_opcodeSize = 4;
		break;
	}

	const auto immType = decodeCode & kImmediateDecodeMask;
	switch (immType)
	{
	case kImmediateNone:
		break;

	case kImmediateUseSize:
		if (m_opcodeSize == 4)
		{
			m_immediateValue = uint32_t(FetchNextOperationWord()) << 16;
			m_immediateValue |= FetchNextOperationWord();
		}
		else
		{
			m_immediateValue = FetchNextOperationWord();
			if (m_opcodeSize == 1)
			{
				m_immediateValue &= 0x00ff;
			}
		}
		break;

	case kImmediateWord:
		m_immediateValue = FetchNextOperationWord();
		break;
	}

	auto GetEAType = [](uint16_t mode, uint16_t xn) -> uint16_t
	{
		if (mode < 7)
		{
			return uint16_t(0x0001 << mode);
		}
		else
		{
			return uint16_t(0x0080 << xn);
		}
	};

	bool hasEa1 = (decodeCode & kEffectiveAddress1) != 0;
	bool hasEa2 = (decodeCode & kEffectiveAddress2) != 0;

	if ((decodeCode & kEffectiveAddress1) != 0)
	{
		const uint32_t mode = (m_operation >> 3) & 0x7;
		const uint32_t xn = (m_operation & 0x7);

		const uint16_t eaType = GetEAType(mode, xn);
		if ((eaType & kDecoding[instIndex].eaMask) == 0)
		{
			// illegal ea mode
			m_regs.pc = m_operationAddr;
			m_currentInstructionIndex = kNumOpcodeEntries; // used to indicate illegal instruction
			return false;
		}

		m_ea[0] = DecodeEffectiveAddress(mode, xn, m_opcodeSize, delay);
		m_ea[0].mode = mode;
		m_ea[0].xn = xn;

	}
	if ((decodeCode & kEffectiveAddress2) != 0)
	{
		const uint32_t mode = (m_operation >> 6) & 0x7;
		const uint32_t xn = (m_operation >> 9) & 0x7;

		m_ea[1] = DecodeEffectiveAddress(mode, xn, m_opcodeSize, delay);
		m_ea[1].mode = mode;
		m_ea[1].xn = xn;
	}

	return true;
}

uint16_t M68000::FetchNextOperationWord()
{
	auto value = m_bus->ReadBusWord(m_regs.pc);
	m_regs.pc += 2;
	return value;
}

uint32_t M68000::ReadBusLong(uint32_t addr)
{
	uint32_t value = m_bus->ReadBusWord(addr);
	value <<= 16;
	value |= m_bus->ReadBusWord(addr + 2);
	return value;
}

void M68000::WriteBusLong(uint32_t addr, uint32_t value)
{
	m_bus->WriteBusWord(addr, uint16_t(value >> 16));
	m_bus->WriteBusWord(addr + 2, uint16_t(value));
}

uint32_t M68000::ReadBus(uint32_t addr, int size)
{
	switch (size)
	{
	case 1:
		return m_bus->ReadBusByte(addr);

	case 2:
		return m_bus->ReadBusWord(addr);

	case 4:
	default:
		return ReadBusLong(addr);
	}
}

void M68000::WriteBus(uint32_t addr, int size, uint32_t value)
{
	switch (size)
	{
	case 1:
		return m_bus->WriteBusByte(addr, uint8_t(value));

	case 2:
		return m_bus->WriteBusWord(addr, uint16_t(value));

	case 4:
	default:
		return WriteBusLong(addr, value);
	}
}

M68000::EA M68000::DecodeEffectiveAddress(uint32_t mode, uint32_t xn, int size, int& delay)
{
	switch (mode)
	{
	case 0b000: // Dn
		return { EffectiveAddressType::DataRegister, xn };

	case 0b001: // An
		return { EffectiveAddressType::AddressRegister, xn };

	case 0b010: // (An)
		return { EffectiveAddressType::MemoryAlterable, m_regs.a[xn] };

	case 0b011: // (An)+
	{
		if (size == 0)
			return { EffectiveAddressType::Bad, };

		const auto addr = m_regs.a[xn];
		auto newAddr = addr;
		if (size == 1 && xn == 7)
		{
			newAddr += 2;
		}
		else
		{
			newAddr += size;
		}
		m_regs.a[xn] = newAddr;
		return { EffectiveAddressType::MemoryAlterable, addr };
	}

	case 0b100: // -(An)
	{
		if (size == 0)
			return { EffectiveAddressType::Bad, };

		auto addr = m_regs.a[xn];
		if (size == 1 && xn == 7)
		{
			addr -= 2;
		}
		else
		{
			addr -= size;
		}
		m_regs.a[xn] = addr;
		delay++;
		return { EffectiveAddressType::MemoryAlterable, addr };
	}

	case 0b101: // (d16, An)
	{
		int16_t disp = FetchNextOperationWord();
		return { EffectiveAddressType::MemoryAlterable, m_regs.a[xn] + int32_t(disp) };
	}

	case 0b110: // (d8, An, Xn)
	{
		uint32_t addr = m_regs.a[xn];
		const uint16_t briefExtensionWord = FetchNextOperationWord();
		const bool wordSized = ((briefExtensionWord & 0b00001000'00000000) == 0);
		addr += int32_t(int8_t(briefExtensionWord & 0xff));
		const auto& regs = (briefExtensionWord & 0x8000) == 0 ? m_regs.d : m_regs.a;
		const auto indexReg = (briefExtensionWord & 0b01110000'00000000) >> 12;
		auto indexValue = regs[indexReg];
		if (wordSized)
		{
			indexValue = SignExtend(indexValue);
		}
		addr += indexValue;
		delay++;
		return { EffectiveAddressType::MemoryAlterable, addr };
	}

	case 0b111:
	{
		switch (xn)
		{
		case 0b000: // (xxx).w
		{
			auto addr = SignExtend(FetchNextOperationWord());
			return { EffectiveAddressType::MemoryAlterable, addr };
		}

		case 0b001: // (xxx).l
		{
			auto addr = uint32_t(FetchNextOperationWord()) << 16;
			addr |= uint32_t(FetchNextOperationWord());
			return { EffectiveAddressType::MemoryAlterable, addr };
		}

		case 0b010: // (d16, pc)
		{
			auto addr = m_regs.pc;
			addr += SignExtend(FetchNextOperationWord());
			return { EffectiveAddressType::MemorySourceOnly, addr };
		}

		case 0b011: // (d8, pc, Xn)
		{
			uint32_t addr = m_regs.pc;
			const uint16_t briefExtensionWord = FetchNextOperationWord();
			const bool wordSized = ((briefExtensionWord & 0b00001000'00000000) == 0);
			addr += int32_t(int8_t(briefExtensionWord & 0xff));
			const auto& regs = (briefExtensionWord & 0x8000) == 0 ? m_regs.d : m_regs.a;
			const auto indexReg = (briefExtensionWord & 0b01110000'00000000) >> 12;
			auto indexValue = regs[indexReg];
			if (wordSized)
			{
				indexValue = SignExtend(indexValue);
			}
			addr += indexValue;
			delay++;
			return { EffectiveAddressType::MemorySourceOnly, addr };
		}

		case 0b100: // #imm
		{
			if (size == 0)
				return { EffectiveAddressType::Bad, };

			uint32_t value = FetchNextOperationWord();

			if (size == 1)
			{
				value &= 0xff;
			}
			else if (size == 4)
			{
				value <<= 16;
				value |= FetchNextOperationWord();
			}
			return { EffectiveAddressType::Immediate, value };
		}

		default:
			return { EffectiveAddressType::Bad, };

		}
	}

	default:
		return { EffectiveAddressType::Bad, };
	}
}

bool M68000::GetEaValue(const EA& ea, int size, uint64_t& value)
{
	switch (ea.type)
	{
	case EffectiveAddressType::DataRegister:
		value = GetReg(m_regs.d[ea.addrIdx], size);
		return true;

	case EffectiveAddressType::AddressRegister:
		if (size == 1)
			return false;
		// TODO : check if word-length address register reads should be sign extended.
		value = GetReg(m_regs.a[ea.addrIdx], size);
		return true;

	case EffectiveAddressType::MemoryAlterable:
	case EffectiveAddressType::MemorySourceOnly:
		if ((ea.addrIdx & 1) != 0 && size > 1)
			return false; // unaligned memory access
		value = ReadBus(ea.addrIdx, size);
		return true;

	case EffectiveAddressType::Immediate:
		value = ea.addrIdx;
		return true;

	default:
		return false;
	}
}

bool M68000::SetEaValue(const EA& ea, int size, uint64_t value)
{
	switch (ea.type)
	{
	case EffectiveAddressType::DataRegister:
		SetReg(m_regs.d[ea.addrIdx], size, uint32_t(value));
		return true;

	case EffectiveAddressType::AddressRegister:
		if (size == 1)
			return false;
		if (size == 2)
			value = SignExtend(uint16_t(value));
		m_regs.a[ea.addrIdx] = uint32_t(value);
		return true;

	case EffectiveAddressType::MemoryAlterable:
		if ((ea.addrIdx & 1) != 0 && size > 1)
			return false; // unaligned memory access
		WriteBus(ea.addrIdx, size, uint32_t(value));
		return true;

	default:
		return false;
	}
}

bool M68000::ExecuteOneInstruction(int& delay)
{
	bool result = false;
	delay = 0;

	if (m_currentInstructionIndex == ~0)
	{
		// tried to execute a privileged instruction not in supervisor mode.
		result = StartInternalException(8);
	}
	else if (m_currentInstructionIndex < kNumOpcodeEntries)
	{
		result = std::invoke(OpcodeFunction[m_currentInstructionIndex], *this, delay);
	}
	else
	{
		// illegal instruction.
		result = StartInternalException(4);
	}

	// For unimplemented opcodes, freeze the CPU at the unimplemented opcode.
	if (result == false)
	{
		m_executeState = ExecuteState::Stopped;
		m_regs.pc = m_operationAddr;
		delay = 0;
	}
	else if (m_executeState == ExecuteState::ReadyToExecute)
	{
		m_executeState = ExecuteState::ReadyToDecode;
	}

	return result;
}

bool M68000::StartInternalException(uint8_t vectorNum)
{
	// * Switches to supervisor mode if not already in it
	// * Pushes pc and status to the stack
	// * Sets pc to address at exception vector index specified

	uint16_t status = m_regs.status;
	SetStatusRegister(m_regs.status | 0x2000);
	auto& sp = m_regs.a[7];

	// Push current PC to stack
	sp -= 4;
	WriteBusLong(sp, m_regs.pc);

	// Push current status register to stack
	sp -= 2;
	m_bus->WriteBusWord(sp, status);

	m_regs.pc = ReadBusLong(uint32_t(vectorNum) * 4);
	return true;
}

void M68000::SetStatusRegister(uint16_t value)
{
	if (((m_regs.status ^ value) & 0x2000) != 0)
	{
		// supervisor mode status has flipped. Swap the USP & SSP
		std::swap(m_regs.a[7], m_regs.altA7);
	}

	m_regs.status = value & 0b11110111'00011111;
}

void M68000::SetFlag(uint16_t flag, bool condition)
{
	if (condition)
	{
		m_regs.status |= flag;
	}
	else
	{
		m_regs.status &= ~flag;
	}
}

bool M68000::EvaluateCondition() const
{
	const int condition = (m_operation & 0b00001111'00000000) >> 8;

	switch (condition)
	{
	case 0b0000: // True
		return true;

	case 0b0001: // False
		return false;

	case 0b0010: // Higher (HI)
		return ((m_regs.status & (Carry | Zero)) == 0);

	case 0b0011: // Lower or Same (LS)
		return ((m_regs.status & (Carry | Zero)) != 0);

	case 0b0100: // Carry Clear (CC)
		return ((m_regs.status & Carry) == 0);

	case 0b0101: // Carry Set (CS)
		return ((m_regs.status & Carry) != 0);

	case 0b0110: // Not Equal (NE)
		return ((m_regs.status & Zero) == 0);

	case 0b0111: // Equal (EQ)
		return ((m_regs.status & Zero) != 0);

	case 0b1000: // Overflow Clear (VC)
		return ((m_regs.status & Overflow) == 0);

	case 0b1001: // Overflow Set (VS)
		return ((m_regs.status & Overflow) != 0);

	case 0b1010: // Plus (PL)
		return ((m_regs.status & Negative) == 0);

	case 0b1011: // Minus (MI)
		return ((m_regs.status & Negative) != 0);

	case 0b1100: // Greater or Equal (GE)
		return ((m_regs.status & (Negative | Overflow)) == (Negative | Overflow) || (m_regs.status & (Negative | Overflow)) == 0);

	case 0b1101: // Less Than (LT)
		return ((m_regs.status & (Negative | Overflow)) == Negative || (m_regs.status & (Negative | Overflow)) == Overflow);

	case 0b1110: // Greater Than (GT)
		return ((m_regs.status & (Negative | Overflow | Zero)) == (Negative | Overflow) || (m_regs.status & (Negative | Overflow | Zero)) == 0);

	case 0b1111: // Less or Equal (LE)
	default:
		return ((m_regs.status & Zero) == Zero || (m_regs.status & (Negative | Overflow)) == Negative || (m_regs.status & (Negative | Overflow)) == Overflow);
	}
}

uint64_t M68000::AluAdd(uint64_t a, uint64_t b, uint64_t c, int m_opcodeSize, uint16_t flagMask)
{
	// 'c' must be either 0 or 1

	uint64_t result = a + b + c;

	if (flagMask)
	{
		const uint64_t mask = ~0u >> ((4 - m_opcodeSize) * 8);
		const uint64_t msb = 1ull << (m_opcodeSize * 8 - 1);

		const auto signBefore = a & msb;
		const auto signAfter = result & msb;

		uint16_t newFlags = 0;

		if ((result & ~mask) != 0)
		{
			newFlags |= Carry|Extend;
		}

		if ((result & msb) != 0)
		{
			newFlags |= Negative;
		}
		else if ((result & mask) == 0)
		{
			newFlags |= Zero;
		}

		if ((signBefore == (b & msb)) && (signBefore != signAfter))
		{
			newFlags |= Overflow;
		}

		m_regs.status &= ~flagMask;
		m_regs.status |= newFlags & flagMask;
	}

	return result;
}

uint64_t M68000::AluSub(uint64_t a, uint64_t b, uint64_t c, int m_opcodeSize, uint16_t flagMask)
{
	// 'c' must be either 0 or 1

	uint64_t result = a - b - c;

	if (flagMask)
	{
		const uint64_t mask = ~0u >> ((4 - m_opcodeSize) * 8);
		const uint64_t msb = 1ull << (m_opcodeSize * 8 - 1);

		const auto signBefore = a & msb;
		const auto signAfter = result & msb;

		uint16_t newFlags = 0;

		if ((result & ~mask) != 0)
		{
			newFlags |= Carry | Extend;
		}

		if ((result & msb) != 0)
		{
			newFlags |= Negative;
		}
		else if ((result & mask) == 0)
		{
			newFlags |= Zero;
		}

		if ((signBefore != (b & msb)) && (signBefore != signAfter))
		{
			newFlags |= Overflow;
		}

		m_regs.status &= ~flagMask;
		m_regs.status |= newFlags & flagMask;
	}

	return result;
}

bool M68000::UnimplementOpcode(int& delay)
{
	return false;
}

bool M68000::Opcode_lea(int& delay)
{
	const int r = (m_operation & 0b00001110'00000000) >> 9;
	m_regs.a[r] = m_ea[0].addrIdx;

	if (m_ea[0].mode == 0b110) // d(An,ix)
	{
		delay += 2;
	}
	else if (m_ea[0].mode == 0b111 && m_ea[0].xn == 0b011) // d(PC,ix)
	{
		delay += 2;
	}

	return true;
}

bool M68000::Opcode_move(int& delay)
{
	uint64_t value;
	if (!GetEaValue(m_ea[0], m_opcodeSize, value))
		return false;

	if (!SetEaValue(m_ea[1], m_opcodeSize, value))
		return false;

	if (m_ea[1].type != EffectiveAddressType::AddressRegister)
	{
		const uint64_t msb = 1ull << (m_opcodeSize * 8 - 1);
		const uint64_t mask = ~0u >> ((4 - m_opcodeSize) * 8);

		SetFlag(Zero, (value & mask) == 0);
		SetFlag(Negative, (value & msb) != 0);
		SetFlag(Overflow | Carry, false);
	}
	return true;
}

bool M68000::Opcode_subq(int& delay)
{
	if (m_ea[0].type == EffectiveAddressType::AddressRegister)
	{
		if (m_opcodeSize == 1)
			return false; // cannot do byte-size subq on address register

		// From PRM: "When subtracting from address registers, the entire destination address register is used,
		// despite the operation size."
		m_opcodeSize = 4;

		delay += 2;
	}
	else if (m_ea[0].type == EffectiveAddressType::DataRegister && m_opcodeSize == 4)
	{
		delay += 2;
	}

	uint64_t value;
	if (!GetEaValue(m_ea[0], m_opcodeSize, value))
		return false;

	auto subValue = (m_operation & 0b00001110'00000000) >> 9;
	if (subValue == 0)
		subValue = 8;

	const uint16_t flagMask = (m_ea[0].type != EffectiveAddressType::AddressRegister) ? AllFlags : 0;
	value = AluSub(value, subValue, 0, m_opcodeSize, flagMask);

	if (!SetEaValue(m_ea[0], m_opcodeSize, value))
		return false;

	return true;
}

bool M68000::Opcode_bcc(int& delay)
{
	int32_t internalDisp = m_operation & 0x00ff;

	if (EvaluateCondition())
	{
		int32_t displacement;

		if (internalDisp == 0)
		{
			displacement = SignExtend(m_bus->ReadBusWord(m_regs.pc));
		}
		else
		{
			m_bus->ReadBusWord(m_regs.pc); // read next instruction (result not used but register the bus usage)
			displacement = internalDisp;
			displacement <<= 24;
			displacement >>= 24;
		}
		delay++;
		m_regs.pc += displacement;
	}
	else
	{
		if (internalDisp == 0)
		{
			m_regs.pc += 2;
			delay = 2;
		}
		delay += 2;
	}

	return true;
}

bool M68000::Opcode_cmpa(int& delay)
{
	auto reg = (m_operation & 0b00001110'00000000) >> 9;
	uint64_t destValue = m_regs.a[reg];

	uint64_t srcValue;
	if (!GetEaValue(m_ea[0], m_opcodeSize, srcValue))
		return false;

	if (m_opcodeSize == 2)
	{
		srcValue = SignExtend(uint16_t(srcValue));
	}

	AluSub(destValue, srcValue, 0, 4, AllFlagsMinusExtend);

	delay += 1;

	return true;
}

bool M68000::Opcode_cmpi(int& delay)
{
	uint64_t destValue;
	if (!GetEaValue(m_ea[0], m_opcodeSize, destValue))
		return false;

	AluSub(destValue, m_immediateValue, 0, m_opcodeSize, AllFlagsMinusExtend);

	if (m_opcodeSize == 4)
	{
		delay += 1;
	}

	return true;
}

bool M68000::Opcode_dbcc(int& delay)
{
	if (EvaluateCondition())
	{
		delay = 2;
	}
	else
	{
		delay = 1;
		const auto branchAddr = (m_regs.pc - 2) + SignExtend(m_immediateValue);
		const int reg = m_operation & 0b111;
		const auto val = (m_regs.d[reg] - 1) & 0xffff;
		m_regs.d[reg] &= 0xffff0000;
		m_regs.d[reg] |= val;

		if (val != 0xffff)
		{
			m_regs.pc = branchAddr;
		}
		else
		{
			m_bus->ReadBusWord(branchAddr); // Read but ignored (branch not taken).
		}
	}

	return true;
}

bool M68000::Opcode_bitop(int& delay)
{
	const bool isDynamic = (m_operation & 0b00000001'00000000) != 0;
	const uint32_t bitop = (m_operation & 0b00000000'11000000);

	uint32_t bitIndex;
	if (isDynamic)
	{
		const auto reg = (m_operation & 0b00001110'00000000) >> 9;
		bitIndex = m_regs.d[reg];
	}
	else
	{
		bitIndex = m_immediateValue;
	}

	if (m_ea[0].type == EffectiveAddressType::DataRegister)
	{
		// If destination is a data register, the full long word is used.
		m_opcodeSize = 4;
		bitIndex &= 0b11111;

		// TODO : try to work out exact timings for register bit operations.
		delay += 1;
		if (bitop != 0)
		{
			// TODO : bclr apparently takes longer still for some reason
			// but is this only when the bit is altered?
			delay += 1;
		}
	}
	else
	{
		bitIndex &= 0b111;
	}

	uint64_t value;
	if (!GetEaValue(m_ea[0], m_opcodeSize, value))
		return false;

	const auto bitmask = 1ull << bitIndex;

	SetFlag(Zero, (value & bitmask) == 0);

	if (bitop != 0)
	{
		if (bitop == 0b01000000) // bchg
		{
			value ^= bitmask;
		}
		else if (bitop == 0b10000000) // bclr
		{
			value &= ~bitmask;
		}
		else if (bitop == 0b11000000) // bset
		{
			value |= bitmask;
		}

		if (!SetEaValue(m_ea[0], m_opcodeSize, value))
			return false;
	}

	return true;
}

bool M68000::Opcode_add(int& delay)
{
	const bool ToEa = (m_operation & 0b00000001'00000000) != 0;
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;

	const uint64_t regValue = GetReg(m_regs.d[reg], m_opcodeSize);

	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], m_opcodeSize, eaValue))
		return false;

	const uint64_t result = AluAdd(regValue, eaValue, 0, m_opcodeSize, AllFlags);

	if (ToEa)
	{
		if (!SetEaValue(m_ea[0], m_opcodeSize, result))
			return false;
	}
	else
	{
		SetReg(m_regs.d[reg], m_opcodeSize, uint32_t(result));

		if (m_opcodeSize == 4)
		{
			delay += 1;
			if (m_ea[0].type == EffectiveAddressType::DataRegister
				|| m_ea[0].type == EffectiveAddressType::Immediate)
			{
				delay += 1;
			}
		}
	}

	return true;
}

bool M68000::Opcode_not(int& delay)
{
	uint64_t value;
	if (!GetEaValue(m_ea[0], m_opcodeSize, value))
		return false;

	value = ~value;

	if (!SetEaValue(m_ea[0], m_opcodeSize, value))
		return false;

	const uint64_t mask = ~0u >> ((4 - m_opcodeSize) * 8);
	const uint64_t msb = 1ull << (m_opcodeSize * 8 - 1);

	SetFlag(Negative, (value & msb) != 0);
	SetFlag(Zero, (value & mask) == 0);
	SetFlag(Carry | Overflow, false);

	if (m_opcodeSize == 4 && m_ea[0].type == EffectiveAddressType::DataRegister)
	{
		delay += 1;
	}

	return true;
}

bool M68000::Opcode_suba(int& delay)
{
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;

	uint64_t value;
	if (!GetEaValue(m_ea[0], m_opcodeSize, value))
		return false;

	if (m_opcodeSize == 2)
	{
		value = SignExtend(uint16_t(value));
		delay += 2;
	}
	else
	{
		delay += 1;
		if (m_ea[0].type == EffectiveAddressType::DataRegister ||
			m_ea[0].type == EffectiveAddressType::AddressRegister ||
			m_ea[0].type == EffectiveAddressType::Immediate)
		{
			delay += 1;
		}
	}

	m_regs.a[reg] -= uint32_t(value);

	return true;
}

bool M68000::Opcode_adda(int& delay)
{
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;

	uint64_t value;
	if (!GetEaValue(m_ea[0], m_opcodeSize, value))
		return false;

	if (m_opcodeSize == 2)
	{
		value = SignExtend(uint16_t(value));
		delay += 2;
	}
	else
	{
		delay += 1;
		if (m_ea[0].type == EffectiveAddressType::DataRegister ||
			m_ea[0].type == EffectiveAddressType::AddressRegister ||
			m_ea[0].type == EffectiveAddressType::Immediate)
		{
			delay += 1;
		}
	}

	m_regs.a[reg] += uint32_t(value);

	return true;
}

bool M68000::Opcode_tst(int& delay)
{
	uint64_t value;
	if (!GetEaValue(m_ea[0], m_opcodeSize, value))
		return false;

	const uint64_t mask = ~0u >> ((4 - m_opcodeSize) * 8);
	const uint64_t msb = 1ull << (m_opcodeSize * 8 - 1);

	SetFlag(Negative, (value & msb) != 0);
	SetFlag(Zero, (value & mask) == 0);
	SetFlag(Carry | Overflow, false);

	return true;
}

bool M68000::Opcode_cmp(int& delay)
{
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;
	uint64_t destValue = GetReg(m_regs.d[reg], m_opcodeSize);

	uint64_t srcValue;
	if (!GetEaValue(m_ea[0], m_opcodeSize, srcValue))
		return false;

	AluSub(destValue, srcValue, 0, m_opcodeSize, AllFlagsMinusExtend);

	if (m_opcodeSize == 4)
	{
		delay += 1;
	}

	return true;
}

bool M68000::Opcode_jmp(int& delay)
{
	if (m_ea[0].mode == 0b010 || m_ea[0].mode == 0b110 || (m_ea[0].mode == 0b111 && m_ea[0].xn == 0b011))
	{
		m_bus->ReadBusWord(m_regs.pc); // ignored
	}
	else if (m_ea[0].mode == 0b101 || (m_ea[0].mode == 0b111 && (m_ea[0].xn == 0b000 || m_ea[0].xn == 0b010)))
	{
		delay += 1;
	}

	if ((m_ea[0].addrIdx & 1) != 0)
		return false; // unaligned memory access

	m_regs.pc = m_ea[0].addrIdx;

	return true;
}

bool M68000::Opcode_moveq(int& delay)
{
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;
	const int32_t value = int32_t(int8_t(m_operation & 0xff));

	m_regs.d[reg] = value;

	SetFlag(Negative, value < 0);
	SetFlag(Zero, (value == 0));
	SetFlag(Overflow | Carry, false);

	return true;
}

bool M68000::Opcode_sub(int& delay)
{
	const bool ToEa = (m_operation & 0b00000001'00000000) != 0;
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;

	const uint64_t regValue = GetReg(m_regs.d[reg], m_opcodeSize);

	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], m_opcodeSize, eaValue))
		return false;

	uint64_t result;
	if (ToEa)
	{
		result = AluSub(eaValue, regValue, 0, m_opcodeSize, AllFlags);
		SetEaValue(m_ea[0], m_opcodeSize, result);
	}
	else
	{
		result = AluSub(regValue, eaValue, 0, m_opcodeSize, AllFlags);
		SetReg(m_regs.d[reg], m_opcodeSize, uint32_t(result));
		if (m_opcodeSize == 4)
		{
			delay += 1;
		}
	}

	return true;
}

bool M68000::Opcode_shift_ea(int& delay)
{
	const bool toLeft = (m_operation & 0b00000001'00000000) != 0;
	const auto op = (m_operation & 0b00000110'00000000);

	uint64_t value;
	if (!GetEaValue(m_ea[0], 2, value))
		return false;

	const uint64_t mask = 0xffff;
	const uint64_t msb = 0x8000;

	// all operations set Extend flag *except* ro(d)
	const uint16_t flags = (op != 0x0600) ? Carry | Extend : Carry;

	const bool extendIsSet = (m_regs.status & Extend) != 0;

	if (toLeft)
	{
		SetFlag(flags, (value & msb) != 0);

		value <<= 1;

		if (op == 0x0400) // roxl
		{
			if (extendIsSet)
			{
				value |= 1;
			}
		}
		else if (op == 0x0600) // rol
		{
			value |= (value >> 16);
		}
	}
	else
	{
		SetFlag(flags, (value & 1) != 0);
		if (op == 0x0000) // asr
		{
			value = int16_t(value) >> 1;
		}
		else if (op == 0x0200) // lsr
		{
			value >>= 1;
		}
		else if (op == 0x0400) // roxr
		{
			if (extendIsSet)
			{
				value |= 0x10000;
			}
			value >>= 1;
		}
		else  // ror
		{
			value |= (value & 1) << 16;
			value >>= 1;
		}
	}

	if (!SetEaValue(m_ea[0], 2, value))
		return false;

	SetFlag(Negative, (value & msb) != 0);
	SetFlag(Zero, (value & mask) == 0);
	SetFlag(Overflow, false);

	return true;
}

bool M68000::Opcode_shift_reg(int& delay)
{
	const bool toLeft = (m_operation & 0b00000001'00000000) != 0;
	const bool regSpecifiesCount = (m_operation & 0b00000000'00100000) != 0;
	int count = (m_operation & 0b00001110'00000000) >> 9;
	const int reg = m_operation & 0b00000000'00000111;
	const auto op = m_operation & 0b00000000'00011000;

	uint64_t value = GetReg(m_regs.d[reg], m_opcodeSize);

	if (regSpecifiesCount)
	{
		// M68000PRM: The shift count is the value in the data
		// register specified in the instruction modulo 64.
		count = m_regs.d[count] & 0x3f;
	}
	else if (count == 0)
	{
		// M68000PRM: The values 1 – 7 represent shifts of 1 – 7;
		// value of zero specifies a shift count of eight.
		count = 8;
	}

	const uint16_t flags = (op != 0x0018) ? Carry | Extend : Carry;

	SetFlag(Carry, false);

	const uint64_t mask = ~0u >> ((4 - m_opcodeSize) * 8);
	const uint64_t msb = 1ull << (m_opcodeSize * 8 - 1);

	for (int i = 0; i < count; i++)
	{
		const bool extendIsSet = (m_regs.status & Extend) != 0;

		delay++;
		if (toLeft)
		{
			SetFlag(flags, (value & msb) != 0);
			value <<= 1;

			if (op == 0x0010) // roxl
			{
				if (extendIsSet)
				{
					value |= 1;
				}
			}
			else if (op ==  0x0018) // rol
			{
				value |= ((value >> (m_opcodeSize * 8) & 1));
			}
		}
		else
		{
			SetFlag(flags, (value & 1) != 0);
			if (op == 0x0000) // asr
			{
				value |= ((value & msb) << 1);
			}
			else if (op == 0x0010) // roxr
			{
				if (extendIsSet)
				{
					value |= (msb << 1);
				}
			}
			else if (op == 0x0018)  // ror
			{
				value |= (value & 1) << (m_opcodeSize * 8);
			}
			value >>= 1;
		}
	}

	SetReg(m_regs.d[reg], m_opcodeSize, uint32_t(value));

	SetFlag(Negative, (value & msb) != 0);
	SetFlag(Zero, (value & mask) == 0);
	SetFlag(Overflow, false);

	delay++;
	if (m_opcodeSize == 4)
	{
		delay++;
	}

	return true;
}

bool M68000::Opcode_swap(int& delay)
{
	const int reg = m_operation & 0b111;

	uint32_t value = m_regs.d[reg];
	value = (value >> 16) | (value << 16);
	m_regs.d[reg] = value;

	const uint64_t msb = 0x80000000;

	SetFlag(Negative, (value & msb) != 0);
	SetFlag(Zero, value == 0);
	SetFlag(Overflow|Carry, false);

	return true;
}

bool M68000::Opcode_movem(int& delay)
{
	const bool toRegister = (m_operation & 0b00000100'00000000) != 0;

	if (toRegister)
	{
		/*
			M68000PRM:
			If the effective address is specified by the postincrement mode, only a memory-to-register
			operation is allowed. The registers are loaded starting at the specified address;
			the address is incremented by the operand length (2 or 4) following each transfer. The
			order of loading is the same as that of control mode addressing. When the instruction
			has completed, the incremented address register contains the address of the last operand
			loaded plus the operand length. If the addressing register is also loaded from
			memory, the memory value is ignored and the register is written with the postincremented
			effective address.
		*/

		uint16_t regMask = uint16_t(m_immediateValue);
		uint32_t addr = m_ea[0].addrIdx;

		for (int i = 0; i < 8; i++)
		{
			if ((regMask & 1) == 1)
			{
				uint32_t value = ReadBus(addr, m_opcodeSize);
				if (m_opcodeSize == 2)
				{
					value = SignExtend(value);
				}
				m_regs.d[i] = value;
				addr += m_opcodeSize;
			}
			regMask >>= 1;
		}
		for (int i = 0; i < 8; i++)
		{
			if ((regMask & 1) == 1)
			{
				uint32_t value = ReadBus(addr, m_opcodeSize);
				if (m_opcodeSize == 2)
				{
					value = SignExtend(value);
				}
				m_regs.a[i] = value;
				addr += m_opcodeSize;
			}
			regMask >>= 1;
		}

		const bool postIncrement = m_ea[0].mode == 0b011;
		if (postIncrement)
		{
			auto aReg = m_ea[0].xn;
			m_regs.a[aReg] = addr;
		}

		delay += 2; // extra unidentified IO according to timings doc. Add delay to compensate
	}
	else
	{
		/*
			If the effective address is specified by the predecrement mode, only a register-to-memory
			operation is allowed. The registers are stored starting at the specified address
			minus the operand length (2 or 4), and the address is decremented by the operand
			length following each transfer. The order of storing is from A7 to A0, then from D7 to
			D0. When the instruction has completed, the decremented address register contains
			the address of the last operand stored. For the MC68020, MC68030, MC68040, and
			CPU32, if the addressing register is also moved to memory, the value written is the initial
			register value decremented by the size of the operation. The MC68000 and
			MC68010 write the initial register value (not decremented).
		*/

		uint16_t regMask = uint16_t(m_immediateValue);
		uint32_t addr = m_ea[0].addrIdx;

		const bool preDecrement = m_ea[0].mode == 0b100;

		if (preDecrement)
		{
			auto aReg = m_ea[0].xn;

			for (int i = 0; i < 8; i++)
			{
				if ((regMask & 1) == 1)
				{
					auto value = GetReg(m_regs.a[7 - i], m_opcodeSize);
					WriteBus(addr, m_opcodeSize, value);
					addr -= m_opcodeSize;
				}
				regMask >>= 1;
			}
			for (int i = 0; i < 8; i++)
			{
				if ((regMask & 1) == 1)
				{
					auto value = GetReg(m_regs.d[7 - i],  m_opcodeSize);
					WriteBus(addr, m_opcodeSize, value);
					addr -= m_opcodeSize;
				}
				regMask >>= 1;
			}

			m_regs.a[aReg] = addr + m_opcodeSize;
		}
		else
		{
			for (int i = 0; i < 8; i++)
			{
				if ((regMask & 1) == 1)
				{
					auto value = GetReg(m_regs.d[i], m_opcodeSize);
					WriteBus(addr, m_opcodeSize, value);
					addr += m_opcodeSize;
				}
				regMask >>= 1;
			}
			for (int i = 0; i < 8; i++)
			{
				if ((regMask & 1) == 1)
				{
					auto value = GetReg(m_regs.a[i], m_opcodeSize);
					WriteBus(addr, m_opcodeSize, value);
					addr += m_opcodeSize;
				}
				regMask >>= 1;
			}
		}

		// TODO - check if more is required!

	}

	return true;
}

bool M68000::Opcode_subi(int& delay)
{
	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], m_opcodeSize, eaValue))
		return false;

	const uint64_t result = AluSub(eaValue, m_immediateValue, 0, m_opcodeSize, AllFlags);

	SetEaValue(m_ea[0], m_opcodeSize, result);

	if (m_opcodeSize == 4 && m_ea[0].type == EffectiveAddressType::DataRegister)
	{
		delay += 2;
	}

	return true;
}

bool M68000::Opcode_bsr(int& delay)
{
	int32_t displacement = int8_t(m_operation & 0x00ff);

	const auto pc = m_regs.pc;
	const auto nextWord = m_bus->ReadBusWord(m_regs.pc); // ignored if 8-bit displacement

	if (displacement == 0)
	{
		displacement = SignExtend(nextWord);
		m_regs.pc += 2;
	}

	m_regs.a[7] -= 4;
	WriteBusLong(m_regs.a[7], m_regs.pc);
	m_regs.pc = pc + displacement;

	delay += 1;

	return true;
}

bool M68000::Opcode_rts(int& delay)
{
	m_bus->ReadBusWord(m_regs.pc); // ignored (pre-fetch of next instruction)
	m_regs.pc = ReadBusLong(m_regs.a[7]);
	m_regs.a[7] += 4;

	return true;
}

bool M68000::Opcode_rtr(int& delay)
{
	auto& sp = m_regs.a[7];

	m_bus->ReadBusWord(m_regs.pc); // ignored (pre-fetch of next instruction)
	uint16_t sr = m_bus->ReadBusWord(sp);
	m_regs.status &= 0xff00;
	m_regs.status |= (sr & 0x00ff);
	sp += 2;
	m_regs.pc = ReadBusLong(sp);
	sp += 4;

	return true;
}

bool M68000::Opcode_bitwise(int& delay)
{
	const bool ToEa = (m_operation & 0b00000001'00000000) != 0;
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;
	const auto op = (m_operation & 0xf000);

	const uint64_t regValue = GetReg(m_regs.d[reg], m_opcodeSize);

	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], m_opcodeSize, eaValue))
		return false;

	uint64_t result;

	if (op == 0x8000) // or
	{
		result = regValue | eaValue;
	}
	else if (op == 0xc000) // and
	{
		result = regValue & eaValue;
	}
	else // eor
	{
		result = regValue ^ eaValue;
	}

	if (ToEa)
	{
		if (!SetEaValue(m_ea[0], m_opcodeSize, result))
			return false;

		if (m_opcodeSize == 4 && m_ea[0].type == EffectiveAddressType::DataRegister)
		{
			// This case only possible for eor
			delay += 1;
		}
	}
	else
	{
		SetReg(m_regs.d[reg], m_opcodeSize, uint32_t(result));

		if (m_opcodeSize == 4)
		{
			delay += 1;
			if (m_ea[0].type == EffectiveAddressType::DataRegister
				|| m_ea[0].type == EffectiveAddressType::Immediate)
			{
				delay += 1;
			}
		}
	}

	const uint64_t mask = ~0u >> ((4 - m_opcodeSize) * 8);
	const uint64_t msb = 1ull << (m_opcodeSize * 8 - 1);

	SetFlag(Negative, (result & msb) != 0);
	SetFlag(Zero, (result & mask) == 0);
	SetFlag(Overflow | Carry, false);

	return true;
}

bool M68000::Opcode_clr(int& delay)
{
	if (!SetEaValue(m_ea[0], m_opcodeSize, 0))
		return false;

	SetFlag(Zero, true);
	SetFlag(Negative | Overflow | Carry, false);

	if (m_opcodeSize == 4 && m_ea[0].type == EffectiveAddressType::DataRegister)
	{
		delay += 1;
	}

	return true;
}

bool M68000::Opcode_addq(int& delay)
{
	if (m_ea[0].type == EffectiveAddressType::AddressRegister)
	{
		if (m_opcodeSize == 1)
			return false; // cannot do byte-size subq on address register

		// From PRM: "When adding to address registers, the condition codes
		// are not altered, and the entire destination address register is
		// used regardless of the operation size."
		m_opcodeSize = 4;

		delay += 2;
	}
	else if (m_ea[0].type == EffectiveAddressType::DataRegister && m_opcodeSize == 4)
	{
		delay += 2;
	}

	auto addValue = (m_operation & 0b00001110'00000000) >> 9;
	if (addValue == 0)
		addValue = 8;

	uint64_t value;
	if (!GetEaValue(m_ea[0], m_opcodeSize, value))
		return false;

	const uint16_t flagMask = (m_ea[0].type != EffectiveAddressType::AddressRegister) ? AllFlags : 0;
	const auto result = AluAdd(value, addValue, 0, m_opcodeSize, flagMask);

	if (!SetEaValue(m_ea[0], m_opcodeSize, result))
		return false;

	return true;
}

bool M68000::Opcode_bitwise_immediate(int& delay)
{
	const auto op = m_operation & 0b00001110'00000000;

	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], m_opcodeSize, eaValue))
		return false;

	uint64_t result;

	if (op == 0x0000) // or
	{
		result = eaValue | m_immediateValue;
	}
	else if (op == 0x0200) // and
	{
		result = eaValue & m_immediateValue;
	}
	else // eor
	{
		result = eaValue ^ m_immediateValue;
	}

	SetEaValue(m_ea[0], m_opcodeSize, result);

	const uint64_t mask = ~0u >> ((4 - m_opcodeSize) * 8);
	const uint64_t msb = 1ull << (m_opcodeSize * 8 - 1);

	SetFlag(Zero, (result & mask) == 0);
	SetFlag(Negative, (result & msb) != 0);
	SetFlag(Overflow | Carry, false);

	if (m_opcodeSize == 4 && m_ea[0].type == EffectiveAddressType::DataRegister)
	{
		delay += 2;
	}

	return true;
}

bool M68000::Opcode_exg(int& delay)
{
	const auto opmode = (m_operation & 0b00000000'11111000);
	const auto rx = (m_operation & 0b00001110'00000000) >> 9;
	const auto ry = (m_operation & 0b00000000'00000111);

	if (opmode == 0b01000000)
	{
		// data registers
		std::swap(m_regs.d[rx], m_regs.d[ry]);
	}
	else if (opmode == 0b01001000)
	{
		// address registers
		std::swap(m_regs.a[rx], m_regs.a[ry]);
	}
	else if (opmode == 0b10001000)
	{
		// data registers and address register
		std::swap(m_regs.d[rx], m_regs.a[ry]);
	}
	else
	{
		// illegal instruction
		return false;
	}

	delay += 1;

	return true;
}

bool M68000::Opcode_jsr(int& delay)
{
	if (m_ea[0].mode == 0b010)
	{
		m_bus->ReadBusWord(m_regs.pc); // ignored
	}
	else if (m_ea[0].mode == 0b101 || (m_ea[0].mode == 0b111 && (m_ea[0].xn == 0b000 || m_ea[0].xn == 0b010)))
	{
		delay += 1;
	}
	else if (m_ea[0].mode == 0b110 || (m_ea[0].mode == 0b111 && m_ea[0].xn == 0b011))
	{
		delay += 2;
	}

	m_regs.a[7] -= 4;
	WriteBusLong(m_regs.a[7], m_regs.pc);

	if ((m_ea[0].addrIdx & 1) != 0)
		return false; // unaligned memory access

	m_regs.pc = m_ea[0].addrIdx;

	return true;
}

bool M68000::Opcode_mulu(int& delay)
{
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;

	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], 2, eaValue))
		return false;

	const uint64_t regValue = GetReg(m_regs.d[reg], 2);

	uint64_t result = eaValue * regValue;
	m_regs.d[reg] = uint32_t(result);

	SetFlag(Zero, result == 0);
	SetFlag(Negative, (result & 0x8000000) != 0);
	SetFlag(Overflow | Carry, false);

	delay += 18;

	int b = 1;
	for (int i = 0; i < 16; i++)
	{
		if ((eaValue & b) == 1)
		{
			delay += 1;
		}
		b <<= 1;
	}

	return true;
}

bool M68000::Opcode_muls(int& delay)
{
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;

	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], 2, eaValue))
		return false;

	const int32_t regValue = int16_t(GetReg(m_regs.d[reg], 2));

	int32_t result = int16_t(eaValue) * regValue;
	m_regs.d[reg] = uint32_t(result);

	SetFlag(Zero, result == 0);
	SetFlag(Negative, (result & 0x8000000) != 0);
	SetFlag(Overflow | Carry, false);

	delay += 18;

	//  MULS,MULU - The multiply algorithm requires 38+2n clocks where
	//              n is defined as:
	//          MULU: n = the number of ones in the <ea>
	//          MULS: n = concatanate the <ea> with a zero as the LSB;
	//                    n is the resultant number of 10 or 01 patterns
	//                    in the 17-bit source; i.e., worst case happens
	//                    when the source is $5555

	auto bits = eaValue << 1;
	while (bits)
	{
		switch (bits & 0b11)
		{
		case 0b01:
		case 0b10:
			delay++;
			break;

		default:
			break;
		}
		bits >>= 1;
	}

	return true;
}

bool M68000::Opcode_addi(int& delay)
{
	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], m_opcodeSize, eaValue))
		return false;

	const uint64_t result = AluAdd(eaValue, m_immediateValue, 0, m_opcodeSize, AllFlags);

	SetEaValue(m_ea[0], m_opcodeSize, result);

	if (m_opcodeSize == 4 && m_ea[0].type == EffectiveAddressType::DataRegister)
	{
		delay += 2;
	}

	return true;
}

bool M68000::Opcode_neg(int& delay)
{
	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], m_opcodeSize, eaValue))
		return false;

	const uint64_t result = AluSub(0, eaValue, 0, m_opcodeSize, AllFlags);

	SetEaValue(m_ea[0], m_opcodeSize, result);

	if (m_opcodeSize == 4 && m_ea[0].type == EffectiveAddressType::DataRegister)
	{
		delay += 1;
	}

	return true;
}

bool M68000::Opcode_move_usp(int& delay)
{
	// Supervisor mode only

	const bool toAddressReg = (m_operation & 0b1000) != 0;
	const int reg = m_operation & 0b111;

	if (toAddressReg)
	{
		m_regs.a[reg] = m_regs.altA7;
	}
	else
	{
		m_regs.altA7 = m_regs.a[reg];
	}

	return true;
}

bool M68000::Opcode_scc(int& delay)
{
	// Note: On the 68000, the EA value is read (despite being unused),
	// this is fixed on later processors.

	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], m_opcodeSize, eaValue))
		return false;

	if (EvaluateCondition())
	{
		eaValue = 0xff;
		if (m_ea[0].type == EffectiveAddressType::DataRegister)
		{
			delay += 1;
		}
	}
	else
	{
		eaValue = 0x00;
	}

	if (!SetEaValue(m_ea[0], m_opcodeSize, eaValue))
		return false;

	return true;
}

bool M68000::Opcode_bitwise_to_status(int& delay)
{
	const auto op = m_operation & 0b00001110'00000000;

	uint16_t value = m_regs.status;

	if (op == 0x0000) // or
	{
		value |= m_immediateValue;
	}
	else if (op == 0x0200) // and
	{
		value &= m_immediateValue;
	}
	else // eor
	{
		value ^= m_immediateValue;
	}

	const uint64_t mask = ~0u >> ((4 - m_opcodeSize) * 8);
	value = (m_regs.status & ~mask) | (value & mask);

	SetStatusRegister(value);

	// TODO : timing guide report a third bus read but what could it be?
	delay += 6;

	return true;
}

bool M68000::Opcode_pea(int& delay)
{
	m_regs.a[7] -= 4;
	WriteBusLong(m_regs.a[7], m_ea[0].addrIdx);

	return true;
}

bool M68000::Opcode_move_from_sr(int& delay)
{
	if (!SetEaValue(m_ea[0], 2, m_regs.status))
		return false;

	if (m_ea[0].type == EffectiveAddressType::DataRegister)
	{
		delay += 1;
	}

	return true;
}

bool M68000::Opcode_move_to_sr(int& delay)
{
	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], 2, eaValue))
		return false;

	SetStatusRegister(uint16_t(eaValue));

	delay += 4;

	return true;
}

bool M68000::Opcode_move_to_ccr(int& delay)
{
	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], 2, eaValue))
		return false;

	m_regs.status &= 0xff00;
	m_regs.status |= (eaValue & 0x1f);

	delay += 4;

	return true;
}

bool M68000::Opcode_rte(int& delay)
{
	m_bus->ReadBusWord(m_regs.pc); // next pc prefetch - ignored
	uint16_t sr = m_bus->ReadBusWord(m_regs.a[7]);
	m_regs.a[7] += 2;
	m_regs.pc = ReadBusLong(m_regs.a[7]);
	m_regs.a[7] += 4;
	SetStatusRegister(sr);

	return true;
}

bool M68000::Opcode_link(int& delay)
{
	const auto reg = (m_operation & 0b00000000'00000111);

	m_regs.a[7] -= 4;
	WriteBusLong(m_regs.a[7], m_regs.a[reg]);
	m_regs.a[reg] = m_regs.a[7];
	m_regs.a[7] += int16_t(m_immediateValue);

	return true;
}

bool M68000::Opcode_unlk(int& delay)
{
	const auto reg = (m_operation & 0b00000000'00000111);

	m_regs.a[7] = m_regs.a[reg];
	m_regs.a[reg] = ReadBusLong(m_regs.a[7]);
	m_regs.a[7] += 4;

	return true;
}

bool M68000::Opcode_cmpm(int& delay)
{
	const auto ay = (m_operation & 0b00000000'00000111);
	const auto ax = (m_operation & 0b00001110'00000000) >> 9;

	const uint64_t axVal = ReadBus(m_regs.a[ax], m_opcodeSize);
	const uint64_t ayVal = ReadBus(m_regs.a[ay], m_opcodeSize);

	AluSub(axVal, ayVal, 0, m_opcodeSize, AllFlagsMinusExtend);

	m_regs.a[ax] += m_opcodeSize;
	m_regs.a[ay] += m_opcodeSize;

	return true;
}

bool M68000::Opcode_ext(int& delay)
{
	const auto reg = (m_operation & 0b00000000'00000111);

	uint64_t value;
	if (m_opcodeSize == 2)
	{
		value = int16_t(int8_t(m_regs.d[reg]));
	}
	else
	{
		value = int32_t(int16_t(m_regs.d[reg]));
	}
	SetReg(m_regs.d[reg], m_opcodeSize, uint32_t(value));

	const uint64_t msb = 1ull << (m_opcodeSize * 8 - 1);

	SetFlag(Zero, value == 0);
	SetFlag(Negative, (value & msb) != 0);
	SetFlag(Overflow | Carry, false);

	return true;
}

bool M68000::Opcode_divu(int& delay)
{
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;
	uint64_t dividend = m_regs.d[reg];

	uint64_t divisor;
	if (!GetEaValue(m_ea[0], 2, divisor))
	{
		return false;
	}

	SetFlag(Carry, false);

	if (divisor == 0)
	{
		StartInternalException(5);
		return true;
	}

	if ((dividend >> 16) >= divisor)
	{
		delay += 5;
		SetFlag(Overflow, true);
		return true;
	}

	const auto quotient = dividend / divisor;
	const auto remainder = dividend % divisor;

	m_regs.d[reg] = uint32_t(quotient | (remainder << 16));

	SetFlag(Overflow, false);
	SetFlag(Zero, quotient == 0);
	SetFlag(Negative, false);

	// Timing for best case scenario
	delay += 38;

	return true;
}

bool M68000::Opcode_divs(int& delay)
{
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;
	int32_t dividend = m_regs.d[reg];

	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], 2, eaValue))
	{
		return false;
	}

	auto divisor = int16_t(eaValue);

	SetFlag(Carry, false);

	if (divisor == 0)
	{
		StartInternalException(5);
		return true;
	}

	if ((abs(dividend) >> 16) >= abs(divisor))
	{
		delay += 8;
		if (dividend < 0)
			delay += 1;
		SetFlag(Overflow, true);
		return true;
	}

	const auto quotient = dividend / divisor;
	const auto remainder = dividend % divisor;

	m_regs.d[reg] = (uint32_t(remainder) << 16) | uint16_t(quotient);

	SetFlag(Overflow, false);
	SetFlag(Zero, quotient == 0);
	SetFlag(Negative, quotient < 0);

	// Timing for best case scenario
	delay += 61;

	return true;
}

bool M68000::Opcode_stop(int& delay)
{
	m_regs.status = uint16_t(m_immediateValue);
	if (!InterruptActive())
	{
		m_executeState = ExecuteState::Stopped;
	}
	return true;
}

bool M68000::Opcode_nop(int& delay)
{
	return true;
}

bool M68000::Opcode_trap(int& delay)
{
	m_bus->ReadBusWord(m_regs.pc); // pre-fetch of next word (ignored)
	const auto v = 32 + (m_operation & 0b00000000'00001111);
	StartInternalException(v);
	delay += 5;
	return true;
}

bool M68000::Opcode_trapv(int& delay)
{
	if (m_regs.status & Overflow)
	{
		m_bus->ReadBusWord(m_regs.pc); // pre-fetch of next word (ignored)
		StartInternalException(7);
		delay += 5;
	}
	return true;
}

bool M68000::Opcode_addx(int& delay)
{
	const auto dReg = (m_operation & 0b00001110'00000000) >> 9;
	const auto sReg = (m_operation & 0b00000000'00000111);

	const bool memMode = (m_operation & 0b1000) != 0;

	const uint64_t extend = (m_regs.status & Extend) ? 1 : 0;
	const uint16_t zero = (m_regs.status & Zero);

	if (memMode)
	{
		const uint32_t sourceAddr = GetReg(m_regs.a[sReg], 4) - m_opcodeSize;
		const uint32_t destAddr = GetReg(m_regs.a[dReg], 4) - m_opcodeSize;
		SetReg(m_regs.a[sReg], 4, sourceAddr);
		SetReg(m_regs.a[dReg], 4, destAddr);

		const uint64_t sourceVal = ReadBus(sourceAddr, m_opcodeSize);
		uint64_t destVal = ReadBus(destAddr, m_opcodeSize);

		destVal = AluAdd(sourceVal, destVal, extend, m_opcodeSize, AllFlags);

		WriteBus(destAddr, m_opcodeSize, uint32_t(destVal));

		delay += 1;
	}
	else
	{
		const uint64_t sourceVal = GetReg(m_regs.d[sReg], m_opcodeSize);
		uint64_t destVal = GetReg(m_regs.d[dReg], m_opcodeSize);
		destVal = AluAdd(sourceVal, destVal, extend, m_opcodeSize, AllFlags);
		SetReg(m_regs.d[dReg], m_opcodeSize, uint32_t(destVal));

		if (m_opcodeSize == 4)
			delay += 2;
	}

	m_regs.status = (m_regs.status & ~Zero) | (m_regs.status & zero);

	return true;
}

bool M68000::Opcode_subx(int& delay)
{
	const auto dReg = (m_operation & 0b00001110'00000000) >> 9;
	const auto sReg = (m_operation & 0b00000000'00000111);

	const bool memMode = (m_operation & 0b1000) != 0;

	const uint64_t extend = (m_regs.status & Extend) ? 1 : 0;
	const uint16_t zero = (m_regs.status & Zero);

	if (memMode)
	{
		const uint32_t sourceAddr = GetReg(m_regs.a[sReg], 4) - m_opcodeSize;
		const uint32_t destAddr = GetReg(m_regs.a[dReg], 4) - m_opcodeSize;
		SetReg(m_regs.a[sReg], 4, sourceAddr);
		SetReg(m_regs.a[dReg], 4, destAddr);

		const uint64_t sourceVal = ReadBus(sourceAddr, m_opcodeSize);
		uint64_t destVal = ReadBus(destAddr, m_opcodeSize);

		destVal = AluSub(destVal, sourceVal, extend, m_opcodeSize, AllFlags);

		WriteBus(destAddr, m_opcodeSize, uint32_t(destVal));

		delay += 1;
	}
	else
	{
		const uint64_t sourceVal = GetReg(m_regs.d[sReg], m_opcodeSize);
		uint64_t destVal = GetReg(m_regs.d[dReg], m_opcodeSize);
		destVal = AluSub(destVal, sourceVal, extend, m_opcodeSize, AllFlags);
		SetReg(m_regs.d[dReg], m_opcodeSize, uint32_t(destVal));

		if (m_opcodeSize == 4)
			delay += 2;
	}

	m_regs.status = (m_regs.status & ~Zero) | (m_regs.status & zero);

	return true;
}

bool M68000::Opcode_abcd(int& delay)
{
	const auto dReg = (m_operation & 0b00001110'00000000) >> 9;
	const auto sReg = (m_operation & 0b00000000'00000111);

	const bool memMode = (m_operation & 0b1000) != 0;

	const uint64_t extend = (m_regs.status & Extend) ? 1 : 0;
	const uint16_t zero = (m_regs.status & Zero);

	uint64_t sourceVal;
	uint64_t destVal;
	uint32_t destAddr;

	if (memMode)
	{
		const uint32_t sourceAddr = GetReg(m_regs.a[sReg], 4) - 1;
		destAddr = GetReg(m_regs.a[dReg], 4) - 1;
		SetReg(m_regs.a[sReg], 4, sourceAddr);
		SetReg(m_regs.a[dReg], 4, destAddr);

		sourceVal = ReadBus(sourceAddr, 1);
		destVal = ReadBus(destAddr, 1);
	}
	else
	{
		sourceVal = GetReg(m_regs.d[sReg], 1);
		destVal = GetReg(m_regs.d[dReg], 1);
	}

	const bool halfCarry = ((sourceVal & 0xf) + (destVal & 0xf) + extend) > 0xf;

	destVal = AluAdd(sourceVal, destVal, extend, 1, AllFlags);

	uint64_t corf = 0;
	if (halfCarry || (destVal & 0xf) > 9)
	{
		corf = 0x06;
	}
	if (destVal > 0x99 || (m_regs.status & Carry))
	{
		corf |= 0x60;
	}

	destVal = AluAdd(destVal, corf, 0, 1, AllFlags);

	if (memMode)
	{
		WriteBus(destAddr, 1, uint32_t(destVal));
	}
	else
	{
		SetReg(m_regs.d[dReg], 1, uint32_t(destVal));
	}

	m_regs.status = (m_regs.status & ~Zero) | (m_regs.status & zero);
	delay += 1;

	return true;
}

bool M68000::Opcode_sbcd(int& delay)
{
	const auto dReg = (m_operation & 0b00001110'00000000) >> 9;
	const auto sReg = (m_operation & 0b00000000'00000111);

	const bool memMode = (m_operation & 0b1000) != 0;

	const uint64_t extend = (m_regs.status & Extend) ? 1 : 0;
	const uint16_t zero = (m_regs.status & Zero);

	uint64_t sourceVal;
	uint64_t destVal;
	uint32_t destAddr;

	if (memMode)
	{
		const uint32_t sourceAddr = GetReg(m_regs.a[sReg], 4) - 1;
		destAddr = GetReg(m_regs.a[dReg], 4) - 1;
		SetReg(m_regs.a[sReg], 4, sourceAddr);
		SetReg(m_regs.a[dReg], 4, destAddr);

		sourceVal = ReadBus(sourceAddr, 1);
		destVal = ReadBus(destAddr, 1);
	}
	else
	{
		sourceVal = GetReg(m_regs.d[sReg], 1);
		destVal = GetReg(m_regs.d[dReg], 1);
	}

	uint64_t result = destVal - sourceVal - extend;

	uint64_t carry = ((~destVal & (sourceVal | result)) | (sourceVal & result)) & 0x88;
	uint64_t corf = carry - (carry >> 2);
	result = AluSub(result, corf, 0, 1, AllFlags);

	if (memMode)
	{
		WriteBus(destAddr, 1, uint32_t(result));
	}
	else
	{
		SetReg(m_regs.d[dReg], 1, uint32_t(result));
	}

	m_regs.status = (m_regs.status & ~Zero) | (m_regs.status & zero);
	SetFlag(Carry|Extend, ((carry & 0x80) != 0));
	delay += 1;

	return true;
}

bool M68000::Opcode_chk(int& delay)
{
	const auto reg = (m_operation & 0b00001110'00000000) >> 9;
	const auto regVal = int16_t(GetReg(m_regs.d[reg], 2));

	uint64_t eaValue;
	if (!GetEaValue(m_ea[0], 2, eaValue))
	{
		return false;
	}

	SetFlag(Overflow|Carry, false);
	SetFlag(Zero, regVal == 0);

	if (regVal < 0 || regVal > int16_t(eaValue))
	{
		SetFlag(Negative, regVal < 0);
		m_bus->ReadBusWord(m_regs.pc); // pre-fetch of next word (ignored)
		StartInternalException(6);
		delay += 5;
	}

	delay += 3;
	return true;
}

template <typename S>
void M68000::Stream(S& s)
{
	using util::Stream;

	Stream(s, m_regs);
	Stream(s, m_executeState);
	Stream(s, m_operationAddr);
	Stream(s, m_currentInstructionIndex);
	Stream(s, m_immediateValue);
	Stream(s, m_interruptControl);
	Stream(s, m_operation);
	Stream(s, m_opcodeSize);
	Stream(s, m_ea[0]);
	Stream(s, m_ea[1]);
	Stream(s, m_operationHistory);
	Stream(s, m_operationHistoryPtr);
}

template void M68000::Stream<>(std::istream& s);
template void M68000::Stream<>(std::ostream& s);