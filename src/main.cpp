#define GLFW_INCLUDE_NONE

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <dwmapi.h>
#endif

#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <glad/glad.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#ifndef SFE_ASSET_DIR
#define SFE_ASSET_DIR "assets"
#endif

#define NANOSVG_IMPLEMENTATION
#include "../third_party/nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../third_party/nanosvg/nanosvgrast.h"
#include <functional>
#include <sstream>
#include <iomanip>
#include <utility>

// ROADSAFE_CORE_MODEL_BINDING_V1
#include "roadsafe/roadsafe_core.h"
#include "roadsafe/roadsafe_renderer.h"
#include "roadsafe/roadsafe_asset_library.h"
// ROADSAFE_UI_COMPONENT_REFACTOR_V20
// ROADSAFE_UI_COMPONENT_REFACTOR_V21

constexpr int WINDOW_WIDTH = 1600;
constexpr int WINDOW_HEIGHT = 900;
constexpr float RIGHT_PANEL_MIN_WIDTH = 360.0f;
constexpr float RIGHT_PANEL_MAX_WIDTH = 520.0f;

// Central RoadSafe AR investigation/case state.
// Existing editor islands will be migrated into this model incrementally.
static roadsafe::RoadSafeCase gRoadSafeCase{};

// ROADSAFE_NATIVE_GLTF_VIEWPORT_V1
static roadsafe::RoadSafeRenderer gRoadSafeRenderer;

// ROADSAFE_ASSET_LIBRARY_BINDING_V1
static roadsafe::AssetLibraryState gRoadSafeAssetLibrary;
// ROADSAFE_EDITOR_RECORD_BINDING_V2
// Seed the existing reconstruction scene into the new case model.
// This only runs for a completely empty case and will not overwrite
// case data once real load/create workflows are connected.
static void initializeRoadSafeEditorRecords()
{
    static bool attempted=false;

    if (attempted)
        return;

    attempted=true;

    if (!gRoadSafeCase.vehicles.empty() ||
        !gRoadSafeCase.evidence.empty() ||
        !gRoadSafeCase.measurements.empty())
    {
        return;
    }

    roadsafe::VehicleRecord vehicleA;
    vehicleA.id="VEH-0001";
    vehicleA.make="Vehicle A";
    vehicleA.model="Unknown";
    vehicleA.vehicleType="Vehicle";
    vehicleA.massKg=1500.0f;
    vehicleA.wheelbaseMeters=2.70f;
    vehicleA.cgHeightMeters=0.55f;
    vehicleA.initialSpeedMetersPerSecond=12.0f;
    vehicleA.sceneHeadingDegrees=0.0f;
    vehicleA.steeringDegrees=0.0f;
    vehicleA.frictionCoefficient=0.70f;
    vehicleA.restitution=0.20f;
    vehicleA.crushDepthMeters=0.0f;
    vehicleA.lineage.provenance=
        roadsafe::Provenance::Observed;
    vehicleA.lineage.confidence=
        roadsafe::Confidence::Moderate;

    roadsafe::VehicleRecord vehicleB;
    vehicleB.id="VEH-0002";
    vehicleB.make="Vehicle B";
    vehicleB.model="Unknown";
    vehicleB.vehicleType="Vehicle";
    vehicleB.massKg=1250.0f;
    vehicleB.wheelbaseMeters=2.60f;
    vehicleB.cgHeightMeters=0.52f;
    vehicleB.initialSpeedMetersPerSecond=-8.0f;
    vehicleB.sceneHeadingDegrees=180.0f;
    vehicleB.steeringDegrees=0.0f;
    vehicleB.frictionCoefficient=0.70f;
    vehicleB.restitution=0.20f;
    vehicleB.crushDepthMeters=0.0f;
    vehicleB.lineage.provenance=
        roadsafe::Provenance::Observed;
    vehicleB.lineage.confidence=
        roadsafe::Confidence::Moderate;

    gRoadSafeCase.vehicles.push_back(vehicleA);
    gRoadSafeCase.vehicles.push_back(vehicleB);

    roadsafe::EvidenceRecord skid;
    skid.id="EV-0001";
    skid.type="Skid Mark";
    skid.description="Skid Mark 01";
    skid.collectionStatus="Observed";
    skid.lengthMeters=24.0f;
    skid.widthMeters=0.18f;
    skid.frictionCoefficient=0.70f;
    skid.categoryIndex=0;
    skid.confidenceScore=0.90f;
    skid.lineage.provenance=
        roadsafe::Provenance::Observed;
    skid.lineage.confidence=
        roadsafe::Confidence::High;
    skid.lineage.sourceId="Scene survey";

    roadsafe::EvidenceRecord marker;
    marker.id="EV-0002";
    marker.type="Scene Marker";
    marker.description="Marker 01";
    marker.collectionStatus="Observed";
    marker.categoryIndex=0;
    marker.confidenceScore=0.95f;
    marker.lineage.provenance=
        roadsafe::Provenance::Observed;
    marker.lineage.confidence=
        roadsafe::Confidence::High;
    marker.lineage.sourceId="Scene survey";

    roadsafe::EvidenceRecord debris;
    debris.id="EV-0003";
    debris.type="Debris Field";
    debris.description="Debris Field 01";
    debris.collectionStatus="Observed";
    debris.fieldRadiusMeters=2.50f;
    debris.estimatedPieceCount=12;
    debris.confidenceScore=0.80f;
    debris.lineage.provenance=
        roadsafe::Provenance::Observed;
    debris.lineage.confidence=
        roadsafe::Confidence::Moderate;
    debris.lineage.sourceId="Scene survey";

    gRoadSafeCase.evidence.push_back(skid);
    gRoadSafeCase.evidence.push_back(marker);
    gRoadSafeCase.evidence.push_back(debris);

    roadsafe::MeasurementRecord distance;
    distance.id="M-0001";
    distance.type="Distance";
    distance.value=12.50f;
    distance.unit="m";
    distance.method="Scene measurement";
    distance.sourceDescription="Scene survey";
    distance.unitSystem=0;
    distance.lineage.provenance=
        roadsafe::Provenance::Measured;
    distance.lineage.confidence=
        roadsafe::Confidence::High;

    roadsafe::MeasurementRecord angle;
    angle.id="M-0002";
    angle.type="Angle";
    angle.value=32.0f;
    angle.unit="deg";
    angle.method="Scene measurement";
    angle.sourceDescription="Scene survey";
    angle.unitSystem=0;
    angle.lineage.provenance=
        roadsafe::Provenance::Measured;
    angle.lineage.confidence=
        roadsafe::Confidence::High;

    gRoadSafeCase.measurements.push_back(distance);
    gRoadSafeCase.measurements.push_back(angle);

    // ROADSAFE_SCENE_ENTITY_BINDING_V2
    if (gRoadSafeCase.sceneEntities.empty())
    {
        const auto addSceneEntity=
            [](
                int legacyId,
                const char* id,
                roadsafe::SceneEntityKind kind,
                int recordIndex,
                const char* name,
                bool locked=false)
            {
                roadsafe::SceneEntityRecord entity;
                entity.id=id;
                entity.legacyId=legacyId;
                entity.kind=kind;
                entity.recordIndex=recordIndex;
                entity.name=name;
                entity.locked=locked;

                gRoadSafeCase.sceneEntities.push_back(
                    entity
                );
            };

        addSceneEntity(
            1,
            "SCN-0001",
            roadsafe::SceneEntityKind::Environment,
            -1,
            "Ground Plane"
        );

        addSceneEntity(
            2,
            "SCN-0002",
            roadsafe::SceneEntityKind::Environment,
            -1,
            "Road Surface",
            true
        );

        addSceneEntity(
            3,
            "SCN-0003",
            roadsafe::SceneEntityKind::Vehicle,
            0,
            "Vehicle A"
        );

        addSceneEntity(
            4,
            "SCN-0004",
            roadsafe::SceneEntityKind::Vehicle,
            1,
            "Vehicle B"
        );

        addSceneEntity(
            5,
            "SCN-0005",
            roadsafe::SceneEntityKind::Evidence,
            0,
            "Skid Mark 01"
        );

        addSceneEntity(
            6,
            "SCN-0006",
            roadsafe::SceneEntityKind::Evidence,
            1,
            "Marker 01"
        );

        addSceneEntity(
            7,
            "SCN-0007",
            roadsafe::SceneEntityKind::Evidence,
            2,
            "Debris Field 01"
        );

        addSceneEntity(
            8,
            "SCN-0008",
            roadsafe::SceneEntityKind::Measurement,
            0,
            "Distance 01"
        );

        addSceneEntity(
            9,
            "SCN-0009",
            roadsafe::SceneEntityKind::Measurement,
            1,
            "Angle 01"
        );
    }
    // Keep generated IDs safely beyond the seeded records.
    gRoadSafeCase.nextSequence=
        std::max<std::uint64_t>(
            gRoadSafeCase.nextSequence,
            10
        );

    // Seeding the built-in starter scene is not a user edit.
    gRoadSafeCase.revision=0;
}

static bool roadSafeInputText(
    const char* label,
    std::string& value,
    std::size_t capacity=128)
{
    const std::size_t bufferSize=
        std::max(
            capacity,
            value.size()+1
        );

    std::vector<char> buffer(
        bufferSize,
        0
    );

    std::snprintf(
        buffer.data(),
        buffer.size(),
        "%s",
        value.c_str()
    );

    const bool changed=
        ImGui::InputText(
            label,
            buffer.data(),
            buffer.size()
        );

    if (changed)
    {
        value=buffer.data();
        gRoadSafeCase.touch();
    }

    return changed;
}

// ROADSAFE_TYPE_AWARE_PROPERTY_HELPERS_V1
static bool roadSafeInputTextMultiline(
    const char* label,
    std::string& value,
    const ImVec2& size,
    std::size_t capacity=512)
{
    const std::size_t bufferSize=
        std::max(
            capacity,
            value.size()+1
        );

    std::vector<char> buffer(
        bufferSize,
        0
    );

    std::snprintf(
        buffer.data(),
        buffer.size(),
        "%s",
        value.c_str()
    );

    const bool changed=
        ImGui::InputTextMultiline(
            label,
            buffer.data(),
            buffer.size(),
            size
        );

    if (changed)
    {
        value=buffer.data();
        gRoadSafeCase.touch();
    }

    return changed;
}

static bool roadSafeProvenanceCombo(
    const char* label,
    roadsafe::Provenance& value)
{
    const char* names[]={
        "Unspecified",
        "Observed",
        "Measured",
        "Imported",
        "Witness Reported",
        "Calculated",
        "AI Derived",
        "Investigator Assumption",
        "Simulated"
    };

    int index=
        static_cast<int>(value);

    if (ImGui::Combo(
        label,
        &index,
        names,
        9))
    {
        value=
            static_cast<
                roadsafe::Provenance
            >(index);

        gRoadSafeCase.touch();
        return true;
    }

    return false;
}

static bool roadSafeConfidenceCombo(
    const char* label,
    roadsafe::Confidence& value)
{
    const char* names[]={
        "Unverified",
        "Low",
        "Moderate",
        "High",
        "Verified"
    };

    int index=
        static_cast<int>(value);

    if (ImGui::Combo(
        label,
        &index,
        names,
        5))
    {
        value=
            static_cast<
                roadsafe::Confidence
            >(index);

        gRoadSafeCase.touch();
        return true;
    }

    return false;
}
// Active domain counts remain correct after non-destructive deletes.
static std::size_t roadSafeActiveVehicleCount()
{
    return static_cast<std::size_t>(
        std::count_if(
            gRoadSafeCase.vehicles.begin(),
            gRoadSafeCase.vehicles.end(),
            [](const roadsafe::VehicleRecord& record)
            {
                return record.active;
            }
        )
    );
}

static std::size_t roadSafeActiveEvidenceCount()
{
    return static_cast<std::size_t>(
        std::count_if(
            gRoadSafeCase.evidence.begin(),
            gRoadSafeCase.evidence.end(),
            [](const roadsafe::EvidenceRecord& record)
            {
                return record.active;
            }
        )
    );
}

static std::size_t roadSafeActiveMeasurementCount()
{
    return static_cast<std::size_t>(
        std::count_if(
            gRoadSafeCase.measurements.begin(),
            gRoadSafeCase.measurements.end(),
            [](const roadsafe::MeasurementRecord& record)
            {
                return record.active;
            }
        )
    );
}
#ifdef _WIN32
static void applyNativeWindowTheme(GLFWwindow* window)
{
    HWND hwnd = glfwGetWin32Window(window);

    if (!hwnd)
        return;

    // Use numeric IDs so this also compiles with older Windows SDK headers.
    constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;
    constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_NEW = 20;
    constexpr DWORD DWMWA_CAPTION_COLOR_COMPAT = 35;
    constexpr DWORD DWMWA_TEXT_COLOR_COMPAT = 36;

    BOOL useDarkMode = TRUE;

    HRESULT darkResult = DwmSetWindowAttribute(
        hwnd,
        DWMWA_USE_IMMERSIVE_DARK_MODE_NEW,
        &useDarkMode,
        sizeof(useDarkMode)
    );

    if (FAILED(darkResult))
    {
        DwmSetWindowAttribute(
            hwnd,
            DWMWA_USE_IMMERSIVE_DARK_MODE_OLD,
            &useDarkMode,
            sizeof(useDarkMode)
        );
    }

    // On supported Windows 11 builds these make the native caption
    // visually match Sovereign's dark editor chrome.
    const COLORREF captionColor = RGB(24, 25, 28);
    const COLORREF captionText  = RGB(238, 240, 244);

    DwmSetWindowAttribute(
        hwnd,
        DWMWA_CAPTION_COLOR_COMPAT,
        &captionColor,
        sizeof(captionColor)
    );

    DwmSetWindowAttribute(
        hwnd,
        DWMWA_TEXT_COLOR_COMPAT,
        &captionText,
        sizeof(captionText)
    );

    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE |
        SWP_NOSIZE |
        SWP_NOZORDER |
        SWP_NOACTIVATE |
        SWP_FRAMECHANGED
    );
}
#endif

static std::array<GLuint, 9> gToolIcons{};

static std::filesystem::path assetPath(const char* relative)
{
    return std::filesystem::path(SFE_ASSET_DIR) / relative;
}

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
        std::fprintf(stderr, "[icons] Missing SVG: %s\n", path);
        return 0;
    }

    std::ifstream file(resolvedPath);
    if (!file) return 0;

    const std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    NSVGimage* image = nsvgParse(const_cast<char*>(source.c_str()), "px", 96.0f);
    if (!image) return 0;

    NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
    if (!rasterizer)
    {
        nsvgDelete(image);
        return 0;
    }

    const int width = std::max(1, static_cast<int>(image->width));
    const int height = std::max(1, static_cast<int>(image->height));
    std::vector<unsigned char> pixels(static_cast<size_t>(width * height * 4));
    nsvgRasterize(rasterizer, image, 0.0f, 0.0f, 1.0f, pixels.data(), width, height, width * 4);

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

static ImVec4 colorWindow() { return ImVec4(0.070f, 0.073f, 0.079f, 1.0f); }
static ImVec4 colorPanel() { return ImVec4(0.095f, 0.100f, 0.108f, 1.0f); }
static ImVec4 colorPanelRaised() { return ImVec4(0.125f, 0.132f, 0.143f, 1.0f); }
static ImVec4 colorBorder() { return ImVec4(0.245f, 0.260f, 0.285f, 1.0f); }
static ImVec4 colorAccent() { return ImVec4(0.98f, 0.68f, 0.08f, 1.0f); }
static ImVec4 colorAccentMuted() { return ImVec4(0.245f, 0.175f, 0.055f, 1.0f); }
static ImVec4 colorText() { return ImVec4(0.965f, 0.972f, 0.982f, 1.0f); }
static ImVec4 colorMuted() { return ImVec4(0.700f, 0.725f, 0.760f, 1.0f); }
static ImVec4 colorSuccess() { return ImVec4(0.48f, 0.84f, 0.46f, 1.0f); }

