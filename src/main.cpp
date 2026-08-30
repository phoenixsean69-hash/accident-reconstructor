#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <cstdio>

constexpr int WINDOW_WIDTH = 1600;
constexpr int WINDOW_HEIGHT = 900;

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

static GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char log[512]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::printf("[ERROR] Shader compilation failed:\n%s\n", log);
    }

    return shader;
}

static GLuint createShaderProgram()
{
    GLuint vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexShaderSource);

    GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        char log[512]{};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::printf("[ERROR] Shader linking failed:\n%s\n", log);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

static void applySovereignTheme()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 2.0f;
    style.ChildRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 2.0f;

    style.WindowPadding = ImVec2(10.0f, 8.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ItemSpacing = ImVec2(7.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.ScrollbarSize = 13.0f;

    ImVec4* c = style.Colors;

    c[ImGuiCol_Text] = ImVec4(0.92f, 0.90f, 0.84f, 1.0f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.52f, 0.51f, 0.47f, 1.0f);

    c[ImGuiCol_WindowBg] = ImVec4(0.025f, 0.027f, 0.030f, 1.0f);
    c[ImGuiCol_ChildBg] = ImVec4(0.045f, 0.047f, 0.050f, 1.0f);
    c[ImGuiCol_PopupBg] = ImVec4(0.075f, 0.075f, 0.070f, 1.0f);

    c[ImGuiCol_Border] = ImVec4(0.25f, 0.23f, 0.17f, 1.0f);
    c[ImGuiCol_Separator] = ImVec4(0.30f, 0.27f, 0.18f, 1.0f);

    c[ImGuiCol_FrameBg] = ImVec4(0.105f, 0.105f, 0.095f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.27f, 0.22f, 0.11f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.42f, 0.31f, 0.10f, 1.0f);

    c[ImGuiCol_TitleBg] = ImVec4(0.045f, 0.047f, 0.050f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.19f, 0.08f, 1.0f);
    c[ImGuiCol_MenuBarBg] = ImVec4(0.075f, 0.075f, 0.070f, 1.0f);

    c[ImGuiCol_Button] = ImVec4(0.13f, 0.12f, 0.09f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.34f, 0.10f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.72f, 0.52f, 0.13f, 1.0f);

    c[ImGuiCol_Header] = ImVec4(0.22f, 0.17f, 0.08f, 1.0f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.32f, 0.10f, 1.0f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.70f, 0.50f, 0.12f, 1.0f);

    c[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.68f, 0.12f, 1.0f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.75f, 0.52f, 0.10f, 1.0f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.98f, 0.73f, 0.16f, 1.0f);

    c[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.085f, 1.0f);
    c[ImGuiCol_TabHovered] = ImVec4(0.50f, 0.36f, 0.10f, 1.0f);
    c[ImGuiCol_TabActive] = ImVec4(0.32f, 0.24f, 0.10f, 1.0f);

    c[ImGuiCol_DockingPreview] = ImVec4(0.95f, 0.68f, 0.12f, 0.70f);
}

struct SidebarState
{
    int selectedTool = 0;
    bool showGrid = true;
    bool showAxes = true;
    bool compact = false;
};

enum class SidebarIcon
{
    Select,
    Move,
    Rotate,
    Scale,
    Vehicle,
    Evidence,
    Measure,
};

struct SidebarTool
{
    SidebarIcon icon;
    const char* label;
    const char* shortcut;
    int value;
};

static void drawSidebarIcon(ImDrawList* drawList, SidebarIcon icon, ImVec2 center, ImU32 color)
{
    const float s = 7.0f;
    const float stroke = 1.8f;

    switch (icon)
    {
        case SidebarIcon::Select:
            drawList->AddTriangleFilled(
                ImVec2(center.x - s, center.y - s),
                ImVec2(center.x - s, center.y + s),
                ImVec2(center.x + s * 0.55f, center.y + s * 2.0f),
                color
            );
            drawList->AddLine(
                ImVec2(center.x - s * 0.1f, center.y + s * 0.4f),
                ImVec2(center.x + s * 0.9f, center.y + s * 2.0f),
                color,
                stroke
            );
            break;
        case SidebarIcon::Move:
            drawList->AddLine(ImVec2(center.x - s, center.y), ImVec2(center.x + s, center.y), color, stroke);
            drawList->AddLine(ImVec2(center.x, center.y - s), ImVec2(center.x, center.y + s), color, stroke);
            drawList->AddTriangleFilled(ImVec2(center.x - s, center.y), ImVec2(center.x - s * 0.35f, center.y - 3.0f), ImVec2(center.x - s * 0.35f, center.y + 3.0f), color);
            drawList->AddTriangleFilled(ImVec2(center.x + s, center.y), ImVec2(center.x + s * 0.35f, center.y - 3.0f), ImVec2(center.x + s * 0.35f, center.y + 3.0f), color);
            drawList->AddTriangleFilled(ImVec2(center.x, center.y - s), ImVec2(center.x - 3.0f, center.y - s * 0.35f), ImVec2(center.x + 3.0f, center.y - s * 0.35f), color);
            drawList->AddTriangleFilled(ImVec2(center.x, center.y + s), ImVec2(center.x - 3.0f, center.y + s * 0.35f), ImVec2(center.x + 3.0f, center.y + s * 0.35f), color);
            break;
        case SidebarIcon::Rotate:
            drawList->AddCircle(center, s, color, 20, stroke);
            drawList->AddTriangleFilled(ImVec2(center.x + s, center.y - 2.0f), ImVec2(center.x + s + 5.0f, center.y - 2.0f), ImVec2(center.x + s + 2.0f, center.y + 4.0f), color);
            break;
        case SidebarIcon::Scale:
            drawList->AddRect(ImVec2(center.x - s, center.y - s), ImVec2(center.x + s, center.y + s), color, 0.0f, 0, stroke);
            drawList->AddLine(ImVec2(center.x - s - 3.0f, center.y - s - 3.0f), ImVec2(center.x - s - 7.0f, center.y - s - 3.0f), color, stroke);
            drawList->AddLine(ImVec2(center.x - s - 3.0f, center.y - s - 3.0f), ImVec2(center.x - s - 3.0f, center.y - s - 7.0f), color, stroke);
            drawList->AddLine(ImVec2(center.x + s + 3.0f, center.y + s + 3.0f), ImVec2(center.x + s + 7.0f, center.y + s + 3.0f), color, stroke);
            drawList->AddLine(ImVec2(center.x + s + 3.0f, center.y + s + 3.0f), ImVec2(center.x + s + 3.0f, center.y + s + 7.0f), color, stroke);
            break;
        case SidebarIcon::Vehicle:
            drawList->AddRect(ImVec2(center.x - 9.0f, center.y - 4.0f), ImVec2(center.x + 9.0f, center.y + 4.0f), color, 2.0f, 0, stroke);
            drawList->AddCircleFilled(ImVec2(center.x - 6.0f, center.y + 5.0f), 2.0f, color);
            drawList->AddCircleFilled(ImVec2(center.x + 6.0f, center.y + 5.0f), 2.0f, color);
            break;
        case SidebarIcon::Evidence:
            drawList->AddRect(ImVec2(center.x - 7.0f, center.y - 8.0f), ImVec2(center.x + 7.0f, center.y + 8.0f), color, 1.0f, 0, stroke);
            drawList->AddLine(ImVec2(center.x - 3.0f, center.y - 3.0f), ImVec2(center.x + 3.0f, center.y - 3.0f), color, stroke);
            drawList->AddLine(ImVec2(center.x - 3.0f, center.y + 2.0f), ImVec2(center.x + 3.0f, center.y + 2.0f), color, stroke);
            break;
        case SidebarIcon::Measure:
            drawList->AddLine(ImVec2(center.x - 9.0f, center.y + 6.0f), ImVec2(center.x + 9.0f, center.y - 6.0f), color, stroke);
            for (int i = -1; i <= 1; ++i)
            {
                const float x = center.x + i * 6.0f;
                drawList->AddLine(ImVec2(x, center.y + 5.0f - i * 2.0f), ImVec2(x + 2.0f, center.y + 1.0f - i * 2.0f), color, stroke);
            }
            break;
    }
}

static bool drawSidebarTool(const SidebarTool& item, SidebarState& state)
{
    const bool active = state.selectedTool == item.value;
    const ImVec2 rowSize(-1.0f, 32.0f);
    const ImVec2 rowStart = ImGui::GetCursorScreenPos();

    ImGui::PushID(item.label);
    if (active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.22f, 0.08f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.38f, 0.28f, 0.09f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.78f, 0.28f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.085f, 0.08f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.16f, 0.11f, 1.0f));
    }

    const bool clicked = ImGui::Button("##tool", rowSize);
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 rowEnd(rowStart.x + ImGui::GetItemRectSize().x, rowStart.y + ImGui::GetItemRectSize().y);

    ImGui::PopStyleColor(active ? 3 : 2);

    if (active)
    {
        ImGui::GetWindowDrawList()->AddRectFilled(
            rowStart,
            ImVec2(rowStart.x + 3.0f, rowEnd.y),
            IM_COL32(238, 174, 38, 255)
        );
    }

    const ImVec4 iconColor = active
        ? ImVec4(0.96f, 0.70f, 0.18f, 1.0f)
        : (hovered ? ImVec4(0.90f, 0.88f, 0.80f, 1.0f) : ImVec4(0.62f, 0.62f, 0.57f, 1.0f));
    drawSidebarIcon(
        ImGui::GetWindowDrawList(),
        item.icon,
        ImVec2(rowStart.x + 22.0f, rowStart.y + rowSize.y * 0.5f),
        ImGui::ColorConvertFloat4ToU32(iconColor)
    );

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 39.0f);
    ImGui::TextUnformatted(item.label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 31.0f);
    ImGui::TextDisabled("%s", item.shortcut);

    if (clicked)
        state.selectedTool = item.value;

    if (hovered)
    {
        ImGui::SetTooltip("%s  [%s]", item.label, item.shortcut);
    }

    ImGui::PopID();
    return clicked;
}

