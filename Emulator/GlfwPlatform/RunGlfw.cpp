
#include "App.h"

#include "GlfwPlatform/shader.h"
#include "GlfwPlatform/audio_openal.h"

#include "amiga/screen_buffer.h"

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
	const char* const title = "Guru Amiga Emulator";


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
	const uint32_t kScreenTextureWidth = 1024;
	const uint32_t kScreenTextureHeight = 512;

	using TextureData = std::array<am::ColourRef, kScreenTextureWidth* kScreenTextureHeight>;

	class Renderer
	{
		friend void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		friend void GLFWMouseKeyCallback(GLFWwindow* window, int button, int action, int mods);
		friend void GLFWMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);

	public:
		explicit Renderer(AmigaApp* app);
		~Renderer();

		bool Init(const std::filesystem::path& resourceDir, int width, int height);
		void SetInputCallbacks(bool amigaHasfocus);
		void SetScreenImage(const am::ScreenBuffer* screen);
		void DrawScreen(int screenWidth, int screenHeight, bool evenFrame, int magAmount);
		void HandleInput();

		int Width() const
		{
			return m_displayWidth;
		}

		int Height() const
		{
			return m_displayHeight;
		}

		GLFWwindow* GetWindow()
		{
			return m_window;
		}

		void NewFrame();
		void Render();

	private:
		void ToggleFullScreen();

	private:
		GLFWwindow* m_window;
		ImGuiContext* m_imguiContext = nullptr;

		GLFWkeyfun m_imguiKeyCallback = nullptr;

		AmigaApp* m_app;

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

		GLFWmousebuttonfun m_savedMouseButtonCallback = nullptr;
		GLFWcursorposfun m_savedMouseMoveCallback = nullptr;

		double m_previousMouseX = 0;
		double m_previousMouseY = 0;

		int m_windowedPosX = 0;
		int m_windowedPosY = 0;
		int m_windowedSizeX = 0;
		int m_windowedSizeY = 0;

		bool m_fullScreen = false;
	};

	void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto renderer = (guru::Renderer*)glfwGetWindowUserPointer(window);
		if (!renderer->m_app->SetKey(key, action, mods))
		{
			renderer->m_imguiKeyCallback(window, key, scancode, action, mods);
		}
	}

	void GLFWMouseKeyCallback(GLFWwindow* window, int button, int action, int mods)
	{
		auto renderer = (guru::Renderer*)glfwGetWindowUserPointer(window);
		renderer->m_app->SetMouseButton(button, action, mods);
	}

	void GLFWMouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto renderer = (guru::Renderer*)glfwGetWindowUserPointer(window);
		renderer->m_app->SetMouseMove(xpos - renderer->m_previousMouseX, ypos - renderer->m_previousMouseY);

		renderer->m_previousMouseX = xpos;
		renderer->m_previousMouseY = ypos;
	}

	Renderer::Renderer(AmigaApp* app)
		: m_app(app)
	{
	}

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

	bool Renderer::Init(const std::filesystem::path& resourceDir, int width, int height)
	{
		using namespace platform_glfw;

		glfwSetErrorCallback(error_callback);
		if (!glfwInit())
		{
			return false;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

		m_window = glfwCreateWindow(width, height, title, NULL, NULL);
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

		m_programNoEffectID = LoadShaders(resourceDir / "shaders" / "normal.vert", resourceDir / "shaders" / "normal.frag");
		if (m_programNoEffectID == 0)
			return false;

		m_programCrtID = LoadShaders(resourceDir / "shaders" / "crt.vert", resourceDir / "shaders" / "crt.frag");
		if (m_programCrtID == 0)
			return false;

		glGenVertexArrays(1, &m_vertexArray);
		glBindVertexArray(m_vertexArray);

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		m_textureUVdata[2] = m_textureUVdata[6] = float(am::kScreenBufferWidth);
		m_textureUVdata[1] = m_textureUVdata[3] = float(am::kScreenBufferHeight);

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

	void Renderer::SetInputCallbacks(bool amigaHasfocus)
	{
		glfwSetWindowUserPointer(m_window, this);

		if (!m_imguiKeyCallback)
		{
			m_imguiKeyCallback = glfwSetKeyCallback(m_window, &GLFWKeyCallback);
		}

		if (amigaHasfocus)
		{
			std::string windowTitle = title;
			windowTitle += " (Shift + F11 to release mouse)";

			glfwSetWindowTitle(m_window, windowTitle.c_str());

			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			if (glfwRawMouseMotionSupported())
				glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

			m_savedMouseButtonCallback = glfwSetMouseButtonCallback(m_window, GLFWMouseKeyCallback);
			m_savedMouseMoveCallback = glfwSetCursorPosCallback(m_window, GLFWMouseMoveCallback);

			glfwGetCursorPos(m_window, &m_previousMouseX, &m_previousMouseY);
		}
		else
		{
			glfwSetWindowTitle(m_window, title);

			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			if (m_savedMouseButtonCallback)
			{
				glfwSetMouseButtonCallback(m_window, m_savedMouseButtonCallback);
			}
			if (m_savedMouseMoveCallback)
			{
				glfwSetCursorPosCallback(m_window, m_savedMouseMoveCallback);
			}
		}
	}

	void Renderer::HandleInput()
	{
		JoystickState state = {};

		int numHats;
		auto hats = glfwGetJoystickHats(GLFW_JOYSTICK_1, &numHats);

		for (int i = 0; i < numHats; i++)
		{
			if (hats[i] & 1)
				state.y = -1;

			if (hats[i] & 2)
				state.x = 1;

			if (hats[i] & 4)
				state.y = 1;

			if (hats[i] & 8)
				state.x = -1;
		}

		int numButtons;
		auto buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &numButtons);

		for (int i = 0; i < 12; i++)
		{
			if (i < numButtons && buttons[i] == GLFW_PRESS)
			{
				state.buttons |= 1 << i;
			}
		}

		m_app->SetJoystickState(state);

		glfwPollEvents();
	}

	void Renderer::SetScreenImage(const am::ScreenBuffer* screen)
	{
		glBindTexture(GL_TEXTURE_2D, m_texture);

		auto destPtr = m_textureData->data();
		auto srcPtr = screen->data();

		for (int y = 0; y < am::kScreenBufferHeight; y++)
		{
			::memcpy(destPtr, srcPtr, am::kScreenBufferWidth * sizeof am::ColourRef);
			destPtr += kScreenTextureWidth;
			srcPtr += am::kScreenBufferWidth;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, kScreenTextureWidth, kScreenTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_textureData->data());
	}

	void Renderer::DrawScreen(int screenWidth, int screenHeight, bool evenFrame, int magAmount)
	{
		auto& settings = m_app->GetFrontEndSettings();

		GLuint program = settings.useCrtEmulation
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

			if (settings.useCrtEmulation)
			{
				const auto warp = glGetUniformLocation(program, "warp");
				if (warp >= 0)
				{
					glUniform2f(warp, settings.crtWarpX, settings.crtWarpY);
				}

				const auto brightnessAdjust = glGetUniformLocation(program, "brightnessAdjust");
				if (brightnessAdjust >= 0)
				{
					glUniform1f(brightnessAdjust, settings.brightnessAdjust);
				}
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

	void Renderer::Render()
	{
		auto& settings = m_app->GetFrontEndSettings();

		if (settings.fullScreen != m_fullScreen)
		{
			ToggleFullScreen();
		}

		glViewport(0, 0, m_displayWidth, m_displayHeight);
		glClear(GL_COLOR_BUFFER_BIT);

		DrawScreen(am::kScreenBufferWidth, am::kScreenBufferHeight, false, 1);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(m_window);
	}

	void Renderer::ToggleFullScreen()
	{
		if (!m_fullScreen)
		{
			auto monitor = glfwGetPrimaryMonitor();
			auto mode = glfwGetVideoMode(monitor);

			glfwGetWindowPos(m_window, &m_windowedPosX, &m_windowedPosY);
			glfwGetWindowSize(m_window, &m_windowedSizeX, &m_windowedSizeY);

			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

			glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);

			glfwSwapInterval(1);
		}
		else
		{
			auto monitor = glfwGetPrimaryMonitor();
			auto mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(m_window, nullptr, m_windowedPosX, m_windowedPosY, m_windowedSizeX, m_windowedSizeY, mode->refreshRate);

			glfwSwapInterval(1);
		}

		m_fullScreen = !m_fullScreen;
	}

}

namespace guru
{

	bool Run(int width, int height, AmigaApp& app)
	{
		Renderer renderer(&app);

		auto& resDir = app.GetProgramDir();

		if (!renderer.Init(resDir, width, height))
			return false;

		app.Setup();

		OpenAlPlayer audioPlayer;
		audioPlayer.Setup();
		app.SetAudioPlayer(&audioPlayer);

		renderer.SetInputCallbacks(false);

		bool appRunning = true;
		bool amigaHasFocus = false;

		while (appRunning && glfwWindowShouldClose(renderer.GetWindow()) == 0)
		{
			if (amigaHasFocus != app.AmigaHasFocus())
			{
				amigaHasFocus = app.AmigaHasFocus();
				renderer.SetInputCallbacks(amigaHasFocus);
			}
			renderer.HandleInput();

			appRunning = app.Update();

			renderer.SetScreenImage(app.GetScreen());
			renderer.NewFrame();
			app.Render(renderer.Width(), renderer.Height());
			renderer.Render();
		}

		app.Shutdown();
		audioPlayer.Shutdown();

		return true;
	}
}
