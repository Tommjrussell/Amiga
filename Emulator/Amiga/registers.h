#pragma once

#include <stdint.h>

namespace am
{
	// Taken from the Amiga Hardware Reference Manual

	// A,D,P    A=Agnus chip, D=Denise chip, P=Paula chip.
	// W,R      W=write-only; R=read-only,
	// ER       Early read.
	// S        Strobe

	// Registers denoted with an "(E)" in the chip column means that
	// those registers have been changed in the Enhanced Chip Set
	// (ECS).

	enum class Register : uint32_t
	{
		BLTDDAT = 0x000,	//  ER  A       Blitter destination early read (dummy address)
		DMACONR = 0x002,	//  R   AP      DMA control(and blitter status) read
		VPOSR	= 0x004,	//  R   A(E)    Read vert most signif.bit(and frame flop)
		VHPOSR	= 0x006,	//  R   A       Read vert and horiz.position of beam
		DSKDATR	= 0x008,	//  ER  P       Disk data early read(dummy address)
		JOY0DAT = 0x00A,	//  R   D       Joystick - mouse 0 data(vert, horiz)
		JOY1DAT = 0x00C,	//  R   D       Joystick - mouse 1 data(vert, horiz)
		CLXDAT  = 0x00E,	//  R   D       Collision data register (read and clear)
		ADKCONR = 0x010,	//  R   P       Audio, disk control register read
		POT0DAT = 0x012,	//  R   P(E)    Pot counter pair 0 data(vert, horiz)
		POT1DAT = 0x014,	//  R   P(E)    Pot counter pair 1 data(vert, horiz)
		POTGOR  = 0x016,	//  R   P       Pot port data read(formerly POTINP)
		SERDATR = 0x018,	//  R   P       Serial port data and status read
		DSKBYTR = 0x01A,	//  R   P       Disk data byte and status read
		INTENAR = 0x01C,	//  R   P       Interrupt enable bits read
		INTREQR = 0x01E,	//  R   P       Interrupt request bits read
		DSKPTH  = 0x020,	//  W   A(E)    Disk pointer(high 3 bits, 5 bits if ECS)
		DSKPTL  = 0x022,	//  W   A       Disk pointer(low 15 bits)
		DSKLEN	= 0x024,	//  W   P       Disk length
		DSKDAT  = 0x026,	//  W   P       Disk DMA data write
		REFPTR  = 0x028,	//  W   A       Refresh pointer
		VPOSW   = 0x02A,	//  W   A       Write vert most signif.bit(and frame flop)
		VHPOSW  = 0x02C,	//  W   A       Write vert and horiz position of beam
		COPCON  = 0x02E,	//  W   A(E)    Coprocessor control register (CDANG)
		SERDAT  = 0x030,	//  W   P       Serial port data and stop bits write
		SERPER	= 0x032,	//  W   P       Serial port period and control
		POTGO   = 0x034,	//  W   P       Pot port data write and start
		JOYTEST = 0x036,	//  W   D       Write to all four joystick - mouse counters at once
		STREQU  = 0x038,	//  S   D       Strobe for horiz sync with VB and EQU
		STRVBL  = 0x03A,	//  S   D       Strobe for horiz sync with VB(vert.blank)
		STRHOR  = 0x03C,	//  S   DP      Strobe for horiz sync
		STRLONG = 0x03E,	//  S   D(E)    Strobe for identification of long horiz.line.
		BLTCON0 = 0x040,	//  W   A       Blitter control register 0
		BLTCON1 = 0x042,	//  W   A(E)    Blitter control register 1
		BLTAFWM = 0x044,	//  W   A       Blitter first word mask for source A
		BLTALWM = 0x046,	//  W   A       Blitter last word mask for source A
		BLTCPTH = 0x048,	//  W   A       Blitter pointer to source C(high 3 bits)
		BLTCPTL = 0x04A,	//  W   A       Blitter pointer to source C(low 15 bits)
		BLTBPTH = 0x04C,	//  W   A       Blitter pointer to source B(high 3 bits)
		BLTBPTL = 0x04E,	//  W   A       Blitter pointer to source B(low 15 bits)
		BLTAPTH = 0x050,	//  W   A(E)    Blitter pointer to source A(high 3 bits)
		BLTAPTL = 0x052,	//  W   A       Blitter pointer to source A(low 15 bits)
		BLTDPTH = 0x054,	//  W   A       Blitter pointer to destination D(high 3 bits)
		BLTDPTL = 0x056,	//  W   A       Blitter pointer to destination D(low 15 bits)
		BLTSIZE = 0x058,	//  W   A       Blitter start and size(window width, height)
		BLTCON0L= 0x05A,	//  W   A(E)    Blitter control 0, lower 8 bits(minterms)
		BLTSIZV = 0x05C,	//  W   A(E)    Blitter V size(for 15 bit vertical size)
		BLTSIZH = 0x05E,	//  W   A(E)    Blitter H size and start(for 11 bit H size)
		BLTCMOD = 0x060,	//  W   A       Blitter modulo for source C
		BLTBMOD = 0x062,	//  W   A       Blitter modulo for source B
		BLTAMOD = 0x064,	//  W   A       Blitter modulo for source A
		BLTDMOD = 0x066,	//  W   A       Blitter modulo for destination D

