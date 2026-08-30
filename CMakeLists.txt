cmake_minimum_required(VERSION 3.28)
project(AccidentReconstructor CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── Dear ImGui ─────────────────────────────────
# Use the docking branch because the application uses ImGuiConfigFlags_DockingEnable.
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)

# ── GLFW ─────────────────────────────────────
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/CMakeLists.txt)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/glfw)
else()
    message(FATAL_ERROR "GLFW not found. Run: git clone https://github.com/glfw/glfw.git third_party/glfw")
endif()

# The ImGui GLFW backend needs GLFW's public headers while it is compiled.
target_link_libraries(imgui PRIVATE glfw)

# ── Source files ─────────────────────────────
add_executable(sfe
    src/main.cpp
    src/glad/glad.c
)

# ── Include directories ──────────────────────
target_include_directories(sfe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ── Link libraries ───────────────────────────
target_link_libraries(sfe PRIVATE glfw)
target_link_libraries(sfe PRIVATE imgui)

# Windows: OpenGL + GDI32
if(WIN32)
    target_link_libraries(sfe PRIVATE opengl32 gdi32)
endif()

# Linux: OpenGL + dl
if(UNIX AND NOT APPLE)
    target_link_libraries(sfe PRIVATE GL dl)
endif()

# Output
set_target_properties(sfe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
