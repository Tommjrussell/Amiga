#include "disk_activity.h"

#include "amiga/amiga.h"

#include <imgui.h>

namespace
{
	constexpr int overlayWidth = 120;
	constexpr int overlayHeight = 80;
	constexpr int overlayPadding = 20;

	constexpr int idleTimeoutLength = 200;
}

guru::DiskActivity::DiskActivity(am::Amiga* amiga)
	: m_amiga(amiga)
{
	idleTimeout.fill(0);
}

void guru::DiskActivity::Draw(int displayWidth, int displayHeight)
{
	static const auto activeCol = IM_COL32(0, 255, 0, 255);
	static const auto inactiveCol = IM_COL32(0, 40, 0, 255);

	ImGui::SetNextWindowPos(ImVec2(float(displayWidth - (overlayWidth + overlayPadding)), float(displayHeight - (overlayHeight + overlayPadding))));
	ImGui::SetNextWindowSize(ImVec2(float(overlayWidth), float(overlayHeight)));

	ImGui::Begin("Disk Overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	for (int i = 0; i < 4; i++)
	{
		auto& drive = m_amiga->GetFloppyDrive(i);

		if (drive.motorOn || idleTimeout[i] > 0)
		{
			ImGui::Text("DF%d:", i);
			ImGui::SameLine();
			ImVec2 p = ImGui::GetCursorScreenPos();
			draw_list->AddRectFilled(p, ImVec2(p.x + 50, p.y + ImGui::GetTextLineHeight()), drive.motorOn ? activeCol : inactiveCol);
			ImGui::NewLine();

			if (!drive.motorOn)
			{
				idleTimeout[i]--;
			}
			else
			{
				idleTimeout[i] = idleTimeoutLength;
			}
		}
		else
		{
			ImGui::Text("");
		}

	}

	ImGui::End();
}