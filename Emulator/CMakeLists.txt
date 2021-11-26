cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(gl3w CONFIG REQUIRED)

add_executable (AmigaEmulator
	"disassembler.cpp" "disassembler.h"
	"main.cpp"
	"App.h" "App.cpp" "GlfwPlatform/RunGlfw.h" "GlfwPlatform/RunGlfw.cpp" "GlfwPlatform/shader.h" "GlfwPlatform/shader.cpp"  "util/file.h" "util/file.cpp" "debugger.cpp" "debugger.h")

target_include_directories(AmigaEmulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(AmigaEmulator PRIVATE glfw imgui::imgui unofficial::gl3w::gl3w)