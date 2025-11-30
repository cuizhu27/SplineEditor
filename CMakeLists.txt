cmake_minimum_required(VERSION 3.16)
project(FittingOpenGL LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========================
# 源文件（显式列出）
# ========================
set(SRC_FILES
    src/main.cpp
    src/renderer.cpp
    src/spline.cpp
)

# ========================
# 包含目录
# ========================
include_directories(
    src
    libs
    libs/glm
    libs/glad/include
    libs/glfw-3.4.bin.WIN64/include
    libs/imgui
    libs/imgui/backends
)

# ========================
# 可执行文件
# ========================
add_executable(app
    ${SRC_FILES}
    libs/glad/src/glad.c
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)

# ========================
# 链接库（Windows + MinGW）
# ========================
target_link_directories(app PRIVATE libs/glfw-3.4.bin.WIN64/lib-mingw-w64)

target_link_libraries(app
    glfw3
    opengl32
    gdi32
)

# ========================
# 复制 DLL（运行时需要）
# ========================
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/libs/glfw-3.4.bin.WIN64/lib-mingw-w64/glfw3.dll"
        "$<TARGET_FILE_DIR:app>"
    COMMENT "Copying glfw3.dll to output directory"
)