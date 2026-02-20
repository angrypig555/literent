cmake_minimum_required(VERSION 3.10.0)
project(literent VERSION 0.1.0 LANGUAGES C CXX)

find_package(LibtorrentRasterbar REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(literent src/main.cpp src/imgui/imgui.cpp src/imgui/imgui_draw.cpp src/imgui/imgui_widgets.cpp src/imgui/imgui_tables.cpp src/imgui/backends/imgui_impl_glfw.cpp src/imgui/backends/imgui_impl_opengl3.cpp src/imgui/imgui_demo.cpp src/imguifd/ImGuiFileDialog.cpp)
target_link_libraries(literent PRIVATE LibtorrentRasterbar::torrent-rasterbar glfw OpenGL::GL)
target_include_directories(literent PRIVATE src/imgui src/imgui/backends src/imguifd)
include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
