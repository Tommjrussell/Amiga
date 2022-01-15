#include "custom_chips_debugger.h"

#include "App.h"

#include "amiga/amiga.h"

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

	// TODO

	ImGui::End();
	return open;
}

void guru::CCDebugger::DrawColourPalette()
{
	auto GetImguiColour = [this](int index)
	{
		const auto colour = m_amiga->GetPaletteColour(index);
		const auto r = ((colour >> 0 ) & 0xff) / 255.0f;
		const auto g = ((colour >> 8 ) & 0xff) / 255.0f;
		const auto b = ((colour >> 16) & 0xff) / 255.0f;
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