static void applySovereignTheme()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    // SOVEREIGN_REAL_DOCK_TABS_V1

    style.WindowRounding = 2.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    style.WindowPadding = ImVec2(10.0f, 8.0f);
    style.FramePadding = ImVec2(7.0f, 5.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.CellPadding = ImVec2(10.0f, 7.0f);
    style.ScrollbarSize = 12.0f;

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = colorText();
    c[ImGuiCol_TextDisabled] = colorMuted();
    c[ImGuiCol_WindowBg] = colorWindow();
    c[ImGuiCol_ChildBg] = colorPanel();
    c[ImGuiCol_PopupBg] = colorPanelRaised();
    c[ImGuiCol_Border] = ImVec4(0.0f,0.0f,0.0f,0.0f);
    c[ImGuiCol_BorderShadow] = ImVec4(0.0f,0.0f,0.0f,0.12f);
    c[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.31f, 0.31f, 0.33f, 1.00f);
    c[ImGuiCol_TitleBg] = ImVec4(0.090f, 0.095f, 0.103f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.125f, 0.132f, 0.145f, 1.0f);
    c[ImGuiCol_MenuBarBg] = ImVec4(0.090f, 0.095f, 0.104f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.46f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.50f, 0.53f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.36f, 0.51f, 0.76f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(0.18f, 0.22f, 0.30f, 0.90f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.31f, 0.42f, 1.00f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.21f, 0.36f, 0.55f, 1.00f);
    c[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
    c[ImGuiCol_SeparatorHovered] = ImVec4(0.40f,0.42f,0.45f,1.0f);
    c[ImGuiCol_SeparatorActive] = colorAccent();
    c[ImGuiCol_CheckMark] = colorAccent();
    c[ImGuiCol_SliderGrab] = ImVec4(0.70f,0.48f,0.07f,1.0f);
    c[ImGuiCol_SliderGrabActive] = colorAccent();
    c[ImGuiCol_Tab] = ImVec4(0.070f, 0.074f, 0.082f, 1.0f);
    c[ImGuiCol_TabHovered] = ImVec4(0.120f, 0.126f, 0.140f, 1.0f);
    c[ImGuiCol_TabActive] = ImVec4(0.185f, 0.194f, 0.214f, 1.0f);
    c[ImGuiCol_TabUnfocused] = ImVec4(0.058f, 0.061f, 0.068f, 1.0f);
    c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.145f, 0.151f, 0.166f, 1.0f);
    c[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(0.090f, 0.095f, 0.105f, 1.0f);
    c[ImGuiCol_DockingPreview] = ImVec4(0.94f,0.64f,0.06f,0.28f);
}

// ROADSAFE_BLENDER_UI_V19
// ROADSAFE_BLENDER_UI_V18
// ROADSAFE_BLENDER_UI_V15
// ROADSAFE_GOOGLE_ICON_WIRING_V14
// ROADSAFE_GOOGLE_ICON_CATALOG_V12
enum class UiGlyph
{
    Folder,
    Hash,
    Calendar,
    Pin,
    Clock,
    Cube,
    Document,
    Bars,
    Pie,
    Image,
    Check,
    Link,
    Ruler,
    Marker,
    Speed,
    Momentum,
    Report,
    Info,
    Eye,
    Lock,
    Unlock,
    Target,
    More,
    Home,
    Dashboard,
    Menu,
    Apps,
    Back,
    Forward,
    Previous,
    Next,
    Up,
    Down,
    ExpandMore,
    ExpandLess,
    ChevronLeft,
    ChevronRight,
    FirstPage,
    LastPage,
    Fullscreen,
    FullscreenExit,
    OpenInNew,
    Close,
    Cancel,
    Refresh,
    Restart,
    Sync,
    Search,
    Filter,
    FilterOff,
    Sort,
    Tune,
    Settings,
    Preferences,
    ViewSidebar,
    Dock,
    Panel,
    SplitView,
    Layout,
    ResetLayout,
    NewFile,
    NewCase,
    OpenFile,
    OpenRecent,
    Save,
    SaveAs,
    Import,
    Export,
    Download,
    Upload,
    Package,
    Archive,
    Unarchive,
    Exit,
    Print,
    Share,
    Cloud,
    CloudUpload,
    CloudDownload,
    Backup,
    Restore,
    History,
    Revision,
    Add,
    AddCircle,
    Remove,
    Delete,
    DeleteSweep,
    Edit,
    Rename,
    Undo,
    Redo,
    Cut,
    Copy,
    Paste,
    Duplicate,
    SelectAll,
    Deselect,
    Clear,
    ClearAll,
    Drag,
    Move,
    Rotate,
    Scale,
    Transform,
    AlignLeft,
    AlignCenter,
    AlignRight,
    Snap,
    Grid,
    GridOff,
    Layers,
    Group,
    Ungroup,
    Scene,
    SceneMap,
    Viewport,
    View2D,
    View3D,
    Perspective,
    TopView,
    FrontView,
    RightView,
    FitScreen,
    FrameAll,
    FrameSelection,
    CenterView,
    ZoomIn,
    ZoomOut,
    Pan,
    Orbit,
    Camera,
    CameraFront,
    CameraRear,
    Screenshot,
    Snapshot,
    Lit,
    Wireframe,
    Shadows,
    Overlay,
    Bounds,
    Axes,
    ObjectNames,
    Statistics,
    SafeFrame,
    Origin,
    ResetOrigin,
    AR,
    ARPreview,
    Device,
    ConnectDevice,
    PairDevice,
    Bluetooth,
    Phone,
    Tablet,
    Tracking,
    Anchor,
    AddAnchor,
    ClearAnchors,
    Reticle,
    Planes,
    Occlusion,
    CollisionGuide,
    SessionStart,
    SessionStop,
    Record,
    StopRecord,
    Preview,
    Timeline,
    Play,
    Pause,
    Stop,
    SkipPrevious,
    SkipNext,
    FastRewind,
    FastForward,
    Replay,
    PlayCircle,
    PauseCircle,
    StopCircle,
    MarkerAdd,
    MarkerClear,
    Playhead,
    Vehicle,
    Car,
    Truck,
    Bus,
    Taxi,
    Motorcycle,
    Bicycle,
    Traffic,
    TrafficLight,
    Road,
    Highway,
    Route,
    Direction,
    Steering,
    Wheel,
    Tire,
    Fuel,
    Engine,
    VehicleIdentity,
    VehiclePhysics,
    Mass,
    Balance,
    Crush,
    Crash,
    Impact,
    Friction,
    Evidence,
    EvidenceAdd,
    EvidenceLink,
    EvidenceUnlink,
    Photo,
    PhotoCamera,
    PhotoLibrary,
    SkidMark,
    Debris,
    Glass,
    Fluid,
    Gouge,
    ForensicMarker,
    Fingerprint,
    Source,
    Provenance,
    Verified,
    Confidence,
    Unverified,
    Observed,
    Measured,
    Calculated,
    Imported,
    Simulated,
    AI,
    Witness,
    Statement,
    CaseValidate,
    CaseWarning,
    CaseError,
    CaseSuccess,
    Measurement,
    Distance,
    Angle,
    Area,
    Height,
    Width,
    Radius,
    Coordinates,
    GPS,
    Compass,
    Heading,
    Location,
    Datum,
    ScaleMeasure,
    Units,
    Analysis,
    Analytics,
    Chart,
    LineChart,
    AreaChart,
    PieChart,
    Scatter,
    SpeedAnalysis,
    MomentumAnalysis,
    SkidAnalysis,
    Trajectory,
    LineOfSight,
    Calculate,
    Science,
    Experiment,
    Module,
    Workflow,
    Result,
    Results,
    Findings,
    Hypothesis,
    Simulation,
    Run,
    RunAll,
    Reconstruct,
    ReportView,
    ReportExport,
    NodeEditor,
    Node,
    AddNode,
    RunGraph,
    Graph,
    InputNode,
    OutputNode,
    LinkNode,
    DeleteLink,
    CenterGraph,
    GridGraph,
    SnapGraph,
    SaveGraph,
    LoadGraph,
    Assets,
    AssetLibrary,
    Asset,
    Assign,
    Unassign,
    Material,
    Texture,
    PBR,
    Model,
    ImportAsset,
    RefreshAssets,
    CopyUrl,
    ExternalLink,
    Category,
    License,
    Person,
    People,
    Driver,
    Pedestrian,
    Cyclist,
    Officer,
    Building,
    Environment,
    Terrain,
    Tree,
    Grass,
    Rock,
    Water,
    Weather,
    Lighting,
    StreetFurniture,
    StreetLight,
    Barrier,
    Cone,
    Sign,
    Success,
    Warning,
    Error,
    Help,
    HelpOutline,
    Keyboard,
    Shortcut,
    Command,
    Terminal,
    Palette,
    Notification,
    Star,
    Favorite,
    Flag,
    Label,
    Tag,
    Badge,
    StatusOnline,
    StatusOffline,
    VisibilityOff,
    LockPerson,
    Shield,
    Security,
    Admin,
    About,
    Documentation
};

enum class StatusTone { Neutral, Accent, Success };

static ImU32 toU32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

static ImVec4 toneColor(StatusTone tone)
{
    switch (tone)
    {
        case StatusTone::Accent: return colorAccent();
        case StatusTone::Success: return colorSuccess();
        default: return colorMuted();
    }
}

static void drawLegacyGlyph(ImDrawList* d, UiGlyph glyph, const ImVec2& center, float size, ImU32 color)
{
    const float s=size, x=center.x, y=center.y, t=std::max(1.0f,size*0.085f);

    switch (glyph)
    {
        case UiGlyph::Folder:
            d->AddRect(ImVec2(x-s*.44f,y-s*.24f),ImVec2(x+s*.44f,y+s*.30f),color,2.0f,0,t);
            d->AddLine(ImVec2(x-s*.38f,y-s*.24f),ImVec2(x-s*.16f,y-s*.42f),color,t);
            d->AddLine(ImVec2(x-s*.16f,y-s*.42f),ImVec2(x+s*.08f,y-s*.42f),color,t);
            break;
        case UiGlyph::Hash:
            d->AddLine(ImVec2(x-s*.18f,y-s*.42f),ImVec2(x-s*.28f,y+s*.42f),color,t);
            d->AddLine(ImVec2(x+s*.18f,y-s*.42f),ImVec2(x+s*.08f,y+s*.42f),color,t);
            d->AddLine(ImVec2(x-s*.40f,y-s*.12f),ImVec2(x+s*.38f,y-s*.12f),color,t);
            d->AddLine(ImVec2(x-s*.42f,y+s*.18f),ImVec2(x+s*.36f,y+s*.18f),color,t);
            break;
        case UiGlyph::Calendar:
            d->AddRect(ImVec2(x-s*.40f,y-s*.34f),ImVec2(x+s*.40f,y+s*.38f),color,2.0f,0,t);
            d->AddLine(ImVec2(x-s*.40f,y-s*.12f),ImVec2(x+s*.40f,y-s*.12f),color,t);
            d->AddLine(ImVec2(x-s*.20f,y-s*.45f),ImVec2(x-s*.20f,y-s*.25f),color,t);
            d->AddLine(ImVec2(x+s*.20f,y-s*.45f),ImVec2(x+s*.20f,y-s*.25f),color,t);
            break;
        case UiGlyph::Pin:
        case UiGlyph::Marker:
            d->AddCircle(ImVec2(x,y-s*.12f),s*.26f,color,24,t);
            d->AddCircleFilled(ImVec2(x,y-s*.12f),s*.07f,color);
            d->AddLine(ImVec2(x-s*.15f,y+s*.10f),ImVec2(x,y+s*.42f),color,t);
            d->AddLine(ImVec2(x+s*.15f,y+s*.10f),ImVec2(x,y+s*.42f),color,t);
            break;
        case UiGlyph::Clock:
            d->AddCircle(center,s*.38f,color,28,t);
            d->AddLine(center,ImVec2(x,y-s*.22f),color,t);
            d->AddLine(center,ImVec2(x+s*.19f,y+s*.12f),color,t);
            break;
        case UiGlyph::Cube:
            d->AddRect(ImVec2(x-s*.30f,y-s*.30f),ImVec2(x+s*.30f,y+s*.30f),color,1.0f,0,t);
            d->AddLine(ImVec2(x-s*.30f,y-s*.30f),ImVec2(x,y-s*.46f),color,t);
            d->AddLine(ImVec2(x+s*.30f,y-s*.30f),ImVec2(x,y-s*.46f),color,t);
            d->AddLine(ImVec2(x,y-s*.46f),ImVec2(x,y+s*.18f),color,t);
            break;
        case UiGlyph::Document:
        case UiGlyph::Report:
            d->AddRect(ImVec2(x-s*.30f,y-s*.40f),ImVec2(x+s*.30f,y+s*.40f),color,1.0f,0,t);
            d->AddLine(ImVec2(x-s*.16f,y-s*.12f),ImVec2(x+s*.16f,y-s*.12f),color,t);
            d->AddLine(ImVec2(x-s*.16f,y+s*.06f),ImVec2(x+s*.16f,y+s*.06f),color,t);
            d->AddLine(ImVec2(x-s*.16f,y+s*.24f),ImVec2(x+s*.10f,y+s*.24f),color,t);
            break;
        case UiGlyph::Bars:
            d->AddRectFilled(ImVec2(x-s*.34f,y+s*.06f),ImVec2(x-s*.20f,y+s*.38f),color);
            d->AddRectFilled(ImVec2(x-s*.07f,y-s*.16f),ImVec2(x+s*.07f,y+s*.38f),color);
            d->AddRectFilled(ImVec2(x+s*.20f,y-s*.38f),ImVec2(x+s*.34f,y+s*.38f),color);
            break;
        case UiGlyph::Pie:
            d->AddCircle(center,s*.36f,color,28,t);
            d->AddLine(center,ImVec2(x,y-s*.36f),color,t);
            d->AddLine(center,ImVec2(x+s*.30f,y+s*.20f),color,t);
            break;
        case UiGlyph::Image:
            d->AddRect(ImVec2(x-s*.42f,y-s*.32f),ImVec2(x+s*.42f,y+s*.32f),color,2.0f,0,t);
            d->AddCircleFilled(ImVec2(x+s*.20f,y-s*.12f),s*.07f,color);
            d->AddLine(ImVec2(x-s*.34f,y+s*.18f),ImVec2(x-s*.08f,y-s*.02f),color,t);
            d->AddLine(ImVec2(x-s*.08f,y-s*.02f),ImVec2(x+s*.10f,y+s*.14f),color,t);
            d->AddLine(ImVec2(x+s*.10f,y+s*.14f),ImVec2(x+s*.30f,y-s*.02f),color,t);
            break;
        case UiGlyph::Check:
            d->AddCircle(center,s*.36f,color,28,t);
            d->AddLine(ImVec2(x-s*.18f,y),ImVec2(x-s*.04f,y+s*.15f),color,t);
            d->AddLine(ImVec2(x-s*.04f,y+s*.15f),ImVec2(x+s*.20f,y-s*.16f),color,t);
            break;
        case UiGlyph::Link:
            d->AddCircle(ImVec2(x-s*.14f,y),s*.22f,color,20,t);
            d->AddCircle(ImVec2(x+s*.14f,y),s*.22f,color,20,t);
            d->AddLine(ImVec2(x-s*.06f,y),ImVec2(x+s*.06f,y),color,t);
            break;
        case UiGlyph::Ruler:
            d->AddLine(ImVec2(x-s*.36f,y+s*.28f),ImVec2(x+s*.36f,y-s*.28f),color,t*1.2f);
            for (int i=-2;i<=2;++i)
            {
                const float px=x+i*s*.12f, py=y-i*s*.09f;
                d->AddLine(ImVec2(px,py),ImVec2(px+s*.08f,py+s*.10f),color,t);
            }
            break;
        case UiGlyph::Speed:
            d->AddCircle(center,s*.36f,color,28,t);
            d->AddLine(center,ImVec2(x+s*.23f,y-s*.18f),color,t);
            d->AddCircleFilled(center,s*.05f,color);
            break;
        case UiGlyph::Momentum:
            d->AddLine(ImVec2(x-s*.34f,y),ImVec2(x+s*.02f,y),color,t);
            d->AddTriangleFilled(ImVec2(x+s*.02f,y),ImVec2(x-s*.10f,y-s*.12f),ImVec2(x-s*.10f,y+s*.12f),color);
            d->AddLine(ImVec2(x+s*.34f,y),ImVec2(x-s*.02f,y),color,t);
            d->AddTriangleFilled(ImVec2(x-s*.02f,y),ImVec2(x+s*.10f,y-s*.12f),ImVec2(x+s*.10f,y+s*.12f),color);
            break;
        case UiGlyph::Eye:
            d->AddBezierCubic(
                ImVec2(x-s*.40f,y),
                ImVec2(x-s*.18f,y-s*.28f),
                ImVec2(x+s*.18f,y-s*.28f),
                ImVec2(x+s*.40f,y),
                color,
                t
            );
            d->AddBezierCubic(
                ImVec2(x-s*.40f,y),
                ImVec2(x-s*.18f,y+s*.28f),
                ImVec2(x+s*.18f,y+s*.28f),
                ImVec2(x+s*.40f,y),
                color,
                t
            );
            d->AddCircleFilled(center,s*.09f,color);
            break;

        case UiGlyph::Target:
            d->AddCircle(center,s*.34f,color,24,t);
            d->AddCircle(center,s*.17f,color,20,t);
            d->AddCircleFilled(center,s*.045f,color);

            d->AddLine(
                ImVec2(x-s*.46f,y),
                ImVec2(x-s*.28f,y),
                color,
                t
            );

            d->AddLine(
                ImVec2(x+s*.28f,y),
                ImVec2(x+s*.46f,y),
                color,
                t
            );

            d->AddLine(
                ImVec2(x,y-s*.46f),
                ImVec2(x,y-s*.28f),
                color,
                t
            );

            d->AddLine(
                ImVec2(x,y+s*.28f),
                ImVec2(x,y+s*.46f),
                color,
                t
            );
            break;

        case UiGlyph::More:
            d->AddCircleFilled(
                ImVec2(x-s*.22f,y),
                s*.055f,
                color
            );

            d->AddCircleFilled(
                ImVec2(x,y),
                s*.055f,
                color
            );

            d->AddCircleFilled(
                ImVec2(x+s*.22f,y),
                s*.055f,
                color
            );
            break;
        case UiGlyph::Unlock:
            d->AddRect(
                ImVec2(x-s*.28f,y-s*.02f),
                ImVec2(x+s*.28f,y+s*.34f),
                color,
                2.0f,
                0,
                t
            );

            d->AddCircleFilled(
                ImVec2(x,y+s*.14f),
                s*.045f,
                color
            );

            d->AddBezierCubic(
                ImVec2(x-s*.20f,y-s*.02f),
                ImVec2(x-s*.28f,y-s*.18f),
                ImVec2(x-s*.12f,y-s*.34f),
                ImVec2(x+s*.08f,y-s*.26f),
                color,
                t
            );

            d->AddLine(
                ImVec2(x+s*.08f,y-s*.26f),
                ImVec2(x+s*.22f,y-s*.18f),
                color,
                t
            );
            break;
        case UiGlyph::Lock:
            d->AddRect(
                ImVec2(x-s*.28f,y-s*.02f),
                ImVec2(x+s*.28f,y+s*.34f),
                color,
                2.0f,
                0,
                t
            );

            d->AddCircleFilled(
                ImVec2(x,y+s*.14f),
                s*.045f,
                color
            );

            d->AddBezierCubic(
                ImVec2(x-s*.18f,y-s*.02f),
                ImVec2(x-s*.18f,y-s*.28f),
                ImVec2(x+s*.18f,y-s*.28f),
                ImVec2(x+s*.18f,y-s*.02f),
                color,
                t
            );
            break;
        case UiGlyph::Info:
            d->AddCircle(center,s*.36f,color,28,t);
            d->AddCircleFilled(ImVec2(x,y-s*.16f),s*.04f,color);
            d->AddLine(ImVec2(x,y-s*.02f),ImVec2(x,y+s*.20f),color,t);
            break;
    }
}

// ROADSAFE_GOOGLE_MATERIAL_SYMBOLS_V1
// Google Material Symbols Rounded replaces RoadSafe's hand-drawn glyphs
// through the central UiGlyph path. The legacy renderer remains as a
// runtime fallback if the external font asset cannot be loaded.
static ImFont* gGoogleMaterialIconFont=nullptr;

static const ImWchar gGoogleMaterialIconRanges[]={
    0xE00D, 0xE00D,
    0xE00F, 0xE00F,
    0xE010, 0xE010,
    0xE01F, 0xE01F,
    0xE020, 0xE020,
    0xE034, 0xE034,
    0xE037, 0xE037,
    0xE042, 0xE042,
    0xE044, 0xE044,
    0xE045, 0xE045,
    0xE047, 0xE047,
    0xE061, 0xE061,
    0xE0B8, 0xE0B8,
    0xE0C9, 0xE0C9,
    0xE13D, 0xE13D,
    0xE145, 0xE145,
    0xE146, 0xE146,
    0xE149, 0xE149,
    0xE14D, 0xE14D,
    0xE14E, 0xE14E,
    0xE14F, 0xE14F,
    0xE15A, 0xE15A,
    0xE15B, 0xE15B,
    0xE161, 0xE161,
    0xE162, 0xE162,
    0xE164, 0xE164,
    0xE166, 0xE166,
    0xE169, 0xE169,
    0xE16C, 0xE16C,
    0xE16F, 0xE16F,
    0xE178, 0xE178,
    0xE1A1, 0xE1A1,
    0xE1A2, 0xE1A2,
    0xE1A7, 0xE1A7,
    0xE1A8, 0xE1A8,
    0xE1C4, 0xE1C4,
    0xE250, 0xE250,
    0xE259, 0xE259,
    0xE25A, 0xE25A,
    0xE268, 0xE268,
    0xE26B, 0xE26B,
    0xE2C0, 0xE2C0,
    0xE2C3, 0xE2C3,
    0xE2C7, 0xE2C7,
    0xE2C8, 0xE2C8,
    0xE2CC, 0xE2CC,
    0xE2E6, 0xE2E6,
    0xE312, 0xE312,
    0xE326, 0xE326,
    0xE32A, 0xE32A,
    0xE32F, 0xE32F,
    0xE3AD, 0xE3AD,
    0xE3B4, 0xE3B4,
    0xE3B5, 0xE3B5,
    0xE3BB, 0xE3BB,
    0xE3BC, 0xE3BC,
    0xE3C2, 0xE3C2,
    0xE3EB, 0xE3EB,
    0xE3EC, 0xE3EC,
    0xE3F4, 0xE3F4,
    0xE412, 0xE412,
    0xE413, 0xE413,
    0xE415, 0xE415,
    0xE41A, 0xE41A,
    0xE41C, 0xE41C,
    0xE421, 0xE421,
    0xE429, 0xE429,
    0xE430, 0xE430,
    0xE4FC, 0xE4FC,
    0xE4FD, 0xE4FD,
    0xE518, 0xE518,
    0xE52E, 0xE52E,
    0xE536, 0xE536,
    0xE53B, 0xE53B,
    0xE546, 0xE546,
    0xE558, 0xE558,
    0xE559, 0xE559,
    0xE55B, 0xE55B,
    0xE55C, 0xE55C,
    0xE55E, 0xE55E,
    0xE564, 0xE564,
    0xE565, 0xE565,
    0xE5C3, 0xE5C3,
    0xE5C4, 0xE5C4,
    0xE5C8, 0xE5C8,
    0xE5CB, 0xE5CB,
    0xE5CC, 0xE5CC,
    0xE5CD, 0xE5CD,
    0xE5CE, 0xE5CE,
    0xE5CF, 0xE5CF,
    0xE5D0, 0xE5D0,
    0xE5D1, 0xE5D1,
    0xE5D2, 0xE5D2,
    0xE5D3, 0xE5D3,
    0xE5D5, 0xE5D5,
    0xE5D8, 0xE5D8,
    0xE5DB, 0xE5DB,
    0xE5DC, 0xE5DC,
    0xE5DD, 0xE5DD,
    0xE627, 0xE627,
    0xE637, 0xE637,
    0xE63E, 0xE63E,
    0xE648, 0xE648,
    0xE65F, 0xE65F,
    0xE693, 0xE693,
    0xE6E1, 0xE6E1,
    0xE72C, 0xE72C,
    0xE770, 0xE770,
    0xE798, 0xE798,
    0xE7BA, 0xE7BA,
    0xE7F5, 0xE7F5,
    0xE80D, 0xE80D,
    0xE836, 0xE836,
    0xE84D, 0xE84D,
    0xE85B, 0xE85B,
    0xE862, 0xE862,
    0xE864, 0xE864,
    0xE871, 0xE871,
    0xE873, 0xE873,
    0xE87A, 0xE87A,
    0xE87B, 0xE87B,
    0xE87E, 0xE87E,
    0xE886, 0xE886,
    0xE888, 0xE888,
    0xE88E, 0xE88E,
    0xE890, 0xE890,
    0xE893, 0xE893,
    0xE898, 0xE898,
    0xE899, 0xE899,
    0xE89C, 0xE89C,
    0xE89E, 0xE89E,
    0xE89F, 0xE89F,
    0xE8AD, 0xE8AD,
    0xE8B3, 0xE8B3,
    0xE8B8, 0xE8B8,
    0xE8D4, 0xE8D4,
    0xE8E1, 0xE8E1,
    0xE8E9, 0xE8E9,
    0xE8F0, 0xE8F0,
    0xE8F1, 0xE8F1,
    0xE8F4, 0xE8F4,
    0xE8F5, 0xE8F5,
    0xE8FD, 0xE8FD,
    0xE8FF, 0xE8FF,
    0xE900, 0xE900,
    0xE90D, 0xE90D,
    0xE90E, 0xE90E,
    0xE90F, 0xE90F,
    0xE915, 0xE915,
    0xE917, 0xE917,
    0xE91F, 0xE91F,
    0xE922, 0xE922,
    0xE925, 0xE925,
    0xE92E, 0xE92E,
    0xE945, 0xE945,
    0xE949, 0xE949,
    0xE94D, 0xE94D,
    0xE97A, 0xE97A,
    0xE990, 0xE990,
    0xE99B, 0xE99B,
    0xE9A2, 0xE9A2,
    0xE9B2, 0xE9B2,
    0xE9BA, 0xE9BA,
    0xE9E0, 0xE9E0,
    0xE9E4, 0xE9E4,
    0xE9EF, 0xE9EF,
    0xE9F9, 0xE9F9,
    0xEA10, 0xEA10,
    0xEA16, 0xEA16,
    0xEA19, 0xEA19,
    0xEA3A, 0xEA3A,
    0xEA3B, 0xEA3B,
    0xEA3E, 0xEA3E,
    0xEA40, 0xEA40,
    0xEA49, 0xEA49,
    0xEA4A, 0xEA4A,
    0xEA4B, 0xEA4B,
    0xEA5F, 0xEA5F,
    0xEA63, 0xEA63,
    0xEA67, 0xEA67,
    0xEACD, 0xEACD,
    0xEAE7, 0xEAE7,
    0xEAF6, 0xEAF6,
    0xEB29, 0xEB29,
    0xEB32, 0xEB32,
    0xEB60, 0xEB60,
    0xEB8E, 0xEB8E,
    0xEB91, 0xEB91,
    0xEBAB, 0xEBAB,
    0xEBB6, 0xEBB6,
    0xEBBE, 0xEBBE,
    0xEBC8, 0xEBC8,
    0xEBCC, 0xEBCC,
    0xEBF2, 0xEBF2,
    0xEF3A, 0xEF3A,
    0xEF3B, 0xEF3B,
    0xEF3D, 0xEF3D,
    0xEF3E, 0xEF3E,
    0xEF42, 0xEF42,
    0xEF4A, 0xEF4A,
    0xEF4F, 0xEF4F,
    0xEF56, 0xEF56,
    0xEF71, 0xEF71,
    0xEF76, 0xEF76,
    0xEF7A, 0xEF7A,
    0xEFC9, 0xEFC9,
    0xEFD6, 0xEFD6,
    0xEFEE, 0xEFEE,
    0xEFF6, 0xEFF6,
    0xEFF7, 0xEFF7,
    0xF013, 0xF013,
    0xF016, 0xF016,
    0xF053, 0xF053,
    0xF056, 0xF056,
    0xF05E, 0xF05E,
    0xF06C, 0xF06C,
    0xF083, 0xF083,
    0xF084, 0xF084,
    0xF08C, 0xF08C,
    0xF090, 0xF090,
    0xF097, 0xF097,
    0xF09A, 0xF09A,
    0xF09B, 0xF09B,
    0xF0BE, 0xF0BE,
    0xF0C5, 0xF0C5,
    0xF0C6, 0xF0C6,
    0xF0CC, 0xF0CC,
    0xF0CF, 0xF0CF,
    0xF0D3, 0xF0D3,
    0xF0DA, 0xF0DA,
    0xF114, 0xF114,
    0xF15C, 0xF15C,
    0xF17D, 0xF17D,
    0xF1C5, 0xF1C5,
    0xF1C8, 0xF1C8,
    0xF1CD, 0xF1CD,
    0xF1DB, 0xF1DB,
    0xF1DF, 0xF1DF,
    0xF205, 0xF205,
    0xF233, 0xF233,
    0xF2C8, 0xF2C8,
    0xF2C9, 0xF2C9,
    0xF2E0, 0xF2E0,
    0xF720, 0xF720,
    0xF8B6, 0xF8B6,
    0xF8F3, 0xF8F3,
    0
};

static unsigned int googleMaterialCodepoint(
    UiGlyph glyph)
{
    switch (glyph)
    {
        case UiGlyph::Folder: return 0xE2C7;
        case UiGlyph::Hash: return 0xE9EF;
        case UiGlyph::Calendar: return 0xEBCC;
        case UiGlyph::Pin: return 0xF1DB;
        case UiGlyph::Clock: return 0xEFD6;
        case UiGlyph::Cube: return 0xEFC9;
        case UiGlyph::Document: return 0xE873;
        case UiGlyph::Bars: return 0xEF3E;
        case UiGlyph::Pie: return 0xE917;
        case UiGlyph::Image: return 0xE3F4;
        case UiGlyph::Check: return 0xF0BE;
        case UiGlyph::Link: return 0xE250;
        case UiGlyph::Ruler: return 0xE41C;
        case UiGlyph::Marker: return 0xE55E;
        case UiGlyph::Speed: return 0xE9E4;
        case UiGlyph::Momentum: return 0xE915;
        case UiGlyph::Report: return 0xEF42;
        case UiGlyph::Info: return 0xE88E;
        case UiGlyph::Eye: return 0xE8F4;
        case UiGlyph::Lock: return 0xE899;
        case UiGlyph::Unlock: return 0xE898;
        case UiGlyph::Target: return 0xE3B4;
        case UiGlyph::More: return 0xE5D3;
        case UiGlyph::Home: return 0xE9B2;
        case UiGlyph::Dashboard: return 0xE871;
        case UiGlyph::Menu: return 0xE5D2;
        case UiGlyph::Apps: return 0xE5C3;
        case UiGlyph::Back: return 0xE5C4;
        case UiGlyph::Forward: return 0xE5C8;
        case UiGlyph::Previous: return 0xE5CB;
        case UiGlyph::Next: return 0xE5CC;
        case UiGlyph::Up: return 0xE5D8;
        case UiGlyph::Down: return 0xE5DB;
        case UiGlyph::ExpandMore: return 0xE5CF;
        case UiGlyph::ExpandLess: return 0xE5CE;
        case UiGlyph::ChevronLeft: return 0xE5CB;
        case UiGlyph::ChevronRight: return 0xE5CC;
        case UiGlyph::FirstPage: return 0xE5DC;
        case UiGlyph::LastPage: return 0xE5DD;
        case UiGlyph::Fullscreen: return 0xE5D0;
        case UiGlyph::FullscreenExit: return 0xE5D1;
        case UiGlyph::OpenInNew: return 0xE89E;
        case UiGlyph::Close: return 0xE5CD;
        case UiGlyph::Cancel: return 0xE888;
        case UiGlyph::Refresh: return 0xE5D5;
        case UiGlyph::Restart: return 0xF053;
        case UiGlyph::Sync: return 0xE627;
        case UiGlyph::Search: return 0xEF7A;
        case UiGlyph::Filter: return 0xEF4F;
        case UiGlyph::FilterOff: return 0xEB32;
        case UiGlyph::Sort: return 0xE164;
        case UiGlyph::Tune: return 0xE429;
        case UiGlyph::Settings: return 0xE8B8;
        case UiGlyph::Preferences: return 0xF05E;
        case UiGlyph::ViewSidebar: return 0xF114;
        case UiGlyph::Dock: return 0xF2E0;
        case UiGlyph::Panel: return 0xE8F1;
        case UiGlyph::SplitView: return 0xE949;
        case UiGlyph::Layout: return 0xE99B;
        case UiGlyph::ResetLayout: return 0xE99B;
        case UiGlyph::NewFile: return 0xE89C;
        case UiGlyph::NewCase: return 0xE2CC;
        case UiGlyph::OpenFile: return 0xE2C8;
        case UiGlyph::OpenRecent: return 0xE8B3;
        case UiGlyph::Save: return 0xE161;
        case UiGlyph::SaveAs: return 0xEB60;
        case UiGlyph::Import: return 0xF09B;
        case UiGlyph::Export: return 0xF090;
        case UiGlyph::Download: return 0xF090;
        case UiGlyph::Upload: return 0xF09B;
        case UiGlyph::Package: return 0xE1A1;
        case UiGlyph::Archive: return 0xE149;
        case UiGlyph::Unarchive: return 0xE169;
        case UiGlyph::Exit: return 0xE9BA;
        case UiGlyph::Print: return 0xE8AD;
        case UiGlyph::Share: return 0xE80D;
        case UiGlyph::Cloud: return 0xF15C;
        case UiGlyph::CloudUpload: return 0xE2C3;
        case UiGlyph::CloudDownload: return 0xE2C0;
        case UiGlyph::Backup: return 0xE864;
        case UiGlyph::Restore: return 0xE8B3;
        case UiGlyph::History: return 0xE8B3;
        case UiGlyph::Revision: return 0xF17D;
        case UiGlyph::Add: return 0xE145;
        case UiGlyph::AddCircle: return 0xE990;
        case UiGlyph::Remove: return 0xE15B;
        case UiGlyph::Delete: return 0xE92E;
        case UiGlyph::DeleteSweep: return 0xE16C;
        case UiGlyph::Edit: return 0xF097;
        case UiGlyph::Rename: return 0xE9A2;
        case UiGlyph::Undo: return 0xE166;
        case UiGlyph::Redo: return 0xE15A;
        case UiGlyph::Cut: return 0xE14E;
        case UiGlyph::Copy: return 0xE14D;
        case UiGlyph::Paste: return 0xE14F;
        case UiGlyph::Duplicate: return 0xE3BB;
        case UiGlyph::SelectAll: return 0xE162;
        case UiGlyph::Deselect: return 0xEBB6;
        case UiGlyph::Clear: return 0xE5CD;
        case UiGlyph::ClearAll: return 0xE0B8;
        case UiGlyph::Drag: return 0xE945;
        case UiGlyph::Move: return 0xE89F;
        case UiGlyph::Rotate: return 0xE41A;
        case UiGlyph::Scale: return 0xE85B;
        case UiGlyph::Transform: return 0xE89F;
        case UiGlyph::AlignLeft: return 0xE00D;
        case UiGlyph::AlignCenter: return 0xE00F;
        case UiGlyph::AlignRight: return 0xE010;
        case UiGlyph::Snap: return 0xF016;
        case UiGlyph::Grid: return 0xE3EC;
        case UiGlyph::GridOff: return 0xE3EB;
        case UiGlyph::Layers: return 0xE53B;
        case UiGlyph::Group: return 0xE886;
        case UiGlyph::Ungroup: return 0xE8F0;
        case UiGlyph::Scene: return 0xF720;
        case UiGlyph::SceneMap: return 0xE55B;
        case UiGlyph::Viewport: return 0xEFC9;
        case UiGlyph::View2D: return 0xE55B;
        case UiGlyph::View3D: return 0xEFC9;
        case UiGlyph::Perspective: return 0xEF4A;
        case UiGlyph::TopView: return 0xE25A;
        case UiGlyph::FrontView: return 0xE8E9;
        case UiGlyph::RightView: return 0xF1DF;
        case UiGlyph::FitScreen: return 0xEA10;
        case UiGlyph::FrameAll: return 0xE162;
        case UiGlyph::FrameSelection: return 0xE3B4;
        case UiGlyph::CenterView: return 0xE3B4;
        case UiGlyph::ZoomIn: return 0xE8FF;
        case UiGlyph::ZoomOut: return 0xE900;
        case UiGlyph::Pan: return 0xE925;
        case UiGlyph::Orbit: return 0xE84D;
        case UiGlyph::Camera: return 0xE412;
        case UiGlyph::CameraFront: return 0xF2C9;
        case UiGlyph::CameraRear: return 0xF2C8;
        case UiGlyph::Screenshot: return 0xF056;
        case UiGlyph::Snapshot: return 0xE412;
        case UiGlyph::Lit: return 0xE518;
        case UiGlyph::Wireframe: return 0xE3EC;
        case UiGlyph::Shadows: return 0xE430;
        case UiGlyph::Overlay: return 0xE53B;
        case UiGlyph::Bounds: return 0xE3C2;
        case UiGlyph::Axes: return 0xE89F;
        case UiGlyph::ObjectNames: return 0xE893;
        case UiGlyph::Statistics: return 0xE4FC;
        case UiGlyph::SafeFrame: return 0xE3BC;
        case UiGlyph::Origin: return 0xE55C;
        case UiGlyph::ResetOrigin: return 0xF053;
        case UiGlyph::AR: return 0xEFC9;
        case UiGlyph::ARPreview: return 0xF1C5;
        case UiGlyph::Device: return 0xE326;
        case UiGlyph::ConnectDevice: return 0xE326;
        case UiGlyph::PairDevice: return 0xE1A8;
        case UiGlyph::Bluetooth: return 0xE1A7;
        case UiGlyph::Phone: return 0xE7BA;
        case UiGlyph::Tablet: return 0xE32F;
        case UiGlyph::Tracking: return 0xE8E1;
        case UiGlyph::Anchor: return 0xF1CD;
        case UiGlyph::AddAnchor: return 0xEF3A;
        case UiGlyph::ClearAnchors: return 0xE16C;
        case UiGlyph::Reticle: return 0xE3B5;
        case UiGlyph::Planes: return 0xE53B;
        case UiGlyph::Occlusion: return 0xE53B;
        case UiGlyph::CollisionGuide: return 0xEBF2;
        case UiGlyph::SessionStart: return 0xE1C4;
        case UiGlyph::SessionStop: return 0xEF71;
        case UiGlyph::Record: return 0xE061;
        case UiGlyph::StopRecord: return 0xE047;
        case UiGlyph::Preview: return 0xF1C5;
        case UiGlyph::Timeline: return 0xE922;
        case UiGlyph::Play: return 0xE037;
        case UiGlyph::Pause: return 0xE034;
        case UiGlyph::Stop: return 0xE047;
        case UiGlyph::SkipPrevious: return 0xE045;
        case UiGlyph::SkipNext: return 0xE044;
        case UiGlyph::FastRewind: return 0xE020;
        case UiGlyph::FastForward: return 0xE01F;
        case UiGlyph::Replay: return 0xE042;
        case UiGlyph::PlayCircle: return 0xE1C4;
        case UiGlyph::PauseCircle: return 0xE1A2;
        case UiGlyph::StopCircle: return 0xEF71;
        case UiGlyph::MarkerAdd: return 0xEF3A;
        case UiGlyph::MarkerClear: return 0xE16C;
        case UiGlyph::Playhead: return 0xE259;
        case UiGlyph::Vehicle: return 0xEFF7;
        case UiGlyph::Car: return 0xEFF7;
        case UiGlyph::Truck: return 0xE558;
        case UiGlyph::Bus: return 0xEFF6;
        case UiGlyph::Taxi: return 0xE559;
        case UiGlyph::Motorcycle: return 0xE9F9;
        case UiGlyph::Bicycle: return 0xEB29;
        case UiGlyph::Traffic: return 0xE565;
        case UiGlyph::TrafficLight: return 0xE565;
        case UiGlyph::Road: return 0xEF3B;
        case UiGlyph::Highway: return 0xEF3B;
        case UiGlyph::Route: return 0xEACD;
        case UiGlyph::Direction: return 0xE52E;
        case UiGlyph::Steering: return 0xEBAB;
        case UiGlyph::Wheel: return 0xEBC8;
        case UiGlyph::Tire: return 0xEBC8;
        case UiGlyph::Fuel: return 0xE546;
        case UiGlyph::Engine: return 0xE8B8;
        case UiGlyph::VehicleIdentity: return 0xEA67;
        case UiGlyph::VehiclePhysics: return 0xEA4B;
        case UiGlyph::Mass: return 0xE13D;
        case UiGlyph::Balance: return 0xEAF6;
        case UiGlyph::Crush: return 0xE94D;
        case UiGlyph::Crash: return 0xEBF2;
        case UiGlyph::Impact: return 0xEBF2;
        case UiGlyph::Friction: return 0xE945;
        case UiGlyph::Evidence: return 0xE873;
        case UiGlyph::EvidenceAdd: return 0xE89C;
        case UiGlyph::EvidenceLink: return 0xE178;
        case UiGlyph::EvidenceUnlink: return 0xE16F;
        case UiGlyph::Photo: return 0xE693;
        case UiGlyph::PhotoCamera: return 0xE412;
        case UiGlyph::PhotoLibrary: return 0xE413;
        case UiGlyph::SkidMark: return 0xEBC8;
        case UiGlyph::Debris: return 0xE268;
        case UiGlyph::Glass: return 0xE3AD;
        case UiGlyph::Fluid: return 0xE798;
        case UiGlyph::Gouge: return 0xF097;
        case UiGlyph::ForensicMarker: return 0xE55E;
        case UiGlyph::Fingerprint: return 0xE90D;
        case UiGlyph::Source: return 0xF1C8;
        case UiGlyph::Provenance: return 0xEA3E;
        case UiGlyph::Verified: return 0xEF76;
        case UiGlyph::Confidence: return 0xF013;
        case UiGlyph::Unverified: return 0xE8FD;
        case UiGlyph::Observed: return 0xE8F4;
        case UiGlyph::Measured: return 0xE41C;
        case UiGlyph::Calculated: return 0xEA5F;
        case UiGlyph::Imported: return 0xF09B;
        case UiGlyph::Simulated: return 0xEA4B;
        case UiGlyph::AI: return 0xF06C;
        case UiGlyph::Witness: return 0xE91F;
        case UiGlyph::Statement: return 0xE0C9;
        case UiGlyph::CaseValidate: return 0xF0C5;
        case UiGlyph::CaseWarning: return 0xF083;
        case UiGlyph::CaseError: return 0xF8B6;
        case UiGlyph::CaseSuccess: return 0xE2E6;
        case UiGlyph::Measurement: return 0xE41C;
        case UiGlyph::Distance: return 0xE41C;
        case UiGlyph::Angle: return 0xEA3B;
        case UiGlyph::Area: return 0xEA49;
        case UiGlyph::Height: return 0xEA16;
        case UiGlyph::Width: return 0xE8D4;
        case UiGlyph::Radius: return 0xE836;
        case UiGlyph::Coordinates: return 0xE55C;
        case UiGlyph::GPS: return 0xE55C;
        case UiGlyph::Compass: return 0xE87A;
        case UiGlyph::Heading: return 0xE87A;
        case UiGlyph::Location: return 0xF1DB;
        case UiGlyph::Datum: return 0xE55C;
        case UiGlyph::ScaleMeasure: return 0xE41C;
        case UiGlyph::Units: return 0xEA5F;
        case UiGlyph::Analysis: return 0xEF3E;
        case UiGlyph::Analytics: return 0xEF3E;
        case UiGlyph::Chart: return 0xE26B;
        case UiGlyph::LineChart: return 0xE6E1;
        case UiGlyph::AreaChart: return 0xE770;
        case UiGlyph::PieChart: return 0xF0DA;
        case UiGlyph::Scatter: return 0xE268;
        case UiGlyph::SpeedAnalysis: return 0xE9E4;
        case UiGlyph::MomentumAnalysis: return 0xE915;
        case UiGlyph::SkidAnalysis: return 0xEBC8;
        case UiGlyph::Trajectory: return 0xEACD;
        case UiGlyph::LineOfSight: return 0xE8F4;
        case UiGlyph::Calculate: return 0xEA5F;
        case UiGlyph::Science: return 0xEA4B;
        case UiGlyph::Experiment: return 0xEA3A;
        case UiGlyph::Module: return 0xE87B;
        case UiGlyph::Workflow: return 0xE97A;
        case UiGlyph::Result: return 0xF0CC;
        case UiGlyph::Results: return 0xF0CC;
        case UiGlyph::Findings: return 0xF0C5;
        case UiGlyph::Hypothesis: return 0xEA4A;
        case UiGlyph::Simulation: return 0xF0CF;
        case UiGlyph::Run: return 0xE037;
        case UiGlyph::RunAll: return 0xE1C4;
        case UiGlyph::Reconstruct: return 0xEA3B;
        case UiGlyph::ReportView: return 0xEF42;
        case UiGlyph::ReportExport: return 0xE415;
        case UiGlyph::NodeEditor: return 0xE97A;
        case UiGlyph::Node: return 0xE97A;
        case UiGlyph::AddNode: return 0xE146;
        case UiGlyph::RunGraph: return 0xE037;
        case UiGlyph::Graph: return 0xE4FD;
        case UiGlyph::InputNode: return 0xE890;
        case UiGlyph::OutputNode: return 0xEBBE;
        case UiGlyph::LinkNode: return 0xE250;
        case UiGlyph::DeleteLink: return 0xE16F;
        case UiGlyph::CenterGraph: return 0xE3B4;
        case UiGlyph::GridGraph: return 0xE3EC;
        case UiGlyph::SnapGraph: return 0xF016;
        case UiGlyph::SaveGraph: return 0xE161;
        case UiGlyph::LoadGraph: return 0xE2C8;
        case UiGlyph::Assets: return 0xE1A1;
        case UiGlyph::AssetLibrary: return 0xE8F0;
        case UiGlyph::Asset: return 0xF720;
        case UiGlyph::Assign: return 0xE862;
        case UiGlyph::Unassign: return 0xE16F;
        case UiGlyph::Material: return 0xE421;
        case UiGlyph::Texture: return 0xE421;
        case UiGlyph::PBR: return 0xE65F;
        case UiGlyph::Model: return 0xF720;
        case UiGlyph::ImportAsset: return 0xF09B;
        case UiGlyph::RefreshAssets: return 0xE5D5;
        case UiGlyph::CopyUrl: return 0xE14D;
        case UiGlyph::ExternalLink: return 0xE89E;
        case UiGlyph::Category: return 0xE72C;
        case UiGlyph::License: return 0xE90E;
        case UiGlyph::Person: return 0xF0D3;
        case UiGlyph::People: return 0xF233;
        case UiGlyph::Driver: return 0xE637;
        case UiGlyph::Pedestrian: return 0xE536;
        case UiGlyph::Cyclist: return 0xEB29;
        case UiGlyph::Officer: return 0xEF56;
        case UiGlyph::Building: return 0xEA40;
        case UiGlyph::Environment: return 0xE564;
        case UiGlyph::Terrain: return 0xE564;
        case UiGlyph::Tree: return 0xEA63;
        case UiGlyph::Grass: return 0xF205;
        case UiGlyph::Rock: return 0xE564;
        case UiGlyph::Water: return 0xF084;
        case UiGlyph::Weather: return 0xF15C;
        case UiGlyph::Lighting: return 0xE518;
        case UiGlyph::StreetFurniture: return 0xEFEE;
        case UiGlyph::StreetLight: return 0xE90F;
        case UiGlyph::Barrier: return 0xF08C;
        case UiGlyph::Cone: return 0xE565;
        case UiGlyph::Sign: return 0xEB91;
        case UiGlyph::Success: return 0xF0BE;
        case UiGlyph::Warning: return 0xF083;
        case UiGlyph::Error: return 0xF8B6;
        case UiGlyph::Help: return 0xE8FD;
        case UiGlyph::HelpOutline: return 0xE8FD;
        case UiGlyph::Keyboard: return 0xE312;
        case UiGlyph::Shortcut: return 0xEAE7;
        case UiGlyph::Command: return 0xEB8E;
        case UiGlyph::Terminal: return 0xEB8E;
        case UiGlyph::Palette: return 0xEF7A;
        case UiGlyph::Notification: return 0xE7F5;
        case UiGlyph::Star: return 0xF09A;
        case UiGlyph::Favorite: return 0xE87E;
        case UiGlyph::Flag: return 0xF0C6;
        case UiGlyph::Label: return 0xE893;
        case UiGlyph::Tag: return 0xE9EF;
        case UiGlyph::Badge: return 0xEA67;
        case UiGlyph::StatusOnline: return 0xE63E;
        case UiGlyph::StatusOffline: return 0xE648;
        case UiGlyph::VisibilityOff: return 0xE8F5;
        case UiGlyph::LockPerson: return 0xF8F3;
        case UiGlyph::Shield: return 0xE9E0;
        case UiGlyph::Security: return 0xE32A;
        case UiGlyph::Admin: return 0xEF3D;
        case UiGlyph::About: return 0xE88E;
        case UiGlyph::Documentation: return 0xEA19;
        default:
            return 0;
    }
}
static int googleMaterialUtf8(
    unsigned int codepoint,
    char output[5])
{
    if (codepoint<=0x7Fu)
    {
        output[0]=
            static_cast<char>(
                codepoint
            );
        output[1]='\0';
        return 1;
    }

    if (codepoint<=0x7FFu)
    {
        output[0]=
            static_cast<char>(
                0xC0u |
                (codepoint>>6)
            );

        output[1]=
            static_cast<char>(
                0x80u |
                (codepoint&0x3Fu)
            );

        output[2]='\0';
        return 2;
    }

    if (codepoint<=0xFFFFu)
    {
        output[0]=
            static_cast<char>(
                0xE0u |
                (codepoint>>12)
            );

        output[1]=
            static_cast<char>(
                0x80u |
                (
                    (codepoint>>6)&
                    0x3Fu
                )
            );

        output[2]=
            static_cast<char>(
                0x80u |
                (codepoint&0x3Fu)
            );

        output[3]='\0';
        return 3;
    }

    output[0]=
        static_cast<char>(
            0xF0u |
            (codepoint>>18)
        );

    output[1]=
        static_cast<char>(
            0x80u |
            (
                (codepoint>>12)&
                0x3Fu
            )
        );

    output[2]=
        static_cast<char>(
            0x80u |
            (
                (codepoint>>6)&
                0x3Fu
            )
        );

    output[3]=
        static_cast<char>(
            0x80u |
            (codepoint&0x3Fu)
        );

    output[4]='\0';
    return 4;
}

static void drawGlyph(
    ImDrawList* drawList,
    UiGlyph glyph,
    const ImVec2& center,
    float size,
    ImU32 color)
{
    // ROADSAFE_GLOBAL_ICON_SCALE_V15
    size *= 1.28f;

    if (!gGoogleMaterialIconFont)
    {
        drawLegacyGlyph(
            drawList,
            glyph,
            center,
            size,
            color
        );
        return;
    }

    const unsigned int codepoint=
        googleMaterialCodepoint(
            glyph
        );

    if (codepoint==0)
    {
        drawLegacyGlyph(
            drawList,
            glyph,
            center,
            size,
            color
        );
        return;
    }

    char utf8[5]{};

    const int byteCount=
        googleMaterialUtf8(
            codepoint,
            utf8
        );

    if (byteCount<=0)
    {
        drawLegacyGlyph(
            drawList,
            glyph,
            center,
            size,
            color
        );
        return;
    }

    const float iconFontSize=
        std::max(
            12.0f,
            size*1.12f
        );

    const ImVec2 textSize=
        gGoogleMaterialIconFont->
            CalcTextSizeA(
                iconFontSize,
                1.0e9f,
                0.0f,
                utf8,
                utf8+byteCount
            );

    const ImVec2 textPosition(
        center.x-
        textSize.x*0.5f,
        center.y-
        textSize.y*0.5f-
        0.5f
    );

    drawList->AddText(
        gGoogleMaterialIconFont,
        iconFontSize,
        textPosition,
        color,
        utf8,
        utf8+byteCount
    );
}

// ROADSAFE_GOOGLE_MATERIAL_SYMBOLS_INIT_V3
static void initializeGoogleMaterialIcons()
{
    if (gGoogleMaterialIconFont)
        return;

    ImGuiIO& googleIconIo=
        ImGui::GetIO();

    const std::filesystem::path iconFontPath=
        assetPath("fonts/MaterialSymbolsRounded.ttf");

    ImFontConfig googleIconConfig;
    googleIconConfig.OversampleH=2;
    googleIconConfig.OversampleV=2;
    googleIconConfig.PixelSnapH=true;

    gGoogleMaterialIconFont=
        googleIconIo.Fonts->AddFontFromFileTTF(
            iconFontPath.string().c_str(),
            24.0f,
            &googleIconConfig,
            gGoogleMaterialIconRanges
        );

    if (!gGoogleMaterialIconFont)
    {
        std::printf(
            "[WARN] Google Material Symbols font unavailable; using RoadSafe legacy icons.\n"
        );
    }
    else
    {
        std::printf(
            "[OK] Google Material Symbols Rounded loaded.\n"
        );
    }
}
static void drawIconBadge(UiGlyph glyph, const ImVec2& pos, float boxSize, bool accent)
{
    ImDrawList* d=ImGui::GetWindowDrawList();
    const ImU32 bg=toU32(accent?colorAccentMuted():colorPanelRaised());
    const ImU32 border=toU32(accent?ImVec4(0.45f,0.31f,0.06f,1.0f):colorBorder());
    const ImU32 fg=toU32(accent?colorAccent():colorText());
    d->AddRectFilled(pos,ImVec2(pos.x+boxSize,pos.y+boxSize),bg,3.0f);
    drawGlyph(d,glyph,ImVec2(pos.x+boxSize*.5f,pos.y+boxSize*.5f),boxSize*.52f,fg);
}

// ROADSAFE_GOOGLE_ICON_HELPERS_V15
static constexpr float kRoadSafeMenuGlyphSize=18.5f;

static void drawRoadSafeMenuGlyph(
    UiGlyph glyph,
    bool enabled,
    bool selected)
{
    const ImVec2 itemMin=
        ImGui::GetItemRectMin();

    const ImVec2 itemMax=
        ImGui::GetItemRectMax();

    if (itemMax.x<=itemMin.x ||
        itemMax.y<=itemMin.y)
    {
        return;
    }

    drawGlyph(
        ImGui::GetWindowDrawList(),
        glyph,
        ImVec2(
            itemMin.x+14.0f,
            (itemMin.y+itemMax.y)*0.5f
        ),
        kRoadSafeMenuGlyphSize,
        toU32(
            !enabled
                ? colorMuted()
                : (
                    selected
                        ? colorAccent()
                        : colorText()
                  )
        )
    );
}

static bool roadSafeMenuItem(
    const char* label,
    UiGlyph glyph,
    const char* shortcut=nullptr,
    bool selected=false,
    bool enabled=true)
{
    std::string paddedLabel="    ";
    paddedLabel+=
        label
            ? label
            : "";

    const bool pressed=
        ImGui::MenuItem(
            paddedLabel.c_str(),
            shortcut,
            selected,
            enabled
        );

    drawRoadSafeMenuGlyph(
        glyph,
        enabled,
        selected
    );

    return pressed;
}

static bool roadSafeMenuItemToggle(
    const char* label,
    UiGlyph glyph,
    const char* shortcut,
    bool* selected,
    bool enabled=true)
{
    std::string paddedLabel="    ";
    paddedLabel+=
        label
            ? label
            : "";

    const bool pressed=
        ImGui::MenuItem(
            paddedLabel.c_str(),
            shortcut,
            selected,
            enabled
        );

    drawRoadSafeMenuGlyph(
        glyph,
        enabled,
        selected
            ? *selected
            : false
    );

    return pressed;
}

// ROADSAFE_EVIDENCE_SEMANTIC_GLYPH_V15
static UiGlyph roadSafeEvidenceGlyph(
    const roadsafe::EvidenceRecord& evidence)
{
    const std::string& type=
        evidence.type;

    const auto contains=
        [&type](const char* a,const char* b=nullptr)
        {
            return
                type.find(a)!=std::string::npos ||
                (
                    b &&
                    type.find(b)!=std::string::npos
                );
        };

    if (contains("Skid","skid") ||
        contains("Tire","tire") ||
        contains("Tyre","tyre"))
    {
        return UiGlyph::SkidMark;
    }

    if (contains("Debris","debris"))
        return UiGlyph::Debris;

    if (contains("Marker","marker"))
        return UiGlyph::ForensicMarker;

    if (contains("Photo","photo") ||
        contains("Image","image"))
    {
        return UiGlyph::Photo;
    }

    if (contains("Glass","glass"))
        return UiGlyph::Glass;

    if (contains("Fluid","fluid"))
        return UiGlyph::Fluid;

    if (contains("Gouge","gouge") ||
        contains("Scrape","scrape"))
    {
        return UiGlyph::Gouge;
    }

    return UiGlyph::Evidence;
}
static void beginSurface(
    const char* id,
    const ImVec2& size,
    bool raised=false,
    ImGuiWindowFlags flags=0)
{
    // SOVEREIGN_SCROLL_ROUTING_V1
    //
    // beginSurface() is for cards, sections and editor chrome.
    // Those nested children must not hijack the mouse wheel.
    // ImGui will pass wheel input through to the parent editor window.
    flags |= ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleColor(
        ImGuiCol_ChildBg,
        raised
            ? colorPanelRaised()
            : colorPanel()
    );

    ImGui::PushStyleColor(
        ImGuiCol_Border,
        colorBorder()
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_ChildRounding,
        3.0f
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(12.0f,10.0f)
    );

    ImGui::BeginChild(
        id,
        size,
        true,
        flags
    );
}

static void endSurface()
{
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

static void drawStatus(const char* text, StatusTone tone)
{
    const ImVec4 c=toneColor(tone);
    const ImVec2 p=ImGui::GetCursorScreenPos();
    const float h=ImGui::GetTextLineHeight();
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x+5.0f,p.y+h*.5f),4.0f,toU32(c));
    ImGui::Dummy(ImVec2(11.0f,h));
    ImGui::SameLine(0.0f,4.0f);
    ImGui::TextColored(c,"%s",text);
}

// SOVEREIGN_BUTTON_ICONIZATION_V1
struct ButtonVisualSpec
{
    bool hasIcon=false;
    UiGlyph glyph=UiGlyph::Info;
    bool iconOnly=false;
    const char* visibleLabel="";
    const char* tooltip="";
};

static ButtonVisualSpec getButtonVisualSpec(const char* label)
{
    ButtonVisualSpec spec;
    spec.visibleLabel = label ? label : "";
    spec.tooltip      = label ? label : "";

    if (!label || !label[0])
        return spec;

    const std::string key(label);

    if (key=="ALL") return {true,UiGlyph::Filter,true,"ALL","All"};
    if (key=="PHOTOS") return {true,UiGlyph::PhotoLibrary,true,"PHOTOS","Photos"};
    if (key=="SKID MARKS") return {true,UiGlyph::SkidMark,true,"SKID MARKS","Skid Marks"};
    if (key=="DEBRIS") return {true,UiGlyph::Debris,true,"DEBRIS","Debris"};
    if (key=="MEASUREMENTS") return {true,UiGlyph::Measurement,true,"MEASUREMENTS","Measurements"};
    if (key=="MARKERS") return {true,UiGlyph::Marker,true,"MARKERS","Markers"};
    if (key=="ADD EVIDENCE") return {true,UiGlyph::EvidenceAdd,true,"ADD EVIDENCE","Add Evidence"};
    if (key=="IMPORT PHOTOS") return {true,UiGlyph::PhotoCamera,false,"IMPORT PHOTOS","Import Photos"};
    if (key=="OPEN VIEWPORT") return {true,UiGlyph::Viewport,false,"OPEN VIEWPORT","Open Viewport"};
    if (key=="EDIT SUMMARY") return {true,UiGlyph::Edit,false,"EDIT SUMMARY","Edit Summary"};
    if (key=="START ANALYSIS") return {true,UiGlyph::Analysis,false,"START ANALYSIS","Start Analysis"};
    if (key=="EXPORT CASE") return {true,UiGlyph::ReportExport,false,"EXPORT CASE","Export Case"};
    if (key=="ADD PARTY") return {true,UiGlyph::Person,false,"ADD PARTY","Add Party"};
    if (key=="VIEW REPORTS") return {true,UiGlyph::ReportView,false,"VIEW REPORTS","View Reports"};
    if (key=="LINK EVIDENCE") return {true,UiGlyph::EvidenceLink,false,"LINK EVIDENCE","Link Evidence"};
    if (key=="OPEN EVIDENCE") return {true,UiGlyph::Evidence,false,"OPEN EVIDENCE","Open Evidence"};
    if (key=="RUN ALL ANALYSES") return {true,UiGlyph::RunAll,false,"RUN ALL ANALYSES","Run All Analyses"};
    if (key=="START SKID ANALYSIS") return {true,UiGlyph::SkidAnalysis,false,"START SKID ANALYSIS","Start Skid Analysis"};
    if (key=="ANALYSIS GUIDE") return {true,UiGlyph::Documentation,true,"ANALYSIS GUIDE","Analysis Guide"};
    if (key=="PREV") return {true,UiGlyph::Previous,false,"PREV","Prev"};
    if (key=="NEXT") return {true,UiGlyph::Next,false,"NEXT","Next"};
    if (key=="PIPELINE") return {true,UiGlyph::Workflow,true,"PIPELINE","Pipeline"};
    if (key=="DRAFT") return {true,UiGlyph::Edit,false,"DRAFT","Draft"};
    if (key=="2D PLAN") return {true,UiGlyph::View2D,false,"2D PLAN","2D Plan"};
    if (key=="3D SCENE") return {true,UiGlyph::View3D,false,"3D SCENE","3D Scene"};
    if (key=="AR PREVIEW") return {true,UiGlyph::ARPreview,false,"AR PREVIEW","Ar Preview"};
    if (key=="EDITOR PREVIEW") return {true,UiGlyph::Preview,false,"EDITOR PREVIEW","Editor Preview"};
    if (key=="EXIT PREVIEW") return {true,UiGlyph::Close,false,"EXIT PREVIEW","Exit Preview"};
    if (key=="CONNECT DEVICE") return {true,UiGlyph::ConnectDevice,false,"CONNECT DEVICE","Connect Device"};
    if (key=="PAIR DEVICE") return {true,UiGlyph::PairDevice,false,"PAIR DEVICE","Pair Device"};
    if (key=="PLACE ANCHOR") return {true,UiGlyph::AddAnchor,false,"PLACE ANCHOR","Place Anchor"};
    if (key=="MORE  v") return {true,UiGlyph::More,false,"MORE  v","More  V"};
    if (key=="MORE v") return {true,UiGlyph::More,false,"MORE v","More V"};
    if (key=="FULL SCREEN") return {true,UiGlyph::Fullscreen,false,"FULL SCREEN","Full Screen"};
    if (key=="EXIT FULL SCREEN") return {true,UiGlyph::FullscreenExit,false,"EXIT FULL SCREEN","Exit Full Screen"};
    if (key=="FRAME SELECT") return {true,UiGlyph::FrameSelection,false,"FRAME SELECT","Frame Select"};
    if (key=="FRAME ALL") return {true,UiGlyph::FrameAll,false,"FRAME ALL","Frame All"};
    if (key=="OVERLAYS") return {true,UiGlyph::Overlay,false,"OVERLAYS","Overlays"};
    if (key=="OVERLAYS v") return {true,UiGlyph::Overlay,false,"OVERLAYS v","Overlays V"};
    if (key=="OVERLAYS  v") return {true,UiGlyph::Overlay,false,"OVERLAYS  v","Overlays  V"};
    if (key=="RESET ORIGIN") return {true,UiGlyph::ResetOrigin,false,"RESET ORIGIN","Reset Origin"};
    if (key=="RECORD") return {true,UiGlyph::Record,false,"RECORD","Record"};
    if (key=="STOP RECORD") return {true,UiGlyph::StopRecord,false,"STOP RECORD","Stop Record"};
    if (key=="CLEAR") return {true,UiGlyph::ClearAll,false,"CLEAR","Clear"};
    if (key=="ASSETS") return {true,UiGlyph::AssetLibrary,false,"ASSETS","Assets"};
    if (key=="BROWSE ASSET LIBRARY") return {true,UiGlyph::AssetLibrary,true,"BROWSE ASSET LIBRARY","Browse Asset Library"};
    if (key=="CLEAR ASSET") return {true,UiGlyph::Unassign,false,"CLEAR ASSET","Clear Asset"};
    if (key=="UNDO") return {true,UiGlyph::Undo,false,"UNDO","Undo"};
    if (key=="REDO") return {true,UiGlyph::Redo,false,"REDO","Redo"};
    if (key=="RUN GRAPH") return {true,UiGlyph::RunGraph,false,"RUN GRAPH","Run Graph"};
    if (key=="DELETE LINK") return {true,UiGlyph::DeleteLink,false,"DELETE LINK","Delete Link"};
    if (key=="SET CASE DETAILS") return {true,UiGlyph::Edit,false,"SET CASE DETAILS","Set Case Details"};
    if (key=="CASE DETAILS") return {true,UiGlyph::Edit,false,"CASE DETAILS","Case Details"};
    if (key=="ADD MEASUREMENTS") return {true,UiGlyph::Measurement,false,"ADD MEASUREMENTS","Add Measurements"};
    if (key=="REVIEW MODULES") return {true,UiGlyph::Module,false,"REVIEW MODULES","Review Modules"};
    if (key=="FIT") return {true,UiGlyph::FitScreen,false,"FIT","Fit"};
    if (key=="ZOOM IN") return {true,UiGlyph::ZoomIn,false,"ZOOM IN","Zoom In"};
    if (key=="ZOOM OUT") return {true,UiGlyph::ZoomOut,false,"ZOOM OUT","Zoom Out"};
    if (key=="ADD") return {true,UiGlyph::Add,false,"ADD","Add"};
    if (key=="FILE") return {true,UiGlyph::Folder,false,"FILE","File"};
    if (key=="ADD NODE") return {true,UiGlyph::AddNode,false,"ADD NODE","Add Node"};
    if (key=="COPY") return {true,UiGlyph::Copy,false,"COPY","Copy"};
    if (key=="DUPLICATE") return {true,UiGlyph::Duplicate,false,"DUPLICATE","Duplicate"};
    if (key=="PASTE") return {true,UiGlyph::Paste,false,"PASTE","Paste"};
    if (key=="DELETE") return {true,UiGlyph::Delete,false,"DELETE","Delete"};
    if (key=="CENTER") return {true,UiGlyph::CenterView,false,"CENTER","Center"};
    if (key=="PLAY") return {true,UiGlyph::Play,false,"PLAY","Play"};
    if (key=="PAUSE") return {true,UiGlyph::Pause,false,"PAUSE","Pause"};
    if (key=="STOP") return {true,UiGlyph::Stop,false,"STOP","Stop"};
    if (key=="FIRST") return {true,UiGlyph::FirstPage,false,"FIRST","First"};
    if (key=="LAST") return {true,UiGlyph::LastPage,false,"LAST","Last"};
    if (key=="STEP BACK") return {true,UiGlyph::SkipPrevious,false,"STEP BACK","Step Back"};
    if (key=="STEP NEXT") return {true,UiGlyph::SkipNext,false,"STEP NEXT","Step Next"};

    return spec;
}
static void drawUnifiedButtonLabel(
    ImDrawList* drawList,
    const ImVec2& minPos,
    const ImVec2& maxPos,
    const char* label,
    const ImVec4& textColor,
    bool enabled);
// SOVEREIGN_VS_ACTIVE_TAB_STYLE_V1
static bool isEditorTabVisualLabel(
    const char* label)
{
    if (!label || !label[0])
        return false;

    return
        // Main workspace/editor tabs.
        strcmp(label,"Case View")==0 ||
        strcmp(label,"Evidence")==0 ||
        strcmp(label,"Analysis")==0 ||
        strcmp(label,"Viewport")==0 ||
        strcmp(label,"Timeline")==0 ||
        strcmp(label,"Node Editor")==0 ||

        // View-mode tabs.
        strcmp(label,"2D PLAN")==0 ||
        strcmp(label,"3D SCENE")==0 ||
        strcmp(label,"AR PREVIEW")==0 ||

        // Workstation mode tabs.
        strcmp(label,"SCENE")==0 ||
        strcmp(label,"EVIDENCE")==0 ||
        strcmp(label,"MEASURE")==0 ||
        strcmp(label,"RECONSTRUCT")==0 ||
        strcmp(label,"REVIEW")==0;
}
static bool editorButton(
    const char* label,
    float width=0.0f,
    bool active=false,
    bool enabled=true)
{
    // SOVEREIGN_CHROME_CONTOUR_TABS_V1
    ImGuiWindow* window=
        ImGui::GetCurrentWindow();

    if (window->SkipItems)
        return false;

    const ButtonVisualSpec spec=
        getButtonVisualSpec(label);

    const ImGuiStyle& style=
        ImGui::GetStyle();

    const char* fullLabel=
        spec.visibleLabel
            ? spec.visibleLabel
            : "";

    const bool tabVisual=
        isEditorTabVisualLabel(fullLabel);

    const ImVec2 fullTextSize=
        fullLabel[0]
            ? ImGui::CalcTextSize(fullLabel)
            : ImVec2(0.0f,0.0f);

    const float iconLayoutWidth=
        spec.hasIcon
            ? 18.0f
            : 0.0f;

    const float contentGap=
        (spec.hasIcon &&
         !spec.iconOnly &&
         fullTextSize.x>0.0f)
            ? 6.0f
            : 0.0f;

    const float normalContentWidth=
        iconLayoutWidth+
        (spec.iconOnly
            ? 0.0f
            : contentGap+fullTextSize.x);

    float autoWidth=
        spec.iconOnly
            ? 42.0f
            : normalContentWidth+
                style.FramePadding.x*2.0f+
                30.0f;

    if (tabVisual)
    {
        autoWidth=
            std::max(
                autoWidth,
                fullTextSize.x+30.0f
            );
    }

    const float availableWidth=
        std::max(
            1.0f,
            ImGui::GetContentRegionAvail().x
        );

    const float requestedWidth=
        width>0.0f
            ? std::max(width,autoWidth)
            : autoWidth;

    const float finalWidth=
        // ROADSAFE_NO_TEXT_CLIP_V19
        std::max(
            1.0f,
            requestedWidth
        );

    const bool compactIconOnly=
        !tabVisual &&
        spec.hasIcon &&
        (
            spec.iconOnly ||
            finalWidth<
                std::min(
                    autoWidth-6.0f,
                    96.0f
                )
        );

    const char* visibleLabel=
        compactIconOnly
            ? ""
            : fullLabel;

    const ImVec2 textSize=
        visibleLabel[0]
            ? ImGui::CalcTextSize(visibleLabel)
            : ImVec2(0.0f,0.0f);

    const float renderedContentWidth=
        spec.hasIcon
            ? (
                compactIconOnly
                    ? iconLayoutWidth
                    : iconLayoutWidth+
                        (textSize.x>0.0f
                            ? contentGap+textSize.x
                            : 0.0f)
              )
            : textSize.x;

    const float buttonHeight=
        tabVisual
            ? 35.0f
            : std::max(
                ImGui::GetFrameHeight()+2.0f,
                std::max(
                    textSize.y+
                        style.FramePadding.y*2.0f+
                        2.0f,
                    30.0f
                )
            );

    const ImVec2 size(
        finalWidth,
        buttonHeight
    );

    const ImVec2 pos=
        ImGui::GetCursorScreenPos();

    ImGui::PushID(
        static_cast<int>(pos.x)
    );

    ImGui::PushID(
        static_cast<int>(pos.y)
    );

    if (!enabled)
        ImGui::BeginDisabled();

    const bool pressed=
        ImGui::InvisibleButton(
            "##editorButton",
            size
        );

    const bool hovered=
        ImGui::IsItemHovered();

    const bool held=
        ImGui::IsItemActive();

    if (!enabled)
        ImGui::EndDisabled();

    ImGui::PopID();
    ImGui::PopID();

    ImDrawList* drawList=
        ImGui::GetWindowDrawList();

    const ImVec2 maxPos(
        pos.x+size.x,
        pos.y+size.y
    );

    ImVec4 textColor=
        enabled
            ? ImVec4(0.96f,0.97f,0.99f,1.0f)
            : ImVec4(0.58f,0.60f,0.64f,1.0f);

    if (tabVisual)
    {
        // Chrome-like tab semantics:
        // selected tab is physically joined to the content below.
        // inactive tabs are flatter, darker and visually recessed.
        if (active)
        {
            ImVec4 activeBg=
                held
                    ? ImVec4(0.165f,0.172f,0.190f,1.0f)
                    : hovered
                        ? ImVec4(0.205f,0.214f,0.235f,1.0f)
                        : ImVec4(0.185f,0.194f,0.214f,1.0f);

            const ImU32 fillCol=
                ImGui::ColorConvertFloat4ToU32(
                    activeBg
                );

            const ImU32 contourCol=
                ImGui::ColorConvertFloat4ToU32(
                    ImVec4(
                        0.42f,
                        0.44f,
                        0.49f,
                        1.0f
                    )
                );

            const float top=
                pos.y+1.0f;

            const float bottom=
                maxPos.y;

            const float shoulder=
                std::max(
                    7.0f,
                    std::min(
                        13.0f,
                        size.x*0.14f
                    )
                );

            // Fill silhouette: bottom baseline -> curved left
            // shoulder -> top -> curved right shoulder -> baseline.
            drawList->PathLineTo(
                ImVec2(
                    pos.x,
                    bottom
                )
            );

            drawList->PathBezierCubicCurveTo(
                ImVec2(
                    pos.x+shoulder*0.28f,
                    bottom
                ),
                ImVec2(
                    pos.x+shoulder*0.32f,
                    top
                ),
                ImVec2(
                    pos.x+shoulder,
                    top
                )
            );

            drawList->PathLineTo(
                ImVec2(
                    maxPos.x-shoulder,
                    top
                )
            );

            drawList->PathBezierCubicCurveTo(
                ImVec2(
                    maxPos.x-shoulder*0.32f,
                    top
                ),
                ImVec2(
                    maxPos.x-shoulder*0.28f,
                    bottom
                ),
                ImVec2(
                    maxPos.x,
                    bottom
                )
            );

            drawList->PathLineTo(
                ImVec2(
                    pos.x,
                    bottom
                )
            );

            drawList->PathFillConvex(
                fillCol
            );

            // Top/shoulder contour only.
            // No bottom line: it must merge into the content.
            drawList->PathLineTo(
                ImVec2(
                    pos.x,
                    bottom
                )
            );

            drawList->PathBezierCubicCurveTo(
                ImVec2(
                    pos.x+shoulder*0.28f,
                    bottom
                ),
                ImVec2(
                    pos.x+shoulder*0.32f,
                    top
                ),
                ImVec2(
                    pos.x+shoulder,
                    top
                )
            );

            drawList->PathLineTo(
                ImVec2(
                    maxPos.x-shoulder,
                    top
                )
            );

            drawList->PathBezierCubicCurveTo(
                ImVec2(
                    maxPos.x-shoulder*0.32f,
                    top
                ),
                ImVec2(
                    maxPos.x-shoulder*0.28f,
                    bottom
                ),
                ImVec2(
                    maxPos.x,
                    bottom
                )
            );

            drawList->PathStroke(
                contourCol,
                0,
                1.0f
            );

            textColor=
                ImVec4(
                    0.99f,
                    0.99f,
                    1.0f,
                    1.0f
                );
        }
        else
        {
            ImVec4 inactiveBg=
                hovered
                    ? ImVec4(0.115f,0.120f,0.132f,1.0f)
                    : ImVec4(0.075f,0.079f,0.088f,1.0f);

            drawList->AddRectFilled(
                pos,
                maxPos,
                ImGui::ColorConvertFloat4ToU32(
                    inactiveBg
                ),
                1.5f
            );

            // Quiet separator instead of a button border.
            drawList->AddLine(
                ImVec2(
                    maxPos.x-0.5f,
                    pos.y+7.0f
                ),
                ImVec2(
                    maxPos.x-0.5f,
                    maxPos.y-7.0f
                ),
                ImGui::ColorConvertFloat4ToU32(
                    ImVec4(
                        0.20f,
                        0.21f,
                        0.23f,
                        1.0f
                    )
                ),
                1.0f
            );

            textColor=
                hovered
                    ? ImVec4(0.88f,0.90f,0.93f,1.0f)
                    : ImVec4(0.70f,0.72f,0.76f,1.0f);
        }
    }
    else
    {
        ImVec4 bg=
            active
                ? ImVec4(0.12f,0.42f,0.78f,1.0f)
                : ImVec4(0.19f,0.20f,0.22f,1.0f);

        ImVec4 border=
            active
                ? ImVec4(0.34f,0.61f,0.95f,1.0f)
                : ImVec4(0.35f,0.37f,0.41f,1.0f);

        if (!enabled)
        {
            bg=ImVec4(0.16f,0.17f,0.18f,1.0f);
            border=ImVec4(0.26f,0.27f,0.29f,1.0f);
        }
        else if (held)
        {
            if (active)
            {
                bg=ImVec4(0.10f,0.36f,0.69f,1.0f);
                border=ImVec4(0.29f,0.54f,0.88f,1.0f);
            }
            else
            {
                bg=ImVec4(0.15f,0.16f,0.18f,1.0f);
                border=ImVec4(0.43f,0.46f,0.51f,1.0f);
            }
        }
        else if (hovered)
        {
            if (active)
            {
                bg=ImVec4(0.15f,0.47f,0.84f,1.0f);
                border=ImVec4(0.48f,0.73f,1.0f,1.0f);
            }
            else
            {
                bg=ImVec4(0.23f,0.24f,0.27f,1.0f);
                border=ImVec4(0.50f,0.53f,0.58f,1.0f);
            }
        }

        // ROADSAFE_BORDERLESS_BUTTON_V18
        drawList->AddRectFilled(
            ImVec2(
                pos.x,
                pos.y+2.0f
            ),
            ImVec2(
                maxPos.x,
                maxPos.y+2.0f
            ),
            ImGui::ColorConvertFloat4ToU32(
                ImVec4(
                    0.0f,
                    0.0f,
                    0.0f,
                    enabled ? 0.24f : 0.12f
                )
            ),
            5.0f
        );

        drawList->AddRectFilled(
            pos,
            maxPos,
            ImGui::ColorConvertFloat4ToU32(bg),
            5.0f
        );
    }

    drawList->PushClipRect(
        ImVec2(
            pos.x+2.0f,
            pos.y+1.0f
        ),
        ImVec2(
            maxPos.x-2.0f,
            maxPos.y-1.0f
        ),
        true
    );

    // ROADSAFE_BUTTON_ICON_LEFT_V18
    float cursorX=
        (
            spec.hasIcon &&
            !compactIconOnly &&
            visibleLabel[0] &&
            !tabVisual
        )
            ? pos.x+12.0f
            : pos.x+
                (size.x-renderedContentWidth)*0.5f;

    const float centerY=
        pos.y+size.y*0.5f;

    if (spec.hasIcon)
    {
        drawGlyph(
            drawList,
            spec.glyph,
            ImVec2(
                cursorX+
                    iconLayoutWidth*0.5f,
                centerY+0.25f
            ),
            18.0f,
            ImGui::ColorConvertFloat4ToU32(
                textColor
            )
        );

        cursorX+=iconLayoutWidth;
    }

    if (visibleLabel[0])
    {
        if (spec.hasIcon)
            cursorX+=contentGap;

        const ImVec2 textPos(
            cursorX,
            pos.y+
                (size.y-textSize.y)*0.5f-
                0.5f
        );

        const ImU32 textU32=
            ImGui::ColorConvertFloat4ToU32(
                textColor
            );

        drawList->AddText(
            textPos,
            textU32,
            visibleLabel
        );

        if (tabVisual && active)
        {
            drawList->AddText(
                ImVec2(
                    textPos.x+0.55f,
                    textPos.y
                ),
                textU32,
                visibleLabel
            );
        }
        else if (!tabVisual)
        {
            drawList->AddText(
                ImVec2(
                    textPos.x+0.70f,
                    textPos.y
                ),
                textU32,
                visibleLabel
            );
        }
    }

    drawList->PopClipRect();

    if (hovered &&
        spec.tooltip &&
        spec.tooltip[0] &&
        (
            compactIconOnly ||
            finalWidth<autoWidth
        ))
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(
            spec.tooltip
        );
        ImGui::EndTooltip();
    }

    return enabled && pressed;
}
// ROADSAFE_UI_DESIGN_SYSTEM_V22
#include "roadsafe/ui/ui_tokens.inl"
#include "roadsafe/ui/ui_theme.inl"
#include "roadsafe/ui/ui_icons.inl"
#include "roadsafe/ui/ui_typography.inl"
#include "roadsafe/ui/ui_layout.inl"
#include "roadsafe/ui/ui_widgets.inl"


