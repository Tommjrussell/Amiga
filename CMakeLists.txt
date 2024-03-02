# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8..3.28)

set(CMAKE_CXX_STANDARD 20)

project ("Amiga")

# Include sub-projects.
add_subdirectory ("DisassemblerTest")
add_subdirectory ("Emulator")