		BLTCDAT = 0x070,	//  W   A       Blitter source C data register
		BLTBDAT = 0x072,	//  W   A       Blitter source B data register
		BLTADAT = 0x074,	//  W   A       Blitter source A data register

		SPRHDAT = 0x078,	//  W   A(E)    Ext.logic UHRES sprite pointer and data id

		DENISEID= 0x07C,	//  R   D(E)    Chip revision level for Denise (video out chip)
		DSKSYNC = 0x07E,	//  W   P       Disk sync pattern register for disk read
		COP1LCH = 0x080,	//  W   A(E)    Coprocessor first location register (high 3 bits, high 5 bits if ECS)
		COP1LCL = 0x082,	//  W   A       Coprocessor first location register (low 15 bits)
		COP2LCH = 0x084,	//  W   A(E)    Coprocessor second location register (high 3 bits, high 5 bits if ECS)
		COP2LCL = 0x086,	//  W   A       Coprocessor second location register (low 15 bits)
		COPJMP1 = 0x088,	//  S   A       Coprocessor restart at first location
		COPJMP2 = 0x08A,	//  S   A       Coprocessor restart at second location
		COPINS  = 0x08C,	//  W   A       Coprocessor instruction fetch identify
		DIWSTRT = 0x08E,	//  W   A       Display window start(upper left vert - horiz position)
		DIWSTOP = 0x090,	//  W   A       Display window stop(lower right vert. - horiz.position)
		DDFSTRT = 0x092,	//  W   A       Display bitplane data fetch start (horiz.position)
		DDFSTOP = 0x094,	//  W   A       Display bitplane data fetch stop (horiz.position)
		DMACON  = 0x096,	//  W   ADP     DMA control write(clear or set)
		CLXCON  = 0x098,	//  W   D       Collision control
		INTENA  = 0x09A,	//  W   P       Interrupt enable bits(clear or set bits)
		INTREQ  = 0x09C,	//  W   P       Interrupt request bits(clear or set bits)
		ADKCON  = 0x09E,	//  W   P       Audio, disk, UART control
		AUD0LCH = 0x0A0,	//  W   A(E)    Audio channel 0 location(high 3 bits, 5 if ECS)
		AUD0LCL = 0x0A2,	//  W   A       Audio channel 0 location(low 15 bits)
		AUD0LEN = 0x0A4,	//  W   P       Audio channel 0 length
		AUD0PER = 0x0A6,	//  W   P(E)    Audio channel 0 period
		AUD0VOL = 0x0A8,	//  W   P       Audio channel 0 volume
		AUD0DAT = 0x0AA,	//  W   P       Audio channel 0 data

		AUD1LCH = 0x0B0,	//	W   A       Audio channel 1 location(high 3 bits)
		AUD1LCL = 0x0B2,	//	W   A       Audio channel 1 location(low 15 bits)
		AUD1LEN = 0x0B4,	//	W   P       Audio channel 1 length
		AUD1PER = 0x0B6, 	//	W   P       Audio channel 1 period
		AUD1VOL = 0x0B8, 	//	W   P       Audio channel 1 volume
		AUD1DAT = 0x0BA, 	//	W   P       Audio channel 1 data