static void drawMetricTile(const char* id, UiGlyph glyph, const char* label, const char* value, const char* note, float width, float height=76.0f, bool accentIcon=false)
{
    beginSurface(id,ImVec2(width,height),true,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 p=ImGui::GetCursorScreenPos();
    const float textX=p.x+50.0f;

    drawIconBadge(glyph,p,38.0f,accentIcon);

    ImGui::SetCursorScreenPos(ImVec2(textX,p.y+1.0f));
    ImGui::TextDisabled("%s",label);

    ImGui::SetCursorScreenPos(ImVec2(textX,p.y+23.0f));
    ImGui::Text("%s",value);

    if (note && note[0])
    {
        ImGui::SetCursorScreenPos(ImVec2(textX,p.y+45.0f));
        ImGui::TextDisabled("%s",note);
    }

    endSurface();
}

static void drawModuleCard(const char* id, UiGlyph glyph, const char* title, const char* description, const char* status, StatusTone tone, const char* action, float width)
{
    beginSurface(id,ImVec2(width,174.0f),false,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 p=ImGui::GetCursorScreenPos();
    drawIconBadge(glyph,p,40.0f,tone==StatusTone::Accent);
    ImGui::SetCursorScreenPos(ImVec2(p.x+52.0f,p.y+2.0f));
    ImGui::Text("%s",title);
    ImGui::SetCursorScreenPos(ImVec2(p.x+52.0f,p.y+27.0f));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+std::max(90.0f,width-70.0f));
    ImGui::TextDisabled("%s",description);
    ImGui::PopTextWrapPos();
    ImGui::SetCursorPosY(100.0f);
    drawStatus(status,tone);
    ImGui::SetCursorPosY(133.0f);
    editorButton(action,ImGui::GetContentRegionAvail().x,tone==StatusTone::Accent);
    endSurface();
}

// ROADSAFE_AR_PIPELINE_V2
// Native RoadSafe AR forensic workflow foundation.
//
// The pipeline deliberately separates evidence collection,
// analytical reasoning, simulation and reconstruction. 2D/3D/AR
// is a visualization/reconstruction stage, not the source of truth.
enum class RoadSafePipelineStage
{
    Overview=0,
    SceneIntake,
    EvidenceRegistry,
    Measurements,
    Vehicles,
    Persons,
    Witnesses,
    Analysis,
    Hypotheses,
    Simulation,
    Reconstruction,
    Findings,
    Report
};

constexpr int ROADSAFE_PIPELINE_STAGE_COUNT=13;

static RoadSafePipelineStage gRoadSafePipelineStage=
    RoadSafePipelineStage::Overview;

static const char* const gRoadSafePipelineNames[
    ROADSAFE_PIPELINE_STAGE_COUNT
]=
{
    "Overview",
    "Scene Intake",
    "Evidence Registry",
    "Measurements",
    "Vehicles",
    "Persons",
    "Witnesses",
    "Analysis",
    "Hypotheses",
    "Simulation",
    "2D / 3D / AR",
    "Findings",
    "Report"
};

static int roadSafePipelineIndex(
    RoadSafePipelineStage stage)
{
    return
        std::max(
            0,
            std::min(
                ROADSAFE_PIPELINE_STAGE_COUNT-1,
                static_cast<int>(stage)
            )
        );
}

static const char* roadSafePipelineName(
    RoadSafePipelineStage stage)
{
    return
        gRoadSafePipelineNames[
            roadSafePipelineIndex(stage)
        ];
}

static void activateRoadSafePipelineStage(
    RoadSafePipelineStage stage)
{
    gRoadSafePipelineStage=stage;

    // Only route to workspaces that genuinely exist today.
    // The remaining stages stay registered in the pipeline until
    // their dedicated native workspaces are implemented.
    switch (stage)
    {
        case RoadSafePipelineStage::Overview:
            ImGui::SetWindowFocus("Case View");
            break;

        case RoadSafePipelineStage::EvidenceRegistry:
            ImGui::SetWindowFocus("Evidence");
            break;

        case RoadSafePipelineStage::Analysis:
            ImGui::SetWindowFocus("Analysis");
            break;

        case RoadSafePipelineStage::Reconstruction:
            ImGui::SetWindowFocus("Viewport");
            break;

        default:
            break;
    }
}

static void drawRoadSafePipelineMenuItems()
{
    for (int i=0;
         i<ROADSAFE_PIPELINE_STAGE_COUNT;
         ++i)
    {
        if (i==0)
        {
            ImGui::TextDisabled(
                "INVESTIGATION"
            );
        }
        else if (i==7)
        {
            ImGui::Separator();
            ImGui::TextDisabled(
                "ANALYSIS & TESTING"
            );
        }
        else if (i==10)
        {
            ImGui::Separator();
            ImGui::TextDisabled(
                "RECONSTRUCTION"
            );
        }
        else if (i==11)
        {
            ImGui::Separator();
            ImGui::TextDisabled(
                "REVIEW"
            );
        }

        char label[128]{};

        std::snprintf(
            label,
            sizeof(label),
            "%02d  %s",
            i+1,
            gRoadSafePipelineNames[i]
        );

        const bool selected=
            roadSafePipelineIndex(
                gRoadSafePipelineStage
            )==i;

        if (ImGui::MenuItem(
            label,
            nullptr,
            selected))
        {
            activateRoadSafePipelineStage(
                static_cast<
                    RoadSafePipelineStage
                >(i)
            );
        }
    }
}

static void drawRoadSafePipelineBar()
{
    beginSurface(
        "RoadSafePipelineBar",
        ImVec2(0.0f,58.0f),
        false,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    const ImVec2 p=
        ImGui::GetCursorScreenPos();

    ImGui::SetCursorScreenPos(
        ImVec2(
            p.x,
            p.y+7.0f
        )
    );

    ImGui::TextColored(
        colorAccent(),
        "ROADSAFE AR"
    );

    ImGui::SameLine(0.0f,10.0f);

    ImGui::TextDisabled(
        "FORENSIC PIPELINE"
    );

    ImGui::SameLine(0.0f,10.0f);

    ImGui::TextDisabled("|");

    ImGui::SameLine(0.0f,10.0f);

    const int current=
        roadSafePipelineIndex(
            gRoadSafePipelineStage
        );

    ImGui::Text(
        "%02d / %02d   %s",
        current+1,
        ROADSAFE_PIPELINE_STAGE_COUNT,
        roadSafePipelineName(
            gRoadSafePipelineStage
        )
    );

    const float right=
        ImGui::GetWindowPos().x+
        ImGui::GetWindowContentRegionMax().x;

    // ROADSAFE_PIPELINE_ACTION_FIT_V23_2
    constexpr float prevW=112.0f;
    constexpr float pipelineW=112.0f;
    constexpr float nextW=112.0f;
    constexpr float gap=6.0f;

    const float actionWidth=
        prevW+
        pipelineW+
        nextW+
        gap*2.0f;

    const float actionX=
        right-
        actionWidth-
        14.0f;

    if (actionX>
        p.x+430.0f)
    {
        ImGui::SetCursorScreenPos(
            ImVec2(
                actionX,
                p.y+2.0f
            )
        );

        ImGui::BeginDisabled(
            current==0
        );

        if (editorButton(
            "PREV",
            prevW))
        {
            activateRoadSafePipelineStage(
                static_cast<
                    RoadSafePipelineStage
                >(current-1)
            );
        }

        ImGui::EndDisabled();

        ImGui::SameLine(
            0.0f,
            gap
        );

        if (editorButton(
            "PIPELINE",
            pipelineW,
            true))
        {
            ImGui::OpenPopup(
                "##RoadSafePipelinePopup"
            );
        }

        ImGui::SameLine(
            0.0f,
            gap
        );

        ImGui::BeginDisabled(
            current>=
            ROADSAFE_PIPELINE_STAGE_COUNT-1
        );

        if (editorButton(
            "NEXT",
            nextW))
        {
            activateRoadSafePipelineStage(
                static_cast<
                    RoadSafePipelineStage
                >(current+1)
            );
        }

        ImGui::EndDisabled();
    }
    else
    {
        ImGui::SameLine(
            0.0f,
            12.0f
        );

        if (editorButton(
            "PIPELINE",
            112.0f,
            true))
        {
            ImGui::OpenPopup(
                "##RoadSafePipelinePopup"
            );
        }
    }

    if (ImGui::BeginPopup(
        "##RoadSafePipelinePopup"))
    {
        ImGui::TextColored(
            colorAccent(),
            "ROADSAFE AR"
        );

        ImGui::SameLine();

        ImGui::TextDisabled(
            "FORENSIC PIPELINE"
        );

        ImGui::Separator();

        drawRoadSafePipelineMenuItems();

        ImGui::EndPopup();
    }

    endSurface();
}
// ROADSAFE_UI_COMPONENT_INCLUDE_V20: Case View
#include "roadsafe/ui/views/case_view.inl"

static void sectionLabel(const char* title, const char* subtitle=nullptr)
{
    ImGui::Text("%s", title);

    if (subtitle && subtitle[0] != '\0')
    {
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::TextDisabled("%s", subtitle);
    }
}
// ROADSAFE_UI_COMPONENT_INCLUDE_V20: Evidence View
#include "roadsafe/ui/views/evidence_view.inl"

// ROADSAFE_UI_COMPONENT_INCLUDE_V20: Analysis View
#include "roadsafe/ui/views/analysis_view.inl"

static void drawRailButton(const char* id, const char* label, int iconIndex, bool active, bool* clicked)
{
    ImGui::PushID(id);
    const float width=std::max(1.0f,ImGui::GetContentRegionAvail().x), height=58.0f;
    const ImVec2 pos=ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##Rail",ImVec2(width,height));
    const bool hovered=ImGui::IsItemHovered();
    if (clicked) *clicked=ImGui::IsItemClicked();
    ImDrawList* d=ImGui::GetWindowDrawList();
    if (active)
    {
        d->AddRectFilled(pos,ImVec2(pos.x+width,pos.y+height),IM_COL32(57,41,12,255),3.0f);
        d->AddRect(pos,ImVec2(pos.x+width,pos.y+height),IM_COL32(225,157,17,170),3.0f,0,1.0f);
        d->AddRectFilled(ImVec2(pos.x,pos.y+7.0f),ImVec2(pos.x+3.0f,pos.y+height-7.0f),IM_COL32(238,174,38,255));
    }
    else if (hovered) d->AddRectFilled(pos,ImVec2(pos.x+width,pos.y+height),IM_COL32(31,34,37,255),3.0f);

    const float iconSize=22.0f;
    if (iconIndex>=0 && iconIndex<(int)gToolIcons.size() && gToolIcons[(size_t)iconIndex])
    {
        d->AddImage((ImTextureID)gToolIcons[(size_t)iconIndex],ImVec2(pos.x+(width-iconSize)*.5f,pos.y+6.0f),ImVec2(pos.x+(width+iconSize)*.5f,pos.y+6.0f+iconSize),ImVec2(0,0),ImVec2(1,1),active?IM_COL32(255,190,51,255):IM_COL32(225,228,232,255));
    }
    const ImVec2 ls=ImGui::CalcTextSize(label);
    d->AddText(ImVec2(pos.x+(width-ls.x)*.5f,pos.y+35.0f),active?IM_COL32(255,184,35,255):(hovered?IM_COL32(224,226,229,255):IM_COL32(155,160,166,255)),label);
    ImGui::PopID();
}

static void beginEditorContextHeader(
    const char* id)
{
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(10.0f,5.0f)
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(7.0f,4.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_ChildBg,
        ImVec4(0.086f,0.091f,0.099f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.22f,0.235f,0.26f,1.0f)
    );

    ImGui::BeginChild(
        id,
        ImVec2(0.0f,34.0f),
        true,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );
}

static void editorContextSeparator()
{
    ImGui::SameLine(0.0f,9.0f);

    ImGui::TextColored(
        ImVec4(0.31f,0.33f,0.37f,1.0f),
        "|"
    );

    ImGui::SameLine(0.0f,9.0f);
}

static void endEditorContextHeader()
{
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::Spacing();
}
static bool shellSnapEnabled();
static float shellSnapValue();
static const char* shellSelectedEntityName();
static int shellSelectedEntityId();
// SOVEREIGN_ENTITY_LOCK_ENFORCEMENT_V2
static bool shellSelectedEntityLocked();
static void drawEditorModeStrip();

struct EditorShellState
{
    bool showOutliner=true;
    bool showProperties=true;
    bool showTimeline=true;
    bool showNodeEditor=true;
    bool showShortcutReference=false;
    bool focusCommandSearch=false;
    bool showCommandPalette=false;
    int commandPaletteSelection=0;
    bool snapEnabled=true;
    bool requestExit=false;
    bool resetLayoutRequested=false;
    int transformMode=0;
    int selectedEntity=0;
    int shortcutFocusRequest=0;
    float snapValue=0.10f;
    char outlinerSearch[96]{};
    char commandSearch[96]{};
    char shortcutToast[128]{};
    double shortcutToastUntil=0.0;
};

static EditorShellState gEditorShell;

// SOVEREIGN_VIEWPORT_FULLSCREEN_V1
// SOVEREIGN_NODE_FULLSCREEN_V1
// SOVEREIGN_RELIABLE_FULLSCREEN_CONTROLS_V1
static bool gViewportFullscreen=false;
static bool gNodeEditorFullscreen=false;
// ROADSAFE_UI_COMPONENT_INCLUDE_V20: Viewport View
#include "roadsafe/ui/views/viewport_view.inl"

// SOVEREIGN_REAL_ENTITY_RENAME_V1
static char gSovereignEntityNames[10][64]=
{
    "",
    "Ground Plane",
    "Road Surface",
    "Vehicle A",
    "Vehicle B",
    "Skid Mark 01",
    "Marker 01",
    "Debris Field 01",
    "Distance 01",
    "Angle 01"
};

// SOVEREIGN_SHARED_ENTITY_STATE_V2
// One source of truth for entity visibility / lock state.
// Entity IDs 1..9 map directly to the current scene objects.
static bool gSovereignEntityVisible[10]=
{
    false,
    true,  // Ground Plane
    true,  // Road Surface
    true,  // Vehicle A
    true,  // Vehicle B
    true,  // Skid Mark 01
    true,  // Marker 01
    true,  // Debris Field 01
    true,  // Distance 01
    true   // Angle 01
};

static bool gSovereignEntityLocked[10]=
{
    false,
    false, // Ground Plane
    true,  // Road Surface
    false, // Vehicle A
    false, // Vehicle B
    false, // Skid Mark 01
    false, // Marker 01
    false, // Debris Field 01
    false, // Distance 01
    false  // Angle 01
};

static roadsafe::SceneEntityRecord* roadSafeSceneEntity(
    int entityId)
{
    return
        gRoadSafeCase.findSceneEntity(
            entityId
        );
}

static const roadsafe::SceneEntityRecord*
roadSafeSceneEntityConst(
    int entityId)
{
    return
        gRoadSafeCase.findSceneEntity(
            entityId
        );
}

static bool* roadSafeEntityVisiblePtr(
    int entityId)
{
    if (auto* entity=
            roadSafeSceneEntity(entityId))
    {
        return &entity->visible;
    }

    return
        &gSovereignEntityVisible[
            std::max(
                0,
                std::min(9,entityId)
            )
        ];
}

static bool* roadSafeEntityLockedPtr(
    int entityId)
{
    if (auto* entity=
            roadSafeSceneEntity(entityId))
    {
        return &entity->locked;
    }

    return
        &gSovereignEntityLocked[
            std::max(
                0,
                std::min(9,entityId)
            )
        ];
}

static float* roadSafeEntityPositionPtr(
    int entityId)
{
    static std::array<float,3> fallback{
        0.0f,0.0f,0.0f
    };

    if (auto* entity=
            roadSafeSceneEntity(entityId))
    {
        return entity->position.data();
    }

    return fallback.data();
}

static float* roadSafeEntityRotationPtr(
    int entityId)
{
    static std::array<float,3> fallback{
        0.0f,0.0f,0.0f
    };

    if (auto* entity=
            roadSafeSceneEntity(entityId))
    {
        return entity->rotationDegrees.data();
    }

    return fallback.data();
}

static float* roadSafeEntityScalePtr(
    int entityId)
{
    static std::array<float,3> fallback{
        1.0f,1.0f,1.0f
    };

    if (auto* entity=
            roadSafeSceneEntity(entityId))
    {
        return entity->scale.data();
    }

    return fallback.data();
}

static std::size_t roadSafeActiveSceneEntityCount()
{
    std::size_t count=0;

    for (const auto& entity :
         gRoadSafeCase.sceneEntities)
    {
        if (entity.active)
            ++count;
    }

    return count;
}
static bool sovereignEntityVisible(
    int entityId)
{
    const auto* entity=
        roadSafeSceneEntityConst(
            entityId
        );

    return
        entity &&
        entity->visible;
}

static bool sovereignEntityLocked(
    int entityId)
{
    const auto* entity=
        roadSafeSceneEntityConst(
            entityId
        );

    return
        entity &&
        entity->locked;
}
static int gSovereignRenameEntityId=0;
static char gSovereignRenameBuffer[64]{};

static const char* sovereignEntityName(
    int entityId)
{
    if (const auto* entity=
            roadSafeSceneEntityConst(entityId))
    {
        return entity->name.c_str();
    }

    if (entityId<1 || entityId>9)
        return "Nothing selected";

    // Temporary fallback during migration.
    return gSovereignEntityNames[entityId];
}

static void beginSovereignEntityRename(
    int entityId)
{
    if (entityId<1 ||
        roadSafeSceneEntityConst(entityId)==nullptr)
    {
        return;
    }

    gSovereignRenameEntityId=entityId;

    std::snprintf(
        gSovereignRenameBuffer,
        sizeof(gSovereignRenameBuffer),
        "%s",
        sovereignEntityName(entityId)
    );

    ImGui::OpenPopup("Rename Entity");
}

static void drawSovereignRenamePopup()
{
    if (gSovereignRenameEntityId==0)
        return;

    bool keepOpen=true;

    ImGui::SetNextWindowSize(
        ImVec2(420.0f,0.0f),
        ImGuiCond_Appearing
    );

    if (ImGui::BeginPopupModal(
        "Rename Entity",
        &keepOpen,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();

        ImGui::TextDisabled("ENTITY");

        ImGui::Text(
            "%s",
            sovereignEntityName(
                gSovereignRenameEntityId
            )
        );

        ImGui::Spacing();

        const bool submitted=
            ImGui::InputText(
                "##RenameEntityInput",
                gSovereignRenameBuffer,
                sizeof(gSovereignRenameBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_AutoSelectAll
            );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const bool validName=
            gSovereignRenameBuffer[0]!=0;

        if (!validName)
            ImGui::BeginDisabled();

        const bool renamePressed=
            ImGui::Button(
                "Rename",
                ImVec2(92.0f,0.0f)
            );

        if (!validName)
            ImGui::EndDisabled();

        ImGui::SameLine();

        const bool cancelPressed=
            ImGui::Button(
                "Cancel",
                ImVec2(92.0f,0.0f)
            );

        if ((submitted || renamePressed) &&
            validName)
        {
            // Preserve the old compatibility name table only for
            // the original starter-scene IDs.
            if (gSovereignRenameEntityId>=1 &&
                gSovereignRenameEntityId<=9)
            {
                std::snprintf(
                    gSovereignEntityNames[
                        gSovereignRenameEntityId
                    ],
                    sizeof(
                        gSovereignEntityNames[
                            gSovereignRenameEntityId
                        ]
                    ),
                    "%s",
                    gSovereignRenameBuffer
                );
            }

            if (auto* sceneEntity=
                    roadSafeSceneEntity(
                        gSovereignRenameEntityId
                    ))
            {
                sceneEntity->name=
                    gSovereignRenameBuffer;

                gRoadSafeCase.touch();
            }

            ImGui::CloseCurrentPopup();
            gSovereignRenameEntityId=0;
            gSovereignRenameBuffer[0]=0;
        }
        else if (cancelPressed ||
                 ImGui::IsKeyPressed(
                     ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
            gSovereignRenameEntityId=0;
            gSovereignRenameBuffer[0]=0;
        }

        ImGui::EndPopup();
    }

    if (!keepOpen)
    {
        gSovereignRenameEntityId=0;
        gSovereignRenameBuffer[0]=0;
    }
}

static const char* selectedEntityName()
{
    return sovereignEntityName(
        gEditorShell.selectedEntity
    );
}
// SOVEREIGN_EDITOR_MODE_SYSTEM_V1
enum class SovereignEditorMode
{
    Scene=0,
    Evidence,
    Measure,
    Reconstruct,
    Review
};

static SovereignEditorMode gSovereignEditorMode=
    SovereignEditorMode::Scene;

static const char* sovereignEditorModeName(
    SovereignEditorMode mode)
{
    switch (mode)
    {
        case SovereignEditorMode::Scene:
            return "SCENE";

        case SovereignEditorMode::Evidence:
            return "EVIDENCE";

        case SovereignEditorMode::Measure:
            return "MEASURE";

        case SovereignEditorMode::Reconstruct:
            return "RECONSTRUCT";

        case SovereignEditorMode::Review:
            return "REVIEW";
    }

    return "SCENE";
}

static const char* sovereignEditorModeHint(
    SovereignEditorMode mode)
{
    switch (mode)
    {
        case SovereignEditorMode::Scene:
            return "Select, position and organize scene objects.";

        case SovereignEditorMode::Evidence:
            return "Place and review scene evidence.";

        case SovereignEditorMode::Measure:
            return "Create distances, angles and dimensions.";

        case SovereignEditorMode::Reconstruct:
            return "Work with trajectories, phases and analysis.";

        case SovereignEditorMode::Review:
            return "Inspect results without accidental scene edits.";
    }

    return "";
}

static void setSovereignEditorMode(
    SovereignEditorMode mode)
{
    if (gSovereignEditorMode==mode)
        return;

    gSovereignEditorMode=mode;

    // Entering a specialized mode begins from a safe selection tool.
    // Existing Q/W/E/R transform shortcuts remain the authority for
    // actual transform-tool selection.
    if (mode!=SovereignEditorMode::Scene)
        gEditorShell.transformMode=0;
}

static void drawEditorModeStrip()
{
    const float available=
        ImGui::GetContentRegionAvail().x;

    if (available<=0.0f)
        return;

    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(5.0f,4.0f)
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(6.0f,5.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_ChildBg,
        ImVec4(0.080f,0.084f,0.091f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.23f,0.245f,0.27f,1.0f)
    );

    ImGui::BeginChild(
        "##SovereignEditorModeStrip",
        ImVec2(0.0f,43.0f),
        true,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    const float innerAvailable=
        ImGui::GetContentRegionAvail().x;

    const float gap=5.0f;
    const float totalGaps=gap*4.0f;

    const float modeWidth=
        std::max(
            68.0f,
            (innerAvailable-totalGaps)/5.0f
        );

    struct ModeButton
    {
        SovereignEditorMode mode;
        const char* label;
    };

    const ModeButton modes[]={
        {SovereignEditorMode::Scene,"SCENE"},
        {SovereignEditorMode::Evidence,"EVIDENCE"},
        {SovereignEditorMode::Measure,"MEASURE"},
        {SovereignEditorMode::Reconstruct,"RECONSTRUCT"},
        {SovereignEditorMode::Review,"REVIEW"}
    };

    for (int i=0;i<5;i++)
    {
        if (i>0)
            ImGui::SameLine(0.0f,gap);

        const bool active=
            gSovereignEditorMode==
            modes[i].mode;

        if (editorButton(
            modes[i].label,
            modeWidth,
            active))
        {
            setSovereignEditorMode(
                modes[i].mode
            );
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();

            ImGui::TextUnformatted(
                modes[i].label
            );

            ImGui::Separator();

            ImGui::TextDisabled(
                "%s",
                sovereignEditorModeHint(
                    modes[i].mode
                )
            );

            ImGui::EndTooltip();
        }
    }

    ImGui::EndChild();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    ImGui::Spacing();
}
struct SharedSelectionState
{
    int revision=0;
    int previousEntity=0;
    char source[32]="None";
    double changedAt=0.0;
};

static SharedSelectionState gSharedSelection;

static void setSharedEntitySelection(
    int entity,
    const char* source)
{
    if (entity!=0 &&
        roadSafeSceneEntityConst(entity)==nullptr)
    {
        entity=0;
    }

    if (gEditorShell.selectedEntity!=entity)
    {
        gSharedSelection.previousEntity=
            gEditorShell.selectedEntity;

        gEditorShell.selectedEntity=
            entity;

        gSharedSelection.revision++;
        gSharedSelection.changedAt=
            ImGui::GetTime();
    }

    std::snprintf(
        gSharedSelection.source,
        sizeof(gSharedSelection.source),
        "%s",
        source && source[0]
            ? source
            : "Editor"
    );
}

static void clearSharedEntitySelection(
    const char* source)
{
    setSharedEntitySelection(
        0,
        source
    );
}

static bool sharedSelectionIsEvidence()
{
    const auto* entity=
        roadSafeSceneEntityConst(
            gEditorShell.selectedEntity
        );

    return
        entity &&
        entity->kind==
            roadsafe::SceneEntityKind::Evidence;
}

static bool sharedSelectionIsMeasurement()
{
    const auto* entity=
        roadSafeSceneEntityConst(
            gEditorShell.selectedEntity
        );

    return
        entity &&
        entity->kind==
            roadsafe::SceneEntityKind::Measurement;
}

// SOVEREIGN_RIGHT_PANEL_CONTENT_FIT_V2
// ROADSAFE_TYPE_AWARE_PROPERTIES_V1
// ROADSAFE_UI_COMPONENT_INCLUDE_V20: Properties Panel
#include "roadsafe/ui/panels/properties_panel.inl"

static bool shellSnapEnabled()
{
    return gEditorShell.snapEnabled;
}

static float shellSnapValue()
{
    return gEditorShell.snapValue;
}

static const char* shellSelectedEntityName()
{
    if (gEditorShell.selectedEntity==0)
        return "";

    return selectedEntityName();
}

static bool shellSelectedEntityLocked()
{
    const int entity=
        gEditorShell.selectedEntity;

    return
        entity>=1 &&
        sovereignEntityLocked(entity);
}

static int shellSelectedEntityId()
{
    return gEditorShell.selectedEntity;
}

static UiGlyph roadSafeSceneEntityGlyph(
    const roadsafe::SceneEntityRecord& entity);


static UiGlyph selectedEntityGlyph()
{
    const auto* entity=
        roadSafeSceneEntityConst(
            gEditorShell.selectedEntity
        );

    return
        entity
            ? roadSafeSceneEntityGlyph(*entity)
            : UiGlyph::Info;
}
static bool shellLabelMatches(const char* label)
{
    if (gEditorShell.outlinerSearch[0]=='\0')
        return true;

    return ImStristr(
        label,
        nullptr,
        gEditorShell.outlinerSearch,
        nullptr
    ) != nullptr;
}

// ROADSAFE_DYNAMIC_OUTLINER_LIFECYCLE_V2
enum class RoadSafePendingSceneAction
{
    None=0,
    AddVehicle,
    AddEvidence,
    AddMeasurement,
    AddMarker,
    Duplicate,
    Delete
};

static RoadSafePendingSceneAction
    gRoadSafePendingSceneAction=
        RoadSafePendingSceneAction::None;

static int gRoadSafePendingSceneEntityId=0;

static int roadSafeNextSceneEntityId()
{
    int nextId=1;

    for (const auto& entity :
         gRoadSafeCase.sceneEntities)
    {
        nextId=
            std::max(
                nextId,
                entity.legacyId+1
            );
    }

    return nextId;
}

static UiGlyph roadSafeSceneEntityGlyph(
    const roadsafe::SceneEntityRecord& entity)
{
    switch (entity.kind)
    {
        case roadsafe::SceneEntityKind::Environment:
            return UiGlyph::Folder;

        case roadsafe::SceneEntityKind::Vehicle:
            return UiGlyph::Cube;

        case roadsafe::SceneEntityKind::Measurement:
            return UiGlyph::Ruler;

        case roadsafe::SceneEntityKind::Evidence:
        {
            if (entity.recordIndex>=0 &&
                static_cast<std::size_t>(
                    entity.recordIndex
                )<gRoadSafeCase.evidence.size())
            {
                const auto& evidence=
                    gRoadSafeCase.evidence[
                        static_cast<std::size_t>(
                            entity.recordIndex
                        )
                    ];

                if (evidence.type=="Scene Marker")
                    return UiGlyph::Marker;
            }

            return UiGlyph::Document;
        }
    }

    return UiGlyph::Info;
}

static void queueRoadSafeSceneAction(
    RoadSafePendingSceneAction action,
    int entityId=0)
{
    gRoadSafePendingSceneAction=action;
    gRoadSafePendingSceneEntityId=entityId;
}

static void roadSafeAddSceneEntity(
    RoadSafePendingSceneAction action)
{
    const int editorId=
        roadSafeNextSceneEntityId();

    roadsafe::SceneEntityRecord scene;
    scene.id=
        gRoadSafeCase.allocateId("SCN");
    scene.legacyId=editorId;
    scene.active=true;
    scene.visible=true;
    scene.locked=false;

    char name[96]{};

    switch (action)
    {
        case RoadSafePendingSceneAction::AddVehicle:
        {
            roadsafe::VehicleRecord record;
            record.id=
                gRoadSafeCase.allocateId("VEH");
            record.active=true;
            record.make="Vehicle";
            record.model="Unknown";
            record.vehicleType="Vehicle";
            record.massKg=1500.0f;
            record.wheelbaseMeters=2.70f;
            record.cgHeightMeters=0.55f;
            record.frictionCoefficient=0.70f;
            record.restitution=0.20f;
            record.lineage.provenance=
                roadsafe::Provenance::InvestigatorAssumption;
            record.lineage.confidence=
                roadsafe::Confidence::Unverified;

            scene.kind=
                roadsafe::SceneEntityKind::Vehicle;
            scene.recordIndex=
                static_cast<int>(
                    gRoadSafeCase.vehicles.size()
                );

            std::snprintf(
                name,
                sizeof(name),
                "Vehicle %d",
                editorId
            );

            record.make=name;

            gRoadSafeCase.vehicles.push_back(
                record
            );
            break;
        }

        case RoadSafePendingSceneAction::AddEvidence:
        {
            roadsafe::EvidenceRecord record;
            record.id=
                gRoadSafeCase.allocateId("EV");
            record.active=true;
            record.type="Evidence";
            record.collectionStatus="Observed";
            record.confidenceScore=0.50f;
            record.lineage.provenance=
                roadsafe::Provenance::Observed;
            record.lineage.confidence=
                roadsafe::Confidence::Unverified;

            scene.kind=
                roadsafe::SceneEntityKind::Evidence;
            scene.recordIndex=
                static_cast<int>(
                    gRoadSafeCase.evidence.size()
                );

            std::snprintf(
                name,
                sizeof(name),
                "Evidence %d",
                editorId
            );

            record.description=name;

            gRoadSafeCase.evidence.push_back(
                record
            );
            break;
        }

        case RoadSafePendingSceneAction::AddMarker:
        {
            roadsafe::EvidenceRecord record;
            record.id=
                gRoadSafeCase.allocateId("EV");
            record.active=true;
            record.type="Scene Marker";
            record.collectionStatus="Observed";
            record.confidenceScore=0.50f;
            record.lineage.provenance=
                roadsafe::Provenance::Observed;
            record.lineage.confidence=
                roadsafe::Confidence::Unverified;

            scene.kind=
                roadsafe::SceneEntityKind::Evidence;
            scene.recordIndex=
                static_cast<int>(
                    gRoadSafeCase.evidence.size()
                );

            std::snprintf(
                name,
                sizeof(name),
                "Marker %d",
                editorId
            );

            record.description=name;

            gRoadSafeCase.evidence.push_back(
                record
            );
            break;
        }

        case RoadSafePendingSceneAction::AddMeasurement:
        {
            roadsafe::MeasurementRecord record;
            record.id=
                gRoadSafeCase.allocateId("M");
            record.active=true;
            record.type="Distance";
            record.unit="m";
            record.method="Scene measurement";
            record.lineage.provenance=
                roadsafe::Provenance::Measured;
            record.lineage.confidence=
                roadsafe::Confidence::Unverified;

            scene.kind=
                roadsafe::SceneEntityKind::Measurement;
            scene.recordIndex=
                static_cast<int>(
                    gRoadSafeCase.measurements.size()
                );

            std::snprintf(
                name,
                sizeof(name),
                "Distance %d",
                editorId
            );

            gRoadSafeCase.measurements.push_back(
                record
            );
            break;
        }

        default:
            return;
    }

    scene.name=name;

    gRoadSafeCase.sceneEntities.push_back(
        scene
    );

    gRoadSafeCase.touch();

    setSharedEntitySelection(
        editorId,
        "Add"
    );
}

static void roadSafeDuplicateSceneEntity(
    int entityId)
{
    const auto* source=
        roadSafeSceneEntityConst(entityId);

    if (!source || !source->active)
        return;

    // Copy before any vector push_back can invalidate pointers.
    roadsafe::SceneEntityRecord duplicate=
        *source;

    const int newEditorId=
        roadSafeNextSceneEntityId();

    duplicate.id=
        gRoadSafeCase.allocateId("SCN");
    duplicate.legacyId=newEditorId;
    duplicate.active=true;
    duplicate.locked=false;
    duplicate.position[0]+=0.50f;
    duplicate.name=
        source->name+
        " Copy";

    switch (source->kind)
    {
        case roadsafe::SceneEntityKind::Vehicle:
        {
            if (source->recordIndex>=0 &&
                static_cast<std::size_t>(
                    source->recordIndex
                )<gRoadSafeCase.vehicles.size())
            {
                auto record=
                    gRoadSafeCase.vehicles[
                        static_cast<std::size_t>(
                            source->recordIndex
                        )
                    ];

                record.id=
                    gRoadSafeCase.allocateId("VEH");
                record.active=true;

                duplicate.recordIndex=
                    static_cast<int>(
                        gRoadSafeCase.vehicles.size()
                    );

                gRoadSafeCase.vehicles.push_back(
                    record
                );
            }
            break;
        }

        case roadsafe::SceneEntityKind::Evidence:
        {
            if (source->recordIndex>=0 &&
                static_cast<std::size_t>(
                    source->recordIndex
                )<gRoadSafeCase.evidence.size())
            {
                auto record=
                    gRoadSafeCase.evidence[
                        static_cast<std::size_t>(
                            source->recordIndex
                        )
                    ];

                record.id=
                    gRoadSafeCase.allocateId("EV");
                record.active=true;

                duplicate.recordIndex=
                    static_cast<int>(
                        gRoadSafeCase.evidence.size()
                    );

                gRoadSafeCase.evidence.push_back(
                    record
                );
            }
            break;
        }

        case roadsafe::SceneEntityKind::Measurement:
        {
            if (source->recordIndex>=0 &&
                static_cast<std::size_t>(
                    source->recordIndex
                )<gRoadSafeCase.measurements.size())
            {
                auto record=
                    gRoadSafeCase.measurements[
                        static_cast<std::size_t>(
                            source->recordIndex
                        )
                    ];

                record.id=
                    gRoadSafeCase.allocateId("M");
                record.active=true;

                duplicate.recordIndex=
                    static_cast<int>(
                        gRoadSafeCase.measurements.size()
                    );

                gRoadSafeCase.measurements.push_back(
                    record
                );
            }
            break;
        }

        case roadsafe::SceneEntityKind::Environment:
            duplicate.recordIndex=-1;
            break;
    }

    gRoadSafeCase.sceneEntities.push_back(
        duplicate
    );

    gRoadSafeCase.touch();

    setSharedEntitySelection(
        newEditorId,
        "Duplicate"
    );
}

static void roadSafeDeleteSceneEntity(
    int entityId)
{
    auto* scene=
        roadSafeSceneEntity(entityId);

    if (!scene ||
        !scene->active ||
        scene->locked)
    {
        return;
    }

    const auto kind=scene->kind;
    const int recordIndex=
        scene->recordIndex;

    scene->active=false;

    switch (kind)
    {
        case roadsafe::SceneEntityKind::Vehicle:
            if (recordIndex>=0 &&
                static_cast<std::size_t>(
                    recordIndex
                )<gRoadSafeCase.vehicles.size())
            {
                gRoadSafeCase.vehicles[
                    static_cast<std::size_t>(
                        recordIndex
                    )
                ].active=false;
            }
            break;

        case roadsafe::SceneEntityKind::Evidence:
            if (recordIndex>=0 &&
                static_cast<std::size_t>(
                    recordIndex
                )<gRoadSafeCase.evidence.size())
            {
                gRoadSafeCase.evidence[
                    static_cast<std::size_t>(
                        recordIndex
                    )
                ].active=false;
            }
            break;

        case roadsafe::SceneEntityKind::Measurement:
            if (recordIndex>=0 &&
                static_cast<std::size_t>(
                    recordIndex
                )<gRoadSafeCase.measurements.size())
            {
                gRoadSafeCase.measurements[
                    static_cast<std::size_t>(
                        recordIndex
                    )
                ].active=false;
            }
            break;

        case roadsafe::SceneEntityKind::Environment:
            break;
    }

    if (gEditorShell.selectedEntity==entityId)
    {
        clearSharedEntitySelection(
            "Delete"
        );
    }

    gRoadSafeCase.touch();
}

static void processRoadSafePendingSceneAction()
{
    const auto action=
        gRoadSafePendingSceneAction;

    const int entityId=
        gRoadSafePendingSceneEntityId;

    gRoadSafePendingSceneAction=
        RoadSafePendingSceneAction::None;
    gRoadSafePendingSceneEntityId=0;

    switch (action)
    {
        case RoadSafePendingSceneAction::AddVehicle:
        case RoadSafePendingSceneAction::AddEvidence:
        case RoadSafePendingSceneAction::AddMeasurement:
        case RoadSafePendingSceneAction::AddMarker:
            roadSafeAddSceneEntity(action);
            break;

        case RoadSafePendingSceneAction::Duplicate:
            roadSafeDuplicateSceneEntity(
                entityId
            );
            break;

        case RoadSafePendingSceneAction::Delete:
            roadSafeDeleteSceneEntity(
                entityId
            );
            break;

        case RoadSafePendingSceneAction::None:
            break;
    }
}
static void toolbarSeparator()
{
    const ImVec2 p=ImGui::GetCursorScreenPos();
    const float h=26.0f;

    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(p.x+4.0f,p.y+3.0f),
        ImVec2(p.x+4.0f,p.y+h-3.0f),
        toU32(colorBorder()),
        1.0f
    );

    ImGui::Dummy(ImVec2(9.0f,h));
}

static bool shellIconButton(
    const char* id,
    UiGlyph glyph,
    const char* tooltip,
    bool active=false,
    bool enabled=true)
{
    ImGui::PushID(id);

    if (!enabled)
        ImGui::BeginDisabled();

    const ImVec2 p=ImGui::GetCursorScreenPos();
    const ImVec2 size(34.0f,30.0f);

    const bool clicked=ImGui::InvisibleButton("##ShellIconButton",size);
    const bool hovered=ImGui::IsItemHovered();

    ImDrawList* dl=ImGui::GetWindowDrawList();

    if (active)
    {
        dl->AddRectFilled(
            p,
            ImVec2(p.x+size.x,p.y+size.y),
            toU32(colorAccentMuted()),
            3.0f
        );
    }
    else if (hovered)
    {
        dl->AddRectFilled(
            p,
            ImVec2(p.x+size.x,p.y+size.y),
            toU32(colorPanelRaised()),
            3.0f
        );
    }

    drawGlyph(
        dl,
        glyph,
        ImVec2(p.x+size.x*0.5f,p.y+size.y*0.5f),
        17.0f,
        toU32(
            active
                ? colorAccent()
                : (enabled ? colorText() : colorMuted())
        )
    );

    if (hovered && tooltip && tooltip[0])
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltip);
        ImGui::EndTooltip();
    }

    if (!enabled)
        ImGui::EndDisabled();

    ImGui::PopID();
    return clicked && enabled;
}

// ROADSAFE_TOOLBAR_TWO_ROW_HEIGHT_V19
// ROADSAFE_UI_COMPONENT_INCLUDE_V20: Editor Toolbar
#include "roadsafe/ui/shell/toolbar.inl"

static bool outlinerMiniIconButton(
    const char* id,
    UiGlyph glyph,
    bool active,
    const char* tooltip)
{
    ImGui::PushID(id);

    const ImVec2 p=ImGui::GetCursorScreenPos();
    const ImVec2 size(34.0f,30.0f);

    const bool pressed=
        ImGui::InvisibleButton(
            "##StateIcon",
            size
        );

    const bool hovered=ImGui::IsItemHovered();
    ImDrawList* dl=ImGui::GetWindowDrawList();

    if (active | hovered)
    {
        dl->AddRectFilled(
            p,
            ImVec2(p.x+size.x,p.y+size.y),
            toU32(active ? colorPanelRaised() : colorPanel()),
            4.0f
        );
    }

    float glyphSize=20.0f;
    if (glyph==UiGlyph::More) glyphSize=16.5f;
    if (glyph==UiGlyph::Lock | glyph==UiGlyph::Unlock) glyphSize=22.0f;
    if (glyph==UiGlyph::Eye) glyphSize=20.5f;
    if (glyph==UiGlyph::Target) glyphSize=20.5f;

    const ImVec2 center(
        p.x+size.x*0.5f,
        p.y+size.y*0.5f + ((glyph==UiGlyph::Lock | glyph==UiGlyph::Unlock) ? 0.5f : 0.0f)
    );

    drawGlyph(
        dl,
        glyph,
        center,
        glyphSize,
        toU32(active ? colorText() : colorMuted())
    );

    if (glyph==UiGlyph::Eye && !active)
    {
        dl->AddLine(
            ImVec2(p.x+7.0f,p.y+22.0f),
            ImVec2(p.x+27.0f,p.y+8.0f),
            toU32(colorMuted()),
            1.7f
        );
    }

    if (hovered && tooltip && tooltip[0])
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltip);
        ImGui::EndTooltip();
    }

    ImGui::PopID();
    return pressed;
}

static void outlinerLeafRow(
    const char* label,
    UiGlyph glyph,
    int entityId,
    bool* visible,
    bool* locked)
{
    if (!shellLabelMatches(label))
        return;

    ImGui::TableNextRow(
        ImGuiTableRowFlags_None,
        32.0f
    );

    ImGui::PushID(entityId);

    // --------------------------------------------------------
    // OBJECT
    // --------------------------------------------------------

    ImGui::TableSetColumnIndex(0);

    const bool selected=
        gEditorShell.selectedEntity==entityId;

    const ImVec2 rowPos=ImGui::GetCursorScreenPos();

    if (ImGui::Selectable(
        "##EntityRow",
        selected,
        ImGuiSelectableFlags_None,
        ImVec2(0.0f,29.0f)))
    {
        setSharedEntitySelection(entityId,"Outliner");
    }

    drawGlyph(
        ImGui::GetWindowDrawList(),
        glyph,
        ImVec2(
            rowPos.x+13.0f,
            rowPos.y+14.0f
        ),
        16.0f,
        toU32(
            selected
                ? colorAccent()
                : colorMuted()
        )
    );

    ImGui::GetWindowDrawList()->AddText(
        ImVec2(
            rowPos.x+30.0f,
            rowPos.y+5.0f
        ),
        toU32(colorText()),
        label
    );

    // Full-row context menu.
    if (ImGui::BeginPopupContextItem("##EntityContext"))
    {
        ImGui::TextDisabled("%s",label);
        ImGui::Separator();

        if (roadSafeMenuItem("Rename",UiGlyph::Rename,"F2"))
            beginSovereignEntityRename(entityId);
        if (roadSafeMenuItem("Duplicate",UiGlyph::Duplicate,"Ctrl+D"))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::Duplicate,
                entityId
            );
        }

        if (roadSafeMenuItem("Focus in Viewport",UiGlyph::FrameSelection,"F"))
            setSharedEntitySelection(entityId,"Outliner");

        ImGui::Separator();
        ImGui::BeginDisabled(*locked);

        if (roadSafeMenuItem("Delete",UiGlyph::Delete,"Del"))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::Delete,
                entityId
            );
        }

        ImGui::EndDisabled();

        ImGui::EndPopup();
    }

    // --------------------------------------------------------
    // VISIBILITY
    // --------------------------------------------------------

    ImGui::TableSetColumnIndex(1);
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX()+2.0f
    );

    if (outlinerMiniIconButton(
        "VisibilityToggle",
        UiGlyph::Eye,
        *visible,
        *visible
            ? "Visible - click to hide"
            : "Hidden - click to show"))
    {
        *visible=!*visible;
        gRoadSafeCase.touch();
    }

    // --------------------------------------------------------
    // LOCK
    // --------------------------------------------------------

    ImGui::TableSetColumnIndex(2);
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX()+2.0f
    );

    if (outlinerMiniIconButton(
        "LockToggle",
        *locked ? UiGlyph::Lock : UiGlyph::Unlock,
        *locked,
        *locked
            ? "Locked - click to unlock"
            : "Unlocked - click to lock"))
    {
        *locked=!*locked;
        gRoadSafeCase.touch();
    }

    // --------------------------------------------------------
    // FOCUS
    // --------------------------------------------------------

    ImGui::TableSetColumnIndex(3);
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX()+2.0f
    );

    if (outlinerMiniIconButton(
        "FocusAction",
        UiGlyph::Target,
        false,
        "Focus object in Viewport"))
    {
        setSharedEntitySelection(entityId,"Outliner");
        // Hook viewport framing here later.
    }

    // --------------------------------------------------------
    // MORE
    // --------------------------------------------------------

    ImGui::TableSetColumnIndex(4);
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX()+2.0f
    );

    if (outlinerMiniIconButton(
        "MoreAction",
        UiGlyph::More,
        false,
        "More object actions"))
    {
        ImGui::OpenPopup("##EntityMorePopup");
    }

    if (ImGui::BeginPopup("##EntityMorePopup"))
    {
        ImGui::TextDisabled("%s",label);
        ImGui::Separator();

        if (roadSafeMenuItem("Rename",UiGlyph::Rename,"F2"))
            beginSovereignEntityRename(entityId);
        if (roadSafeMenuItem("Duplicate",UiGlyph::Duplicate,"Ctrl+D"))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::Duplicate,
                entityId
            );
        }

        if (roadSafeMenuItem("Focus in Viewport",UiGlyph::FrameSelection,"F"))
            setSharedEntitySelection(entityId,"Outliner");

        ImGui::Separator();

        if (roadSafeMenuItem(
            *visible ? "Hide" : "Show",
            *visible ? UiGlyph::VisibilityOff : UiGlyph::Eye))
        {
            *visible=!*visible;
        gRoadSafeCase.touch();
        }

        if (roadSafeMenuItem(
            *locked ? "Unlock" : "Lock",
            *locked ? UiGlyph::Unlock : UiGlyph::Lock))
        {
            *locked=!*locked;
        gRoadSafeCase.touch();
        }

        ImGui::Separator();
        ImGui::BeginDisabled(*locked);

        if (roadSafeMenuItem("Delete",UiGlyph::Delete,"Del"))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::Delete,
                entityId
            );
        }

        ImGui::EndDisabled();

        ImGui::EndPopup();
    }

    ImGui::PopID();
}

