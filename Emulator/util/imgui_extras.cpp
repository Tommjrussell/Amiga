#include "imgui_extras.h"

bool util::ActiveButton(const char* name, bool active, const ImVec2& size)
{
	if (!active)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 128, 128, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(128, 128, 128, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(128, 128, 128, 255));
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
	}

	auto ret = ImGui::Button(name, size);

	if (!active)
	{
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);
	}

	return active && ret;
};