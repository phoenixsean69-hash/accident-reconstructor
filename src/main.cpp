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

struct SidebarTool
{
    const char* icon;
    const char* label;
    const char* shortcut;
    int value;
};

static bool drawSidebarTool(const SidebarTool& item, SidebarState& state)
{
    const bool active = state.selectedTool == item.value;
    const ImVec2 rowSize(-1.0f, 38.0f);
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
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(rowStart.x + 13.0f, rowStart.y + 10.0f),
        ImGui::ColorConvertFloat4ToU32(iconColor),
        item.icon
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
    ImGui::Spacing();
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
            0.19f,
            &left,
            &center
        );

        ImGui::DockBuilderSplitNode(
            center,
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

        ImGui::DockBuilderDockWindow("Tools", left);
        ImGui::DockBuilderDockWindow("Viewport", center);
        ImGui::DockBuilderDockWindow("Timeline", bottom);
        ImGui::DockBuilderDockWindow("Outliner", rightTop);
        ImGui::DockBuilderDockWindow("Properties", right);

        ImGui::DockBuilderFinish(dockspace);
        layoutBuilt = true;
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
    ImGui::BeginChild("SidebarContent", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysUseWindowPadding);

    ImGui::TextColored(ImVec4(0.95f, 0.68f, 0.15f, 1.0f), "SOVEREIGN");
    ImGui::SameLine();
    ImGui::TextDisabled("/ TOOLS");
    ImGui::TextDisabled("ACCIDENT RECONSTRUCTOR");

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.095f, 0.075f, 1.0f));
    ImGui::BeginChild("SidebarContext", ImVec2(0.0f, 42.0f), true);
    ImGui::Text("CASE  /  UNTITLED");
    ImGui::TextDisabled("EDITOR FOUNDATION");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    drawSidebarSectionLabel("TRANSFORM");
    const SidebarTool transformTools[] = {
        {"+", "Select", "Q", 0},
        {"↔", "Move", "W", 1},
        {"⟳", "Rotate", "E", 2},
        {"□", "Scale", "R", 3},
    };
    for (const SidebarTool& item : transformTools)
        drawSidebarTool(item, sidebar);

    drawSidebarSectionLabel("SCENE ACTIONS");
    if (ImGui::Button("+   Add Vehicle", ImVec2(-1.0f, 36.0f))) {}
    if (ImGui::Button("+   Add Evidence", ImVec2(-1.0f, 36.0f))) {}
    if (ImGui::Button("⌖   Measure", ImVec2(-1.0f, 36.0f))) {}

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

    // Right outliner
    ImGui::Begin("Outliner");

    ImGui::Text("SCENE OUTLINER");
    ImGui::Separator();

    if (ImGui::TreeNodeEx(
        "Environment",
        ImGuiTreeNodeFlags_DefaultOpen
    ))
    {
        ImGui::BulletText("Ground Plane");
        ImGui::BulletText("Road Surface");
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx(
        "Vehicles",
        ImGuiTreeNodeFlags_DefaultOpen
    ))
    {
        ImGui::Selectable("Vehicle A");
        ImGui::Selectable("Vehicle B");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Evidence"))
    {
        ImGui::Selectable("Skid Mark 01");
        ImGui::Selectable("Marker 01");
        ImGui::Selectable("Debris Field 01");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Measurements"))
    {
        ImGui::Selectable("Distance 01");
        ImGui::Selectable("Angle 01");
        ImGui::TreePop();
    }

    ImGui::End();

    // Right properties
    ImGui::Begin("Properties");

    ImGui::Text("INSPECTOR");
    ImGui::Separator();

    ImGui::TextDisabled("Nothing selected");

    ImGui::Spacing();

    if (ImGui::CollapsingHeader(
        "Transform",
        ImGuiTreeNodeFlags_DefaultOpen
    ))
    {
        static float position[3] = { 0.0f, 0.0f, 0.0f };
        static float rotation[3] = { 0.0f, 0.0f, 0.0f };
        static float scale[3] = { 1.0f, 1.0f, 1.0f };

        ImGui::DragFloat3("Position", position, 0.1f);
        ImGui::DragFloat3("Rotation", rotation, 1.0f);
        ImGui::DragFloat3("Scale", scale, 0.01f);
    }

    if (ImGui::CollapsingHeader("Metadata"))
    {
        static char objectName[128] = "Untitled Object";
        ImGui::InputText("Name", objectName, sizeof(objectName));
        ImGui::Text("Type: Scene Entity");
        ImGui::Text("Units: meters");
    }

    if (ImGui::CollapsingHeader("Analysis"))
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