static void propertyVec3Row(
    const char* label,
    const char* id,
    float values[3],
    float speed)
{
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s",label);

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::DragFloat3(id,values,speed);
}

static void propertyTextRow(
    const char* label,
    const char* value)
{
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::TextDisabled("%s",label);

    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(value);
}
// ROADSAFE_UI_COMPONENT_INCLUDE_V21: Outliner Panel
#include "roadsafe/ui/panels/outliner_panel.inl"

// ROADSAFE_UI_COMPONENT_INCLUDE_V21: Properties Window
#include "roadsafe/ui/panels/properties_window.inl"

// ROADSAFE_UI_COMPONENT_INCLUDE_V21: Node Editor Panel
#include "roadsafe/ui/panels/node_editor_panel.inl"

// ROADSAFE_UI_COMPONENT_INCLUDE_V21: Timeline Panel
#include "roadsafe/ui/panels/timeline_panel.inl"

static void enforceRightPanelBounds(ImGuiID rightNodeId)
{
    ImGuiDockNode* rightNode=ImGui::DockBuilderGetNode(rightNodeId);
    if (!rightNode) return;
    ImGuiDockNode* parent=rightNode->ParentNode;
    if (!parent | parent->SplitAxis!=ImGuiAxis_X) return;
    const float screenWidth=ImGui::GetMainViewport()->WorkSize.x;
    float desired=std::clamp(rightNode->Size.x,RIGHT_PANEL_MIN_WIDTH,RIGHT_PANEL_MAX_WIDTH);
    if (parent->ChildNodes[1]==rightNode)
    {
        const float splitX=parent->Pos.x+parent->Size.x-desired;
        if (parent->ChildNodes[0]) parent->ChildNodes[0]->SizeRef.x=splitX-parent->Pos.x;
        if (parent->ChildNodes[1]) parent->ChildNodes[1]->SizeRef.x=desired;
    }
    else rightNode->SizeRef.x=desired;
    if (screenWidth<
        RIGHT_PANEL_MAX_WIDTH+
        RIGHT_PANEL_MIN_WIDTH)
    {
        rightNode->SizeRef.x=
            std::max(
                330.0f,
                std::min(
                    RIGHT_PANEL_MAX_WIDTH,
                    screenWidth*0.33f
                )
            );
    }
}

