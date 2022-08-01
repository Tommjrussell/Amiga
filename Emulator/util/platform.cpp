#include "platform.h"

#ifdef _MSC_VER
#include <shlobj_core.h>
#endif

std::filesystem::path util::GetLocalDataDirectory()
{

#ifdef _MSC_VER

	std::filesystem::path path;
	PWSTR result;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &result);
	if (hr == S_OK)
	{
		path.assign(result);
	}
	::CoTaskMemFree(result);

	return path;

#else
#	error "Implement util::GetLocalDataDirectory"
#endif
}

std::filesystem::path util::GetAppDataDirectory()
{
#ifdef _MSC_VER

	std::filesystem::path path;
	PWSTR result;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &result);
	if (hr == S_OK)
	{
		path.assign(result);
	}
	::CoTaskMemFree(result);

	return path;

#else
#	error "Implement util::GetAppDataDirectory"
#endif
}