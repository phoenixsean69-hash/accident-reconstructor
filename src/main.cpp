#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <cstdio>
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

#ifndef SFE_ASSET_DIR
#define SFE_ASSET_DIR "assets"
#endif

#define NANOSVG_IMPLEMENTATION
#include "../third_party/nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../third_party/nanosvg/nanosvgrast.h"

constexpr int WINDOW_WIDTH = 1600;
constexpr int WINDOW_HEIGHT = 900;

// ============================================================
// RIGHT PANEL WIDTH LIMITS
// ============================================================

constexpr float RIGHT_PANEL_MIN_WIDTH = 260.0f;
constexpr float RIGHT_PANEL_MAX_WIDTH = 430.0f;

enum class ToolIcon { Select, Move, Rotate, Scale, Vehicle, Evidence, Measure, Grid, Axes };
static std::array<GLuint, 9> gToolIcons{};

static GLuint loadSvgTexture(const char* path)
{
    const std::filesystem::path relativePath(path);
    const std::array<std::filesystem::path, 5> candidates = {
        relativePath,
        std::filesystem::path(SFE_ASSET_DIR).parent_path() / relativePath,
        std::filesystem::path(SFE_ASSET_DIR) / relativePath.filename(),
        std::filesystem::current_path() / ".." / relativePath,
        std::filesystem::current_path() / ".." / ".." / relativePath
    };

    std::filesystem::path resolvedPath;
    for (const auto& candidate : candidates)
    {
        if (std::filesystem::exists(candidate))
        {
            resolvedPath = candidate;
            break;
        }
    }

    if (resolvedPath.empty())
    {
        std::fprintf(stderr, "[icons] Missing SVG: %s\\n", path);
        return 0;
    }

    std::ifstream file(resolvedPath);
    if (!file) return 0;
    const std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    NSVGimage* image = nsvgParse(const_cast<char*>(source.c_str()), "px", 96.0f);
    if (!image) return 0;
    NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
    if (!rasterizer) { nsvgDelete(image); return 0; }
    const int width = std::max(1, static_cast<int>(image->width));
    const int height = std::max(1, static_cast<int>(image->height));
    std::vector<unsigned char> pixels(static_cast<size_t>(width * height * 4));
    nsvgRasterize(rasterizer, image, 0, 0, 1.0f, pixels.data(), width, height, width * 4);
    for (size_t i = 0; i + 3 < pixels.size(); i += 4)
    {
        if (pixels[i + 3] > 0)
        {
            pixels[i] = 255;
            pixels[i + 1] = 255;
            pixels[i + 2] = 255;
        }
    }
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    nsvgDeleteRasterizer(rasterizer);
    nsvgDelete(image);
    return texture;
}

// ============================================================
// SHADERS
// ============================================================

const char* vertexShaderSource = R"(
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vColor;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 460 core

in vec3 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
)";

// ============================================================
// SHADER COMPILATION
// ============================================================

static GLuint compileShader(
    GLenum type,
    const char* source
)
{
    GLuint shader =
        glCreateShader(type);

    glShaderSource(
        shader,
        1,
        &source,
        nullptr
    );

    glCompileShader(shader);

    GLint success =
        GL_FALSE;

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &success
    );

    if (!success)
    {
        char log[512]{};

        glGetShaderInfoLog(
            shader,
            sizeof(log),
            nullptr,
            log
        );

        std::printf(
            "[ERROR] Shader compilation failed:\n%s\n",
            log
        );
    }

    return shader;
}

static GLuint createShaderProgram()
{
    GLuint vertexShader =
        compileShader(
            GL_VERTEX_SHADER,
            vertexShaderSource
        );

    GLuint fragmentShader =
        compileShader(
            GL_FRAGMENT_SHADER,
            fragmentShaderSource
        );

    GLuint program =
        glCreateProgram();

    glAttachShader(
        program,
        vertexShader
    );

    glAttachShader(
        program,
        fragmentShader
    );

    glLinkProgram(
        program
    );

    GLint success =
        GL_FALSE;

    glGetProgramiv(
        program,
        GL_LINK_STATUS,
        &success
    );

    if (!success)
    {
        char log[512]{};

        glGetProgramInfoLog(
            program,
            sizeof(log),
            nullptr,
            log
        );

        std::printf(
            "[ERROR] Shader linking failed:\n%s\n",
            log
        );
    }

    glDeleteShader(
        vertexShader
    );

    glDeleteShader(
        fragmentShader
    );

    return program;
}

// ============================================================
// THEME
// ============================================================

static void applySovereignTheme()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style =
        ImGui::GetStyle();

    style.WindowRounding = 2.0f;
    style.ChildRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 2.0f;

    style.WindowPadding =
        ImVec2(
            10.0f,
            8.0f
        );

    style.FramePadding =
        ImVec2(
            8.0f,
            5.0f
        );

    style.ItemSpacing =
        ImVec2(
            7.0f,
            6.0f
        );

    style.ItemInnerSpacing =
        ImVec2(
            6.0f,
            4.0f
        );

    style.ScrollbarSize =
        13.0f;

    ImVec4* c =
        style.Colors;

    c[ImGuiCol_Text] =
        ImVec4(
            0.92f,
            0.90f,
            0.84f,
            1.0f
        );

    c[ImGuiCol_TextDisabled] =
        ImVec4(
            0.52f,
            0.51f,
            0.47f,
            1.0f
        );

    c[ImGuiCol_WindowBg] =
        ImVec4(
            0.025f,
            0.027f,
            0.030f,
            1.0f
        );

    c[ImGuiCol_ChildBg] =
        ImVec4(
            0.045f,
            0.047f,
            0.050f,
            1.0f
        );

    c[ImGuiCol_PopupBg] =
        ImVec4(
            0.075f,
            0.075f,
            0.070f,
            1.0f
        );

    c[ImGuiCol_Border] =
        ImVec4(
            0.25f,
            0.23f,
            0.17f,
            1.0f
        );

    c[ImGuiCol_Separator] =
    ImVec4(
        0.180f,
        0.185f,
        0.195f,
        1.0f
    );

    c[ImGuiCol_FrameBg] =
        ImVec4(
            0.105f,
            0.105f,
            0.095f,
            1.0f
        );

    c[ImGuiCol_FrameBgHovered] =
        ImVec4(
            0.27f,
            0.22f,
            0.11f,
            1.0f
        );

    c[ImGuiCol_FrameBgActive] =
        ImVec4(
            0.42f,
            0.31f,
            0.10f,
            1.0f
        );

    c[ImGuiCol_TitleBg] =
    ImVec4(
        0.045f,
        0.047f,
        0.050f,
        1.0f
    );

    c[ImGuiCol_TitleBgActive] =
    ImVec4(
        0.105f,
        0.110f,
        0.118f,
        1.0f
    );

    c[ImGuiCol_MenuBarBg] =
        ImVec4(
            0.075f,
            0.075f,
            0.070f,
            1.0f
        );

    c[ImGuiCol_Button] =
        ImVec4(
            0.13f,
            0.12f,
            0.09f,
            1.0f
        );

    c[ImGuiCol_ButtonHovered] =
        ImVec4(
            0.46f,
            0.34f,
            0.10f,
            1.0f
        );

    c[ImGuiCol_ButtonActive] =
        ImVec4(
            0.72f,
            0.52f,
            0.13f,
            1.0f
        );
    c[ImGuiCol_Header] =
    ImVec4(
        0.075f,
        0.078f,
        0.082f,
        1.0f
    );
    c[ImGuiCol_HeaderHovered] =
    ImVec4(
        0.135f,
        0.140f,
        0.148f,
        1.0f
    );
    c[ImGuiCol_HeaderActive] =
    ImVec4(
        0.105f,
        0.110f,
        0.118f,
        1.0f
    );

    c[ImGuiCol_CheckMark] =
        ImVec4(
            0.95f,
            0.68f,
            0.12f,
            1.0f
        );

    c[ImGuiCol_SliderGrab] =
        ImVec4(
            0.75f,
            0.52f,
            0.10f,
            1.0f
        );

    c[ImGuiCol_SliderGrabActive] =
        ImVec4(
            0.98f,
            0.73f,
            0.16f,
            1.0f
        );
    // ========================================================
    // DARK SOVEREIGN TAB PALETTE
    // ========================================================
    //
    // Tabs use neutral charcoal instead of the old muddy gold.
    // Gold remains an accent color for selections/controls.
    //

    // Normal tab
    c[ImGuiCol_Tab] =
    ImVec4(
        0.075f,
        0.078f,
        0.082f,
        1.0f
    );

    // Hovered tab
    c[ImGuiCol_TabHovered] =
    ImVec4(
        0.135f,
        0.140f,
        0.148f,
        1.0f
    );

    // Active tab
    c[ImGuiCol_TabActive] =
    ImVec4(
        0.105f,
        0.110f,
        0.118f,
        1.0f
    );

    // Active tab when its dock is unfocused
    c[ImGuiCol_TabUnfocused] =
        ImVec4(
            0.050f,
            0.053f,
            0.058f,
            1.0f
        );
    c[ImGuiCol_TabUnfocusedActive] =
        ImVec4(
            0.090f,
            0.095f,
            0.102f,
            1.0f
        );


        // Docking preview keeps the Sovereign gold accent.
    c[ImGuiCol_DockingPreview] =
    ImVec4(
        0.180f,
        0.190f,
        0.205f,
        0.60f
    );
}