// ROADSAFE_UI_COMPONENT_INCLUDE_V20: Main Menu
#include "roadsafe/ui/shell/main_menu.inl"

static bool shellShortcutPressed(
    ImGuiKey key,
    bool ctrl=false,
    bool shift=false,
    bool alt=false)
{
    const ImGuiIO& io=ImGui::GetIO();

    if (!ImGui::IsKeyPressed(key,false))
        return false;

    return
        io.KeyCtrl==ctrl &&
        io.KeyShift==shift &&
        io.KeyAlt==alt &&
        !io.KeySuper;
}

static void setShortcutToast(const char* text)
{
    std::snprintf(
        gEditorShell.shortcutToast,
        sizeof(gEditorShell.shortcutToast),
        "%s",
        text ? text : ""
    );

    gEditorShell.shortcutToastUntil=
        ImGui::GetTime()+1.35;
}

static void requestShortcutFocus(
    int request,
    const char* toast)
{
    gEditorShell.shortcutFocusRequest=request;
    setShortcutToast(toast);
}

struct SovereignCommand
{
    int id;
    const char* name;
    const char* detail;
    const char* shortcut;
};

static bool sovereignCommandMatches(
    const char* text,
    const char* query)
{
    if (!query || !query[0])
        return true;

    if (!text)
        return false;

    std::string haystack(text);
    std::string needle(query);

    for (char& ch: haystack)
    {
        if (ch>='A' && ch<='Z')
            ch=static_cast<char>(
                ch-'A'+'a'
            );
    }

    for (char& ch: needle)
    {
        if (ch>='A' && ch<='Z')
            ch=static_cast<char>(
                ch-'A'+'a'
            );
    }

    return haystack.find(needle)!=
        std::string::npos;
}

