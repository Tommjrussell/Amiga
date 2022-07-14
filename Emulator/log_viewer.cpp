#include "log_viewer.h"

#include "Amiga/amiga.h"

#include <imgui.h>

guru::LogViewer::LogViewer(am::Amiga* amiga)
	: m_amiga(amiga)
{
}

bool guru::LogViewer::Draw()
{
	bool open = true;
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	bool expanded = ImGui::Begin("Log Viewer", &open);

	if (!(open && expanded))
	{
		ImGui::End();
		return open;
	}

	ImGui::BeginChild("##LogOptions", ImVec2(180, 0), true, 0);

	if (ImGui::Button("Clear Log"))
	{
		m_amiga->GetLog()->Clear();
	}

	int options = m_amiga->GetLogOptions();

	auto LogOptionCheckBox = [&](const char* label, int flag)
	{
		bool setting = (options & flag) != 0;
		if (ImGui::Checkbox(label, &setting))
		{
			if (setting)
			{
				options |= flag;
			}
			else
			{
				options &= ~flag;
			}
			m_amiga->SetLogOptions(options);
		}
	};

	ImGui::Text("Log Events of type:");
	LogOptionCheckBox("Disk", am::LogOptions::Disk);
	LogOptionCheckBox("Blitter", am::LogOptions::Blitter);

	ImGui::EndChild();

	ImGui::SameLine();

	float height = ImGui::GetWindowHeight();
	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

	ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;

	if (ImGui::BeginTable("log entry table", 2, flags, ImVec2(0, 0)))
	{
		auto& messages = m_amiga->GetLog()->GetMessages();

		ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
		ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 96.0f);
		ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_None);
		ImGui::TableHeadersRow();

		ImGuiListClipper clipper;
		clipper.Begin(int(messages.size()));
		while (clipper.Step())
		{
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
			{
				auto& msg = messages[row];

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%012llu", msg.first);
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s", msg.second.c_str());
			}
		}

		ImGui::EndTable();
	}

	ImGui::End();
	return open;
}