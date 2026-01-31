cmake_minimum_required(VERSION 3.20)
project(lumos VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui
add_subdirectory(third_party/imgui)

# Main executable (WIN32 = no console window)
add_executable(lumos WIN32
    src/main.cpp
    src/config.cpp
    src/platform/win32/gamma.cpp
    src/platform/win32/tray.cpp
    resources/lumos.rc
)

target_include_directories(lumos PRIVATE src)

target_link_libraries(lumos PRIVATE
    imgui
    dwmapi
)

# Enable warnings
if(MSVC)
    target_compile_options(lumos PRIVATE /W4)
endif()