static void executeSovereignCommand(
    int commandId)
{
    switch (commandId)
    {
        // Workspaces / editors.
        case 1:
            gEditorShell.shortcutFocusRequest=1;
            break;

        case 2:
            gEditorShell.shortcutFocusRequest=2;
            break;

        case 3:
            gEditorShell.shortcutFocusRequest=3;
            break;

        case 4:
            gEditorShell.shortcutFocusRequest=4;
            break;

        case 5:
            gEditorShell.showTimeline=true;
            gEditorShell.shortcutFocusRequest=5;
            break;

        case 6:
            gEditorShell.showNodeEditor=true;
            gEditorShell.shortcutFocusRequest=6;
            break;

        // Panels.
        case 10:
            gEditorShell.showOutliner=
                !gEditorShell.showOutliner;
            break;

        case 11:
            gEditorShell.showProperties=
                !gEditorShell.showProperties;
            break;

        case 12:
            gEditorShell.showTimeline=
                !gEditorShell.showTimeline;
            break;

        case 13:
            gEditorShell.showNodeEditor=
                !gEditorShell.showNodeEditor;
            break;

        // Layout / editor state.
        case 20:
            gEditorShell.resetLayoutRequested=true;
            break;

        case 21:
            gEditorShell.snapEnabled=
                !gEditorShell.snapEnabled;
            break;

        case 22:
            gEditorShell.showShortcutReference=true;
            break;

        // Transform tools.
        case 30:
            gEditorShell.transformMode=0;
            break;

        case 31:
            gEditorShell.transformMode=1;
            break;

        case 32:
            gEditorShell.transformMode=2;
            break;

        case 33:
            gEditorShell.transformMode=3;
            break;

        // Workstation modes.
        case 40:
            gSovereignEditorMode=
                SovereignEditorMode::Scene;
            break;

        case 41:
            gSovereignEditorMode=
                SovereignEditorMode::Evidence;
            break;

        case 42:
            gSovereignEditorMode=
                SovereignEditorMode::Measure;
            break;

        case 43:
            gSovereignEditorMode=
                SovereignEditorMode::Reconstruct;
            break;

        case 44:
            gSovereignEditorMode=
                SovereignEditorMode::Review;
            break;

        default:
            break;
    }

    gEditorShell.showCommandPalette=false;
    gEditorShell.commandPaletteSelection=0;
    gEditorShell.commandSearch[0]=0;
}