static void drawSidebarSectionLabel(const char* label)
{
    ImGui::Spacing();
    ImGui::TextDisabled("%s", label);
    ImGui::Separator();
}

static void drawInterface()
{
    static SidebarState sidebar;
    static int currentFrame = 1;

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::Begin("Sovereign Workspace", nullptr, hostFlags);

    ImGuiID dockspace = ImGui::GetID("SovereignDockspace");
    ImGui::DockSpace(
        dockspace,
        ImVec2(0.0f, 0.0f),
        ImGuiDockNodeFlags_PassthruCentralNode
    );

    static bool layoutBuilt = false;
    static ImGuiID sideSplit = 0;

    if (!layoutBuilt)
    {
        ImGui::DockBuilderRemoveNode(dockspace);
        ImGui::DockBuilderAddNode(
            dockspace,
            ImGuiDockNodeFlags_DockSpace
        );

        ImGui::DockBuilderSetNodeSize(
            dockspace,
            viewport->WorkSize
        );

        ImGuiID center = dockspace;
        ImGuiID left = 0;
        ImGuiID right = 0;
        ImGuiID bottom = 0;
        ImGuiID rightTop = 0;

        ImGui::DockBuilderSplitNode(
            center,
            ImGuiDir_Left,
            0.145f,
            &left,
            &center
        );

        // This node owns the left/right boundary. Lock the boundary itself,
        // rather than the child panel, so dragging cannot swallow the viewport.
        sideSplit = center;
        ImGui::DockBuilderSplitNode(
            sideSplit,
            ImGuiDir_Right,
            0.245f,
            &right,
            &center
        );

        ImGui::DockBuilderSplitNode(
            center,
            ImGuiDir_Down,
            0.16f,
            &bottom,
            &center
        );

        ImGui::DockBuilderSplitNode(
            right,
            ImGuiDir_Up,
            0.46f,
            &rightTop,
            &right
        );

        // Keep the center viewport flexible while the side boundary stays fixed.

        ImGui::DockBuilderDockWindow("Tools", left);
        ImGui::DockBuilderDockWindow("Viewport", center);
        ImGui::DockBuilderDockWindow("Timeline", bottom);
        ImGui::DockBuilderDockWindow("Outliner", rightTop);
        ImGui::DockBuilderDockWindow("Properties", right);

        ImGui::DockBuilderFinish(dockspace);
        if (ImGuiDockNode* sideSplitNode = ImGui::DockBuilderGetNode(sideSplit))
            sideSplitNode->LocalFlags |= ImGuiDockNodeFlags_NoResize;
        layoutBuilt = true;
    }

    // Enforce the lock after ImGui restores or updates the dock tree.
    if (sideSplit != 0)
    {
        if (ImGuiDockNode* sideSplitNode = ImGui::DockBuilderGetNode(sideSplit))
        {
            sideSplitNode->LocalFlags |= ImGuiDockNodeFlags_NoResize;
            sideSplitNode->SizeRef.x = viewport->WorkSize.x * 0.755f;
        }
    }

    ImGui::End();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("New Case");
            ImGui::MenuItem("Open Case");
            ImGui::MenuItem("Save Case");
            ImGui::Separator();
            ImGui::MenuItem("Exit");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Tools");
            ImGui::MenuItem("Outliner");
            ImGui::MenuItem("Properties");
            ImGui::MenuItem("Timeline");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene"))
        {
            ImGui::MenuItem("Add Vehicle");
            ImGui::MenuItem("Add Evidence");
            ImGui::MenuItem("Add Measurement");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Analysis"))
        {
            ImGui::MenuItem("Skid Analysis");
            ImGui::MenuItem("Momentum Analysis");
            ImGui::MenuItem("Speed Analysis");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem("About Sovereign");
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // Left tool shelf
    ImGui::Begin("Tools");

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.055f, 0.057f, 0.060f, 1.0f));
    ImGui::BeginChild("SidebarContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding, 0);

    ImGui::TextColored(ImVec4(0.95f, 0.68f, 0.15f, 1.0f), "SOVEREIGN");
    ImGui::SameLine();
    ImGui::TextDisabled("/ TOOLS");
    ImGui::TextDisabled("ACCIDENT RECONSTRUCTOR");

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.095f, 0.075f, 1.0f));
    ImGui::BeginChild("SidebarContext", ImVec2(0.0f, 52.0f), true);
    ImGui::Text("CASE  /  UNTITLED");
    ImGui::TextDisabled("EDITOR FOUNDATION");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    drawSidebarSectionLabel("TRANSFORM");
    const SidebarTool transformTools[] = {
        {SidebarIcon::Select, "Select", "Q", 0},
        {SidebarIcon::Move, "Move", "W", 1},
        {SidebarIcon::Rotate, "Rotate", "E", 2},
        {SidebarIcon::Scale, "Scale", "R", 3},
    };
    for (const SidebarTool& item : transformTools)
        drawSidebarTool(item, sidebar);

    drawSidebarSectionLabel("SCENE ACTIONS");
    auto drawActionButton = [](const char* label, SidebarIcon icon)
    {
        ImGui::Button("  ", ImVec2(-1.0f, 32.0f));
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        drawSidebarIcon(ImGui::GetWindowDrawList(), icon, ImVec2(min.x + 22.0f, (min.y + max.y) * 0.5f), IM_COL32(180, 178, 166, 255));
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 39.0f);
        ImGui::TextUnformatted(label);
    };
    drawActionButton("Add Vehicle", SidebarIcon::Vehicle);
    drawActionButton("Add Evidence", SidebarIcon::Evidence);
    drawActionButton("Measure", SidebarIcon::Measure);

    drawSidebarSectionLabel("DISPLAY");
    ImGui::Checkbox("Grid", &sidebar.showGrid);
    ImGui::Checkbox("Axes", &sidebar.showAxes);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("SHORTCUTS  Q / W / E / R");
    ImGui::TextDisabled("READY  •  NO OBJECT SELECTED");

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();

    // Central viewport
    ImGui::Begin("Viewport");

    ImGui::Text("PERSPECTIVE");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::TextDisabled("CASE VIEW");

    ImGui::Separator();

    ImVec2 available = ImGui::GetContentRegionAvail();

    ImGui::BeginChild(
        "SceneCanvas",
        available,
        true,
        ImGuiWindowFlags_NoScrollbar
    );

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(
        canvasPos,
        ImVec2(
            canvasPos.x + canvasSize.x,
            canvasPos.y + canvasSize.y
        ),
        IM_COL32(18, 20, 22, 255)
    );

    if (sidebar.showGrid)
    {
        const float gridSize = 32.0f;

        for (float x = canvasPos.x; x < canvasPos.x + canvasSize.x; x += gridSize)
        {
            drawList->AddLine(
                ImVec2(x, canvasPos.y),
                ImVec2(x, canvasPos.y + canvasSize.y),
                IM_COL32(48, 48, 43, 255)
            );
        }

        for (float y = canvasPos.y; y < canvasPos.y + canvasSize.y; y += gridSize)
        {
            drawList->AddLine(
                ImVec2(canvasPos.x, y),
                ImVec2(canvasPos.x + canvasSize.x, y),
                IM_COL32(48, 48, 43, 255)
            );
        }
    }

    ImVec2 centerPoint(
        canvasPos.x + canvasSize.x * 0.50f,
        canvasPos.y + canvasSize.y * 0.50f
    );

    drawList->AddCircleFilled(
        centerPoint,
        7.0f,
        IM_COL32(238, 174, 38, 255)
    );

    if (sidebar.showAxes)
    {
        drawList->AddLine(
            centerPoint,
            ImVec2(centerPoint.x + 100.0f, centerPoint.y),
            IM_COL32(190, 65, 55, 255),
            2.0f
        );

        drawList->AddLine(
            centerPoint,
            ImVec2(centerPoint.x, centerPoint.y - 100.0f),
            IM_COL32(70, 145, 80, 255),
            2.0f
        );

        drawList->AddText(
            ImVec2(centerPoint.x + 105.0f, centerPoint.y - 10.0f),
            IM_COL32(220, 90, 75, 255),
            "X"
        );

        drawList->AddText(
            ImVec2(centerPoint.x + 7.0f, centerPoint.y - 120.0f),
            IM_COL32(100, 190, 110, 255),
            "Y"
        );
    }

    ImGui::SetCursorScreenPos(
        ImVec2(canvasPos.x + 16.0f, canvasPos.y + 16.0f)
    );

    ImGui::Text("SCENE VIEWPORT");
    ImGui::TextDisabled("No scene objects loaded");

    ImGui::EndChild();
    ImGui::End();

    // Right outliner: keep the right dock compact even when an old ImGui layout is restored.
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 160.0f), ImVec2(480.0f, 10000.0f));
    ImGui::Begin("Outliner");

    ImGui::TextColored(ImVec4(0.95f, 0.68f, 0.15f, 1.0f), "SCENE OUTLINER");
    ImGui::SameLine();
    ImGui::TextDisabled("/ 4 COLLECTIONS");
    ImGui::Separator();
    ImGui::TextDisabled("CASE HIERARCHY");
    ImGui::Spacing();

    auto drawOutlinerItem = [](const char* label, bool selected = false)
    {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.18f, 0.14f, 0.07f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.24f, 0.19f, 0.09f, 1.0f));
        ImGui::Selectable(label, selected, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, 25.0f));
        ImGui::PopStyleColor(2);
    };

    if (ImGui::TreeNodeEx("ENVIRONMENT", ImGuiTreeNodeFlags_DefaultOpen))
    {
        drawOutlinerItem("  Ground Plane");
        drawOutlinerItem("  Road Surface");
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("VEHICLES", ImGuiTreeNodeFlags_DefaultOpen))
    {
        drawOutlinerItem("  Vehicle A", true);
        drawOutlinerItem("  Vehicle B");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("EVIDENCE"))
    {
        drawOutlinerItem("  Skid Mark 01");
        drawOutlinerItem("  Marker 01");
        drawOutlinerItem("  Debris Field 01");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("MEASUREMENTS"))
    {
        drawOutlinerItem("  Distance 01");
        drawOutlinerItem("  Angle 01");
        ImGui::TreePop();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("ACTIVE COLLECTION  /  CASE DATA");

    ImGui::End();

    // Keep the lower right inspector in the same bounded dock.
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 160.0f), ImVec2(480.0f, 10000.0f));
    ImGui::Begin("Properties");

    ImGui::TextColored(ImVec4(0.95f, 0.68f, 0.15f, 1.0f), "INSPECTOR");
    ImGui::SameLine();
    ImGui::TextDisabled("/ PROPERTIES");
    ImGui::Separator();

    ImGui::TextDisabled("SELECTION");
    ImGui::Text("Nothing selected");
    ImGui::TextDisabled("Select an object in the viewport or outliner.");
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("TRANSFORM", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static float position[3] = { 0.0f, 0.0f, 0.0f };
        static float rotation[3] = { 0.0f, 0.0f, 0.0f };
        static float scale[3] = { 1.0f, 1.0f, 1.0f };

        auto drawTransformRow = [](const char* label, const char* id, float values[3], float speed, const char* format)
        {
            const float labelWidth = 70.0f;
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);
            ImGui::SameLine(labelWidth);
            ImGui::PushItemWidth(-1.0f);
            ImGui::DragFloat3(id, values, speed, 0.0f, 0.0f, format);
            ImGui::PopItemWidth();
        };

        drawTransformRow("Position", "##Position", position, 0.1f, "%.3f");
        drawTransformRow("Rotation", "##Rotation", rotation, 1.0f, "%.1f deg");
        drawTransformRow("Scale", "##Scale", scale, 0.01f, "%.3f");
    }

    if (ImGui::CollapsingHeader("METADATA"))
    {
        static char objectName[128] = "Untitled Object";
        ImGui::InputText("Name", objectName, sizeof(objectName));
        ImGui::TextDisabled("Type");
        ImGui::SameLine(110.0f);
        ImGui::Text("Scene Entity");
        ImGui::TextDisabled("Units");
        ImGui::SameLine(110.0f);
        ImGui::Text("meters");
    }

    if (ImGui::CollapsingHeader("ANALYSIS"))
    {
        ImGui::TextDisabled("No analysis data available.");
        ImGui::TextDisabled("Add evidence to begin.");
    }

    ImGui::End();

    // Bottom timeline
    ImGui::Begin("Timeline");

    ImGui::Text("TIMELINE");
    ImGui::SameLine();

    if (ImGui::Button("|<"))
        currentFrame = 1;

    ImGui::SameLine();

    if (ImGui::Button("Play"))
    {
    }

    ImGui::SameLine();

    if (ImGui::Button("Stop"))
        currentFrame = 1;

    ImGui::SameLine();

    ImGui::Text("Frame");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(100.0f);
    ImGui::DragInt("##Frame", &currentFrame, 1.0f, 1, 1000);

    ImGui::SameLine();
    ImGui::TextDisabled("Pre-impact  →  Impact  →  Rest");

    ImGui::Separator();

    ImGui::SetNextItemWidth(-1.0f);
    static float timelinePosition = 0.0f;
    ImGui::SliderFloat(
        "##TimelinePosition",
        &timelinePosition,
        0.0f,
        100.0f,
        "%.0f"
    );

    ImGui::End();
}

