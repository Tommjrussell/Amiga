#pragma once

#include <gl/gl3w.h>
#include <string>

namespace platform_glfw
{
	GLuint LoadShaders(const std::string& vertexShaderFile, const std::string& fragmentShaderFile);
}