// ROADSAFE_UI_COMPONENT_INCLUDE_V21: Command Palette
#include "roadsafe/ui/shell/command_palette.inl"

static void handleGlobalShortcuts()
{
    const ImGuiIO& io=ImGui::GetIO();

    // --------------------------------------------------------
    // HELP / COMMANDS
    // --------------------------------------------------------

    if (shellShortcutPressed(ImGuiKey_F1))
    {
        gEditorShell.showShortcutReference=
            !gEditorShell.showShortcutReference;

        setShortcutToast("Keyboard Shortcuts");
    }

    if (shellShortcutPressed(
        ImGuiKey_P,
        true,
        true,
        false))
    {
        gEditorShell.focusCommandSearch=true;
        gEditorShell.showCommandPalette=true;
        setShortcutToast("Command Search");
    }

    if (shellShortcutPressed(
        ImGuiKey_K,
        true,
        false,
        false))
    {
        gEditorShell.focusCommandSearch=true;
        gEditorShell.showCommandPalette=true;
        setShortcutToast("Command Search");
    }

    // --------------------------------------------------------
    // WORKSPACE TABS
    // --------------------------------------------------------

    if (shellShortcutPressed(
        ImGuiKey_1,
        true))
    {
        requestShortcutFocus(
            1,
            "Case View"
        );
    }

    if (shellShortcutPressed(
        ImGuiKey_2,
        true))
    {
        requestShortcutFocus(
            2,
            "Evidence"
        );
    }

    if (shellShortcutPressed(
        ImGuiKey_3,
        true))
    {
        requestShortcutFocus(
            3,
            "Analysis"
        );
    }

    if (shellShortcutPressed(
        ImGuiKey_4,
        true))
    {
        requestShortcutFocus(
            4,
            "Viewport"
        );
    }

    if (shellShortcutPressed(
        ImGuiKey_5,
        true))
    {
        gEditorShell.showTimeline=true;

        requestShortcutFocus(
            5,
            "Timeline"
        );
    }

    if (shellShortcutPressed(
        ImGuiKey_6,
        true))
    {
        gEditorShell.showNodeEditor=true;

        requestShortcutFocus(
            6,
            "Node Editor"
        );
    }

    // --------------------------------------------------------
    // PANEL VISIBILITY
    // --------------------------------------------------------

    if (shellShortcutPressed(
        ImGuiKey_O,
        true,
        true))
    {
        gEditorShell.showOutliner=
            !gEditorShell.showOutliner;

        setShortcutToast(
            gEditorShell.showOutliner
                ? "Outliner Shown"
                : "Outliner Hidden"
        );
    }

    if (shellShortcutPressed(
        ImGuiKey_I,
        true,
        true))
    {
        gEditorShell.showProperties=
            !gEditorShell.showProperties;

        setShortcutToast(
            gEditorShell.showProperties
                ? "Properties Shown"
                : "Properties Hidden"
        );
    }

    if (shellShortcutPressed(
        ImGuiKey_T,
        true,
        true))
    {
        gEditorShell.showTimeline=
            !gEditorShell.showTimeline;

        setShortcutToast(
            gEditorShell.showTimeline
                ? "Timeline Shown"
                : "Timeline Hidden"
        );
    }

    if (shellShortcutPressed(
        ImGuiKey_N,
        true,
        true))
    {
        gEditorShell.showNodeEditor=
            !gEditorShell.showNodeEditor;

        setShortcutToast(
            gEditorShell.showNodeEditor
                ? "Node Editor Shown"
                : "Node Editor Hidden"
        );
    }

    if (shellShortcutPressed(
        ImGuiKey_R,
        true,
        true))
    {
        gEditorShell.resetLayoutRequested=true;
        setShortcutToast("Workspace Layout Reset");
    }

    // --------------------------------------------------------
    // UNMODIFIED EDITOR HOTKEYS
    // --------------------------------------------------------

    if (io.WantTextInput)
        return;

    if (shellShortcutPressed(
        ImGuiKey_Tab,
        false,
        true))
    {
        gEditorShell.snapEnabled=
            !gEditorShell.snapEnabled;

        setShortcutToast(
            gEditorShell.snapEnabled
                ? "Snapping On"
                : "Snapping Off"
        );
    }

    if (shellShortcutPressed(ImGuiKey_Q))
    {
        gEditorShell.transformMode=0;
        setShortcutToast("Select Tool");
    }

    if (shellShortcutPressed(ImGuiKey_W))
    {
        gEditorShell.transformMode=1;
        setShortcutToast("Move Tool");
    }

    if (shellShortcutPressed(ImGuiKey_E))
    {
        gEditorShell.transformMode=2;
        setShortcutToast("Rotate Tool");
    }

    if (shellShortcutPressed(ImGuiKey_R))
    {
        gEditorShell.transformMode=3;
        setShortcutToast("Scale Tool");
    }

    if (shellShortcutPressed(
        ImGuiKey_A,
        false,
        true))
    {
        clearSharedEntitySelection("Shortcut");
        setShortcutToast("Selection Cleared");
    }
}

