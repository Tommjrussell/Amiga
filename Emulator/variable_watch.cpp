#include "variable_watch.h"

#include "amiga/symbols.h"
#include "amiga/amiga.h"

#include "util/strings.h"

#include <imgui.h>

guru::VariableWatch::VariableWatch(/*guru::AmigaApp* app, */am::Amiga* amiga, am::Symbols* symbols)
	: m_amiga(amiga)
	, m_symbols(symbols)
{
}

guru::VariableWatch::~VariableWatch()
{
}

bool guru::VariableWatch::Draw()
{
	bool open = true;
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	bool expanded = ImGui::Begin("Variable Watch", &open);

	if (!(open && expanded))
	{
		ImGui::End();
		return open;
	}

	{
		ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable
			| ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV
			| ImGuiTableFlags_ContextMenuInBody;

		if (ImGui::BeginTable("Variables", 4, flags))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Address");
			ImGui::TableSetupColumn("Size");
			ImGui::TableSetupColumn("Value");
			ImGui::TableHeadersRow();

			auto& variables = m_symbols->GetVariables();

			constexpr const char* sizes[] = { "byte", "word", "", "long" };

			std::string value;
			int value10 = 0;

			for (auto& var : variables)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(var.name.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(util::HexToString(var.addr).c_str());
				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(sizes[(var.size - 1) & 0b11]);
				ImGui::TableSetColumnIndex(3);

				value.clear();
				switch (var.size)
				{
				case 1:
				{
					uint8_t u8 = m_amiga->PeekByte(var.addr);
					value = util::HexToString(u8);
					value10 = int8_t(u8);
				}	break;
				case 2:
				{
					uint16_t u16 = m_amiga->PeekWord(var.addr);
					value = util::HexToString(u16);
					value10 = int16_t(u16);
				}	break;
				case 4:
				{
					uint32_t u32 = m_amiga->PeekWord(var.addr);
					u32 <<= 16;
					u32 |= m_amiga->PeekWord(var.addr + 2);
					value = util::HexToString(u32);
					value10 = int32_t(u32);
				}	break;
				default:
					value = "???";
					break;
				}
				ImGui::Text("%s (%d)", value.c_str(), value10);
			}

			ImGui::EndTable();
		}

	}

	ImGui::End();
	return open;
}