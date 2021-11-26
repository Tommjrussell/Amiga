
#include "App.h"

#include "GlfwPlatform/shader.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <array>

namespace
{

	void error_callback(int error, const char* description)
	{
		std::cerr << "Error " << error << ": " << description << "\n";
	}

	const GLfloat g_vertex_buffer_data[] =
	{
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		1.0f,  1.0f, 0.0f,
	};

}

namespace guru
{
	const int kScreenWidth = 672;
	const int kScreenHeight = 272;

	const uint32_t kScreenTextureWidth = 1024;
	const uint32_t kScreenTextureHeight = 512;

	using ColourRef = uint32_t;
	using TextureData = std::array<ColourRef, kScreenTextureWidth* kScreenTextureHeight>;
	using ScreenBuffer = std::array<ColourRef, kScreenWidth* kScreenHeight>;

	class Renderer
	{
	public:
		~Renderer();

		bool Init(const std::string& resDir, int width, int height);
		void SetScreenImage(const ScreenBuffer* screen);
		void DrawScreen(int screenWidth, int screenHeight, bool useCrtEmulation, bool evenFrame, int magAmount);

		GLFWwindow* GetWindow() { return m_window; }

		void NewFrame();
		void Render(bool useCrtEmulation);

	private:
		GLFWwindow* m_window;
		ImGuiContext* m_imguiContext = nullptr;

		GLuint m_programNoEffectID = 0;
		GLuint m_programCrtID = 0;

		GLuint m_vertexArray = 0;
		GLuint m_texture = 0;
		GLuint m_vertexBuffer = 0;
		GLuint m_uvBuffer = 0;
		int m_displayWidth = 0;
		int m_displayHeight = 0;
		float m_textureUVdata[8] = {};
		std::unique_ptr<TextureData> m_textureData;
	};

	Renderer::~Renderer()
	{
		glDeleteProgram(m_programNoEffectID);
		glDeleteProgram(m_programCrtID);

		glDeleteBuffers(1, &m_uvBuffer);
		glDeleteTextures(1, &m_texture);
		glDeleteBuffers(1, &m_vertexBuffer);

		if (m_imguiContext)
		{
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext(m_imguiContext);
		}

		glfwTerminate();
	}

	bool Renderer::Init(const std::string& resourceDir, int width, int height)
	{
		using namespace platform_glfw;

		glfwSetErrorCallback(error_callback);
		if (!glfwInit())
		{
			return false;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

		m_window = glfwCreateWindow(width, height, "Guru Amiga Emulator", NULL, NULL);
		if (!m_window)
		{
			glfwTerminate();
			return false;
		}

		glfwMakeContextCurrent(m_window);
		glfwSwapInterval(1); // Enable vsync
		gl3wInit();

		glGenBuffers(1, &m_vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

		m_programNoEffectID = LoadShaders(resourceDir + "\\shaders\\normal.vert", resourceDir + "\\shaders\\normal.frag");
		if (m_programNoEffectID == 0)
			return false;

		m_programCrtID = LoadShaders(resourceDir + "\\shaders\\crt.vert", resourceDir + "\\shaders\\crt.frag");
		if (m_programCrtID == 0)
			return false;

		glGenVertexArrays(1, &m_vertexArray);
		glBindVertexArray(m_vertexArray);

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		m_textureUVdata[2] = m_textureUVdata[6] = float(kScreenWidth);
		m_textureUVdata[1] = m_textureUVdata[3] = float(kScreenHeight);

		glGenBuffers(1, &m_uvBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(m_textureUVdata), m_textureUVdata, GL_STATIC_DRAW);

		m_textureData = std::make_unique<TextureData>();

		glClearColor(0.00f, 0.00f, 0.00f, 1.00f);

		const char* glsl_version = "#version 130";
		IMGUI_CHECKVERSION();
		m_imguiContext = ImGui::CreateContext();
		ImGui_ImplGlfw_InitForOpenGL(m_window, true);
		ImGui_ImplOpenGL3_Init(glsl_version);

		return true;
	}

	void Renderer::SetScreenImage(const ScreenBuffer* screen)
	{
		glBindTexture(GL_TEXTURE_2D, m_texture);

		auto destPtr = m_textureData->data();
		auto srcPtr = screen->data();

		for (int y = 0; y < kScreenHeight; y++)
		{
			::memcpy(destPtr, srcPtr, kScreenWidth * sizeof ColourRef);
			destPtr += kScreenTextureWidth;
			srcPtr += kScreenWidth;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, kScreenTextureWidth, kScreenTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_textureData->data());
	}

	void Renderer::DrawScreen(int screenWidth, int screenHeight, bool useCrtEmulation, bool evenFrame, int magAmount)
	{
		GLuint program = useCrtEmulation
			? m_programCrtID
			: m_programNoEffectID;

		glUseProgram(program);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_texture);

		{
			const auto myTextureSampler = glGetUniformLocation(program, "myTextureSampler");
			glUniform1i(myTextureSampler, 0);

			const auto screenSize = glGetUniformLocation(program, "screenSize");
			if (screenSize >= 0)
			{
				glUniform2f(screenSize, float(screenWidth), float(screenHeight));
			}

			const auto winSize = glGetUniformLocation(program, "winSize");
			if (winSize >= 0)
			{
				glUniform2f(winSize, float(m_displayWidth), float(m_displayHeight));
			}

			const auto magnification = glGetUniformLocation(program, "magnification");
			if (magnification >= 0)
			{
				glUniform1f(magnification, float(magAmount));
			}

			const auto scanlineOffset = glGetUniformLocation(program, "scanlineOffset");
			if (scanlineOffset >= 0)
			{
				glUniform1f(scanlineOffset, float(evenFrame ? 0.0f : 0.5f));
			}

		}

		// 1st attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}

	void Renderer::NewFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glfwGetFramebufferSize(m_window, &m_displayWidth, &m_displayHeight);
	}

	void Renderer::Render(bool useCrtEmulation)
	{
		glViewport(0, 0, m_displayWidth, m_displayHeight);
		glClear(GL_COLOR_BUFFER_BIT);

		DrawScreen(kScreenWidth, kScreenHeight, useCrtEmulation, false, 1);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(m_window);
	}

}

namespace guru
{

	bool Run(int width, int height, const std::string& resourceDir, AmigaApp& app)
	{
		Renderer renderer;
		if (!renderer.Init(resourceDir, width, height))
			return false;

		bool appRunning = true;

		auto screen = std::make_unique<ScreenBuffer>();
		screen->fill(ColourRef(0xffffffff));

		while (appRunning && glfwWindowShouldClose(renderer.GetWindow()) == 0)
		{
			renderer.SetScreenImage(screen.get());

			appRunning = app.Update();

			glfwPollEvents();

			renderer.NewFrame();
			app.Render();
			renderer.Render(app.UseCrtEmulation());
		}

		return true;
	}
}