static void applyShortcutFocusRequest()
{
    switch (gEditorShell.shortcutFocusRequest)
    {
        case 1:
            ImGui::SetWindowFocus("Case View");
            break;

        case 2:
            ImGui::SetWindowFocus("Evidence");
            break;

        case 3:
            ImGui::SetWindowFocus("Analysis");
            break;

        case 4:
            ImGui::SetWindowFocus("Viewport");
            break;

        case 5:
            ImGui::SetWindowFocus("Timeline");
            break;

        case 6:
            ImGui::SetWindowFocus("Node Editor");
            break;

        default:
            break;
    }

    gEditorShell.shortcutFocusRequest=0;
}

// ROADSAFE_UI_COMPONENT_INCLUDE_V21: Shortcut Toast
#include "roadsafe/ui/shell/shortcut_toast.inl"

// ROADSAFE_UI_COMPONENT_INCLUDE_V21: Shortcut Reference
#include "roadsafe/ui/shell/shortcut_reference.inl"

static void pushUnifiedButtonTheme()
{
    ImGui::PushStyleVar(
        ImGuiStyleVar_FrameRounding,
        5.0f
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_FrameBorderSize,
        1.0f
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(12.0f,7.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(0.18f,0.19f,0.21f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(0.23f,0.25f,0.28f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.14f,0.36f,0.67f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.34f,0.37f,0.42f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.95f,0.96f,0.98f,1.0f)
    );
}

static void popUnifiedButtonTheme()
{
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(3);
}

static void drawUnifiedButtonLabel(
    ImDrawList* drawList,
    const ImVec2& minPos,
    const ImVec2& maxPos,
    const char* label,
    const ImVec4& textColor,
    bool enabled)
{
    if (!label | !label[0])
        return;

    const ImVec2 textSize=
        ImGui::CalcTextSize(label);

    const ImVec2 textPos=
        ImVec2(
            minPos.x+(maxPos.x-minPos.x-textSize.x)*0.5f,
            minPos.y+(maxPos.y-minPos.y-textSize.y)*0.5f-0.5f
        );

    ImVec4 primary=textColor;
    ImVec4 secondary=textColor;

    if (!enabled)
    {
        primary=ImVec4(
            primary.x*0.75f,
            primary.y*0.75f,
            primary.z*0.75f,
            0.85f
        );

        secondary=primary;
    }

    drawList->AddText(
        textPos,
        ImGui::ColorConvertFloat4ToU32(primary),
        label
    );

    drawList->AddText(
        ImVec2(textPos.x+0.55f,textPos.y),
        ImGui::ColorConvertFloat4ToU32(secondary),
        label
    );
}static constexpr float SOVEREIGN_STATUS_BAR_HEIGHT=30.0f;

// SOVEREIGN_STATUS_DIAGNOSTICS_V1
static int sovereignProblemErrorCount();
static int sovereignProblemWarningCount();
// ROADSAFE_UI_COMPONENT_INCLUDE_V21: Status Bar
#include "roadsafe/ui/shell/status_bar.inl"

// SOVEREIGN_OUTPUT_PROBLEMS_V2
enum class SovereignDiagnosticLevel
{
    Info=0,
    Warning,
    Error
};

struct SovereignOutputEntry
{
    double timeSeconds=0.0;
    SovereignDiagnosticLevel level=
        SovereignDiagnosticLevel::Info;
    std::string source;
    std::string message;
};

struct SovereignProblemEntry
{
    SovereignDiagnosticLevel level=
        SovereignDiagnosticLevel::Info;
    std::string source;
    std::string message;
};

static std::vector<SovereignOutputEntry>
    gSovereignOutputEntries;

static std::vector<SovereignProblemEntry>
    gSovereignProblemEntries;

static void sovereignOutputWrite(
    SovereignDiagnosticLevel level,
    const char* source,
    const char* message)
{
    SovereignOutputEntry entry;

    entry.timeSeconds=
        ImGui::GetTime();

    entry.level=level;

    entry.source=
        source
            ? source
            : "Application";

    entry.message=
        message
            ? message
            : "";

    gSovereignOutputEntries.push_back(
        entry
    );

    if (gSovereignOutputEntries.size()>500)
    {
        gSovereignOutputEntries.erase(
            gSovereignOutputEntries.begin(),
            gSovereignOutputEntries.begin()+100
        );
    }
}

static void sovereignProblemAdd(
    SovereignDiagnosticLevel level,
    const char* source,
    const char* message)
{
    SovereignProblemEntry entry;

    entry.level=level;

    entry.source=
        source
            ? source
            : "Application";

    entry.message=
        message
            ? message
            : "";

    gSovereignProblemEntries.push_back(
        entry
    );
}

static const char*
sovereignDiagnosticLevelName(
    SovereignDiagnosticLevel level)
{
    switch (level)
    {
        case SovereignDiagnosticLevel::Info:
            return "INFO";

        case SovereignDiagnosticLevel::Warning:
            return "WARNING";

        case SovereignDiagnosticLevel::Error:
            return "ERROR";
    }

    return "INFO";
}

static ImVec4
sovereignDiagnosticLevelColor(
    SovereignDiagnosticLevel level)
{
    switch (level)
    {
        case SovereignDiagnosticLevel::Info:
            return ImVec4(
                0.66f,
                0.74f,
                0.86f,
                1.0f
            );

        case SovereignDiagnosticLevel::Warning:
            return ImVec4(
                0.95f,
                0.72f,
                0.24f,
                1.0f
            );

        case SovereignDiagnosticLevel::Error:
            return ImVec4(
                0.92f,
                0.38f,
                0.38f,
                1.0f
            );
    }

    return colorMuted();
}

// ROADSAFE_UI_COMPONENT_INCLUDE_V21: Output Panel
#include "roadsafe/ui/panels/output_panel.inl"

static int sovereignProblemErrorCount()
{
    int count=0;

    for (const SovereignProblemEntry& entry:
         gSovereignProblemEntries)
    {
        if (entry.level==
            SovereignDiagnosticLevel::Error)
        {
            count++;
        }
    }

    return count;
}

static int sovereignProblemWarningCount()
{
    int count=0;

    for (const SovereignProblemEntry& entry:
         gSovereignProblemEntries)
    {
        if (entry.level==
            SovereignDiagnosticLevel::Warning)
        {
            count++;
        }
    }

    return count;
}
// ROADSAFE_UI_COMPONENT_INCLUDE_V21: Problems Panel
#include "roadsafe/ui/panels/problems_panel.inl"

static void drawInterface()
{
    initializeRoadSafeEditorRecords();
    processRoadSafePendingSceneAction();
static bool layoutBuilt=false;
    static ImGuiID rightDockNodeId=0;

    if (!gViewportFullscreen &&
        !gNodeEditorFullscreen)
    {
        drawMainMenuBar();
    }
    handleGlobalShortcuts();
    pushUnifiedButtonTheme();

    if (gEditorShell.resetLayoutRequested)
    {
        layoutBuilt=false;
        gEditorShell.resetLayoutRequested=false;
    }

    ImGuiViewport* viewport=ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(
        ImVec2(
            viewport->WorkSize.x,
            std::max(
                100.0f,
                viewport->WorkSize.y-
                SOVEREIGN_STATUS_BAR_HEIGHT
            )
        ),
        ImGuiCond_Always
    );
    ImGui::SetNextWindowViewport(viewport->ID);

    const ImGuiWindowFlags hostFlags=
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(7.0f,6.0f)
    );

    ImGui::Begin(
        "Sovereign Workspace",
        nullptr,
        hostFlags
    );

    // Persistent editor toolbar above the dockspace.
    drawEditorToolbar();

    ImGui::Spacing();

    const ImGuiID dockspace=
        ImGui::GetID("SovereignDockspace");

    const ImVec2 dockSize=
        ImGui::GetContentRegionAvail();

    ImGui::DockSpace(
        dockspace,
        dockSize,
        ImGuiDockNodeFlags_PassthruCentralNode
    );

    if (!layoutBuilt)
    {
        ImGui::DockBuilderRemoveNode(dockspace);

        ImGui::DockBuilderAddNode(
            dockspace,
            ImGuiDockNodeFlags_DockSpace
        );

        ImGui::DockBuilderSetNodeSize(
            dockspace,
            dockSize
        );

        ImGuiID center=dockspace;
        ImGuiID right=0;
        ImGuiID bottom=0;
        ImGuiID rightTop=0;

        ImGui::DockBuilderSplitNode(
            center,
            ImGuiDir_Right,
            0.245f,
            &right,
            &center
        );

        rightDockNodeId=right;

        ImGui::DockBuilderSplitNode(center,ImGuiDir_Down,0.26f,
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

        ImGui::DockBuilderDockWindow(
            "Case View",
            center
        );

        ImGui::DockBuilderDockWindow(
            "Evidence",
            center
        );

        ImGui::DockBuilderDockWindow(
            "Viewport",
            center
        );

        ImGui::DockBuilderDockWindow(
            "Analysis",
            center
        );

        ImGui::DockBuilderDockWindow(
            "Asset Library",
            center
        );

        ImGui::DockBuilderDockWindow(
            "Timeline",
            bottom
        );
        ImGui::DockBuilderDockWindow(
            "Output",
            bottom
        );

        ImGui::DockBuilderDockWindow(
            "Problems",
            bottom
        );

        ImGui::DockBuilderDockWindow(
            "Node Editor",
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

        ImGui::DockBuilderFinish(dockspace);
        layoutBuilt=true;
    }

    ImGui::End();
    ImGui::PopStyleVar();

    if (rightDockNodeId)
        enforceRightPanelBounds(rightDockNodeId);

    if (gViewportFullscreen)
    {
        // Dockspace/layout code above still runs so every original
        // dock node remains alive while the Viewport is full screen.
        drawViewportView();

        // These may intentionally overlay the full-screen viewport.
        drawCommandPalette();
        drawShortcutReferenceWindow();
        drawShortcutToast();

        popUnifiedButtonTheme();
        return;
    }
    if (gNodeEditorFullscreen)
    {
        // Keep the dockspace alive but render only Node Editor
        // and intentional overlays while fullscreen.
        drawNodeEditor();

        drawCommandPalette();
        drawShortcutReferenceWindow();
        drawShortcutToast();

        popUnifiedButtonTheme();
        return;
    }
drawCaseView();
    drawEvidenceView();
    drawAnalysisView();
    drawViewportView();

    if (gEditorShell.showOutliner)
        drawOutliner();

    if (gEditorShell.showProperties)
        drawProperties();

    if (gEditorShell.showTimeline)
        drawTimeline();

    if (gEditorShell.showNodeEditor)
        drawNodeEditor();

    roadsafe::drawAssetLibrary(
        gRoadSafeAssetLibrary,
        gRoadSafeCase,
        gEditorShell.selectedEntity,
        gRoadSafeRenderer,
        std::filesystem::path(
            SFE_ASSET_DIR
        )
    );

    applyShortcutFocusRequest();
    drawCommandPalette();
    drawShortcutReferenceWindow();
    drawShortcutToast();

    drawSovereignStatusBar();

    popUnifiedButtonTheme();
    drawOutputPanel();
    drawProblemsPanel();}

int main()
{
    if (!glfwInit()) { std::printf("[FATAL] Failed to initialize GLFW.\n"); return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,6);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE,GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);

    GLFWwindow* window=glfwCreateWindow(WINDOW_WIDTH,WINDOW_HEIGHT,"RoadSafe AR v0.1.0 - OpenGL 4.6",nullptr,nullptr);
    if (!window) { glfwTerminate(); return -1; }

    // Keep the workstation usable: prevents dock panels from being
    // crushed into the narrow state shown by the previous layout.
    glfwSetWindowSizeLimits(
        window,
        1100,
        700,
        GLFW_DONT_CARE,
        GLFW_DONT_CARE
    );
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

#ifdef _WIN32
    applyNativeWindowTheme(window);
#endif

    glfwShowWindow(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) { glfwDestroyWindow(window); glfwTerminate(); return -1; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    initializeGoogleMaterialIcons();
    ImGuiIO& io=ImGui::GetIO();
    io.IniFilename=nullptr;
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags|=ImGuiConfigFlags_DockingEnable;
    applySovereignTheme();

    const std::filesystem::path rubikPath=assetPath("fonts/Rubik-Regular.ttf");
    ImFont* rubik=io.Fonts->AddFontFromFileTTF(rubikPath.string().c_str(),18.5f);
    if (!rubik) std::printf("[WARN] Could not load %s\n",rubikPath.string().c_str());

    if (!ImGui_ImplGlfw_InitForOpenGL(window,true)) { ImGui::DestroyContext(); glfwDestroyWindow(window); glfwTerminate(); return -1; }
    if (!ImGui_ImplOpenGL3_Init("#version 460")) { ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext(); glfwDestroyWindow(window); glfwTerminate(); return -1; }

    const char* iconPaths[]={
        "assets/icons/blender/select.svg","assets/icons/blender/move.svg","assets/icons/blender/rotate.svg",
        "assets/icons/blender/scale.svg","assets/icons/blender/vehicle.svg","assets/icons/blender/evidence.svg",
        "assets/icons/blender/measure.svg","assets/icons/blender/grid.svg","assets/icons/blender/axes.svg"
    };
    for (size_t i=0;i<gToolIcons.size();++i) gToolIcons[i]=loadSvgTexture(iconPaths[i]);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (rubik) ImGui::PushFont(rubik);
        drawInterface();
        
        if (gEditorShell.requestExit) glfwSetWindowShouldClose(window,GLFW_TRUE);
if (rubik) ImGui::PopFont();
        ImGui::Render();
        glClearColor(0.024f,0.026f,0.029f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Tool icon textures are owned by the OpenGL context.
// They are released automatically when the context is destroyed.
// Do not place ImGui shutdown inside the icon loop.
    roadsafe::shutdownAssetLibraryPreviewRenderer();
    gRoadSafeRenderer.shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}









































































