cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(gl3w CONFIG REQUIRED)

add_executable (AmigaEmulator
	"main.cpp"
	"app.h" "app.cpp"
	"GlfwPlatform/RunGlfw.h" "GlfwPlatform/RunGlfw.cpp"
	"GlfwPlatform/shader.h" "GlfwPlatform/shader.cpp"
	"util/file.h" "util/file.cpp"
	"util/endian.h" "util/strings.h"
	"debugger.cpp" "debugger.h"
	"custom_chips_debugger.cpp" "custom_chips_debugger.h"
	"memory_editor.cpp" "memory_editor.h"
	"amiga/amiga.cpp" "amiga/amiga.h"
	"amiga/disassembler.cpp" "amiga/disassembler.h"
	"amiga/68000.h" "amiga/68000.cpp"
	"amiga/registers.h" "Amiga/registers.cpp"
	"amiga/screen_buffer.h"
	"util/imgui_extras.h" "util/imgui_extras.cpp")

target_include_directories(AmigaEmulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(AmigaEmulator PRIVATE glfw imgui::imgui unofficial::gl3w::gl3w)