// ============================================================
// LIT TOOL ICON
// ============================================================

static void drawRailTool(
    const char* id,
    const char* label,
    const char* tooltip,
    int& selectedTool,
    int toolValue
)
{
    ImGui::PushID(id);

    const float availableWidth =
        ImGui::GetContentRegionAvail().x;

    const float buttonWidth =
        std::max(
            1.0f,
            availableWidth - 2.0f
        );

      const float iconSize =
        std::min(
            22.0f,
            buttonWidth - 10.0f
        );

    // Consistent vertical tile size.
    // The previous 48px rows were too tight for icon + label.
    const float totalHeight =
        58.0f;

    ImVec2 pos =
        ImGui::GetCursorScreenPos();

    ImVec2 max(
        pos.x + buttonWidth,
        pos.y + totalHeight
    );

    ImDrawList* drawList =
        ImGui::GetWindowDrawList();

    const bool hovered =
        ImGui::IsMouseHoveringRect(
            pos,
            max
        );

    const bool active =
        selectedTool == toolValue;

    // --------------------------------------------------------
    // BACKGROUND
    // --------------------------------------------------------

    if (active)
    {
        drawList->AddRect(
            ImVec2(
                pos.x - 2.0f,
                pos.y - 2.0f
            ),
            ImVec2(
                max.x + 2.0f,
                max.y + 2.0f
            ),
            IM_COL32(
                238,
                174,
                38,
                90
            ),
            4.0f,
            0,
            1.0f
        );

        drawList->AddRectFilled(
            pos,
            max,
            IM_COL32(
                61,
                45,
                17,
                255
            ),
            4.0f
        );
    }
    else if (hovered)
    {
        drawList->AddRect(
            pos,
            max,
            IM_COL32(145, 113, 54, 150),
            4.0f,
            0,
            1.0f
        );
    }

    // --------------------------------------------------------
    // ACTIVE BAR
    // --------------------------------------------------------

    if (active)
    {
        drawList->AddRectFilled(
            ImVec2(
                pos.x - 2.0f,
                pos.y + 7.0f
            ),
            ImVec2(
                pos.x + 2.0f,
                max.y - 7.0f
            ),
            IM_COL32(
                238,
                174,
                38,
                255
            ),
            2.0f
        );
    }

    // --------------------------------------------------------
    // INVISIBLE BUTTON
    // --------------------------------------------------------

    ImGui::InvisibleButton(
        "##tool",
        ImVec2(
            buttonWidth,
            totalHeight
        )
    );

    if (ImGui::IsItemClicked())
    {
        selectedTool =
            toolValue;
    }

    // --------------------------------------------------------
    // ICON
    // --------------------------------------------------------

    if (gToolIcons[toolValue] != 0)
    {
        // Draw icon without disturbing the row layout cursor.
        ImGui::SetCursorScreenPos(
            ImVec2(
                pos.x + (buttonWidth - iconSize) * 0.5f,
                pos.y + 5.0f
            )
        );

        ImGui::Image(
            static_cast<ImTextureID>(gToolIcons[toolValue]),
            ImVec2(iconSize, iconSize)
        );

        // Label sits below the icon with a fixed gap.
        ImVec2 labelSize =
            ImGui::CalcTextSize(label);

        drawList->AddText(
            ImVec2(
                pos.x + (buttonWidth - labelSize.x) * 0.5f,
                pos.y + 34.0f
            ),
            active
                ? IM_COL32(238, 174, 38, 255)
                : IM_COL32(155, 158, 163, 255),
            label
        );

        if (hovered)
            ImGui::SetTooltip("%s", tooltip);

        // IMPORTANT:
        // Restore the layout cursor to the bottom of the
        // complete tile so the next tool starts below it.
        ImGui::SetCursorScreenPos(
            ImVec2(
                pos.x,
                pos.y + totalHeight
            )
        );

        ImGui::Dummy(
            ImVec2(
                buttonWidth,
                4.0f
            )
        );

        ImGui::PopID();
        return;
    }

    ImVec2 center(
        pos.x +
        buttonWidth * 0.5f,

        pos.y +
        iconSize * 0.5f
    );

    ImU32 iconColor =
        active || hovered
        ? IM_COL32(
            244,
            184,
            48,
            255
        )
        : IM_COL32(
            165,
            168,
            173,
            255
        );

    // SELECT
    if (toolValue == 0)
    {
        drawList->AddCircle(
            center,
            9.0f,
            iconColor,
            24,
            2.0f
        );

        drawList->AddCircleFilled(
            center,
            3.0f,
            iconColor
        );
    }

    // MOVE
    else if (toolValue == 1)
    {
        drawList->AddLine(
            ImVec2(
                center.x - 9.0f,
                center.y
            ),
            ImVec2(
                center.x + 9.0f,
                center.y
            ),
            iconColor,
            2.0f
        );

        drawList->AddLine(
            ImVec2(
                center.x,
                center.y - 9.0f
            ),
            ImVec2(
                center.x,
                center.y + 9.0f
            ),
            iconColor,
            2.0f
        );

        drawList->AddTriangleFilled(
            ImVec2(
                center.x - 13.0f,
                center.y
            ),
            ImVec2(
                center.x - 6.0f,
                center.y - 4.0f
            ),
            ImVec2(
                center.x - 6.0f,
                center.y + 4.0f
            ),
            iconColor
        );

        drawList->AddTriangleFilled(
            ImVec2(
                center.x + 13.0f,
                center.y
            ),
            ImVec2(
                center.x + 6.0f,
                center.y - 4.0f
            ),
            ImVec2(
                center.x + 6.0f,
                center.y + 4.0f
            ),
            iconColor
        );

        drawList->AddTriangleFilled(
            ImVec2(
                center.x,
                center.y - 13.0f
            ),
            ImVec2(
                center.x - 4.0f,
                center.y - 6.0f
            ),
            ImVec2(
                center.x + 4.0f,
                center.y - 6.0f
            ),
            iconColor
        );

        drawList->AddTriangleFilled(
            ImVec2(
                center.x,
                center.y + 13.0f
            ),
            ImVec2(
                center.x - 4.0f,
                center.y + 6.0f
            ),
            ImVec2(
                center.x + 4.0f,
                center.y + 6.0f
            ),
            iconColor
        );
    }

    // ROTATE
    else if (toolValue == 2)
    {
        drawList->AddCircle(
            center,
            10.0f,
            iconColor,
            30,
            2.0f
        );

        drawList->AddTriangleFilled(
            ImVec2(
                center.x + 8.0f,
                center.y - 12.0f
            ),
            ImVec2(
                center.x + 14.0f,
                center.y - 2.0f
            ),
            ImVec2(
                center.x + 3.0f,
                center.y - 5.0f
            ),
            iconColor
        );
    }

    // SCALE
    else if (toolValue == 3)
    {
        drawList->AddRect(
            ImVec2(
                center.x - 9.0f,
                center.y - 9.0f
            ),
            ImVec2(
                center.x + 9.0f,
                center.y + 9.0f
            ),
            iconColor,
            2.0f,
            0,
            2.0f
        );

        drawList->AddLine(
            ImVec2(
                center.x - 13.0f,
                center.y
            ),
            ImVec2(
                center.x + 13.0f,
                center.y
            ),
            iconColor,
            1.5f
        );

        drawList->AddLine(
            ImVec2(
                center.x,
                center.y - 13.0f
            ),
            ImVec2(
                center.x,
                center.y + 13.0f
            ),
            iconColor,
            1.5f
        );
    }

    // --------------------------------------------------------
    // LABEL
    // --------------------------------------------------------

    ImVec2 labelSize =
        ImGui::CalcTextSize(
            label
        );

    drawList->AddText(
        ImVec2(
            pos.x +
            (buttonWidth -
                labelSize.x) *
            0.5f,

            pos.y +
            iconSize +
            1.0f
        ),
        active || hovered
        ? IM_COL32(
            238,
            174,
            38,
            255
        )
        : IM_COL32(
            155,
            158,
            163,
            255
        ),
        label
    );

    if (hovered)
    {
        ImGui::SetTooltip(
            "%s",
            tooltip
        );
    }

    ImGui::Dummy(
        ImVec2(
            buttonWidth,
            2.0f
        )
    );

    ImGui::PopID();
}

