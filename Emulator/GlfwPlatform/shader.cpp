#include "shader.h"

#include "util/file.h"

#include <GL/gl3w.h>

#include <string>
#include <fstream>
#include <vector>
#include <iostream>

namespace
{
	bool Compile(const std::string& filename, const std::string& code, GLuint id)
	{
		GLint result = GL_FALSE;
		int infoLogLength;

		std::cout << "Compiling shader : " << filename << " ... ";
		char const* srcPtr = code.c_str();
		glShaderSource(id, 1, &srcPtr, NULL);
		glCompileShader(id);

		glGetShaderiv(id, GL_COMPILE_STATUS, &result);
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);

		std::cout << ((result == GL_TRUE) ? "OK\n" : "FAIL\n");

		if (infoLogLength > 0)
		{
			std::vector<char> err(size_t(infoLogLength) + 1);
			glGetShaderInfoLog(id, infoLogLength, NULL, &err[0]);
			std::cout << err.data() << "\n";
		}

		return result == GL_TRUE;
	}
}

GLuint  platform_glfw::LoadShaders(const std::string& vertexShaderFile, const std::string& fragmentShaderFile)
{
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	std::string vertexShaderCode;
	if (!util::LoadUtf8File(vertexShaderFile, vertexShaderCode))
	{
		std::cerr << "Error : Cannot read vertex shader file '" << vertexShaderFile << "'\n";
		return 0;
	}

	std::string fragmentShaderCode;
	if (!util::LoadUtf8File(fragmentShaderFile, fragmentShaderCode))
	{
		std::cerr << "Error : Cannot read fragment shader file '" << fragmentShaderFile << "'\n";
		return 0;
	}

	if (!Compile(vertexShaderFile, vertexShaderCode, vertexShaderID))
	{
		return 0;
	}

	if (!Compile(fragmentShaderFile, fragmentShaderCode, fragmentShaderID))
	{
		glDeleteShader(vertexShaderID);
		return 0;
	}

	GLint result = GL_FALSE;
	int infoLogLength;

	std::cout << "Linking program ... ";
	GLuint programId = glCreateProgram();
	glAttachShader(programId, vertexShaderID);
	glAttachShader(programId, fragmentShaderID);
	glLinkProgram(programId);

	glGetProgramiv(programId, GL_LINK_STATUS, &result);


	std::cout << ((result == GL_TRUE) ? "OK\n" : "FAIL\n");

	glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0)
	{
		std::vector<char> err(size_t(infoLogLength) + 1);
		glGetProgramInfoLog(programId, infoLogLength, NULL, &err[0]);
		std::cout << err.data() << "\n";
	}

	glDetachShader(programId, vertexShaderID);
	glDetachShader(programId, fragmentShaderID);

	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	if (result == GL_FALSE)
	{
		glDeleteProgram(programId);
		programId = 0;
	}

	return programId;
}