int main()
{
    if (!glfwInit())
    {
        std::printf("[FATAL] Failed to initialize GLFW.\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        "Sovereign Accident Reconstructor v0.1.0 — OpenGL 4.6",
        nullptr,
        nullptr
    );

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(
        reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    applySovereignTheme();

    io.FontGlobalScale = 1.0f;

    ImFont* rubik = io.Fonts->AddFontFromFileTTF(
        "assets/fonts/Rubik-Regular.ttf",
        18.0f
    );

    if (!rubik)
    {
        std::printf(
            "[ERROR] Could not load assets/fonts/Rubik-Regular.ttf\n"
        );
    }

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
    {
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 460"))
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    GLuint program = createShaderProgram();

    float vertices[] =
    {
         0.0f,  0.5f, 0.0f, 1.0f, 0.6f, 0.2f,
        -0.5f, -0.5f, 0.0f, 0.2f, 0.6f, 1.0f,
         0.5f, -0.5f, 0.0f, 0.2f, 0.8f, 0.4f
    };

    GLuint vao = 0;
    GLuint vbo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        nullptr
    );

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );

    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (rubik)
            ImGui::PushFont(rubik);

        drawInterface();

        if (rubik)
            ImGui::PopFont();

        glClearColor(0.025f, 0.027f, 0.030f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(
            ImGui::GetDrawData()
        );

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
