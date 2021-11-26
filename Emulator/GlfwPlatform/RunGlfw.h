#pragma once

#include <string>

namespace guru
{
	class AmigaApp;

	bool Run(int width, int height, const std::string& resourceDir, AmigaApp& app);
}