// ============================================================
// LIT ACTION TOOL
// ============================================================

static bool drawRailAction(
    const char* id,
    const char* label,
    const char* tooltip,
    int iconIndex
)
{
    ImGui::PushID(id);

    const float availableWidth =
        ImGui::GetContentRegionAvail().x;

    const float buttonWidth =
        std::max(
            1.0f,
            availableWidth - 2.0f
        );

      const float iconSize =
        std::min(
            22.0f,
            buttonWidth - 10.0f
        );

    // Consistent vertical tile size.
    // The previous 48px rows were too tight for icon + label.
    const float totalHeight =
        58.0f;

    ImVec2 pos =
        ImGui::GetCursorScreenPos();

    ImVec2 max(
        pos.x + buttonWidth,
        pos.y + totalHeight
    );

    ImDrawList* drawList =
        ImGui::GetWindowDrawList();

    const bool hovered =
        ImGui::IsMouseHoveringRect(
            pos,
            max
        );

    if (hovered)
    {
        drawList->AddRect(
            pos,
            max,
            IM_COL32(145, 113, 54, 150),
            4.0f,
            0,
            1.0f
        );
    }

    bool clicked =
        ImGui::InvisibleButton(
            "##action",
            ImVec2(
                buttonWidth,
                totalHeight
            )
        );

    if (gToolIcons[iconIndex] != 0)
    {
        ImGui::SetCursorScreenPos(
            ImVec2(
                pos.x + (buttonWidth - iconSize) * 0.5f,
                pos.y + 5.0f
            )
        );

        ImGui::Image(
            static_cast<ImTextureID>(gToolIcons[iconIndex]),
            ImVec2(iconSize, iconSize)
        );

        ImVec2 labelSize =
            ImGui::CalcTextSize(label);

        drawList->AddText(
            ImVec2(
                pos.x + (buttonWidth - labelSize.x) * 0.5f,
                pos.y + 34.0f
            ),
            hovered
                ? IM_COL32(238, 174, 38, 255)
                : IM_COL32(155, 158, 163, 255),
            label
        );

        if (hovered)
            ImGui::SetTooltip("%s", tooltip);

        // Restore cursor to the bottom of the tile.
        ImGui::SetCursorScreenPos(
            ImVec2(
                pos.x,
                pos.y + totalHeight
            )
        );

        ImGui::Dummy(
            ImVec2(
                buttonWidth,
                4.0f
            )
        );

        ImGui::PopID();
        return clicked;
    }

    ImVec2 center(
        pos.x +
        buttonWidth * 0.5f,

        pos.y +
        iconSize * 0.5f
    );

    ImU32 iconColor =
        hovered
        ? IM_COL32(
            244,
            184,
            48,
            255
        )
        : IM_COL32(
            165,
            168,
            173,
            255
        );

    // ========================================================
    // VEHICLE
    // ========================================================

    if (id[0] == 'v')
    {
        drawList->AddRectFilled(
            ImVec2(
                center.x - 11.0f,
                center.y - 4.0f
            ),
            ImVec2(
                center.x + 11.0f,
                center.y + 5.0f
            ),
            iconColor,
            3.0f
        );

        drawList->AddRect(
            ImVec2(
                center.x - 7.0f,
                center.y - 9.0f
            ),
            ImVec2(
                center.x + 7.0f,
                center.y - 3.0f
            ),
            iconColor,
            2.0f
        );

        drawList->AddCircleFilled(
            ImVec2(
                center.x - 7.0f,
                center.y + 7.0f
            ),
            3.0f,
            IM_COL32(
                28,
                28,
                30,
                255
            )
        );

        drawList->AddCircleFilled(
            ImVec2(
                center.x + 7.0f,
                center.y + 7.0f
            ),
            3.0f,
            IM_COL32(
                28,
                28,
                30,
                255
            )
        );
    }

    // ========================================================
    // EVIDENCE
    // ========================================================

    else if (id[0] == 'e')
    {
        drawList->AddCircle(
            center,
            10.0f,
            iconColor,
            24,
            2.0f
        );

        drawList->AddLine(
            ImVec2(
                center.x - 5.0f,
                center.y
            ),
            ImVec2(
                center.x + 5.0f,
                center.y
            ),
            iconColor,
            2.0f
        );

        drawList->AddLine(
            ImVec2(
                center.x,
                center.y - 5.0f
            ),
            ImVec2(
                center.x,
                center.y + 5.0f
            ),
            iconColor,
            2.0f
        );
    }

    // ========================================================
    // MEASURE
    // ========================================================

    else
    {
        drawList->AddLine(
            ImVec2(
                center.x - 11.0f,
                center.y + 7.0f
            ),
            ImVec2(
                center.x + 11.0f,
                center.y - 7.0f
            ),
            iconColor,
            2.0f
        );

        for (
            int i = -6;
            i <= 6;
            i += 4
            )
        {
            drawList->AddLine(
                ImVec2(
                    center.x + i,
                    center.y -
                    i * 0.7f -
                    3.0f
                ),
                ImVec2(
                    center.x + i + 1.5f,
                    center.y -
                    i * 0.7f +
                    3.0f
                ),
                iconColor,
                1.2f
            );
        }
    }

    // ========================================================
    // LABEL
    // ========================================================

    ImVec2 labelSize =
        ImGui::CalcTextSize(
            label
        );

    drawList->AddText(
        ImVec2(
            pos.x +
            (buttonWidth -
                labelSize.x) *
            0.5f,

            pos.y +
            iconSize +
            1.0f
        ),
        hovered
        ? IM_COL32(
            238,
            174,
            38,
            255
        )
        : IM_COL32(
            155,
            158,
            163,
            255
        ),
        label
    );

    if (hovered)
    {
        ImGui::SetTooltip(
            "%s",
            tooltip
        );
    }

    ImGui::Dummy(
        ImVec2(
            buttonWidth,
            2.0f
        )
    );

    ImGui::PopID();

    return clicked;
}

