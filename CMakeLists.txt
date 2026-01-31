cmake_minimum_required(VERSION 3.20)
project(lumos VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui library
add_library(imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_win32.cpp
    third_party/imgui/backends/imgui_impl_dx11.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
)

target_link_libraries(imgui PUBLIC d3d11 dxgi)

# Main executable (WIN32 = no console window)
add_executable(lumos WIN32
    src/main.cpp
    src/app.cpp
    src/cli.cpp
    src/config.cpp
    src/platform/win32/gamma.cpp
    src/platform/win32/hotkeys.cpp
    src/platform/win32/tray.cpp
    src/ui/about_dialog.cpp
    src/ui/main_window.cpp
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
