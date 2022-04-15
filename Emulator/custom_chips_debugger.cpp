#include "custom_chips_debugger.h"

#include "App.h"

#include "amiga/amiga.h"
#include "amiga/registers.h"

#include "util/imgui_extras.h"

#include <imgui.h>

guru::CCDebugger::CCDebugger(guru::AmigaApp* app, am::Amiga* amiga)
	: m_app(app)
	, m_amiga(amiga)
{
}

guru::CCDebugger::~CCDebugger()
{
}

bool guru::CCDebugger::Draw()
{
	bool open = true;
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	bool expanded = ImGui::Begin("Custom Chips Debugger", &open);

	if (!(open && expanded))
	{
		ImGui::End();
		return open;
	}

	{
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
		ImGui::BeginChild("Child_Side", ImVec2(240, 0), true, 0);

		if (ImGui::CollapsingHeader("Beam Info", ImGuiTreeNodeFlags_DefaultOpen))
		{
			DrawBeamInfo();
		}

		if (ImGui::CollapsingHeader("Colour Palette"))
		{
			DrawColourPalette();
		}

		if (ImGui::CollapsingHeader("Bitplane Control"))
		{
			DrawBitplaneControl();
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::SameLine();

	{
		ImGui::BeginChild("##CopperChild", ImVec2(0, 0), false, 0);
		DrawCopper();
		ImGui::EndChild();
	}

	ImGui::End();
	return open;
}

void guru::CCDebugger::DrawBeamInfo()
{
	ImGuiInputTextFlags beamValueflags = ImGuiInputTextFlags_ReadOnly;
	if (m_beamValuesInHex)
	{
		beamValueflags |= ImGuiInputTextFlags_CharsHexadecimal;
	}

	const auto vposr = m_amiga->PeekRegister(am::Register::VPOSR);
	const auto vhposr = m_amiga->PeekRegister(am::Register::VHPOSR);

	int hpos = vhposr & 0xff;
	int vpos = (vhposr >> 8) | ((vposr & 1) << 8);

	ImGui::Text("Scan rate mode : %s", m_amiga->isNTSC() ? "NTSC" : "PAL");

	ImGui::PushItemWidth(64);
	ImGui::InputInt("Beam Pos H", &hpos, 0, 0, beamValueflags);
	ImGui::InputInt("Beam Pos V", &vpos, 0, 0, beamValueflags);
	ImGui::PopItemWidth();

	ImGui::Checkbox("In hex", &m_beamValuesInHex);
}

void guru::CCDebugger::DrawColourPalette()
{
	auto GetImguiColour = [this](int index)
	{
		const auto colourReg = am::Register(int(am::Register::COLOR00) + (index * 2));
		const auto colour = m_amiga->PeekRegister(colourReg);
		const auto r = ((colour >> 8 ) & 0xf) / 15.0f;
		const auto g = ((colour >> 4 ) & 0xf) / 15.0f;
		const auto b = ((colour >> 0 ) & 0xf) / 15.0f;
		return ImVec4(r,g,b,1.0f);
	};

	ImGui::SetColorEditOptions(ImGuiColorEditFlags_NoPicker|ImGuiColorEditFlags_NoOptions|ImGuiColorEditFlags_Uint8|ImGuiColorEditFlags_NoAlpha);
	for (int i = 0; i < 32; i++)
	{
		char colourName[16];
		sprintf_s(colourName, "COLOR%02d", i);
		ImGui::ColorButton(colourName, GetImguiColour(i));
		if ((i % 8) != 7) ImGui::SameLine();
	}
}

void guru::CCDebugger::DrawBitplaneControl()
{
	const auto bplcon0 = m_amiga->PeekRegister(am::Register::BPLCON0);
	const auto bplcon1 = m_amiga->PeekRegister(am::Register::BPLCON1);
	const auto bplcon2 = m_amiga->PeekRegister(am::Register::BPLCON2);

	const auto numBitPlanes = ((bplcon0 & 0x7000) >> 12);

	ImGui::Text("BitPlanes Enabled : %d", numBitPlanes);

	ImGui::Separator();

	ImGui::Columns(2, 0, false);

	auto hires = (bplcon0 & 0b1000'0000'0000'0000) != 0;
	auto ham   = (bplcon0 & 0b0000'1000'0000'0000) != 0;
	auto dblpf = (bplcon0 & 0b0000'0100'0000'0000) != 0;
	auto color = (bplcon0 & 0b0000'0010'0000'0000) != 0;
	ImGui::Checkbox("HiRes", &hires);
	ImGui::Checkbox("Ham",   &ham);
	ImGui::Checkbox("DblPf", &dblpf);
	ImGui::Checkbox("Color", &color);

	ImGui::NextColumn();

	auto gaud = (bplcon0 & 0b0000'0001'0000'0000) != 0;
	auto lpen = (bplcon0 & 0b0000'0000'0000'1000) != 0;
	auto lace = (bplcon0 & 0b0000'0000'0000'0100) != 0;
	auto ersy = (bplcon0 & 0b0000'0000'0000'0010) != 0;
	ImGui::Checkbox("GAud", &gaud);
	ImGui::Checkbox("LPen", &lpen);
	ImGui::Checkbox("Lace", &lace);
	ImGui::Checkbox("ERsy", &ersy);

	ImGui::Columns(1);
	ImGui::Separator();
	ImGui::Columns(2, 0, false);

	int pf1h = (bplcon1 & 0x000f);
	int pf2h = (bplcon1 & 0x00f0) >> 4;
	ImGui::PushItemWidth(64);
	ImGui::InputInt("PF1H", &pf1h, 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::InputInt("PF2H", &pf2h, 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::PopItemWidth();

	ImGui::NextColumn();

	int pf1p = (bplcon2 & 0x0007);
	int pf2p = (bplcon2 & 0x0038) >> 3;
	ImGui::PushItemWidth(64);
	ImGui::InputInt("PF1P", &pf1p, 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::InputInt("PF2P", &pf2p, 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::PopItemWidth();

	ImGui::NextColumn();

	auto pf2pri = (bplcon2 & 0b0000'0000'0100'0000) != 0;
	ImGui::Checkbox("PF2PRI", &pf2pri);

	ImGui::Columns(1);
	ImGui::Separator();
	ImGui::Columns(2, 0, false);

	const auto diwstrt = m_amiga->PeekRegister(am::Register::DIWSTRT);
	const auto diwstop = m_amiga->PeekRegister(am::Register::DIWSTOP);

	auto startX = diwstrt & 0x00ff;
	auto stopX = 0x0100 | (diwstop & 0x00ff);
	auto startY = diwstrt >> 8;
	auto stopY = ((~diwstop >> 7) & 0x0100) | (diwstop >> 8);

	ImGui::PushItemWidth(64);
	ImGui::InputInt("X Start", &startX, 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::InputInt("X Stop", &stopX, 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::PopItemWidth();

	ImGui::NextColumn();

	ImGui::PushItemWidth(64);
	ImGui::InputInt("Y Start", &startY, 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::InputInt("Y Stop", &stopY, 0, 0, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::PopItemWidth();

	ImGui::Columns(1);
	ImGui::Separator();
	ImGui::Columns(2, 0, false);

	const int ddfstrt = m_amiga->PeekRegister(am::Register::DDFSTRT);
	const int ddfstop = m_amiga->PeekRegister(am::Register::DDFSTOP);
	const int bpl1mod = m_amiga->PeekRegister(am::Register::BPL1MOD);
	const int bpl2mod = m_amiga->PeekRegister(am::Register::BPL2MOD);

	ImGui::Text("DDFSTRT : %.2X", ddfstrt);
	ImGui::Text("BPL1MOD : %.2X", bpl1mod);
	ImGui::NextColumn();
	ImGui::Text("DDFSTOP : %.2X", ddfstop);
	ImGui::Text("BPL2MOD : %.2X", bpl2mod);

	ImGui::Columns(1);
}

void guru::CCDebugger::DrawCopper()
{
	using util::ActiveButton;

	auto& copper = m_amiga->GetCopper();
	const bool running = m_app->IsRunning();

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("Copper_Controls", ImVec2(0, 104), true, 0);



	int cop1lc = uint32_t(m_amiga->PeekRegister(am::Register::COP1LCH)) << 16
		| m_amiga->PeekRegister(am::Register::COP1LCL);

	int cop2lc = uint32_t(m_amiga->PeekRegister(am::Register::COP2LCH)) << 16
		| m_amiga->PeekRegister(am::Register::COP2LCL);

	ImGui::Columns(2, 0, false);

	{
		const auto dmaconr = m_amiga->PeekRegister(am::Register::DMACONR);
		const uint16_t enabled = uint16_t(am::Dma::COPEN) | uint16_t(am::Dma::DMAEN);
		bool dma = (dmaconr & enabled) == enabled;
		ImGui::Checkbox("DMA Enabled", &dma);
	}

	{
		ImGui::PushItemWidth(64);
		int pc = copper.pc;
		ImGui::InputInt("PC", &pc, 0, 0, ImGuiInputTextFlags_ReadOnly|ImGuiInputTextFlags_CharsHexadecimal);
		ImGui::PopItemWidth();
	}

	ImGui::NextColumn();

	ImGui::PushItemWidth(64);
	ImGui::InputInt("COP1LC", &cop1lc, 0, 0, ImGuiInputTextFlags_ReadOnly|ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::InputInt("COP2LC", &cop2lc, 0, 0, ImGuiInputTextFlags_ReadOnly|ImGuiInputTextFlags_CharsHexadecimal);
	ImGui::PopItemWidth();

	ImGui::Columns(1);

	if (ActiveButton("Start", !running))
	{
		m_amiga->ClearBreakpoint();
		m_app->SetRunning(true);
	}
	ImGui::SameLine();
	if (ActiveButton("Stop", running))
	{
		m_app->SetRunning(false);
		m_amiga->ClearBreakpoint();
	}
	ImGui::SameLine();
	if (ActiveButton("Step", !running))
	{
		m_amiga->ClearBreakpoint();
		m_amiga->EnableBreakOnCopper();
		m_app->SetRunning(true);
	}

	auto DoBreakOnLine = [this](int inc)
	{
		auto frameLength = m_amiga->GetFrameLength();
		auto vpos = m_amiga->GetVPos();
		vpos += inc;
		if (vpos > frameLength)
		{
			vpos -= frameLength;
		}
		m_amiga->EnableBreakOnLine(vpos);
	};

	if (ActiveButton("+1 Line", !running))
	{
		m_amiga->ClearBreakpoint();
		DoBreakOnLine(1);
		m_app->SetRunning(true);
	}
	ImGui::SameLine();
	if (ActiveButton("+10 Lines", !running))
	{
		m_amiga->ClearBreakpoint();
		DoBreakOnLine(10);
		m_app->SetRunning(true);
	}
	ImGui::SameLine();
	if (ActiveButton("+50 Lines", !running))
	{
		m_amiga->ClearBreakpoint();
		DoBreakOnLine(50);
		m_app->SetRunning(true);
	}
	ImGui::SameLine();
	if (ActiveButton("+1 Frame", !running))
	{
		m_amiga->ClearBreakpoint();
		m_amiga->EnableBreakOnLine(0);
		m_app->SetRunning(true);
	}

	ImGui::EndChild();

	ImGui::TextUnformatted("Disassemble from :");

	ImGui::PushItemWidth(128);
	ImGui::Combo("##SELECT", &m_disassembleMode, "PC\0COP1LC\0COP2LC\0Address\0");
	ImGui::PopItemWidth();
	if (m_disassembleMode == 3)
	{
		ImGui::SameLine();
		ImGui::PushItemWidth(64);
		ImGui::InputInt("##ADDR", (int*)&m_disassembleAddr, 0, 0, ImGuiInputTextFlags_CharsHexadecimal);
		ImGui::PopItemWidth();
	}

	uint32_t startAddr = 0;

	switch(m_disassembleMode)
	{
	case 0: // PC
		startAddr = copper.pc;
		break;
	case 1: // COP1LC
		startAddr = cop1lc;
		break;
	case 2: // COP2LC
		startAddr = cop2lc;
		break;
	case 3:
		startAddr = m_disassembleAddr;
		break;
	}

	const uint32_t endAddr = startAddr + 0x80;

	ImGui::BeginChild("Copper_Disassembly", ImVec2(0, 0), true, 0);

	for (uint32_t pc = startAddr; pc < endAddr; pc += 4)
	{
		auto instr = DisassembleCopperInstruction(pc);
		ImGui::Text("%08X %c %s", pc, (pc == copper.pc) ? '>' : ':', instr.c_str());
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
}

std::string guru::CCDebugger::DisassembleCopperInstruction(uint32_t addr) const
{
	const auto ir1 = m_amiga->PeekWord(addr);
	const auto ir2 = m_amiga->PeekWord(addr+2);

	char buffer[40];
	auto charleft = sizeof(buffer);

	if ((ir1 & 1) == 0)
	{
		const char* regName = "???";
		const int reg = uint16_t(ir1 & 0x1ff);
		if (reg < int(am::Register::End))
		{
			regName = am::kRegisterNames[reg/2];
		}
		const auto count = sprintf_s(buffer, charleft, "MOVE %03x(%s), %04x", reg, regName, ir2);
	}
	else
	{
		const auto verticalBeamPos = ir1 >> 8;
		const auto horizontalBeamPos = ir1 & 0xff;
		const auto verticalPositionCompare = (ir2 >> 8) & 0x7f;
		const auto horizontalPositionCompare = ir2 & 0xfe;

		const bool isWait = (ir2 & 1) == 0;

		const auto count = sprintf_s(buffer, charleft, "%s VP=%02x HP=%02x VE=%02x HE=%02x",
			(isWait ? "WAIT" : "SKIP"),
			verticalBeamPos, horizontalBeamPos,
			verticalPositionCompare, horizontalPositionCompare);
	}

	return buffer;
}