		AUD2LCH = 0x0C0,  	//	W   A       Audio channel 2 location(high 3 bits)
		AUD2LCL = 0x0C2,  	//	W   A       Audio channel 2 location(low 15 bits)
		AUD2LEN = 0x0C4,  	//	W   P       Audio channel 2 length
		AUD2PER = 0x0C6,  	//	W   P       Audio channel 2 period
		AUD2VOL = 0x0C8,  	//	W   P       Audio channel 2 volume
		AUD2DAT = 0x0CA,  	//	W   P       Audio channel 2 data

		AUD3LCH = 0x0D0,  	//	W   A       Audio channel 3 location(high 3 bits)
		AUD3LCL = 0x0D2,  	//	W   A       Audio channel 3 location(low 15 bits)
		AUD3LEN = 0x0D4,  	//	W   P       Audio channel 3 length
		AUD3PER = 0x0D6,  	//	W   P       Audio channel 3 period
		AUD3VOL = 0x0D8,  	//	W   P       Audio channel 3 volume
		AUD3DAT = 0x0DA,  	//	W   P       Audio channel 3 data

		BPL1PTH = 0x0E0,  	//	W   A       Bitplane 1 pointer(high 3 bits)
		BPL1PTL = 0x0E2,  	//	W   A       Bitplane 1 pointer(low 15 bits)
		BPL2PTH = 0x0E4,  	//	W   A       Bitplane 2 pointer(high 3 bits)
		BPL2PTL = 0x0E6,  	//	W   A       Bitplane 2 pointer(low 15 bits)
		BPL3PTH = 0x0E8,  	//	W   A       Bitplane 3 pointer(high 3 bits)
		BPL3PTL = 0x0EA,  	//	W   A       Bitplane 3 pointer(low 15 bits)
		BPL4PTH = 0x0EC,  	//	W   A       Bitplane 4 pointer(high 3 bits)
		BPL4PTL = 0x0EE,  	//	W   A       Bitplane 4 pointer(low 15 bits)
		BPL5PTH = 0x0F0,  	//	W   A       Bitplane 5 pointer(high 3 bits)
		BPL5PTL = 0x0F2,  	//	W   A       Bitplane 5 pointer(low 15 bits)
		BPL6PTH = 0x0F4,  	//	W   A       Bitplane 6 pointer(high 3 bits)
		BPL6PTL = 0x0F6,  	//	W   A       Bitplane 6 pointer(low 15 bits)

		BPLCON0 = 0x100,  	//	W   AD(E)   Bitplane control register (misc.control bits)
		BPLCON1 = 0x102,  	//	W   D       Bitplane control reg. (scroll value PF1, PF2)
		BPLCON2 = 0x104,  	//	W   D(E)    Bitplane control reg. (priority control)
		BPLCON3 = 0x106,  	//	W   D(E)    Bitplane control(enhanced features)
		BPL1MOD = 0x108,  	//	W   A       Bitplane modulo(odd planes)
		BPL2MOD = 0x10A,  	//	W   A       Bitplane modulo(even planes)

		BPL1DAT = 0x110,  	//	W   D       Bitplane 1 data(parallel - to - serial convert)
		BPL2DAT = 0x112,  	//	W   D       Bitplane 2 data(parallel - to - serial convert)
		BPL3DAT = 0x114,  	//	W   D       Bitplane 3 data(parallel - to - serial convert)
		BPL4DAT = 0x116, 	//	W   D       Bitplane 4 data(parallel - to - serial convert)
		BPL5DAT = 0x118,  	//	W   D       Bitplane 5 data(parallel - to - serial convert)
		BPL6DAT = 0x11A,  	//	W   D       Bitplane 6 data(parallel - to - serial convert)