// ============================================================
// DISPLAY TOGGLE
// ============================================================

static void drawDisplayToggle(
    const char* id,
    const char* label,
    const char* tooltip,
    bool& enabled,
    bool drawGridIcon
)
{
    ImGui::PushID(id);

    const float availableWidth =
        ImGui::GetContentRegionAvail().x;

    const float buttonWidth =
        std::max(
            1.0f,
            availableWidth - 2.0f
        );

      const float iconSize =
        std::min(
            22.0f,
            buttonWidth - 10.0f
        );

    // Consistent vertical tile size.
    // The previous 48px rows were too tight for icon + label.
    const float totalHeight =
        58.0f;

    ImVec2 pos =
        ImGui::GetCursorScreenPos();

    ImVec2 max(
        pos.x + buttonWidth,
        pos.y + totalHeight
    );

    ImDrawList* drawList =
        ImGui::GetWindowDrawList();

    const bool hovered =
        ImGui::IsMouseHoveringRect(
            pos,
            max
        );

    if (hovered)
    {
        drawList->AddRect(
            pos,
            max,
            IM_COL32(145, 113, 54, 150),
            4.0f,
            0,
            1.0f
        );
    }

    ImGui::InvisibleButton(
        "##display",
        ImVec2(
            buttonWidth,
            totalHeight
        )
    );

    if (ImGui::IsItemClicked())
    {
        enabled =
            !enabled;
    }

    int iconIndex = drawGridIcon ? 7 : 8;
    if (gToolIcons[iconIndex] != 0)
    {
        ImGui::SetCursorScreenPos(
            ImVec2(
                pos.x + (buttonWidth - iconSize) * 0.5f,
                pos.y + 5.0f
            )
        );

        ImGui::Image(
            static_cast<ImTextureID>(gToolIcons[iconIndex]),
            ImVec2(iconSize, iconSize)
        );

        ImVec2 labelSize =
            ImGui::CalcTextSize(label);

        drawList->AddText(
            ImVec2(
                pos.x + (buttonWidth - labelSize.x) * 0.5f,
                pos.y + 34.0f
            ),
            enabled
                ? IM_COL32(238, 174, 38, 255)
                : IM_COL32(155, 158, 163, 255),
            label
        );

        if (hovered)
            ImGui::SetTooltip("%s", tooltip);

        // Restore cursor to the bottom of the tile.
        ImGui::SetCursorScreenPos(
            ImVec2(
                pos.x,
                pos.y + totalHeight
            )
        );

        ImGui::Dummy(
            ImVec2(
                buttonWidth,
                4.0f
            )
        );

        ImGui::PopID();
        return;
    }

    ImVec2 center(
        pos.x +
        buttonWidth * 0.5f,

        pos.y +
        iconSize * 0.5f
    );

    ImU32 color =
        enabled
        ? IM_COL32(238, 174, 38, 255)
        : IM_COL32(155, 158, 163, 255);

    if (drawGridIcon)
    {
        const float spacing =
            6.0f;

        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                drawList->AddCircleFilled(
                    ImVec2(
                        center.x +
                        x * spacing,

                        center.y +
                        y * spacing
                    ),
                    1.8f,
                    color
                );
            }
        }
    }
    else
    {
        drawList->AddLine(
            center,
            ImVec2(
                center.x + 11.0f,
                center.y
            ),
            IM_COL32(
                210,
                70,
                65,
                255
            ),
            2.0f
        );

        drawList->AddLine(
            center,
            ImVec2(
                center.x,
                center.y - 11.0f
            ),
            IM_COL32(
                80,
                170,
                95,
                255
            ),
            2.0f
        );

        drawList->AddCircleFilled(
            center,
            2.5f,
            IM_COL32(
                238,
                174,
                38,
                255
            )
        );
    }

    // --------------------------------------------------------
    // LABEL
    // --------------------------------------------------------

    ImVec2 labelSize =
        ImGui::CalcTextSize(
            label
        );

    drawList->AddText(
        ImVec2(
            pos.x +
            (buttonWidth -
                labelSize.x) *
            0.5f,

            pos.y +
            iconSize +
            1.0f
        ),
        enabled
        ? IM_COL32(
            238,
            174,
            38,
            255
        )
        : IM_COL32(
            155,
            158,
            163,
            255
        ),
        label
    );

    if (hovered)
    {
        ImGui::SetTooltip(
            "%s",
            tooltip
        );
    }

    ImGui::Dummy(
        ImVec2(
            buttonWidth,
            2.0f
        )
    );

    ImGui::PopID();
}

