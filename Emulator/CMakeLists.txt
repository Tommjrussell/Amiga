﻿cmake_minimum_required (VERSION 3.8..3.28)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(gl3w CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(minizip CONFIG REQUIRED)

add_executable (AmigaEmulator
	"main.cpp"
	"app.h" "app.cpp"
	"GlfwPlatform/RunGlfw.h" "GlfwPlatform/RunGlfw.cpp"
	"GlfwPlatform/shader.h" "GlfwPlatform/shader.cpp"
	"GlfwPlatform/audio_openal.h" "GlfwPlatform/audio_openal.cpp"
	"util/file.h" "util/file.cpp"
	"util/endian.h" "util/strings.h"
	"util/imgui_extras.h" "util/imgui_extras.cpp"
	"util/scope_guard.h"
	"util/platform.h" "util/platform.cpp"
	"util/key_codes.h"
	"util/log.h" "util/log.cpp"
	"util/tokens.h" "util/tokens.cpp"
	"util/stream.h"
	"util/ini_file.h" "util/ini_file.cpp"
	"debugger.cpp" "debugger.h"
	"custom_chips_debugger.cpp" "custom_chips_debugger.h"
	"variable_watch.cpp" "variable_watch.h"
	"memory_editor.cpp" "memory_editor.h"
	"disk_manager.h" "disk_manager.cpp"
	"disk_image.h" "disk_image.cpp"
	"disk_activity.h" "disk_activity.cpp"
	"log_viewer.h" "log_viewer.cpp"
	"amiga/amiga.cpp" "amiga/amiga.h"
	"amiga/disassembler.cpp" "amiga/disassembler.h"
	"amiga/symbols.cpp" "amiga/symbols.h"
	"amiga/68000.h" "amiga/68000.cpp"
	"amiga/registers.h" "amiga/registers.cpp"
	"amiga/screen_buffer.h"
	"amiga/mfm.h" "amiga/mfm.cpp"
	"amiga/audio.h")

target_include_directories(AmigaEmulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(AmigaEmulator PRIVATE glfw imgui::imgui unofficial::gl3w::gl3w OpenAL::OpenAL ZLIB::ZLIB minizip::minizip)