		SPR0PTH = 0x120,  	//	W   A       Sprite 0 pointer(high 3 bits)
		SPR0PTL = 0x122,  	//	W   A       Sprite 0 pointer(low 15 bits)
		SPR1PTH = 0x124,  	//	W   A       Sprite 1 pointer(high 3 bits)
		SPR1PTL = 0x126,  	//	W   A       Sprite 1 pointer(low 15 bits)
		SPR2PTH = 0x128,  	//	W   A       Sprite 2 pointer(high 3 bits)
		SPR2PTL = 0x12A,  	//	W   A       Sprite 2 pointer(low 15 bits)
		SPR3PTH = 0x12C,  	//	W   A       Sprite 3 pointer(high 3 bits)
		SPR3PTL = 0x12E,  	//	W   A       Sprite 3 pointer(low 15 bits)
		SPR4PTH = 0x130,  	//	W   A       Sprite 4 pointer(high 3 bits)
		SPR4PTL = 0x132,  	//	W   A       Sprite 4 pointer(low 15 bits)
		SPR5PTH = 0x134,  	//	W   A       Sprite 5 pointer(high 3 bits)
		SPR5PTL = 0x136,  	//	W   A       Sprite 5 pointer(low 15 bits)
		SPR6PTH = 0x138,  	//	W   A       Sprite 6 pointer(high 3 bits)
		SPR6PTL = 0x13A,  	//	W   A       Sprite 6 pointer(low 15 bits)
		SPR7PTH = 0x13C,  	//	W   A       Sprite 7 pointer(high 3 bits)
		SPR7PTL = 0x13E,  	//	W   A       Sprite 7 pointer(low 15 bits)
		SPR0POS = 0x140,  	//	W   AD      Sprite 0 vert - horiz start position data
		SPR0CTL = 0x142,  	//	W   AD(E)   Sprite 0 vert stop position and control data
		SPR0DATA= 0x144,  	//	W   D       Sprite 0 image data register A
		SPR0DATB= 0x146,  	//	W   D       Sprite 0 image data register B
		SPR1POS = 0x148,  	//	W   AD      Sprite 1 vert - horiz start position data
		SPR1CTL = 0x14A,  	//	W   AD      Sprite 1 vert stop position and control data
		SPR1DATA= 0x14C,  	//	W   D       Sprite 1 image data register A
		SPR1DATB= 0x14E,  	//	W   D       Sprite 1 image data register B
		SPR2POS = 0x150,  	//	W   AD      Sprite 2 vert - horiz start position data
		SPR2CTL = 0x152,  	//	W   AD      Sprite 2 vert stop position and control data
		SPR2DATA= 0x154,  	//	W   D       Sprite 2 image data register A
		SPR2DATB= 0x156,  	//	W   D       Sprite 2 image data register B
		SPR3POS = 0x158,  	//	W   AD      Sprite 3 vert - horiz start position data
		SPR3CTL = 0x15A,  	//	W   AD      Sprite 3 vert stop position and control data
		SPR3DATA= 0x15C, 	//	W   D       Sprite 3 image data register A
		SPR3DATB= 0x15E,  	//	W   D       Sprite 3 image data register B
		SPR4POS = 0x160,  	//	W   AD      Sprite 4 vert - horiz start position data
		SPR4CTL = 0x162,  	//	W   AD      Sprite 4 vert stop position and control data
		SPR4DATA= 0x164,  	//	W   D       Sprite 4 image data register A
		SPR4DATB= 0x166,  	//	W   D       Sprite 4 image data register B
		SPR5POS = 0x168,  	//	W   AD      Sprite 5 vert - horiz start position data
		SPR5CTL = 0x16A,  	//	W   AD      Sprite 5 vert stop position and control data
		SPR5DATA= 0x16C,  	//	W   D       Sprite 5 image data register A
		SPR5DATB= 0x16E,  	//	W   D       Sprite 5 image data register B
		SPR6POS = 0x170,  	//	W   AD      Sprite 6 vert - horiz start position data
		SPR6CTL = 0x172,  	//	W   AD      Sprite 6 vert stop position and control data
		SPR6DATA= 0x174,  	//	W   D       Sprite 6 image data register A
		SPR6DATB= 0x176,  	//	W   D       Sprite 6 image data register B
		SPR7POS = 0x178,  	//	W   AD      Sprite 7 vert - horiz start position data
		SPR7CTL = 0x17A,  	//	W   AD      Sprite 7 vert stop position and control data
		SPR7DATA= 0x17C,  	//	W   D       Sprite 7 image data register A
		SPR7DATB= 0x17E,  	//	W   D       Sprite 7 image data register B
		COLOR00 = 0x180,  	//	W   D       Color table 00
		COLOR01 = 0x182,  	//	W   D       Color table 01
		COLOR02 = 0x184,  	//	W   D       Color table 02
		COLOR03 = 0x186,  	//	W   D       Color table 03
		COLOR04 = 0x188,  	//	W   D       Color table 04
		COLOR05 = 0x18A,  	//	W   D       Color table 05
		COLOR06 = 0x18C, 	//	W   D       Color table 06
		COLOR07 = 0x18E,  	//	W   D       Color table 07
		COLOR08 = 0x190,  	//	W   D       Color table 08
		COLOR09 = 0x192,  	//	W   D       Color table 09
		COLOR10 = 0x194,  	//	W   D       Color table 10
		COLOR11 = 0x196,  	//	W   D       Color table 11
		COLOR12 = 0x198, 	//	W   D       Color table 12
		COLOR13 = 0x19A,  	//	W   D       Color table 13
		COLOR14 = 0x19C,  	//	W   D       Color table 14
		COLOR15 = 0x19E,  	//	W   D       Color table 15
		COLOR16 = 0x1A0,  	//	W   D       Color table 16
		COLOR17 = 0x1A2,  	//	W   D       Color table 17
		COLOR18 = 0x1A4,  	//	W   D       Color table 18
		COLOR19 = 0x1A6,  	//	W   D       Color table 19
		COLOR20 = 0x1A8,  	//	W   D       Color table 20
		COLOR21 = 0x1AA,  	//	W   D       Color table 21
		COLOR22 = 0x1AC,  	//	W   D       Color table 22
		COLOR23 = 0x1AE,  	//	W   D       Color table 23
		COLOR24 = 0x1B0,  	//	W   D       Color table 24
		COLOR25 = 0x1B2,  	//	W   D       Color table 25
		COLOR26 = 0x1B4,  	//	W   D       Color table 26
		COLOR27 = 0x1B6,  	//	W   D       Color table 27
		COLOR28 = 0x1B8,  	//	W   D       Color table 28
		COLOR29 = 0x1BA,  	//	W   D       Color table 29
		COLOR30 = 0x1BC,  	//	W   D       Color table 30
		COLOR31 = 0x1BE,  	//	W   D       Color table 31
		HTOTAL  = 0x1C0,  	//	W   A(E)    Highest number count, horiz line (VARBEAMEN = 1)
		HSSTOP  = 0x1C2,  	//	W   A(E)    Horizontal line position for HSYNC stop
		HBSTRT  = 0x1C4,  	//	W   A(E)    Horizontal line position for HBLANK start
		HBSTOP  = 0x1C6,  	//	W   A(E)    Horizontal line position for HBLANK stop
		VTOTAL  = 0x1C8,  	//	W   A(E)    Highest numbered vertical line (VARBEAMEN = 1)
		VSSTOP  = 0x1CA,  	//	W   A(E)    Vertical line position for VSYNC stop
		VBSTRT  = 0x1CC, 	//	W   A(E)    Vertical line for VBLANK start
		VBSTOP  = 0x1CE,  	//	W   A(E)    Vertical line for VBLANK stop