// ============================================================
// RIGHT DOCK LOCK
// ============================================================

static void enforceRightPanelBounds(
    ImGuiID rightNodeId
)
{
    ImGuiDockNode* rightNode =
        ImGui::DockBuilderGetNode(
            rightNodeId
        );

    if (!rightNode)
        return;

    ImGuiDockNode* parent =
        rightNode->ParentNode;

    if (!parent)
        return;

    // We only want to constrain the horizontal
    // right-side docking split.
    if (parent->SplitAxis != ImGuiAxis_X)
        return;

    const float screenWidth =
        ImGui::GetMainViewport()->WorkSize.x;

    float desiredWidth =
        rightNode->Size.x;

    desiredWidth =
        std::clamp(
            desiredWidth,
            RIGHT_PANEL_MIN_WIDTH,
            RIGHT_PANEL_MAX_WIDTH
        );

    // Right child of an X-axis split.
    if (parent->ChildNodes[1] == rightNode)
    {
        const float splitX =
            parent->Pos.x +
            parent->Size.x -
            desiredWidth;

        parent->ChildNodes[0]->SizeRef.x =
            splitX -
            parent->Pos.x;

        parent->ChildNodes[1]->SizeRef.x =
            desiredWidth;
    }
    else
    {
        // Safety fallback.
        rightNode->SizeRef.x =
            desiredWidth;
    }

    // Prevent a ridiculous panel size on very small windows.
    if (screenWidth <
        RIGHT_PANEL_MAX_WIDTH +
        RIGHT_PANEL_MIN_WIDTH)
    {
        rightNode->SizeRef.x =
            std::max(
                180.0f,
                screenWidth * 0.28f
            );
    }
}

// ============================================================
// INTERFACE
// ============================================================

static void drawCaseView()
{
    ImGui::Begin("Case View");

    ImGui::Text("CASE OVERVIEW");
    ImGui::SameLine();
    ImGui::TextDisabled("/ CASE FILE");
    ImGui::Separator();

    ImGui::BeginChild("CaseOverviewContent", ImVec2(0.0f, 0.0f), false);
    ImGui::Text("UNTITLED CASE");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 110.0f);
    ImGui::TextColored(ImVec4(0.93f, 0.68f, 0.15f, 1.0f), "DRAFT");
    ImGui::Separator();

    ImGui::Columns(2, "CaseOverviewColumns", false);
    ImGui::TextDisabled("CASE ID");
    ImGui::Text("UNASSIGNED");
    ImGui::NextColumn();
    ImGui::TextDisabled("STATUS");
    ImGui::Text("DRAFT");
    ImGui::NextColumn();
    ImGui::TextDisabled("INCIDENT DATE");
    ImGui::Text("NOT SET");
    ImGui::NextColumn();
    ImGui::TextDisabled("LOCATION");
    ImGui::Text("NOT SET");
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Text("INCIDENT SUMMARY");
    ImGui::Separator();
    ImGui::TextWrapped("No incident summary has been entered. Add the essential case facts, scene location, and reconstruction notes here.");

    ImGui::Spacing();
    ImGui::Text("CASE CONTENT");
    ImGui::Separator();
    ImGui::BeginTable("CaseContentTable", 3, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg);
    ImGui::TableSetupColumn("CATEGORY");
    ImGui::TableSetupColumn("COUNT");
    ImGui::TableSetupColumn("STATUS");
    ImGui::TableHeadersRow();
    const char* rows[][3] = {{"VEHICLES", "2", "READY"}, {"EVIDENCE", "0", "EMPTY"}, {"MEASUREMENTS", "0", "EMPTY"}};
    for (const auto& row : rows)
    {
        ImGui::TableNextRow();
        for (int column = 0; column < 3; ++column)
        {
            ImGui::TableSetColumnIndex(column);
            ImGui::TextUnformatted(row[column]);
        }
    }
    ImGui::EndTable();

    ImGui::Spacing();
    ImGui::Text("INVOLVED PARTIES");
    ImGui::Separator();
    ImGui::TextDisabled("No parties have been added.");
    ImGui::EndChild();
    ImGui::End();
}

