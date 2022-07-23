
#include "App.h"
#include "glfwplatform/RunGlfw.h"

#include "3rd Party/cxxopts.hpp"

int main(int argc, char** argv)
{
	std::string resDir = ".";
	std::string romFile;
	std::string df0;
	std::string symbolsFile;

	cxxopts::Options options("guru", "Guru Amiga Emulator");
	options.add_options()
		("r,res", "Path to resource directory", cxxopts::value<std::string>()->default_value(""))
		("m,rom", "Path to rom file", cxxopts::value<std::string>()->default_value(""))
		("df0", "disk image file for drive DF0", cxxopts::value<std::string>()->default_value(""))
		("syms", "symbols file for disassembler", cxxopts::value<std::string>()->default_value(""));

	try
	{
		auto result = options.parse(argc, argv);
		resDir = result["res"].as<std::string>();
		romFile = result["rom"].as<std::string>();
		df0 = result["df0"].as<std::string>();
		symbolsFile = result["syms"].as<std::string>();
	}
	catch (cxxopts::OptionParseException& e)
	{
		printf("Error : %s\n\n", e.what());
		printf("%s\n", options.help().c_str());
		return -1;
	}

	guru::AmigaApp app(resDir, romFile);

	if (!df0.empty())
	{
		app.SetDiskImage(0, df0);
	}

	if (!symbolsFile.empty())
	{
		app.SetSymbolsFile(symbolsFile);
	}

	return guru::Run(1024, 768, resDir, app) ? 0 : 1;
}