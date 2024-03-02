#pragma once

#include <gl/gl3w.h>
#include <filesystem>

namespace platform_glfw
{
	GLuint LoadShaders(const std::filesystem::path& vertexShaderFile, const std::filesystem::path& fragmentShaderFile);
}