static void drawInterface()
{
    static int selectedTool =
        0;

    static int currentFrame =
        1;

    static bool showGrid =
        true;

    static bool showAxes =
        true;

    static bool layoutBuilt =
        false;

    static ImGuiID rightDockNodeId =
        0;

    ImGuiViewport* viewport =
        ImGui::GetMainViewport();

    // ========================================================
    // WORKSPACE HOST
    // ========================================================

    ImGui::SetNextWindowPos(
        viewport->WorkPos
    );

    ImGui::SetNextWindowSize(
        viewport->WorkSize
    );

    ImGui::SetNextWindowViewport(
        viewport->ID
    );

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::Begin(
        "Sovereign Workspace",
        nullptr,
        hostFlags
    );

    ImGuiID dockspace =
        ImGui::GetID(
            "SovereignDockspace"
        );

    ImGui::DockSpace(
        dockspace,
        ImVec2(
            0.0f,
            0.0f
        ),
        ImGuiDockNodeFlags_PassthruCentralNode
    );

    // ========================================================
    // BUILD DOCK LAYOUT
    // ========================================================

    if (!layoutBuilt)
    {
        ImGui::DockBuilderRemoveNode(
            dockspace
        );

        ImGui::DockBuilderAddNode(
            dockspace,
            ImGuiDockNodeFlags_DockSpace
        );

        ImGui::DockBuilderSetNodeSize(
            dockspace,
            viewport->WorkSize
        );

        ImGuiID center =
            dockspace;

        ImGuiID right =
            0;

        ImGuiID bottom =
            0;

        ImGuiID rightTop =
            0;

        // ----------------------------------------------------
        // RIGHT PANEL
        // ----------------------------------------------------

        ImGui::DockBuilderSplitNode(
            center,
            ImGuiDir_Right,
            0.245f,
            &right,
            &center
        );

        rightDockNodeId =
            right;

        // ----------------------------------------------------
        // TIMELINE
        // ----------------------------------------------------

        ImGui::DockBuilderSplitNode(
            center,
            ImGuiDir_Down,
            0.16f,
            &bottom,
            &center
        );

        // ----------------------------------------------------
        // OUTLINER / PROPERTIES
        // ----------------------------------------------------

        ImGui::DockBuilderSplitNode(
            right,
            ImGuiDir_Up,
            0.46f,
            &rightTop,
            &right
        );

        // ----------------------------------------------------
        // DOCK WINDOWS
        // ----------------------------------------------------

        ImGui::DockBuilderDockWindow(
            "Case View",
            center
        );

        ImGui::DockBuilderDockWindow(
            "Viewport",
            center
        );

        ImGui::DockBuilderDockWindow(
            "Case View",
            center
        );

        ImGui::DockBuilderDockWindow(
            "Evidence",
            center
        );

        ImGui::DockBuilderDockWindow(
            "Analysis",
            center
        );

        ImGui::DockBuilderDockWindow(
            "Timeline",
            bottom
        );

        ImGui::DockBuilderDockWindow(
            "Outliner",
            rightTop
        );

        ImGui::DockBuilderDockWindow(
            "Properties",
            right
        );

        ImGui::DockBuilderFinish(
            dockspace
        );

        // ----------------------------------------------------
        // LOCK RIGHT NODES
        // ----------------------------------------------------

        ImGuiDockNode* outlinerNode =
            ImGui::DockBuilderGetNode(
                rightTop
            );

        ImGuiDockNode* propertiesNode =
            ImGui::DockBuilderGetNode(
                right
            );

        ImGuiDockNode* rightStackNode =
            propertiesNode
            ? propertiesNode->ParentNode
            : nullptr;

        if (outlinerNode)
        {
            outlinerNode->LocalFlags |=
                ImGuiDockNodeFlags_NoDocking;
        }

        if (propertiesNode)
        {
            propertiesNode->LocalFlags |=
                ImGuiDockNodeFlags_NoDocking;
        }

        if (rightStackNode)
        {
            rightStackNode->LocalFlags |=
                ImGuiDockNodeFlags_NoDocking;
        }

        layoutBuilt =
            true;
    }

    ImGui::End();

    // ========================================================
    // ENFORCE RIGHT PANEL SIZE
    // ========================================================

    if (rightDockNodeId != 0)
    {
        enforceRightPanelBounds(
            rightDockNodeId
        );
    }

    // ========================================================
    // MENU BAR
    // ========================================================

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem(
                "New Case"
            );

            ImGui::MenuItem(
                "Open Case"
            );

            ImGui::MenuItem(
                "Save Case"
            );

            ImGui::Separator();

            ImGui::MenuItem(
                "Exit"
            );

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem(
                "Undo"
            );

            ImGui::MenuItem(
                "Redo"
            );

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {

            ImGui::MenuItem(
                "Outliner"
            );

            ImGui::MenuItem(
                "Properties"
            );

            ImGui::MenuItem(
                "Timeline"
            );

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene"))
        {
            ImGui::MenuItem(
                "Add Vehicle"
            );

            ImGui::MenuItem(
                "Add Evidence"
            );

            ImGui::MenuItem(
                "Add Measurement"
            );

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Analysis"))
        {
            ImGui::MenuItem(
                "Skid Analysis"
            );

            ImGui::MenuItem(
                "Momentum Analysis"
            );

            ImGui::MenuItem(
                "Speed Analysis"
            );

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem(
                "About Sovereign"
            );

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // The former outer Tools dock is intentionally removed.
    if (false)
    {
        ImGui::Begin(
            "Tools",
            nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize
        );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(
            5.0f,
            7.0f
        )
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(
            0.0f,
            2.0f
        )
    );

    // --------------------------------------------------------
    // BRAND MARK
    // --------------------------------------------------------

    {
        const float width =
            ImGui::GetContentRegionAvail().x;

        const float size =
            std::min(
                44.0f,
                width - 2.0f
            );

        ImVec2 pos =
            ImGui::GetCursorScreenPos();

        ImVec2 max(
            pos.x + size,
            pos.y + size
        );

        ImDrawList* drawList =
            ImGui::GetWindowDrawList();

        drawList->AddRectFilled(
            pos,
            max,
            IM_COL32(
                55,
                39,
                11,
                255
            ),
            4.0f
            );

        drawList->AddRect(
            ImVec2(
                pos.x - 1.0f,
                pos.y - 1.0f
            ),
            ImVec2(
                max.x + 1.0f,
                max.y + 1.0f
            ),
            IM_COL32(
                238,
                174,
                38,
                170
            ),
            10.0f,
            0,
            1.2f
        );

        ImVec2 center(
            pos.x + size * 0.5f,
            pos.y + size * 0.5f
        );

        drawList->AddCircle(
            center,
            10.0f,
            IM_COL32(
                238,
                174,
                38,
                255
            ),
            24,
            2.0f
        );

        drawList->AddCircleFilled(
            center,
            3.0f,
            IM_COL32(
                255,
                211,
                93,
                255
            )
        );

        drawList->AddLine(
            ImVec2(
                center.x - 7.0f,
                center.y + 7.0f
            ),
            ImVec2(
                center.x + 7.0f,
                center.y - 7.0f
            ),
            IM_COL32(
                238,
                174,
                38,
                210
            ),
            1.5f
        );

        ImGui::InvisibleButton(
            "##brand",
            ImVec2(
                size,
                size
            )
        );
    }

    // The outer Tools dock is intentionally empty for now.
        ImGui::PopStyleVar(2);
        ImGui::End();
    }

    drawCaseView();

    // ========================================================
    // VIEWPORT
    // ========================================================

        // ========================================================
    // CASE VIEW TAB
    // ========================================================

    ImGui::Begin(
        "Case View"
    );

    ImGui::Text("CASE VIEW");
    ImGui::Separator();

    ImGui::TextDisabled(
        "Case reconstruction workspace"
    );

    ImGui::Spacing();

    ImGui::Text(
        "Case overview and reconstruction data."
    );

    ImGui::Text(
        "Use the scene viewport to construct the case."
    );

    ImGui::End();

    // ========================================================
    // EVIDENCE TAB
    // ========================================================

    ImGui::Begin(
        "Evidence"
    );

    ImGui::Text("EVIDENCE");
    ImGui::Separator();

    ImGui::TextDisabled(
        "Evidence workspace"
    );

    ImGui::Spacing();

    ImGui::BulletText("Skid marks");
    ImGui::BulletText("Debris fields");
    ImGui::BulletText("Scene markers");
    ImGui::BulletText("Photographic evidence");

    ImGui::End();

    // ========================================================
    // ANALYSIS TAB
    // ========================================================

    ImGui::Begin(
        "Analysis"
    );

    ImGui::Text("ANALYSIS");
    ImGui::Separator();

    ImGui::TextDisabled(
        "Accident analysis workspace"
    );

    ImGui::Spacing();

    ImGui::BulletText("Skid analysis");
    ImGui::BulletText("Momentum analysis");
    ImGui::BulletText("Speed analysis");
    ImGui::BulletText("Reconstruction results");

    ImGui::End();

ImGui::Begin(
        "Viewport"
    );

        // Central workspace navigation is provided by
    // the real ImGui dock tab bar.

    ImGui::BeginChild("ViewportToolRail", ImVec2(112.0f, 0.0f), true);
    drawRailTool("viewport_select", "SELECT", "Select object (Q)", selectedTool, 0);
    drawRailTool("viewport_move", "MOVE", "Move object (W)", selectedTool, 1);
    drawRailTool("viewport_rotate", "ROTATE", "Rotate object (E)", selectedTool, 2);
    drawRailTool("viewport_scale", "SCALE", "Scale object (R)", selectedTool, 3);
    ImGui::Separator();
    drawRailAction("viewport_vehicle", "VEHICLE", "Add Vehicle", 4);
    drawRailAction("viewport_evidence", "EVIDENCE", "Add Evidence", 5);
    drawRailAction("viewport_measure", "MEASURE", "Measure Scene", 6);
    ImGui::Separator();
    drawDisplayToggle("viewport_grid", "GRID", "Toggle Grid", showGrid, true);
    drawDisplayToggle("viewport_axes", "AXES", "Toggle Axes", showAxes, false);
    ImGui::EndChild();
    ImGui::SameLine(0.0f, 6.0f);

    ImVec2 available =
        ImGui::GetContentRegionAvail();

    ImGui::BeginChild(
        "SceneCanvas",
        available,
        true,
        ImGuiWindowFlags_NoScrollbar
    );

    ImVec2 canvasPos =
        ImGui::GetCursorScreenPos();

    ImVec2 canvasSize =
        ImGui::GetContentRegionAvail();

    ImDrawList* drawList =
        ImGui::GetWindowDrawList();

    // --------------------------------------------------------
    // BACKGROUND
    // --------------------------------------------------------

    drawList->AddRectFilled(
        canvasPos,
        ImVec2(
            canvasPos.x +
            canvasSize.x,

            canvasPos.y +
            canvasSize.y
        ),
        IM_COL32(
            18,
            20,
            22,
            255
        )
    );

    // --------------------------------------------------------
    // GRID
    // --------------------------------------------------------

    if (showGrid)
    {
        const float gridSize =
            32.0f;

        for (
            float x = canvasPos.x;
            x <
            canvasPos.x +
            canvasSize.x;
            x += gridSize
            )
        {
            drawList->AddLine(
                ImVec2(
                    x,
                    canvasPos.y
                ),
                ImVec2(
                    x,
                    canvasPos.y +
                    canvasSize.y
                ),
                IM_COL32(
                    48,
                    48,
                    43,
                    255
                )
            );
        }

        for (
            float y = canvasPos.y;
            y <
            canvasPos.y +
            canvasSize.y;
            y += gridSize
            )
        {
            drawList->AddLine(
                ImVec2(
                    canvasPos.x,
                    y
                ),
                ImVec2(
                    canvasPos.x +
                    canvasSize.x,
                    y
                ),
                IM_COL32(
                    48,
                    48,
                    43,
                    255
                )
            );
        }
    }

    // --------------------------------------------------------
    // CENTER POINT
    // --------------------------------------------------------

    ImVec2 centerPoint(
        canvasPos.x +
        canvasSize.x * 0.50f,

        canvasPos.y +
        canvasSize.y * 0.50f
    );

    drawList->AddCircleFilled(
        centerPoint,
        7.0f,
        IM_COL32(
            238,
            174,
            38,
            255
        )
    );

    // --------------------------------------------------------
    // AXES
    // --------------------------------------------------------

    if (showAxes)
    {
        drawList->AddLine(
            centerPoint,
            ImVec2(
                centerPoint.x + 100.0f,
                centerPoint.y
            ),
            IM_COL32(
                190,
                65,
                55,
                255
            ),
            2.0f
        );

        drawList->AddLine(
            centerPoint,
            ImVec2(
                centerPoint.x,
                centerPoint.y - 100.0f
            ),
            IM_COL32(
                70,
                145,
                80,
                255
            ),
            2.0f
        );

        drawList->AddText(
            ImVec2(
                centerPoint.x + 105.0f,
                centerPoint.y - 10.0f
            ),
            IM_COL32(
                220,
                90,
                75,
                255
            ),
            "X"
        );

        drawList->AddText(
            ImVec2(
                centerPoint.x + 7.0f,
                centerPoint.y - 120.0f
            ),
            IM_COL32(
                100,
                190,
                110,
                255
            ),
            "Y"
        );
    }

    // --------------------------------------------------------
    // VIEWPORT TEXT
    // --------------------------------------------------------

    ImGui::SetCursorScreenPos(
        ImVec2(
            canvasPos.x + 16.0f,
            canvasPos.y + 16.0f
        )
    );

    ImGui::Text(
        "SCENE VIEWPORT"
    );

    ImGui::TextDisabled(
        "No scene objects loaded"
    );

    ImGui::EndChild();

    ImGui::End();

    // ========================================================
    // RIGHT PANEL FLAGS
    // ========================================================

    const ImGuiWindowFlags rightPanelFlags =
        ImGuiWindowFlags_NoMove;

    // ========================================================
    // OUTLINER
    // ========================================================

    ImGui::Begin(
        "Outliner",
        nullptr,
        rightPanelFlags
    );

    ImGui::Text(
        "SCENE OUTLINER"
    );

    ImGui::Separator();

    if (ImGui::TreeNodeEx(
        "Environment",
        ImGuiTreeNodeFlags_DefaultOpen
    ))
    {
        ImGui::BulletText(
            "Ground Plane"
        );

        ImGui::BulletText(
            "Road Surface"
        );

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx(
        "Vehicles",
        ImGuiTreeNodeFlags_DefaultOpen
    ))
    {
        ImGui::Selectable(
            "Vehicle A"
        );

        ImGui::Selectable(
            "Vehicle B"
        );

        ImGui::TreePop();
    }

    if (ImGui::TreeNode(
        "Evidence"
    ))
    {
        ImGui::Selectable(
            "Skid Mark 01"
        );

        ImGui::Selectable(
            "Marker 01"
        );

        ImGui::Selectable(
            "Debris Field 01"
        );

        ImGui::TreePop();
    }

    if (ImGui::TreeNode(
        "Measurements"
    ))
    {
        ImGui::Selectable(
            "Distance 01"
        );

        ImGui::Selectable(
            "Angle 01"
        );

        ImGui::TreePop();
    }

    ImGui::End();

    // ========================================================
    // PROPERTIES
    // ========================================================

    ImGui::Begin(
        "Properties",
        nullptr,
        rightPanelFlags
    );

    ImGui::Text(
        "INSPECTOR"
    );

    ImGui::Separator();

    ImGui::TextDisabled(
        "Nothing selected"
    );

    ImGui::Spacing();

    if (ImGui::CollapsingHeader(
        "Transform",
        ImGuiTreeNodeFlags_DefaultOpen
    ))
    {
        static float position[3] =
        {
            0.0f,
            0.0f,
            0.0f
        };

        static float rotation[3] =
        {
            0.0f,
            0.0f,
            0.0f
        };

        static float scale[3] =
        {
            1.0f,
            1.0f,
            1.0f
        };

        ImGui::DragFloat3(
            "Position",
            position,
            0.1f
        );

        ImGui::DragFloat3(
            "Rotation",
            rotation,
            1.0f
        );

        ImGui::DragFloat3(
            "Scale",
            scale,
            0.01f
        );
    }

    if (ImGui::CollapsingHeader(
        "Metadata"
    ))
    {
        static char objectName[128] =
            "Untitled Object";

        ImGui::InputText(
            "Name",
            objectName,
            sizeof(objectName)
        );

        ImGui::Text(
            "Type: Scene Entity"
        );

        ImGui::Text(
            "Units: meters"
        );
    }

    if (ImGui::CollapsingHeader(
        "Analysis"
    ))
    {
        ImGui::TextDisabled(
            "No analysis data available."
        );

        ImGui::TextDisabled(
            "Add evidence to begin."
        );
    }

    ImGui::End();

    // ========================================================
    // TIMELINE
    // ========================================================

    ImGui::Begin(
        "Timeline"
    );

    ImGui::Text(
        "TIMELINE"
    );

    ImGui::SameLine();

    if (ImGui::Button(
        "|<"
    ))
    {
        currentFrame =
            1;
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Play"
    ))
    {
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Stop"
    ))
    {
        currentFrame =
            1;
    }

    ImGui::SameLine();

    ImGui::Text(
        "Frame"
    );

    ImGui::SameLine();

    ImGui::SetNextItemWidth(
        100.0f
    );

    ImGui::DragInt(
        "##Frame",
        &currentFrame,
        1.0f,
        1,
        1000
    );

    ImGui::SameLine();

    ImGui::TextDisabled(
        "Pre-impact  â†’  Impact  â†’  Rest"
    );

    ImGui::Separator();

    ImGui::SetNextItemWidth(
        -1.0f
    );

    static float timelinePosition =
        0.0f;

    ImGui::SliderFloat(
        "##TimelinePosition",
        &timelinePosition,
        0.0f,
        100.0f,
        "%.0f"
    );

    ImGui::End();
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    // ========================================================
    // GLFW
    // ========================================================

    if (!glfwInit())
    {
        std::printf(
            "[FATAL] Failed to initialize GLFW.\n"
        );

        return -1;
    }

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MAJOR,
        4
    );

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MINOR,
        6
    );

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    glfwWindowHint(
        GLFW_RESIZABLE,
        GLFW_TRUE
    );

    GLFWwindow* window =
        glfwCreateWindow(
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            "Sovereign Accident Reconstructor v0.1.0 â€” OpenGL 4.6",
            nullptr,
            nullptr
        );

    if (!window)
    {
        glfwTerminate();

        return -1;
    }

    glfwMakeContextCurrent(
        window
    );

    glfwSwapInterval(
        1
    );

    // ========================================================
    // GLAD
    // ========================================================

    if (!gladLoadGLLoader(
        reinterpret_cast<GLADloadproc>(
            glfwGetProcAddress
            )
    ))
    {
        glfwDestroyWindow(
            window
        );

        glfwTerminate();

        return -1;
    }

    // ========================================================
    // IMGUI
    // ========================================================

    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

