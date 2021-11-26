#include "App.h"

#include "debugger.h"

#include <imgui.h>

guru::AmigaApp::AmigaApp()
	: m_debugger(new Debugger)
{
}

guru::AmigaApp::~AmigaApp()
{
}

bool guru::AmigaApp::Update()
{
	return !m_isQuitting;
}

void guru::AmigaApp::Render()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Main"))
		{
			if (ImGui::MenuItem("Debugger", ""))
			{
				m_debuggerOpen = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Quit", "ALT+F4"))
			{
				m_isQuitting = true;
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Options"))
		{
			ImGui::MenuItem("CRT Emulation", "", &m_useCrtEmulation);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (m_debuggerOpen)
	{
		m_debuggerOpen = m_debugger->Draw();
	}

};