		//1D0              Reserved
		//1D2              Reserved
		//1D4              Reserved
		//1D6              Reserved
		//1D8              Reserved
		//1DA              Reserved

		BEAMCON0= 0x1DC,  	//	W   A(E)    Beam counter control register (SHRES, PAL)
		HSSTRT  = 0x1DE,  	//	W   A(E)    Horizontal sync start(VARHSY)
		VSSTRT  = 0x1E0,  	//	W   A(E)    Vertical sync start(VARVSY)
		HCENTER = 0x1E2,  	//	W   A(E)    Horizontal position for Vsync on interlace
		DIWHIGH = 0x1E4,  	//	W   AD(E)   Display window - upper bits for start, stop

		// Combined registers - these are pseudo 32-bit registers made by combining H and L registers from the list above.
		// The address is the same as the H component register.
		DSKPT	= 0x020,
		BLTCPT	= 0x048,
		BLTBPT	= 0x04C,
		BLTAPT	= 0x050,
		BLTDPT	= 0x054,

		COP1LC = 0x080,
		COP2LC = 0x084,

		AUD0LC = 0x0A0,
		AUD1LC = 0x0B0,
		AUD2LC = 0x0C0,
		AUD3LC = 0x0D0,

		BPL1PT = 0x0E0,
		BPL2PT = 0x0E4,
		BPL3PT = 0x0E8,
		BPL4PT = 0x0EC,
		BPL5PT = 0x0F0,
		BPL6PT = 0x0F4,

		SPR0PT = 0x120,
		SPR1PT = 0x124,
		SPR2PT = 0x128,
		SPR3PT = 0x12C,
		SPR4PT = 0x130,
		SPR5PT = 0x134,
		SPR6PT = 0x138,
		SPR7PT = 0x13C,
	};

}