// Application owns the dock layout.
// Do not restore a previously dragged ImGui layout.
io.IniFilename = nullptr;

    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;

    io.ConfigFlags |=
        ImGuiConfigFlags_DockingEnable;

    applySovereignTheme();

    io.FontGlobalScale =
        1.0f;

    // ========================================================
    // FONT
    // ========================================================

    ImFont* rubik =
        io.Fonts->AddFontFromFileTTF(
            "assets/fonts/Rubik-Regular.ttf",
            18.0f
        );

    if (!rubik)
    {
        std::printf(
            "[ERROR] Could not load "
            "assets/fonts/Rubik-Regular.ttf\n"
        );
    }

    // ========================================================
    // IMGUI GLFW
    // ========================================================

    if (!ImGui_ImplGlfw_InitForOpenGL(
        window,
        true
    ))
    {
        ImGui::DestroyContext();

        glfwDestroyWindow(
            window
        );

        glfwTerminate();

        return -1;
    }

    // ========================================================
    // IMGUI OPENGL
    // ========================================================

    if (!ImGui_ImplOpenGL3_Init(
        "#version 460"
    ))
    {
        ImGui_ImplGlfw_Shutdown();

        ImGui::DestroyContext();

        glfwDestroyWindow(
            window
        );

        glfwTerminate();

        return -1;
    }

    // ========================================================
    // SHADER
    // ========================================================

    GLuint program =
        createShaderProgram();

    // ========================================================
    // TRIANGLE
    // ========================================================

    float vertices[] =
    {
         0.0f,
         0.5f,
         0.0f,

         1.0f,
         0.6f,
         0.2f,

        -0.5f,
        -0.5f,
         0.0f,

         0.2f,
         0.6f,
         1.0f,

         0.5f,
        -0.5f,
         0.0f,

         0.2f,
         0.8f,
         0.4f
    };

    GLuint vao =
        0;

    GLuint vbo =
        0;

    glGenVertexArrays(
        1,
        &vao
    );

    glGenBuffers(
        1,
        &vbo
    );

    glBindVertexArray(
        vao
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        vbo
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    // Position
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        nullptr
    );

    glEnableVertexAttribArray(
        0
    );

    // Color
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        reinterpret_cast<void*>(
            3 * sizeof(float)
            )
    );

    glEnableVertexAttribArray(
        1
    );

    glBindVertexArray(
        0
    );

    const char* iconPaths[] = {
        "assets/icons/blender/select.svg", "assets/icons/blender/move.svg", "assets/icons/blender/rotate.svg",
        "assets/icons/blender/scale.svg", "assets/icons/blender/vehicle.svg", "assets/icons/blender/evidence.svg",
        "assets/icons/blender/measure.svg", "assets/icons/blender/grid.svg", "assets/icons/blender/axes.svg"
    };
    for (size_t i = 0; i < std::size(iconPaths); ++i)
        gToolIcons[i] = loadSvgTexture(iconPaths[i]);

    // ========================================================
    // MAIN LOOP
    // ========================================================

    while (!glfwWindowShouldClose(
        window
    ))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();

        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();

        if (rubik)
        {
            ImGui::PushFont(
                rubik
            );
        }

        drawInterface();

        if (rubik)
        {
            ImGui::PopFont();
        }

        // ----------------------------------------------------
        // CLEAR
        // ----------------------------------------------------

        glClearColor(
            0.025f,
            0.027f,
            0.030f,
            1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT
        );

        // ----------------------------------------------------
        // TRIANGLE
        // ----------------------------------------------------

        glUseProgram(
            program
        );

        glBindVertexArray(
            vao
        );

        glDrawArrays(
            GL_TRIANGLES,
            0,
            3
        );

        glBindVertexArray(
            0
        );

        // ----------------------------------------------------
        // IMGUI
        // ----------------------------------------------------

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(
            ImGui::GetDrawData()
        );

        glfwSwapBuffers(
            window
        );
    }

    // ========================================================
    // CLEANUP
    // ========================================================

    glDeleteVertexArrays(
        1,
        &vao
    );

    glDeleteBuffers(
        1,
        &vbo
    );

    glDeleteProgram(
        program
    );

    ImGui_ImplOpenGL3_Shutdown();

    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();

    glfwDestroyWindow(
        window
    );

    glfwTerminate();

    return 0;
}





