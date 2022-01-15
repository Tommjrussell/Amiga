#include "custom_chips_debugger.h"

#include "App.h"

#include "amiga/amiga.h"
#include "amiga/registers.h"

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

	if (ImGui::CollapsingHeader("Colour Palette"))
	{
		DrawColourPalette();
	}

	if (ImGui::CollapsingHeader("Bitplane Control"))
	{
		DrawBitplaneControl();
	}

	// TODO

	ImGui::End();
	return open;
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
}
