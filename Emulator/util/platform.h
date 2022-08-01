#pragma once

#include <filesystem>

// Platform Specific Utility code goes here.

#ifdef _MSC_VER
#	define DEBUGGER_BREAK() __debugbreak()
#else
	// TODO: break to debugger for other platforms?
#	define DEBUGGER_BREAK()
#endif

namespace util
{
	std::filesystem::path GetLocalDataDirectory();
	std::filesystem::path GetAppDataDirectory();
}