cmake_minimum_required(VERSION 3.20)
project(FrameForgeRuntime VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# DLL Output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(
    src
    ../shared
    ../external/minhook/include
    ../external/imgui
    ../external/imgui/backends
)

# MinHook Source
file(GLOB_RECURSE MINHOOK_SRC "../external/minhook/src/*.c")

# ImGui Source
set(IMGUI_SRC
    ../external/imgui/imgui.cpp
    ../external/imgui/imgui_draw.cpp
    ../external/imgui/imgui_tables.cpp
    ../external/imgui/imgui_widgets.cpp
    ../external/imgui/imgui_demo.cpp
    ../external/imgui/backends/imgui_impl_dx11.cpp
    ../external/imgui/backends/imgui_impl_win32.cpp
)

# Runtime Source
file(GLOB_RECURSE RUNTIME_SRC 
    "src/*.cpp"
    "src/*.h"
)

add_library(FrameForgeRuntime SHARED
    ${RUNTIME_SRC}
    ${MINHOOK_SRC}
    ${IMGUI_SRC}
)

# Link Libraries
target_link_libraries(FrameForgeRuntime 
    PRIVATE 
    d3d11
    dxgi
    d3dcompiler
)

target_compile_definitions(FrameForgeRuntime PRIVATE UNICODE _UNICODE)
