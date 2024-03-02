
#include "App.h"
#include "glfwplatform/RunGlfw.h"

#include "3rd Party/cxxopts.hpp"

int main(int argc, char** argv)
{
	std::filesystem::path programPath;
	std::filesystem::path configurationPath;

	std::string programDirString = ".";
	std::string configDirString;
	std::string romFile;
	std::string df0;
	std::string symbolsFile;

	cxxopts::Options options("guru", "Guru Amiga Emulator");
	options.add_options()
		("p,programDir", "Path to program directory", cxxopts::value<std::string>()->default_value(""))
		("c,configDir", "Path to use as local config directory", cxxopts::value<std::string>()->default_value(""))
		("df0", "disk image file for drive DF0", cxxopts::value<std::string>()->default_value(""))
		("syms", "symbols file for disassembler", cxxopts::value<std::string>()->default_value(""));

	try
	{
		auto result = options.parse(argc, argv);
		programDirString = result["programDir"].as<std::string>();
		configDirString = result["configDir"].as<std::string>();
		df0 = result["df0"].as<std::string>();
		symbolsFile = result["syms"].as<std::string>();
	}
	catch (cxxopts::OptionParseException& e)
	{
		printf("Error : %s\n\n", e.what());
		printf("%s\n", options.help().c_str());
		return -1;
	}

	if (!programDirString.empty())
	{
		programPath = programDirString;
	}
	else
	{
		programPath = std::filesystem::current_path();
	}

	if (!configDirString.empty())
	{
		configurationPath = configDirString;
	}
	else
	{
		configurationPath = guru::AmigaApp::GetOrCreateLocalAppDir();
	}

	guru::AmigaApp app(programPath, configurationPath);
	if (!df0.empty())
	{
		app.SetDiskImage(0, df0);
	}

	if (!symbolsFile.empty())
	{
		app.SetSymbolsFile(symbolsFile);
	}

	return guru::Run(1024, 768, app) ? 0 : 1;
}