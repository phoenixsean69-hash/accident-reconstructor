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
    if (key=="SET CASE DETAILS") return {true,UiGlyph::Edit,true,"SET CASE DETAILS","Set Case Details"};
    if (key=="CASE DETAILS") return {true,UiGlyph::Edit,true,"CASE DETAILS","Case Details"};
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

    constexpr float prevW=66.0f;
    constexpr float pipelineW=112.0f;
    constexpr float nextW=66.0f;
    constexpr float gap=6.0f;

    const float actionWidth=
        prevW+
        pipelineW+
        nextW+
        gap*2.0f;

    const float actionX=
        right-
        actionWidth-
        4.0f;

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
static void drawOutliner()
{
    static bool groundVisible=true;
    static bool roadVisible=true;
    static bool vehicleAVisible=true;
    static bool vehicleBVisible=true;
    static bool skidVisible=true;
    static bool markerVisible=true;
    static bool debrisVisible=true;
    static bool distanceVisible=true;
    static bool angleVisible=true;

    static bool groundLocked=false;
    static bool roadLocked=true;
    static bool vehicleALocked=false;
    static bool vehicleBLocked=false;
    static bool skidLocked=false;
    static bool markerLocked=false;
    static bool debrisLocked=false;
    static bool distanceLocked=false;
    static bool angleLocked=false;

    ImGui::Begin(
        "Outliner",
        &gEditorShell.showOutliner,
        ImGuiWindowFlags_NoMove
    );
    // SOVEREIGN_PANEL_SCROLL_LIMITS_V1
    // Inspector-style panels never keep stale horizontal scroll.
    ImGui::SetScrollX(0.0f);
    beginEditorContextHeader(
        "##OutlinerContextHeader"
    );

    ImGui::TextDisabled("SCENE");

    editorContextSeparator();

    ImGui::Text(
        "%zu objects",
        roadSafeActiveSceneEntityCount()
    );

    editorContextSeparator();

    ImGui::TextDisabled("Selected");
    ImGui::SameLine(0.0f,5.0f);

    ImGui::Text(
        "%s",
        gEditorShell.selectedEntity!=0
            ? selectedEntityName()
            : "None"
    );

    endEditorContextHeader();

    // Header / filter.
    ImGui::Text("SCENE OUTLINER");
    ImGui::SameLine();

    const float addW=34.0f;
    ImGui::SetCursorPosX(
        std::max(
            ImGui::GetCursorPosX(),
            ImGui::GetWindowWidth()-addW-10.0f
        )
    );

    if (ImGui::SmallButton("+"))
        ImGui::OpenPopup("##OutlinerAddPopup");

    if (ImGui::BeginPopup("##OutlinerAddPopup"))
    {
        ImGui::TextDisabled("ADD OBJECT");
        ImGui::Separator();

        if (roadSafeMenuItem("Vehicle", UiGlyph::Vehicle))
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddVehicle
            );

        if (roadSafeMenuItem("Evidence", UiGlyph::Evidence))
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddEvidence
            );

        if (roadSafeMenuItem("Measurement", UiGlyph::Measurement))
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddMeasurement
            );

        if (roadSafeMenuItem("Scene Marker", UiGlyph::ForensicMarker))
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddMarker
            );

        ImGui::EndPopup();
    }

    ImGui::Separator();

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint(
        "##OutlinerSearch",
        "Filter scene...",
        gEditorShell.outlinerSearch,
        sizeof(gEditorShell.outlinerSearch)
    );

    ImGui::Spacing();

    if (ImGui::BeginTable(
        "SceneOutlinerTable",
            5,
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerH))
    {
        ImGui::TableSetupColumn(
            "OBJECT",
            ImGuiTableColumnFlags_WidthStretch,
            1.0f
        );

        ImGui::TableSetupColumn(
            "##VisibilityColumn",
            ImGuiTableColumnFlags_WidthFixed,
            36.0f
        );

        ImGui::TableSetupColumn(
            "##LockColumn",
            ImGuiTableColumnFlags_WidthFixed,
            36.0f
        );

        ImGui::TableSetupColumn(
            "##FocusColumn",
            ImGuiTableColumnFlags_WidthFixed,
            36.0f
        );

        ImGui::TableSetupColumn(
            "##MoreColumn",
            ImGuiTableColumnFlags_WidthFixed,
            36.0f
        );

        ImGui::TableNextRow(
            ImGuiTableRowFlags_Headers,
            31.0f
        );

        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("OBJECT");

        auto drawOutlinerHeaderIcon = [](
            UiGlyph glyph,
            const char* tooltip)
        {
            const ImVec2 hp=ImGui::GetCursorScreenPos();

            drawGlyph(
                ImGui::GetWindowDrawList(),
                glyph,
                ImVec2(
                    hp.x+16.0f,
                    hp.y+14.0f
                ),
                20.0f,
                toU32(colorMuted())
            );

            ImGui::Dummy(
                ImVec2(30.0f,26.0f)
            );

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s",tooltip);
        };

        ImGui::TableSetColumnIndex(1);
        drawOutlinerHeaderIcon(
            UiGlyph::Eye,
            "Visibility"
        );

        ImGui::TableSetColumnIndex(2);
        drawOutlinerHeaderIcon(
            UiGlyph::Lock,
            "Lock state"
        );

        ImGui::TableSetColumnIndex(3);
        drawOutlinerHeaderIcon(
            UiGlyph::Target,
            "Focus in Viewport"
        );

        ImGui::TableSetColumnIndex(4);
        drawOutlinerHeaderIcon(
            UiGlyph::More,
            "More actions"
        );


        const auto groupHasMatches=
            [](roadsafe::SceneEntityKind kind)
        {
            for (const auto& entity :
                 gRoadSafeCase.sceneEntities)
            {
                if (entity.active &&
                    entity.kind==kind &&
                    shellLabelMatches(
                        entity.name.c_str()
                    ))
                {
                    return true;
                }
            }

            return false;
        };

        const auto drawSceneGroup=
            [&](const char* label,
                roadsafe::SceneEntityKind kind)
        {
            if (!groupHasMatches(kind))
                return;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            const bool open=
                ImGui::TreeNodeEx(
                    label,
                    ImGuiTreeNodeFlags_DefaultOpen |
                    ImGuiTreeNodeFlags_SpanAvailWidth
                );

            if (!open)
                return;

            for (auto& entity :
                 gRoadSafeCase.sceneEntities)
            {
                if (!entity.active ||
                    entity.kind!=kind ||
                    !shellLabelMatches(
                        entity.name.c_str()
                    ))
                {
                    continue;
                }

                outlinerLeafRow(
                    entity.name.c_str(),
                    roadSafeSceneEntityGlyph(entity),
                    entity.legacyId,
                    &entity.visible,
                    &entity.locked
                );
            }

            ImGui::TreePop();
        };

        drawSceneGroup(
            "Environment",
            roadsafe::SceneEntityKind::Environment
        );

        drawSceneGroup(
            "Vehicles",
            roadsafe::SceneEntityKind::Vehicle
        );

        drawSceneGroup(
            "Evidence",
            roadsafe::SceneEntityKind::Evidence
        );

        drawSceneGroup(
            "Measurements",
            roadsafe::SceneEntityKind::Measurement
        );        ImGui::EndTable();
    }

    // Bottom selection summary.
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("SELECTION");

    if (gEditorShell.selectedEntity==0)
        ImGui::TextDisabled("No scene object selected.");
    else
        ImGui::Text("%s",selectedEntityName());

    
    // Real entity rename.
    if (gEditorShell.selectedEntity!=0 &&
        ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows) &&
        ImGui::IsKeyPressed(ImGuiKey_F2))
    {
        beginSovereignEntityRename(
            gEditorShell.selectedEntity
        );
    }

    drawSovereignRenamePopup();

    const bool outlinerFocused=
        ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows
        );

    const bool renameOpen=
        ImGui::IsPopupOpen(
            "Rename Entity"
        );

    if (outlinerFocused &&
        !renameOpen &&
        gEditorShell.selectedEntity!=0)
    {
        if (ImGui::GetIO().KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_D))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::Duplicate,
                gEditorShell.selectedEntity
            );
        }

        if (ImGui::IsKeyPressed(
                ImGuiKey_Delete) &&
            !sovereignEntityLocked(
                gEditorShell.selectedEntity
            ))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::Delete,
                gEditorShell.selectedEntity
            );
        }
    }

    // Row/menu actions are deferred until all scene-vector pointers
    // used by this frame have finished rendering.
    processRoadSafePendingSceneAction();

ImGui::End();
}

static void drawProperties()
{
    static float position[3]={0.0f,0.0f,0.0f};
    static float rotation[3]={0.0f,0.0f,0.0f};
    static float scale[3]={1.0f,1.0f,1.0f};

    static bool objectVisible=true;
    static bool objectLocked=false;
    static char objectName[128]="Untitled Object";

    ImGui::Begin(
        "Properties",
        &gEditorShell.showProperties,
        ImGuiWindowFlags_NoMove
    );
    // SOVEREIGN_PANEL_SCROLL_LIMITS_V1
    // Inspector-style panels never keep stale horizontal scroll.
    ImGui::SetScrollX(0.0f);
    // Responsive Properties context header.
    // The previous single-line header clipped "Object Properties"
    // in the normal narrow inspector width.
    beginSurface(
        "##PropertiesContextHeader",
        ImVec2(0.0f,64.0f),
        false,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    const ImVec2 propertyHeaderPos=
        ImGui::GetCursorScreenPos();

    UiGlyph propertyHeaderGlyph=
        UiGlyph::Info;

    if (const auto* propertyEntity=
            roadSafeSceneEntityConst(
                gEditorShell.selectedEntity
            ))
    {
        switch (propertyEntity->kind)
        {
            case roadsafe::SceneEntityKind::Vehicle:
                propertyHeaderGlyph=UiGlyph::Vehicle;
                break;

            case roadsafe::SceneEntityKind::Evidence:
                propertyHeaderGlyph=UiGlyph::Evidence;
                break;

            case roadsafe::SceneEntityKind::Measurement:
                propertyHeaderGlyph=UiGlyph::Measurement;
                break;

            case roadsafe::SceneEntityKind::Environment:
                propertyHeaderGlyph=UiGlyph::Environment;
                break;
        }
    }

    drawIconBadge(
        propertyHeaderGlyph,
        propertyHeaderPos,
        30.0f,
        false
    );

    ImGui::SetCursorScreenPos(
        ImVec2(
            propertyHeaderPos.x+42.0f,
            propertyHeaderPos.y
        )
    );

    ImGui::TextDisabled(
        gEditorShell.selectedEntity!=0
            ? "INSPECTOR / OBJECT"
            : "INSPECTOR / SCENE"
    );

    ImGui::SetCursorScreenPos(
        ImVec2(
            propertyHeaderPos.x+42.0f,
            propertyHeaderPos.y+23.0f
        )
    );

    ImGui::PushTextWrapPos(
        ImGui::GetWindowPos().x+
        ImGui::GetWindowContentRegionMax().x
    );

    ImGui::Text(
        "%s",
        gEditorShell.selectedEntity!=0
            ? selectedEntityName()
            : "Scene Settings"
    );

    ImGui::PopTextWrapPos();

    endSurface();
    ImGui::Spacing();
    drawDeepPropertiesInspectorBody();
    // SOVEREIGN_PROPERTIES_LEGACY_CLEANED_V1
    // Deep Properties V3 is now the single inspector body.
    // The older duplicate selection / transform / object / analysis
    // inspector has intentionally been removed.
ImGui::End();
}

static void drawNodeEditor()
{
    enum class PinType
    {
        EvidenceDistance=0,
        VehicleState,
        Scalar
    };

    enum class NodeType
    {
        EvidenceInput=0,
        VehicleInput,
        SkidAnalysis,
        SpeedAnalysis,
        MomentumAnalysis,
        ResultOutput
    };

    struct Node
    {
        int id=0;
        NodeType type=NodeType::EvidenceInput;
        std::string title;
        ImVec2 pos{0.0f,0.0f};
        bool enabled=true;

        // Generic editable parameters:
        // Evidence: p0 = distance m
        // Vehicle:  p0 = mass kg, p1 = velocity m/s
        // Skid:     p0 = friction coefficient
        // Speed:    p0 = multiplier
        float p0=0.0f;
        float p1=0.0f;

        float result=0.0f;
        float result2=0.0f;
        bool resultValid=false;
        std::string error;
    };

    struct Link
    {
        int id=0;
        int fromNode=0;
        int fromPin=0;
        int toNode=0;
        int toPin=0;
    };

    struct EvalValue
    {
        PinType type=PinType::Scalar;
        bool valid=false;
        float a=0.0f;
        float b=0.0f;
        std::string error;
    };

    static bool initialized=false;

    static std::vector<Node> nodes;
    static std::vector<Link> links;

    static std::vector<int> selectedNodes;
    static std::vector<int> selectedLinks;

    static int nextNodeId=1;
    static int nextLinkId=1;

    static bool showGrid=true;
    static bool snapToGrid=true;
    static float zoom=1.0f;
    static ImVec2 pan(0.0f,0.0f);

    static bool linkDragActive=false;
    static int linkDragNode=-1;
    static int linkDragPin=-1;
    static PinType linkDragType=PinType::Scalar;

    static bool boxSelecting=false;
    static ImVec2 boxStart(0.0f,0.0f);
    static ImVec2 boxEnd(0.0f,0.0f);

    static bool nodeDragging=false;
    static ImVec2 dragMouseStart(0.0f,0.0f);
    static std::vector<std::pair<int,ImVec2>> dragStartPositions;

    static std::vector<std::string> undoStack;
    static std::vector<std::string> redoStack;

    static std::string internalClipboard;

    static char graphPath[260]="node_graph.sargraph";

    static std::string graphMessage="Graph ready";
    static bool graphMessageError=false;
    static int lastSceneSelectionRevision=-1;

    static int lastExecutedNodes=0;
    static int lastExecutionErrors=0;

    static std::string propertyEditBefore;
    static bool propertyEditChanged=false;

    auto defaultTitle = [](NodeType type) -> const char*
    {
        switch (type)
        {
            case NodeType::EvidenceInput:
                return "Evidence Input";

            case NodeType::VehicleInput:
                return "Vehicle State";

            case NodeType::SkidAnalysis:
                return "Skid Analysis";

            case NodeType::SpeedAnalysis:
                return "Speed Analysis";

            case NodeType::MomentumAnalysis:
                return "Momentum Analysis";

            case NodeType::ResultOutput:
                return "Reconstruction Result";
        }

        return "Node";
    };

    auto pinTypeLabel = [](PinType type) -> const char*
    {
        switch (type)
        {
            case PinType::EvidenceDistance:
                return "Evidence Distance";

            case PinType::VehicleState:
                return "Vehicle State";

            case PinType::Scalar:
                return "Scalar";
        }

        return "Unknown";
    };

    auto inputCount = [](NodeType type) -> int
    {
        switch (type)
        {
            case NodeType::EvidenceInput:
            case NodeType::VehicleInput:
                return 0;

            case NodeType::SkidAnalysis:
            case NodeType::SpeedAnalysis:
                return 1;

            case NodeType::MomentumAnalysis:
            case NodeType::ResultOutput:
                return 2;
        }

        return 0;
    };

    auto outputCount = [](NodeType type) -> int
    {
        return type==NodeType::ResultOutput
            ? 0
            : 1;
    };

    auto inputType = [](NodeType type,int pin) -> PinType
    {
        switch (type)
        {
            case NodeType::SkidAnalysis:
            case NodeType::SpeedAnalysis:
                return PinType::EvidenceDistance;

            case NodeType::MomentumAnalysis:
                return PinType::VehicleState;

            case NodeType::ResultOutput:
                return PinType::Scalar;

            default:
                break;
        }

        return PinType::Scalar;
    };

    auto outputType = [](NodeType type,int) -> PinType
    {
        switch (type)
        {
            case NodeType::EvidenceInput:
                return PinType::EvidenceDistance;

            case NodeType::VehicleInput:
                return PinType::VehicleState;

            case NodeType::SkidAnalysis:
            case NodeType::SpeedAnalysis:
            case NodeType::MomentumAnalysis:
                return PinType::Scalar;

            case NodeType::ResultOutput:
                break;
        }

        return PinType::Scalar;
    };

    auto inputLabel = [](NodeType type,int pin) -> const char*
    {
        switch (type)
        {
            case NodeType::SkidAnalysis:
            case NodeType::SpeedAnalysis:
                return "Evidence";

            case NodeType::MomentumAnalysis:
                return pin==0
                    ? "Vehicle A"
                    : "Vehicle B";

            case NodeType::ResultOutput:
                return pin==0
                    ? "Result A"
                    : "Result B";

            default:
                break;
        }

        return "Input";
    };

    auto outputLabel = [](NodeType type,int) -> const char*
    {
        switch (type)
        {
            case NodeType::EvidenceInput:
                return "Distance";

            case NodeType::VehicleInput:
                return "State";

            case NodeType::SkidAnalysis:
            case NodeType::SpeedAnalysis:
                return "Speed";

            case NodeType::MomentumAnalysis:
                return "Momentum";

            default:
                break;
        }

        return "Output";
    };

    auto nodeSize = [](NodeType type) -> ImVec2
    {
        if (type==NodeType::ResultOutput)
            return ImVec2(270.0f,154.0f);

        return ImVec2(252.0f,154.0f);
    };

    auto findNode = [&](int id) -> Node*
    {
        for (auto& node : nodes)
        {
            if (node.id==id)
                return &node;
        }

        return nullptr;
    };

    auto isNodeSelected = [&](int id)
    {
        return std::find(
            selectedNodes.begin(),
            selectedNodes.end(),
            id
        ) != selectedNodes.end();
    };

    auto isLinkSelected = [&](int id)
    {
        return std::find(
            selectedLinks.begin(),
            selectedLinks.end(),
            id
        ) != selectedLinks.end();
    };

    auto clearSelection = [&]()
    {
        selectedNodes.clear();
        selectedLinks.clear();
    };

    auto serializeGraph = [&]() -> std::string
    {
        std::ostringstream out;

        out << "SAR_NODE_GRAPH_V2\n";

        for (const Node& node : nodes)
        {
            out
                << "N "
                << node.id << " "
                << static_cast<int>(node.type) << " "
                << std::quoted(node.title) << " "
                << node.pos.x << " "
                << node.pos.y << " "
                << (node.enabled ? 1 : 0) << " "
                << node.p0 << " "
                << node.p1
                << "\n";
        }

        for (const Link& link : links)
        {
            out
                << "L "
                << link.id << " "
                << link.fromNode << " "
                << link.fromPin << " "
                << link.toNode << " "
                << link.toPin
                << "\n";
        }

        return out.str();
    };

    auto restoreGraph = [&](const std::string& data) -> bool
    {
        std::istringstream in(data);

        std::string header;

        if (!std::getline(in,header) |
            header!="SAR_NODE_GRAPH_V2")
        {
            return false;
        }

        std::vector<Node> restoredNodes;
        std::vector<Link> restoredLinks;

        std::string line;

        int maxNodeId=0;
        int maxLinkId=0;

        while (std::getline(in,line))
        {
            if (line.empty())
                continue;

            std::istringstream row(line);

            char kind=0;
            row >> kind;

            if (kind=='N')
            {
                Node node;
                int typeInt=0;
                int enabledInt=1;

                row
                    >> node.id
                    >> typeInt
                    >> std::quoted(node.title)
                    >> node.pos.x
                    >> node.pos.y
                    >> enabledInt
                    >> node.p0
                    >> node.p1;

                if (!row)
                    return false;

                if (typeInt<0 |
                    typeInt>
                    static_cast<int>(
                        NodeType::ResultOutput
                    ))
                {
                    return false;
                }

                node.type=
                    static_cast<NodeType>(
                        typeInt
                    );

                node.enabled=
                    enabledInt!=0;

                restoredNodes.push_back(node);

                maxNodeId=
                    std::max(
                        maxNodeId,
                        node.id
                    );
            }
            else if (kind=='L')
            {
                Link link;

                row
                    >> link.id
                    >> link.fromNode
                    >> link.fromPin
                    >> link.toNode
                    >> link.toPin;

                if (!row)
                    return false;

                restoredLinks.push_back(link);

                maxLinkId=
                    std::max(
                        maxLinkId,
                        link.id
                    );
            }
        }

        nodes=std::move(restoredNodes);
        links=std::move(restoredLinks);

        nextNodeId=maxNodeId+1;
        nextLinkId=maxLinkId+1;

        clearSelection();

        for (Node& node : nodes)
        {
            node.resultValid=false;
            node.result=0.0f;
            node.result2=0.0f;
            node.error.clear();
        }

        return true;
    };

    auto pushSnapshot = [&](const std::string& snapshot)
    {
        if (!undoStack.empty() &&
            undoStack.back()==snapshot)
        {
            return;
        }

        undoStack.push_back(snapshot);

        if (undoStack.size()>64)
        {
            undoStack.erase(
                undoStack.begin()
            );
        }

        redoStack.clear();
    };

    auto pushUndo = [&]()
    {
        pushSnapshot(
            serializeGraph()
        );
    };

    auto doUndo = [&]()
    {
        if (undoStack.empty())
            return;

        const std::string current=
            serializeGraph();

        redoStack.push_back(current);

        const std::string target=
            undoStack.back();

        undoStack.pop_back();

        if (restoreGraph(target))
        {
            graphMessage="Undo";
            graphMessageError=false;
        }
    };

    auto doRedo = [&]()
    {
        if (redoStack.empty())
            return;

        const std::string current=
            serializeGraph();

        undoStack.push_back(current);

        const std::string target=
            redoStack.back();

        redoStack.pop_back();

        if (restoreGraph(target))
        {
            graphMessage="Redo";
            graphMessageError=false;
        }
    };

    auto addNode = [&](NodeType type,ImVec2 pos)
    {
        pushUndo();

        Node node;
        node.id=nextNodeId++;
        node.type=type;
        node.title=defaultTitle(type);
        node.pos=pos;

        switch (type)
        {
            case NodeType::EvidenceInput:
                node.p0=24.0f;
                break;

            case NodeType::VehicleInput:
                node.p0=1500.0f;
                node.p1=12.0f;
                break;

            case NodeType::SkidAnalysis:
                node.p0=0.70f;
                break;

            case NodeType::SpeedAnalysis:
                node.p0=1.0f;
                break;

            case NodeType::MomentumAnalysis:
                break;

            case NodeType::ResultOutput:
                break;
        }

        nodes.push_back(node);

        clearSelection();
        selectedNodes.push_back(node.id);
    };

    auto deleteSelection = [&]()
    {
        if (selectedNodes.empty() &&
            selectedLinks.empty())
        {
            return;
        }

        pushUndo();

        links.erase(
            std::remove_if(
                links.begin(),
                links.end(),
                [&](const Link& link) -> bool
                {
                    if (isLinkSelected(link.id))
                        return true;

                    return
                        isNodeSelected(link.fromNode) ||
                        isNodeSelected(link.toNode);
                }
            ),
            links.end()
        );

        nodes.erase(
            std::remove_if(
                nodes.begin(),
                nodes.end(),
                [&](const Node& node)
                {
                    return isNodeSelected(node.id);
                }
            ),
            nodes.end()
        );

        clearSelection();

        graphMessage="Selection deleted";
        graphMessageError=false;
    };

    auto copySelection = [&]()
    {
        if (selectedNodes.empty())
            return;

        std::ostringstream out;
        out << "SAR_NODE_CLIP_V1\n";

        for (const Node& node : nodes)
        {
            if (!isNodeSelected(node.id))
                continue;

            out
                << "N "
                << node.id << " "
                << static_cast<int>(node.type) << " "
                << std::quoted(node.title) << " "
                << node.pos.x << " "
                << node.pos.y << " "
                << (node.enabled ? 1 : 0) << " "
                << node.p0 << " "
                << node.p1
                << "\n";
        }

        for (const Link& link : links)
        {
            if (isNodeSelected(link.fromNode) &&
                isNodeSelected(link.toNode))
            {
                out
                    << "L "
                    << link.id << " "
                    << link.fromNode << " "
                    << link.fromPin << " "
                    << link.toNode << " "
                    << link.toPin
                    << "\n";
            }
        }

        internalClipboard=out.str();

        ImGui::SetClipboardText(
            internalClipboard.c_str()
        );

        graphMessage="Copied selection";
        graphMessageError=false;
    };

    auto pasteFromText = [&](const std::string& data)
    {
        std::istringstream in(data);

        std::string header;

        if (!std::getline(in,header) |
            header!="SAR_NODE_CLIP_V1")
        {
            graphMessage=
                "Clipboard does not contain Sovereign nodes";

            graphMessageError=true;
            return;
        }

        struct ClipNode
        {
            int oldId=0;
            Node node;
        };

        std::vector<ClipNode> clipNodes;
        std::vector<Link> clipLinks;

        std::string line;

        while (std::getline(in,line))
        {
            if (line.empty())
                continue;

            std::istringstream row(line);

            char kind=0;
            row >> kind;

            if (kind=='N')
            {
                ClipNode item;
                int typeInt=0;
                int enabledInt=1;

                row
                    >> item.oldId
                    >> typeInt
                    >> std::quoted(item.node.title)
                    >> item.node.pos.x
                    >> item.node.pos.y
                    >> enabledInt
                    >> item.node.p0
                    >> item.node.p1;

                if (!row)
                    continue;

                item.node.type=
                    static_cast<NodeType>(
                        typeInt
                    );

                item.node.enabled=
                    enabledInt!=0;

                clipNodes.push_back(item);
            }
            else if (kind=='L')
            {
                Link link;

                row
                    >> link.id
                    >> link.fromNode
                    >> link.fromPin
                    >> link.toNode
                    >> link.toPin;

                if (row)
                    clipLinks.push_back(link);
            }
        }

        if (clipNodes.empty())
            return;

        pushUndo();

        std::vector<std::pair<int,int>> idMap;

        clearSelection();

        for (ClipNode& item : clipNodes)
        {
            const int newId=
                nextNodeId++;

            idMap.push_back(
                {item.oldId,newId}
            );

            item.node.id=newId;

            item.node.pos.x += 32.0f;
            item.node.pos.y += 32.0f;

            item.node.resultValid=false;
            item.node.error.clear();

            nodes.push_back(item.node);

            selectedNodes.push_back(newId);
        }

        auto remapId = [&](int oldId) -> int
        {
            for (const auto& pair : idMap)
            {
                if (pair.first==oldId)
                    return pair.second;
            }

            return -1;
        };

        for (const Link& oldLink : clipLinks)
        {
            const int from=
                remapId(oldLink.fromNode);

            const int to=
                remapId(oldLink.toNode);

            if (from<0 | to<0)
                continue;

            links.push_back({
                nextLinkId++,
                from,
                oldLink.fromPin,
                to,
                oldLink.toPin
            });
        }

        graphMessage="Pasted selection";
        graphMessageError=false;
    };

    auto pasteSelection = [&]()
    {
        const char* clipboard=
            ImGui::GetClipboardText();

        if (clipboard &&
            std::string(clipboard).find(
                "SAR_NODE_CLIP_V1"
            )==0)
        {
            pasteFromText(clipboard);
            return;
        }

        if (!internalClipboard.empty())
        {
            pasteFromText(
                internalClipboard
            );
        }
    };

    auto duplicateSelection = [&]()
    {
        if (selectedNodes.empty())
            return;

        copySelection();

        pasteFromText(
            internalClipboard
        );

        graphMessage="Duplicated selection";
        graphMessageError=false;
    };

    auto compatible = [&](PinType from,PinType to)
    {
        return from==to;
    };

    auto saveGraph = [&]()
    {
        std::ofstream out(
            graphPath,
            std::ios::binary
        );

        if (!out)
        {
            graphMessage=
                "Could not open graph file for saving";

            graphMessageError=true;
            return;
        }

        const std::string data=
            serializeGraph();

        out.write(
            data.data(),
            static_cast<std::streamsize>(
                data.size()
            )
        );

        graphMessage=
            std::string("Saved: ")+
            graphPath;

        graphMessageError=false;
    };

    auto loadGraph = [&]()
    {
        std::ifstream in(
            graphPath,
            std::ios::binary
        );

        if (!in)
        {
            graphMessage=
                std::string("File not found: ")+
                graphPath;

            graphMessageError=true;
            return;
        }

        std::ostringstream buffer;
        buffer << in.rdbuf();

        const std::string before=
            serializeGraph();

        if (!restoreGraph(
            buffer.str()
        ))
        {
            graphMessage=
                "Invalid Sovereign graph file";

            graphMessageError=true;
            return;
        }

        pushSnapshot(before);

        graphMessage=
            std::string("Loaded: ")+
            graphPath;

        graphMessageError=false;
    };

    if (!initialized)
    {
        Node evidence;
        evidence.id=nextNodeId++;
        evidence.type=NodeType::EvidenceInput;
        evidence.title="Skid Evidence";
        evidence.pos=ImVec2(100.0f,88.0f);
        evidence.p0=24.0f;

        Node vehicleA;
        vehicleA.id=nextNodeId++;
        vehicleA.type=NodeType::VehicleInput;
        vehicleA.title="Vehicle A";
        vehicleA.pos=ImVec2(100.0f,286.0f);
        vehicleA.p0=1500.0f;
        vehicleA.p1=12.0f;

        Node vehicleB;
        vehicleB.id=nextNodeId++;
        vehicleB.type=NodeType::VehicleInput;
        vehicleB.title="Vehicle B";
        vehicleB.pos=ImVec2(100.0f,470.0f);
        vehicleB.p0=1250.0f;
        vehicleB.p1=-8.0f;

        Node skid;
        skid.id=nextNodeId++;
        skid.type=NodeType::SkidAnalysis;
        skid.title="Skid Analysis";
        skid.pos=ImVec2(430.0f,88.0f);
        skid.p0=0.70f;

        Node momentum;
        momentum.id=nextNodeId++;
        momentum.type=NodeType::MomentumAnalysis;
        momentum.title="Momentum Analysis";
        momentum.pos=ImVec2(430.0f,350.0f);

        Node output;
        output.id=nextNodeId++;
        output.type=NodeType::ResultOutput;
        output.title="Reconstruction Result";
        output.pos=ImVec2(770.0f,210.0f);

        nodes={
            evidence,
            vehicleA,
            vehicleB,
            skid,
            momentum,
            output
        };

        links={
            {
                nextLinkId++,
                evidence.id,
                0,
                skid.id,
                0
            },
            {
                nextLinkId++,
                vehicleA.id,
                0,
                momentum.id,
                0
            },
            {
                nextLinkId++,
                vehicleB.id,
                0,
                momentum.id,
                1
            },
            {
                nextLinkId++,
                skid.id,
                0,
                output.id,
                0
            },
            {
                nextLinkId++,
                momentum.id,
                0,
                output.id,
                1
            }
        };

        initialized=true;
    }

    // ========================================================
    // SHARED SCENE SELECTION -> NODE GRAPH
    // ========================================================

    if (lastSceneSelectionRevision!=
        gSharedSelection.revision)
    {
        lastSceneSelectionRevision=
            gSharedSelection.revision;

        int targetNode=-1;

        for (const Node& candidate : nodes)
        {
            if (gEditorShell.selectedEntity==5 &&
                candidate.type==
                    NodeType::EvidenceInput)
            {
                targetNode=candidate.id;
                break;
            }

            if (gEditorShell.selectedEntity==3 &&
                candidate.type==
                    NodeType::VehicleInput &&
                candidate.title=="Vehicle A")
            {
                targetNode=candidate.id;
                break;
            }

            if (gEditorShell.selectedEntity==4 &&
                candidate.type==
                    NodeType::VehicleInput &&
                candidate.title=="Vehicle B")
            {
                targetNode=candidate.id;
                break;
            }
        }

        if (targetNode!=-1)
        {
            selectedNodes.clear();
            selectedLinks.clear();
            selectedNodes.push_back(
                targetNode
            );

            graphMessage=
                std::string("Linked selection: ")+
                selectedEntityName();

            graphMessageError=false;
        }
    }
    // ========================================================
    // GRAPH EXECUTION
    // ========================================================

    auto executeGraph = [&]()
    {
        for (Node& node : nodes)
        {
            node.result=0.0f;
            node.result2=0.0f;
            node.resultValid=false;
            node.error.clear();
        }

        lastExecutedNodes=0;
        lastExecutionErrors=0;

        std::function<EvalValue(
            int,
            int,
            std::vector<int>&
        )> evalOutput;

        auto resolveInput =
            [&](int nodeId,
                int inputPin,
                PinType expected,
                std::vector<int>& visiting)
                -> EvalValue
        {
            for (const Link& link : links)
            {
                if (link.toNode==nodeId &&
                    link.toPin==inputPin)
                {
                    Node* source=
                        findNode(link.fromNode);

                    if (!source)
                    {
                        return {
                            expected,
                            false,
                            0.0f,
                            0.0f,
                            "Missing source node"
                        };
                    }

                    const PinType sourceType=
                        outputType(
                            source->type,
                            link.fromPin
                        );

                    if (!compatible(
                        sourceType,
                        expected))
                    {
                        return {
                            expected,
                            false,
                            0.0f,
                            0.0f,
                            "Incompatible pin type"
                        };
                    }

                    return evalOutput(
                        link.fromNode,
                        link.fromPin,
                        visiting
                    );
                }
            }

            return {
                expected,
                false,
                0.0f,
                0.0f,
                "Input not connected"
            };
        };

        evalOutput =
            [&](int nodeId,
                int outputPin,
                std::vector<int>& visiting)
                -> EvalValue
        {
            Node* node=
                findNode(nodeId);

            if (!node)
            {
                return {
                    PinType::Scalar,
                    false,
                    0.0f,
                    0.0f,
                    "Node missing"
                };
            }

            if (!node->enabled)
            {
                return {
                    outputType(
                        node->type,
                        outputPin
                    ),
                    false,
                    0.0f,
                    0.0f,
                    "Node disabled"
                };
            }

            if (std::find(
                visiting.begin(),
                visiting.end(),
                nodeId
            )!=visiting.end())
            {
                node->error=
                    "Cycle detected";

                return {
                    outputType(
                        node->type,
                        outputPin
                    ),
                    false,
                    0.0f,
                    0.0f,
                    "Cycle detected"
                };
            }

            visiting.push_back(nodeId);

            EvalValue result;

            switch (node->type)
            {
                case NodeType::EvidenceInput:
                {
                    result={
                        PinType::EvidenceDistance,
                        node->p0>=0.0f,
                        std::max(
                            0.0f,
                            node->p0
                        ),
                        0.0f,
                        node->p0>=0.0f
                            ? ""
                            : "Distance must be non-negative"
                    };
                    break;
                }

                case NodeType::VehicleInput:
                {
                    result={
                        PinType::VehicleState,
                        node->p0>0.0f,
                        node->p0,
                        node->p1,
                        node->p0>0.0f
                            ? ""
                            : "Mass must be positive"
                    };
                    break;
                }

                case NodeType::SkidAnalysis:
                {
                    EvalValue evidence=
                        resolveInput(
                            nodeId,
                            0,
                            PinType::EvidenceDistance,
                            visiting
                        );

                    if (!evidence.valid)
                    {
                        result={
                            PinType::Scalar,
                            false,
                            0.0f,
                            0.0f,
                            evidence.error
                        };
                        break;
                    }

                    if (node->p0<=0.0f)
                    {
                        result={
                            PinType::Scalar,
                            false,
                            0.0f,
                            0.0f,
                            "Friction must be positive"
                        };
                        break;
                    }

                    constexpr float g=
                        9.80665f;

                    const float speed=
                        std::sqrt(
                            std::max(
                                0.0f,
                                2.0f*
                                node->p0*
                                g*
                                evidence.a
                            )
                        );

                    node->result=speed;
                    node->resultValid=true;

                    result={
                        PinType::Scalar,
                        true,
                        speed,
                        0.0f,
                        ""
                    };
                    break;
                }

                case NodeType::SpeedAnalysis:
                {
                    EvalValue evidence=
                        resolveInput(
                            nodeId,
                            0,
                            PinType::EvidenceDistance,
                            visiting
                        );

                    if (!evidence.valid)
                    {
                        result={
                            PinType::Scalar,
                            false,
                            0.0f,
                            0.0f,
                            evidence.error
                        };
                        break;
                    }

                    constexpr float g=
                        9.80665f;

                    const float baseSpeed=
                        std::sqrt(
                            std::max(
                                0.0f,
                                2.0f*
                                g*
                                evidence.a
                            )
                        );

                    const float speed=
                        baseSpeed*
                        std::max(
                            0.0f,
                            node->p0
                        );

                    node->result=speed;
                    node->resultValid=true;

                    result={
                        PinType::Scalar,
                        true,
                        speed,
                        0.0f,
                        ""
                    };
                    break;
                }

                case NodeType::MomentumAnalysis:
                {
                    EvalValue a=
                        resolveInput(
                            nodeId,
                            0,
                            PinType::VehicleState,
                            visiting
                        );

                    EvalValue b=
                        resolveInput(
                            nodeId,
                            1,
                            PinType::VehicleState,
                            visiting
                        );

                    if (!a.valid | !b.valid)
                    {
                        result={
                            PinType::Scalar,
                            false,
                            0.0f,
                            0.0f,
                            !a.valid
                                ? a.error
                                : b.error
                        };
                        break;
                    }

                    const float momentum=
                        a.a*a.b+
                        b.a*b.b;

                    node->result=momentum;
                    node->resultValid=true;

                    result={
                        PinType::Scalar,
                        true,
                        momentum,
                        0.0f,
                        ""
                    };
                    break;
                }

                case NodeType::ResultOutput:
                {
                    result={
                        PinType::Scalar,
                        false,
                        0.0f,
                        0.0f,
                        "Result nodes have no output"
                    };
                    break;
                }
            }

            visiting.pop_back();

            if (!result.valid)
            {
                node->error=result.error;
            }

            return result;
        };

        for (Node& node : nodes)
        {
            if (node.type==
                NodeType::ResultOutput)
            {
                std::vector<int> visiting;

                EvalValue a=
                    resolveInput(
                        node.id,
                        0,
                        PinType::Scalar,
                        visiting
                    );

                visiting.clear();

                EvalValue b=
                    resolveInput(
                        node.id,
                        1,
                        PinType::Scalar,
                        visiting
                    );

                if (a.valid | b.valid)
                {
                    node.result=
                        a.valid
                            ? a.a
                            : 0.0f;

                    node.result2=
                        b.valid
                            ? b.a
                            : 0.0f;

                    node.resultValid=true;
                    node.error.clear();
                }
                else
                {
                    node.resultValid=false;
                    node.error=
                        "No valid result inputs";
                }

                continue;
            }

            if (outputCount(node.type)>0)
            {
                std::vector<int> visiting;

                EvalValue value=
                    evalOutput(
                        node.id,
                        0,
                        visiting
                    );

                if (node.type==
                        NodeType::SkidAnalysis |
                    node.type==
                        NodeType::SpeedAnalysis |
                    node.type==
                        NodeType::MomentumAnalysis)
                {
                    lastExecutedNodes++;

                    if (!value.valid)
                        lastExecutionErrors++;
                }
            }
        }

        if (lastExecutionErrors==0)
        {
            graphMessage=
                "Graph executed successfully";

            graphMessageError=false;
        }
        else
        {
            graphMessage=
                "Graph executed with errors";

            graphMessageError=true;
        }
    };

    // ========================================================
    // WINDOW
    // ========================================================

    // Keep the original docked Node Editor registered while its
    // temporary fullscreen presentation is active.
    if (gNodeEditorFullscreen)
    {
        ImGui::Begin(
            "Node Editor",
            nullptr,
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoBringToFrontOnFocus
        );

        ImGui::End();

        const ImGuiViewport* mainViewport=
            ImGui::GetMainViewport();

        if (mainViewport)
        {
            ImGui::SetNextWindowPos(
                mainViewport->Pos,
                ImGuiCond_Always
            );

            ImGui::SetNextWindowSize(
                mainViewport->Size,
                ImGuiCond_Always
            );

            ImGui::SetNextWindowViewport(
                mainViewport->ID
            );
        }
    }

    const ImGuiWindowFlags nodeEditorWindowFlags=
        gNodeEditorFullscreen
            ? (
                ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoSavedSettings
              )
            : ImGuiWindowFlags_NoMove;

    const char* nodeEditorWindowName=
        gNodeEditorFullscreen
            ? "Node Editor Full Screen###SovereignNodeEditorFullscreen"
            : "Node Editor";

    bool* nodeEditorOpenPtr=
        gNodeEditorFullscreen
            ? nullptr
            : &gEditorShell.showNodeEditor;

    ImGui::Begin(
        nodeEditorWindowName,
        nodeEditorOpenPtr,
        nodeEditorWindowFlags
    );

    auto setNodeEditorFullscreen=
        [&](bool enable)
        {
            if (enable==gNodeEditorFullscreen)
                return;

            if (enable)
                gViewportFullscreen=false;

            gNodeEditorFullscreen=enable;
        };

    const bool nodeEditorHasFocus=
        ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows
        );

    if (nodeEditorHasFocus &&
        ImGui::IsKeyPressed(ImGuiKey_F11))
    {
        setNodeEditorFullscreen(
            !gNodeEditorFullscreen
        );
    }

    if (gNodeEditorFullscreen &&
        ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        setNodeEditorFullscreen(false);
    }
    beginEditorContextHeader(
        "##NodeEditorContextHeader"
    );

    ImGui::TextDisabled("NODE GRAPH");

    editorContextSeparator();

    ImGui::Text(
        "%d nodes",
        static_cast<int>(
            nodes.size()
        )
    );

    editorContextSeparator();

    ImGui::Text(
        "%d links",
        static_cast<int>(
            links.size()
        )
    );

    editorContextSeparator();

    if (!selectedNodes.empty())
    {
        ImGui::Text(
            "%d selected",
            static_cast<int>(
                selectedNodes.size()
            )
        );
    }
    else if (!selectedLinks.empty())
    {
        ImGui::Text(
            "%d link selected",
            static_cast<int>(
                selectedLinks.size()
            )
        );
    }
    else
    {
        ImGui::TextDisabled(
            "Nothing selected"
        );
    }

    editorContextSeparator();

    if (graphMessageError)
    {
        ImGui::TextColored(
            ImVec4(0.90f,0.58f,0.45f,1.0f),
            "%s",
            graphMessage.c_str()
        );
    }
    else
    {
        ImGui::TextDisabled(
            "%s",
            graphMessage.c_str()
        );
    }

    
    // Fullscreen control is part of the visible NODE GRAPH header.
    {
        const ImVec2 nodeHeaderWindowPos=
            ImGui::GetWindowPos();

        const float nodeFullscreenW=
            gNodeEditorFullscreen
                ? 172.0f
                : 128.0f;

        ImGui::SetCursorScreenPos(
            ImVec2(
                nodeHeaderWindowPos.x+
                    ImGui::GetWindowContentRegionMax().x-
                    nodeFullscreenW-
                    8.0f,
                nodeHeaderWindowPos.y+
                    5.0f
            )
        );

        if (editorButton(
            gNodeEditorFullscreen
                ? "EXIT FULL SCREEN"
                : "FULL SCREEN",
            nodeFullscreenW,
            gNodeEditorFullscreen,
            true))
        {
            if (gNodeEditorFullscreen)
            {
                gNodeEditorFullscreen=false;
            }
            else
            {
                gViewportFullscreen=false;
                gNodeEditorFullscreen=true;
            }
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "%s Node Editor | F11 toggle | Esc exit",
                gNodeEditorFullscreen
                    ? "Exit full screen:"
                    : "Full screen:"
            );
        }
    }
endEditorContextHeader();

    // ========================================================
    // SHORTCUTS
    // ========================================================

    const bool focused=
        ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows
        );

    if (focused)
    {
        const ImGuiIO& io=
            ImGui::GetIO();

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            doUndo();
        }

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_Y))
        {
            doRedo();
        }

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_C))
        {
            copySelection();
        }

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_V))
        {
            pasteSelection();
        }

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_D))
        {
            duplicateSelection();
        }

        if (ImGui::IsKeyPressed(
            ImGuiKey_Delete))
        {
            deleteSelection();
        }

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_Enter))
        {
            executeGraph();
        }
        if (io.KeyCtrl &&
            !io.KeyShift &&
            ImGui::IsKeyPressed(ImGuiKey_S))
        {
            saveGraph();
        }

        if (io.KeyCtrl &&
            !io.KeyShift &&
            ImGui::IsKeyPressed(ImGuiKey_O))
        {
            loadGraph();
        }

        if (!io.WantTextInput)
        {
            if (io.KeyCtrl &&
                !io.KeyShift &&
                ImGui::IsKeyPressed(ImGuiKey_A))
            {
                clearSelection();

                for (const Node& node : nodes)
                {
                    selectedNodes.push_back(
                        node.id
                    );
                }
            }

            if (!io.KeyCtrl &&
                !io.KeyShift &&
                !io.KeyAlt &&
                ImGui::IsKeyPressed(ImGuiKey_Home))
            {
                pan=ImVec2(0.0f,0.0f);
                zoom=1.0f;
            }

            if (!io.KeyCtrl &&
                !io.KeyShift &&
                !io.KeyAlt &&
                ImGui::IsKeyPressed(ImGuiKey_G))
            {
                showGrid=!showGrid;
            }

            if (io.KeyShift &&
                !io.KeyCtrl &&
                !io.KeyAlt &&
                ImGui::IsKeyPressed(ImGuiKey_Tab))
            {
                snapToGrid=!snapToGrid;
            }

            if (!io.KeyCtrl &&
                !io.KeyShift &&
                !io.KeyAlt &&
                ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                linkDragActive=false;
                linkDragNode=-1;
                linkDragPin=-1;
                boxSelecting=false;
                nodeDragging=false;
                clearSelection();
            }
        }
    }

    // ========================================================
    // TOOLBAR ROW 1 - GRAPH COMMANDS
    // ========================================================

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(8.0f,5.0f)
    );

    if (editorButton("FILE",88.0f))
    {
        ImGui::OpenPopup(
            "##NodeGraphFilePopup"
        );
    }

    if (ImGui::BeginPopup(
        "##NodeGraphFilePopup"))
    {
        ImGui::TextDisabled(
            "GRAPH FILE"
        );

        ImGui::Separator();

        ImGui::SetNextItemWidth(
            260.0f
        );

        ImGui::InputText(
            "##NodeGraphPath",
            graphPath,
            sizeof(graphPath)
        );

        ImGui::Spacing();

        if (ImGui::MenuItem(
            "Save Graph",
            "Ctrl+S"))
        {
            saveGraph();
        }

        if (ImGui::MenuItem(
            "Load Graph",
            "Ctrl+O"))
        {
            loadGraph();
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine(0.0f,6.0f);

    if (editorButton("ADD NODE",112.0f))
    {
        ImGui::OpenPopup(
            "##NodeAddPopup"
        );
    }

    if (ImGui::BeginPopup(
        "##NodeAddPopup"))
    {
        ImGui::TextDisabled(
            "ADD NODE"
        );

        ImGui::Separator();

        const ImVec2 base(
            220.0f-pan.x,
            120.0f-pan.y
        );

        if (ImGui::MenuItem(
            "Evidence Input"))
        {
            addNode(
                NodeType::EvidenceInput,
                base
            );
        }

        if (ImGui::MenuItem(
            "Vehicle State"))
        {
            addNode(
                NodeType::VehicleInput,
                base
            );
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Skid Analysis"))
        {
            addNode(
                NodeType::SkidAnalysis,
                base
            );
        }

        if (ImGui::MenuItem(
            "Speed Analysis"))
        {
            addNode(
                NodeType::SpeedAnalysis,
                base
            );
        }

        if (ImGui::MenuItem(
            "Momentum Analysis"))
        {
            addNode(
                NodeType::MomentumAnalysis,
                base
            );
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Result Output"))
        {
            addNode(
                NodeType::ResultOutput,
                base
            );
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine(0.0f,8.0f);

    if (editorButton(
        "RUN GRAPH",
        104.0f,
        true,
        true))
    {
        executeGraph();
    }

    ImGui::SameLine(0.0f,12.0f);

    ImGui::BeginDisabled(
        undoStack.empty()
    );

    if (editorButton("UNDO",84.0f))
    {
        doUndo();
    }

    ImGui::EndDisabled();

    ImGui::SameLine(0.0f,5.0f);

    ImGui::BeginDisabled(
        redoStack.empty()
    );

    if (editorButton("REDO",84.0f))
    {
        doRedo();
    }

    ImGui::EndDisabled();

    ImGui::SameLine(0.0f,12.0f);

    ImGui::BeginDisabled(
        selectedNodes.empty()
    );

    if (editorButton("COPY",84.0f))
    {
        copySelection();
    }

    ImGui::SameLine(0.0f,5.0f);

    if (editorButton("DUPLICATE",116.0f))
    {
        duplicateSelection();
    }

    ImGui::EndDisabled();

    ImGui::SameLine(0.0f,5.0f);

    if (editorButton("PASTE",86.0f))
    {
        pasteSelection();
    }

    ImGui::SameLine(0.0f,5.0f);

    ImGui::BeginDisabled(
        selectedNodes.empty() &&
        selectedLinks.empty()
    );

    if (editorButton("DELETE",94.0f))
    {
        deleteSelection();
    }

    ImGui::EndDisabled();

    // ========================================================
    // TOOLBAR ROW 2 - VIEW
    // ========================================================

    ImGui::Spacing();

    ImGui::Checkbox(
        "Grid",
        &showGrid
    );

    ImGui::SameLine(0.0f,10.0f);

    ImGui::Checkbox(
        "Snap",
        &snapToGrid
    );

    ImGui::SameLine(0.0f,12.0f);

    ImGui::TextDisabled("ZOOM");
    ImGui::SameLine(0.0f,6.0f);

    ImGui::SetNextItemWidth(
        118.0f
    );

    ImGui::SliderFloat(
        "##NodeGraphZoom",
        &zoom,
        0.70f,
        1.55f,
        "%.2fx"
    );

    ImGui::SameLine(0.0f,10.0f);

    if (editorButton("CENTER",96.0f))
    {
        pan=ImVec2(0.0f,0.0f);
        zoom=1.0f;
    }

    ImGui::SameLine(0.0f,14.0f);

    if (graphMessageError)
    {
        ImGui::TextColored(
            ImVec4(
                0.90f,
                0.58f,
                0.45f,
                1.0f
            ),
            "%s",
            graphMessage.c_str()
        );
    }
    else
    {
        ImGui::TextDisabled(
            "%s",
            graphMessage.c_str()
        );
    }

    ImGui::PopStyleVar();

    ImGui::Separator();

    // ========================================================
    // CANVAS + INSPECTOR SPLIT
    // ========================================================

    const float inspectorW=
        std::min(
            330.0f,
            std::max(
                260.0f,
                ImGui::GetContentRegionAvail().x*
                0.24f
            )
        );

    ImGui::BeginChild(
        "NodeGraphCanvasPane",
        ImVec2(
            -inspectorW-8.0f,
            0.0f
        ),
        false,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    const ImVec2 canvasPos=
        ImGui::GetCursorScreenPos();

    const ImVec2 canvasSize=
        ImGui::GetContentRegionAvail();

    ImGui::InvisibleButton(
        "##NodeGraphCanvas",
        canvasSize,
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonMiddle |
        ImGuiButtonFlags_MouseButtonRight
    );

    const bool canvasHovered=
        ImGui::IsItemHovered();

    ImDrawList* dl=
        ImGui::GetWindowDrawList();

    dl->PushClipRect(
        canvasPos,
        ImVec2(
            canvasPos.x+canvasSize.x,
            canvasPos.y+canvasSize.y
        ),
        true
    );

    dl->AddRectFilled(
        canvasPos,
        ImVec2(
            canvasPos.x+canvasSize.x,
            canvasPos.y+canvasSize.y
        ),
        IM_COL32(20,23,27,255)
    );

    // --------------------------------------------------------
    // GRID
    // --------------------------------------------------------

    if (showGrid)
    {
        const float minor=
            24.0f*zoom;

        const float major=
            minor*4.0f;

        float startX=
            canvasPos.x+
            std::fmod(
                pan.x*zoom,
                minor
            );

        float startY=
            canvasPos.y+
            std::fmod(
                pan.y*zoom,
                minor
            );

        for (
            float x=startX;
            x<canvasPos.x+canvasSize.x;
            x+=minor)
        {
            dl->AddLine(
                ImVec2(x,canvasPos.y),
                ImVec2(
                    x,
                    canvasPos.y+canvasSize.y
                ),
                IM_COL32(43,47,53,135),
                1.0f
            );
        }

        for (
            float y=startY;
            y<canvasPos.y+canvasSize.y;
            y+=minor)
        {
            dl->AddLine(
                ImVec2(canvasPos.x,y),
                ImVec2(
                    canvasPos.x+canvasSize.x,
                    y
                ),
                IM_COL32(43,47,53,135),
                1.0f
            );
        }

        startX=
            canvasPos.x+
            std::fmod(
                pan.x*zoom,
                major
            );

        startY=
            canvasPos.y+
            std::fmod(
                pan.y*zoom,
                major
            );

        for (
            float x=startX;
            x<canvasPos.x+canvasSize.x;
            x+=major)
        {
            dl->AddLine(
                ImVec2(x,canvasPos.y),
                ImVec2(
                    x,
                    canvasPos.y+canvasSize.y
                ),
                IM_COL32(59,64,72,155),
                1.0f
            );
        }

        for (
            float y=startY;
            y<canvasPos.y+canvasSize.y;
            y+=major)
        {
            dl->AddLine(
                ImVec2(canvasPos.x,y),
                ImVec2(
                    canvasPos.x+canvasSize.x,
                    y
                ),
                IM_COL32(59,64,72,155),
                1.0f
            );
        }
    }

    auto screenPos = [&](const Node& node)
    {
        return ImVec2(
            canvasPos.x+
            (node.pos.x+pan.x)*zoom,
            canvasPos.y+
            (node.pos.y+pan.y)*zoom
        );
    };

    auto inputPinPos =
        [&](const Node& node,int pin)
    {
        const ImVec2 p=
            screenPos(node);

        const ImVec2 size=
            nodeSize(node.type);

        const int count=
            inputCount(node.type);

        const float bodyTop=
            48.0f*zoom;

        const float bodyHeight=
            size.y*zoom-
            bodyTop-
            18.0f*zoom;

        const float spacing=
            bodyHeight/
            static_cast<float>(
                count+1
            );

        return ImVec2(
            p.x,
            p.y+
            bodyTop+
            spacing*
            static_cast<float>(pin+1)
        );
    };

    auto outputPinPos =
        [&](const Node& node,int pin)
    {
        const ImVec2 p=
            screenPos(node);

        const ImVec2 size=
            nodeSize(node.type);

        const int count=
            outputCount(node.type);

        const float bodyTop=
            48.0f*zoom;

        const float bodyHeight=
            size.y*zoom-
            bodyTop-
            18.0f*zoom;

        const float spacing=
            bodyHeight/
            static_cast<float>(
                count+1
            );

        return ImVec2(
            p.x+
            size.x*zoom,
            p.y+
            bodyTop+
            spacing*
            static_cast<float>(pin+1)
        );
    };

    const ImVec2 mouse=
        ImGui::GetIO().MousePos;

    int hoveredInputNode=-1;
    int hoveredInputPin=-1;
    int hoveredOutputNode=-1;
    int hoveredOutputPin=-1;

    const float pinHitRadius=
        11.0f;

    for (const Node& node : nodes)
    {
        for (
            int pin=0;
            pin<inputCount(node.type);
            ++pin)
        {
            const ImVec2 p=
                inputPinPos(node,pin);

            const float dx=
                mouse.x-p.x;

            const float dy=
                mouse.y-p.y;

            if (dx*dx+dy*dy <=
                pinHitRadius*
                pinHitRadius)
            {
                hoveredInputNode=node.id;
                hoveredInputPin=pin;
            }
        }

        for (
            int pin=0;
            pin<outputCount(node.type);
            ++pin)
        {
            const ImVec2 p=
                outputPinPos(node,pin);

            const float dx=
                mouse.x-p.x;

            const float dy=
                mouse.y-p.y;

            if (dx*dx+dy*dy <=
                pinHitRadius*
                pinHitRadius)
            {
                hoveredOutputNode=node.id;
                hoveredOutputPin=pin;
            }
        }
    }

    auto bezierPoint =
        [](float t,
           ImVec2 a,
           ImVec2 c1,
           ImVec2 c2,
           ImVec2 b)
    {
        const float u=
            1.0f-t;

        const float tt=
            t*t;

        const float uu=
            u*u;

        const float uuu=
            uu*u;

        const float ttt=
            tt*t;

        return ImVec2(
            uuu*a.x+
            3.0f*uu*t*c1.x+
            3.0f*u*tt*c2.x+
            ttt*b.x,

            uuu*a.y+
            3.0f*uu*t*c1.y+
            3.0f*u*tt*c2.y+
            ttt*b.y
        );
    };

    auto pointSegmentDistance =
        [](ImVec2 p,ImVec2 a,ImVec2 b)
    {
        const float vx=
            b.x-a.x;

        const float vy=
            b.y-a.y;

        const float wx=
            p.x-a.x;

        const float wy=
            p.y-a.y;

        const float vv=
            vx*vx+vy*vy;

        float t=
            vv>0.0001f
                ? (wx*vx+wy*vy)/vv
                : 0.0f;

        t=
            std::max(
                0.0f,
                std::min(
                    1.0f,
                    t
                )
            );

        const float px=
            a.x+t*vx;

        const float py=
            a.y+t*vy;

        const float dx=
            p.x-px;

        const float dy=
            p.y-py;

        return std::sqrt(
            dx*dx+dy*dy
        );
    };

    // --------------------------------------------------------
    // LINKS + LINK HIT TEST
    // --------------------------------------------------------

    int hoveredLink=-1;
    float hoveredLinkDistance=8.0f;

    for (const Link& link : links)
    {
        Node* from=
            findNode(link.fromNode);

        Node* to=
            findNode(link.toNode);

        if (!from | !to)
            continue;

        if (link.fromPin>=
                outputCount(from->type) |
            link.toPin>=
                inputCount(to->type))
        {
            continue;
        }

        const ImVec2 a=
            outputPinPos(
                *from,
                link.fromPin
            );

        const ImVec2 b=
            inputPinPos(
                *to,
                link.toPin
            );

        const float tangent=
            std::max(
                70.0f*zoom,
                std::fabs(
                    b.x-a.x
                )*0.45f
            );

        const ImVec2 c1(
            a.x+tangent,
            a.y
        );

        const ImVec2 c2(
            b.x-tangent,
            b.y
        );

        const PinType fromType=
            outputType(
                from->type,
                link.fromPin
            );

        const PinType toType=
            inputType(
                to->type,
                link.toPin
            );

        const bool validType=
            compatible(
                fromType,
                toType
            );

        const bool selected=
            isLinkSelected(
                link.id
            );

        dl->AddBezierCubic(
            a,
            c1,
            c2,
            b,
            !validType
                ? IM_COL32(205,95,88,235)
                : (selected
                    ? toU32(colorAccent())
                    : IM_COL32(120,145,178,225)),
            selected
                ? 3.4f
                : 2.2f
        );

        if (canvasHovered)
        {
            ImVec2 previous=a;

            for (int i=1;i<=24;++i)
            {
                const float t=
                    static_cast<float>(i)/
                    24.0f;

                const ImVec2 current=
                    bezierPoint(
                        t,
                        a,
                        c1,
                        c2,
                        b
                    );

                const float distance=
                    pointSegmentDistance(
                        mouse,
                        previous,
                        current
                    );

                if (distance<
                    hoveredLinkDistance)
                {
                    hoveredLinkDistance=
                        distance;

                    hoveredLink=
                        link.id;
                }

                previous=current;
            }
        }
    }

    // --------------------------------------------------------
    // NODES
    // --------------------------------------------------------

    int hoveredNode=-1;
    bool interactionHandled=false;

    for (Node& node : nodes)
    {
        const ImVec2 p=
            screenPos(node);

        const ImVec2 baseSize=
            nodeSize(node.type);

        const ImVec2 size(
            baseSize.x*zoom,
            baseSize.y*zoom
        );

        const ImVec2 max(
            p.x+size.x,
            p.y+size.y
        );

        const bool selected=
            isNodeSelected(
                node.id
            );

        const bool hovered=
            mouse.x>=p.x &&
            mouse.x<=max.x &&
            mouse.y>=p.y &&
            mouse.y<=max.y;

        if (hovered)
            hoveredNode=node.id;

        dl->AddRectFilled(
            p,
            max,
            selected
                ? IM_COL32(42,47,54,255)
                : IM_COL32(32,36,42,255),
            6.0f
        );

        dl->AddRect(
            p,
            max,
            selected
                ? toU32(colorAccent())
                : IM_COL32(78,85,95,245),
            6.0f,
            0,
            selected
                ? 1.8f
                : 1.0f
        );

        const float titleH=
            36.0f*zoom;

        dl->AddRectFilled(
            p,
            ImVec2(
                max.x,
                p.y+titleH
            ),
            selected
                ? IM_COL32(61,55,41,255)
                : IM_COL32(43,48,56,255),
            6.0f,
            ImDrawFlags_RoundCornersTop
        );

        dl->AddText(
            ImVec2(
                p.x+12.0f*zoom,
                p.y+9.0f*zoom
            ),
            toU32(colorText()),
            node.title.c_str()
        );

        char bodyText[160]{};

        switch (node.type)
        {
            case NodeType::EvidenceInput:
                std::snprintf(
                    bodyText,
                    sizeof(bodyText),
                    "Distance %.2f m",
                    node.p0
                );
                break;

            case NodeType::VehicleInput:
                std::snprintf(
                    bodyText,
                    sizeof(bodyText),
                    "%.0f kg  |  %.2f m/s",
                    node.p0,
                    node.p1
                );
                break;

            case NodeType::SkidAnalysis:
                if (node.resultValid)
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "mu %.2f  |  %.2f m/s",
                        node.p0,
                        node.result
                    );
                }
                else
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "mu %.2f",
                        node.p0
                    );
                }
                break;

            case NodeType::SpeedAnalysis:
                if (node.resultValid)
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "Scale %.2f  |  %.2f m/s",
                        node.p0,
                        node.result
                    );
                }
                else
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "Scale %.2f",
                        node.p0
                    );
                }
                break;

            case NodeType::MomentumAnalysis:
                if (node.resultValid)
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "%.2f kg m/s",
                        node.result
                    );
                }
                else
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "Awaiting vehicle states"
                    );
                }
                break;

            case NodeType::ResultOutput:
                if (node.resultValid)
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "A %.2f   |   B %.2f",
                        node.result,
                        node.result2
                    );
                }
                else
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "Awaiting analysis results"
                    );
                }
                break;
        }

        dl->AddText(
            ImVec2(
                p.x+12.0f*zoom,
                p.y+48.0f*zoom
            ),
            toU32(colorMuted()),
            bodyText
        );

        if (!node.error.empty())
        {
            dl->AddText(
                ImVec2(
                    p.x+12.0f*zoom,
                    max.y-26.0f*zoom
                ),
                IM_COL32(222,125,112,230),
                node.error.c_str()
            );
        }
        else
        {
            dl->AddText(
                ImVec2(
                    p.x+12.0f*zoom,
                    max.y-26.0f*zoom
                ),
                node.enabled
                    ? IM_COL32(150,158,169,220)
                    : IM_COL32(105,110,118,190),
                node.enabled
                    ? "Enabled"
                    : "Disabled"
            );
        }

        for (
            int pin=0;
            pin<inputCount(node.type);
            ++pin)
        {
            const ImVec2 pinPos=
                inputPinPos(node,pin);

            const bool pinHovered=
                hoveredInputNode==node.id &&
                hoveredInputPin==pin;

            dl->AddCircleFilled(
                pinPos,
                pinHovered
                    ? 7.0f
                    : 5.5f,
                IM_COL32(102,137,178,255)
            );

            dl->AddCircle(
                pinPos,
                8.0f,
                IM_COL32(177,197,220,225),
                20,
                1.2f
            );

            dl->AddText(
                ImVec2(
                    pinPos.x+12.0f,
                    pinPos.y-8.0f
                ),
                IM_COL32(158,164,174,225),
                inputLabel(
                    node.type,
                    pin
                )
            );
        }

        for (
            int pin=0;
            pin<outputCount(node.type);
            ++pin)
        {
            const ImVec2 pinPos=
                outputPinPos(node,pin);

            const bool pinHovered=
                hoveredOutputNode==node.id &&
                hoveredOutputPin==pin;

            dl->AddCircleFilled(
                pinPos,
                pinHovered
                    ? 7.0f
                    : 5.5f,
                IM_COL32(168,177,190,255)
            );

            dl->AddCircle(
                pinPos,
                8.0f,
                IM_COL32(213,218,227,230),
                20,
                1.2f
            );

            const char* label=
                outputLabel(
                    node.type,
                    pin
                );

            const float labelW=
                ImGui::CalcTextSize(
                    label
                ).x;

            dl->AddText(
                ImVec2(
                    pinPos.x-
                    labelW-
                    12.0f,
                    pinPos.y-8.0f
                ),
                IM_COL32(175,181,191,225),
                label
            );
        }
    }

    // --------------------------------------------------------
    // PIN TOOLTIPS
    // --------------------------------------------------------

    if (canvasHovered &&
        hoveredInputNode!=-1)
    {
        Node* node=
            findNode(
                hoveredInputNode
            );

        if (node)
        {
            ImGui::BeginTooltip();

            ImGui::Text(
                "%s input",
                inputLabel(
                    node->type,
                    hoveredInputPin
                )
            );

            ImGui::TextDisabled(
                "%s",
                pinTypeLabel(
                    inputType(
                        node->type,
                        hoveredInputPin
                    )
                )
            );

            ImGui::EndTooltip();
        }
    }
    else if (
        canvasHovered &&
        hoveredOutputNode!=-1)
    {
        Node* node=
            findNode(
                hoveredOutputNode
            );

        if (node)
        {
            ImGui::BeginTooltip();

            ImGui::Text(
                "%s output",
                outputLabel(
                    node->type,
                    hoveredOutputPin
                )
            );

            ImGui::TextDisabled(
                "%s",
                pinTypeLabel(
                    outputType(
                        node->type,
                        hoveredOutputPin
                    )
                )
            );

            ImGui::EndTooltip();
        }
    }

    // --------------------------------------------------------
    // LINK DRAG PREVIEW
    // --------------------------------------------------------

    if (linkDragActive)
    {
        Node* source=
            findNode(
                linkDragNode
            );

        if (source)
        {
            const ImVec2 a=
                outputPinPos(
                    *source,
                    linkDragPin
                );

            const ImVec2 b=
                mouse;

            const float tangent=
                std::max(
                    70.0f,
                    std::fabs(
                        b.x-a.x
                    )*0.45f
                );

            bool targetCompatible=false;

            if (hoveredInputNode!=-1)
            {
                Node* target=
                    findNode(
                        hoveredInputNode
                    );

                if (target)
                {
                    targetCompatible=
                        compatible(
                            linkDragType,
                            inputType(
                                target->type,
                                hoveredInputPin
                            )
                        );
                }
            }

            dl->AddBezierCubic(
                a,
                ImVec2(
                    a.x+tangent,
                    a.y
                ),
                ImVec2(
                    b.x-tangent,
                    b.y
                ),
                b,
                hoveredInputNode==-1
                    ? IM_COL32(170,176,186,210)
                    : (targetCompatible
                        ? IM_COL32(110,190,135,245)
                        : IM_COL32(215,92,82,245)),
                2.6f
            );
        }
    }

    // ========================================================
    // INTERACTION
    // ========================================================

    const bool leftClicked=
        canvasHovered &&
        ImGui::IsMouseClicked(
            ImGuiMouseButton_Left
        );

    if (leftClicked &&
        hoveredOutputNode!=-1)
    {
        Node* source=
            findNode(
                hoveredOutputNode
            );

        if (source)
        {
            linkDragActive=true;
            linkDragNode=
                hoveredOutputNode;

            linkDragPin=
                hoveredOutputPin;

            linkDragType=
                outputType(
                    source->type,
                    hoveredOutputPin
                );

            interactionHandled=true;
        }
    }

    if (leftClicked &&
        !interactionHandled)
    {
        // Node selection / drag.
        for (
            auto it=nodes.rbegin();
            it!=nodes.rend();
            ++it)
        {
            Node& node=*it;

            const ImVec2 p=
                screenPos(node);

            const ImVec2 size=
                nodeSize(node.type);

            const ImVec2 scaled(
                size.x*zoom,
                size.y*zoom
            );

            const float headerH=
                36.0f*zoom;

            const bool headerHit=
                mouse.x>=p.x &&
                mouse.x<=p.x+scaled.x &&
                mouse.y>=p.y &&
                mouse.y<=p.y+headerH;

            const bool bodyHit=
                mouse.x>=p.x &&
                mouse.x<=p.x+scaled.x &&
                mouse.y>=p.y &&
                mouse.y<=p.y+scaled.y;

            if (!bodyHit)
                continue;

            if (ImGui::GetIO().KeyCtrl)
            {
                if (isNodeSelected(node.id))
                {
                    selectedNodes.erase(
                        std::remove(
                            selectedNodes.begin(),
                            selectedNodes.end(),
                            node.id
                        ),
                        selectedNodes.end()
                    );
                }
                else
                {
                    selectedNodes.push_back(
                        node.id
                    );
                }
            }
            else if (!isNodeSelected(node.id))
            {
                clearSelection();

                selectedNodes.push_back(
                    node.id
                );
            }

            // Linked input nodes select their real scene entity.
            if (node.type==
                NodeType::EvidenceInput)
            {
                setSharedEntitySelection(
                    5,
                    "Node Editor"
                );
            }
            else if (node.type==
                NodeType::VehicleInput)
            {
                if (node.title=="Vehicle A")
                {
                    setSharedEntitySelection(
                        3,
                        "Node Editor"
                    );
                }
                else if (node.title=="Vehicle B")
                {
                    setSharedEntitySelection(
                        4,
                        "Node Editor"
                    );
                }
            }
            if (headerHit &&
                isNodeSelected(node.id))
            {
                pushUndo();

                nodeDragging=true;

                dragMouseStart=mouse;

                dragStartPositions.clear();

                for (int id : selectedNodes)
                {
                    Node* selected=
                        findNode(id);

                    if (selected)
                    {
                        dragStartPositions.push_back(
                            {
                                id,
                                selected->pos
                            }
                        );
                    }
                }
            }

            interactionHandled=true;
            break;
        }
    }

    if (leftClicked &&
        !interactionHandled &&
        hoveredLink!=-1)
    {
        if (!ImGui::GetIO().KeyCtrl)
        {
            clearSelection();
        }

        if (!isLinkSelected(
            hoveredLink))
        {
            selectedLinks.push_back(
                hoveredLink
            );
        }

        interactionHandled=true;
    }

    if (leftClicked &&
        !interactionHandled)
    {
        if (!ImGui::GetIO().KeyCtrl)
        {
            clearSelection();
        }

        boxSelecting=true;
        boxStart=mouse;
        boxEnd=mouse;
    }

    if (nodeDragging)
    {
        if (ImGui::IsMouseDown(
            ImGuiMouseButton_Left))
        {
            const ImVec2 delta(
                (mouse.x-dragMouseStart.x)/
                    zoom,
                (mouse.y-dragMouseStart.y)/
                    zoom
            );

            for (
                const auto& start :
                dragStartPositions)
            {
                Node* node=
                    findNode(
                        start.first
                    );

                if (!node)
                    continue;

                ImVec2 target(
                    start.second.x+
                        delta.x,
                    start.second.y+
                        delta.y
                );

                if (snapToGrid)
                {
                    constexpr float snap=
                        24.0f;

                    target.x=
                        std::round(
                            target.x/snap
                        )*snap;

                    target.y=
                        std::round(
                            target.y/snap
                        )*snap;
                }

                node->pos=target;
            }
        }

        if (ImGui::IsMouseReleased(
            ImGuiMouseButton_Left))
        {
            nodeDragging=false;
            dragStartPositions.clear();
        }
    }

    if (boxSelecting)
    {
        if (ImGui::IsMouseDown(
            ImGuiMouseButton_Left))
        {
            boxEnd=mouse;
        }

        const ImVec2 min(
            std::min(
                boxStart.x,
                boxEnd.x
            ),
            std::min(
                boxStart.y,
                boxEnd.y
            )
        );

        const ImVec2 max(
            std::max(
                boxStart.x,
                boxEnd.x
            ),
            std::max(
                boxStart.y,
                boxEnd.y
            )
        );

        dl->AddRectFilled(
            min,
            max,
            IM_COL32(90,130,180,42)
        );

        dl->AddRect(
            min,
            max,
            IM_COL32(125,162,208,200),
            0.0f,
            0,
            1.2f
        );

        if (ImGui::IsMouseReleased(
            ImGuiMouseButton_Left))
        {
            for (const Node& node : nodes)
            {
                const ImVec2 p=
                    screenPos(node);

                const ImVec2 base=
                    nodeSize(node.type);

                const ImVec2 nmax(
                    p.x+
                    base.x*zoom,
                    p.y+
                    base.y*zoom
                );

                const bool intersects=
                    nmax.x>=min.x &&
                    p.x<=max.x &&
                    nmax.y>=min.y &&
                    p.y<=max.y;

                if (intersects &&
                    !isNodeSelected(
                        node.id))
                {
                    selectedNodes.push_back(
                        node.id
                    );
                }
            }

            boxSelecting=false;
        }
    }

    if (linkDragActive &&
        ImGui::IsMouseReleased(
            ImGuiMouseButton_Left))
    {
        bool connected=false;

        Node* source=
            findNode(
                linkDragNode
            );

        Node* target=
            findNode(
                hoveredInputNode
            );

        if (source &&
            target &&
            source->id!=target->id &&
            hoveredInputPin>=0)
        {
            const PinType targetType=
                inputType(
                    target->type,
                    hoveredInputPin
                );

            if (compatible(
                linkDragType,
                targetType))
            {
                pushUndo();

                // One incoming link per input pin.
                links.erase(
                    std::remove_if(
                        links.begin(),
                        links.end(),
                        [&](const Link& link)
                        {
                            return
                                link.toNode==
                                    target->id &&
                                link.toPin==
                                    hoveredInputPin;
                        }
                    ),
                    links.end()
                );

                links.push_back({
                    nextLinkId++,
                    source->id,
                    linkDragPin,
                    target->id,
                    hoveredInputPin
                });

                graphMessage=
                    "Link created";

                graphMessageError=false;
                connected=true;
            }
            else
            {
                graphMessage=
                    std::string(
                        "Cannot connect "
                    )+
                    pinTypeLabel(
                        linkDragType
                    )+
                    " to "+
                    pinTypeLabel(
                        targetType
                    );

                graphMessageError=true;
            }
        }

        if (!connected &&
            hoveredInputNode==-1)
        {
            graphMessage=
                "Link cancelled";

            graphMessageError=false;
        }

        linkDragActive=false;
        linkDragNode=-1;
        linkDragPin=-1;
    }

    // --------------------------------------------------------
    // PAN + MOUSE-WHEEL ZOOM
    // --------------------------------------------------------

    if (canvasHovered &&
        ImGui::IsMouseDragging(
            ImGuiMouseButton_Middle,
            0.0f))
    {
        const ImVec2 delta=
            ImGui::GetIO().MouseDelta;

        pan.x += delta.x/zoom;
        pan.y += delta.y/zoom;
    }

    if (canvasHovered &&
        std::fabs(
            ImGui::GetIO().MouseWheel
        )>0.001f)
    {
        const float oldZoom=
            zoom;

        zoom=
            std::max(
                0.70f,
                std::min(
                    1.55f,
                    zoom+
                    ImGui::GetIO().MouseWheel*
                    0.08f
                )
            );

        if (std::fabs(
            zoom-oldZoom)>0.0001f)
        {
            const float worldX=
                (mouse.x-canvasPos.x)/
                    oldZoom-
                pan.x;

            const float worldY=
                (mouse.y-canvasPos.y)/
                    oldZoom-
                pan.y;

            pan.x=
                (mouse.x-canvasPos.x)/
                    zoom-
                worldX;

            pan.y=
                (mouse.y-canvasPos.y)/
                    zoom-
                worldY;
        }
    }

    // --------------------------------------------------------
    // CANVAS CONTEXT MENU
    // --------------------------------------------------------

    if (ImGui::BeginPopupContextItem(
        "##NodeGraphContext"))
    {
        ImGui::TextDisabled(
            "NODE GRAPH"
        );

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Run Graph",
            "Ctrl+Enter"))
        {
            executeGraph();
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Copy",
            "Ctrl+C",
            false,
            !selectedNodes.empty()))
        {
            copySelection();
        }

        if (ImGui::MenuItem(
            "Paste",
            "Ctrl+V"))
        {
            pasteSelection();
        }

        if (ImGui::MenuItem(
            "Duplicate",
            "Ctrl+D",
            false,
            !selectedNodes.empty()))
        {
            duplicateSelection();
        }

        if (ImGui::MenuItem(
            "Delete",
            "Del",
            false,
            !selectedNodes.empty() |
            !selectedLinks.empty()))
        {
            deleteSelection();
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Center View"))
        {
            pan=ImVec2(0.0f,0.0f);
            zoom=1.0f;
        }

        roadSafeMenuItemToggle("Grid",UiGlyph::Grid,nullptr,&showGrid);

        roadSafeMenuItemToggle("Snap to Grid",UiGlyph::SnapGraph,nullptr,&snapToGrid);

        ImGui::EndPopup();
    }

    dl->PopClipRect();

    ImGui::EndChild();

    // ========================================================
    // NODE INSPECTOR
    // ========================================================

    ImGui::SameLine(0.0f,8.0f);

    ImGui::BeginChild(
        "NodeGraphInspector",
        ImVec2(0.0f,0.0f),
        true
    );

    ImGui::Text("NODE PROPERTIES");
    ImGui::Separator();

    if (selectedNodes.size()==1)
    {
        Node* node=
            findNode(
                selectedNodes.front()
            );

        if (node)
        {
            ImGui::TextDisabled(
                "ID %d",
                node->id
            );

            char titleBuffer[128]{};

            std::snprintf(
                titleBuffer,
                sizeof(titleBuffer),
                "%s",
                node->title.c_str()
            );

            if (ImGui::IsItemActivated())
            {
                propertyEditBefore=
                    serializeGraph();

                propertyEditChanged=false;
            }

            const bool titleChanged=
                ImGui::InputText(
                    "Name",
                    titleBuffer,
                    sizeof(titleBuffer)
                );

            if (ImGui::IsItemActivated())
            {
                propertyEditBefore=
                    serializeGraph();

                propertyEditChanged=false;
            }

            if (titleChanged)
            {
                node->title=titleBuffer;
                propertyEditChanged=true;
            }

            if (ImGui::IsItemDeactivatedAfterEdit() &&
                propertyEditChanged)
            {
                pushSnapshot(
                    propertyEditBefore
                );

                propertyEditChanged=false;
            }

            const bool enabledBefore=
                node->enabled;

            const std::string beforeEnabled=
                serializeGraph();

            if (ImGui::Checkbox(
                "Enabled",
                &node->enabled))
            {
                pushSnapshot(
                    beforeEnabled
                );
            }

            ImGui::Spacing();
            ImGui::Separator();

            auto propertyFloat =
                [&](const char* label,
                    float* value,
                    float speed,
                    float minValue,
                    float maxValue,
                    const char* format)
            {
                const std::string before=
                    serializeGraph();

                ImGui::SetNextItemWidth(
                    -1.0f
                );

                const bool changed=
                    ImGui::DragFloat(
                        label,
                        value,
                        speed,
                        minValue,
                        maxValue,
                        format
                    );

                if (ImGui::IsItemActivated())
                {
                    propertyEditBefore=
                        before;

                    propertyEditChanged=false;
                }

                if (changed)
                {
                    propertyEditChanged=true;
                }

                if (ImGui::IsItemDeactivatedAfterEdit() &&
                    propertyEditChanged)
                {
                    pushSnapshot(
                        propertyEditBefore
                    );

                    propertyEditChanged=false;
                }
            };

            switch (node->type)
            {
                case NodeType::EvidenceInput:
                    ImGui::TextDisabled(
                        "EVIDENCE INPUT"
                    );

                    propertyFloat(
                        "Distance (m)",
                        &node->p0,
                        0.10f,
                        0.0f,
                        10000.0f,
                        "%.2f"
                    );
                    break;

                case NodeType::VehicleInput:
                    ImGui::TextDisabled(
                        "VEHICLE STATE"
                    );

                    propertyFloat(
                        "Mass (kg)",
                        &node->p0,
                        5.0f,
                        1.0f,
                        100000.0f,
                        "%.1f"
                    );

                    propertyFloat(
                        "Velocity (m/s)",
                        &node->p1,
                        0.10f,
                        -200.0f,
                        200.0f,
                        "%.2f"
                    );
                    break;

                case NodeType::SkidAnalysis:
                    ImGui::TextDisabled(
                        "SKID ANALYSIS"
                    );

                    propertyFloat(
                        "Friction coefficient",
                        &node->p0,
                        0.01f,
                        0.01f,
                        2.0f,
                        "%.2f"
                    );
                    break;

                case NodeType::SpeedAnalysis:
                    ImGui::TextDisabled(
                        "SPEED ANALYSIS"
                    );

                    propertyFloat(
                        "Speed multiplier",
                        &node->p0,
                        0.01f,
                        0.0f,
                        10.0f,
                        "%.2f"
                    );
                    break;

                case NodeType::MomentumAnalysis:
                    ImGui::TextDisabled(
                        "MOMENTUM ANALYSIS"
                    );

                    ImGui::TextWrapped(
                        "Consumes two Vehicle State inputs and computes combined linear momentum."
                    );
                    break;

                case NodeType::ResultOutput:
                    ImGui::TextDisabled(
                        "RESULT OUTPUT"
                    );

                    ImGui::TextWrapped(
                        "Collects scalar results from upstream analysis nodes."
                    );
                    break;
            }

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Text("PINS");

            for (
                int pin=0;
                pin<inputCount(node->type);
                ++pin)
            {
                ImGui::TextDisabled(
                    "IN  %s",
                    inputLabel(
                        node->type,
                        pin
                    )
                );

                ImGui::SameLine();

                ImGui::Text(
                    "%s",
                    pinTypeLabel(
                        inputType(
                            node->type,
                            pin
                        )
                    )
                );
            }

            for (
                int pin=0;
                pin<outputCount(node->type);
                ++pin)
            {
                ImGui::TextDisabled(
                    "OUT %s",
                    outputLabel(
                        node->type,
                        pin
                    )
                );

                ImGui::SameLine();

                ImGui::Text(
                    "%s",
                    pinTypeLabel(
                        outputType(
                            node->type,
                            pin
                        )
                    )
                );
            }

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Text("EXECUTION");

            if (node->resultValid)
            {
                if (node->type==
                    NodeType::ResultOutput)
                {
                    ImGui::Text(
                        "Result A: %.3f",
                        node->result
                    );

                    ImGui::Text(
                        "Result B: %.3f",
                        node->result2
                    );
                }
                else
                {
                    ImGui::Text(
                        "Result: %.3f",
                        node->result
                    );
                }
            }
            else if (!node->error.empty())
            {
                ImGui::TextColored(
                    ImVec4(
                        0.90f,
                        0.58f,
                        0.45f,
                        1.0f
                    ),
                    "%s",
                    node->error.c_str()
                );
            }
            else
            {
                ImGui::TextDisabled(
                    "Not executed"
                );
            }
        }
    }
    else if (selectedNodes.size()>1)
    {
        ImGui::Text(
            "%d nodes selected",
            static_cast<int>(
                selectedNodes.size()
            )
        );

        ImGui::TextDisabled(
            "Ctrl-click or box-select supports multi-selection."
        );

        ImGui::Spacing();

        editorButton(
            "RUN GRAPH",
            ImGui::GetContentRegionAvail().x,
            true,
            true
        );
    }
    else if (!selectedLinks.empty())
    {
        ImGui::Text(
            "%d link(s) selected",
            static_cast<int>(
                selectedLinks.size()
            )
        );

        ImGui::Spacing();

        if (editorButton(
            "DELETE LINK",
            ImGui::GetContentRegionAvail().x,
            false,
            true))
        {
            deleteSelection();
        }
    }
    else
    {
        ImGui::TextDisabled(
            "Nothing selected"
        );

        ImGui::Spacing();

        ImGui::TextWrapped(
            "Select a node to edit its properties, or select a link and press Delete."
        );

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Text("GRAPH EXECUTION");

        ImGui::TextDisabled(
            "Executed nodes"
        );

        ImGui::SameLine();

        ImGui::Text(
            "%d",
            lastExecutedNodes
        );

        ImGui::TextDisabled(
            "Errors"
        );

        ImGui::SameLine();

        ImGui::Text(
            "%d",
            lastExecutionErrors
        );
    }

    ImGui::EndChild();

    
    const ImVec2 nodeFullscreenControlWindowPos=
        ImGui::GetWindowPos();

    const ImVec2 nodeFullscreenControlContentMin=
        ImGui::GetWindowContentRegionMin();

    const ImVec2 nodeFullscreenControlContentMax=
        ImGui::GetWindowContentRegionMax();

    const ImGuiViewport* nodeFullscreenControlViewport=
        ImGui::GetWindowViewport();
ImGui::End();
    {
        const float nodeControlWidth=
            gNodeEditorFullscreen
                ? 170.0f
                : 128.0f;

        ImGui::SetNextWindowPos(
            ImVec2(
                nodeFullscreenControlWindowPos.x+
                    nodeFullscreenControlContentMax.x-
                    12.0f,
                nodeFullscreenControlWindowPos.y+
                    nodeFullscreenControlContentMin.y+
                    8.0f
            ),
            ImGuiCond_Always,
            ImVec2(1.0f,0.0f)
        );

        if (nodeFullscreenControlViewport)
        {
            ImGui::SetNextWindowViewport(
                nodeFullscreenControlViewport->ID
            );
        }

        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGui::PushStyleVar(
            ImGuiStyleVar_WindowPadding,
            ImVec2(0.0f,0.0f)
        );

        const ImGuiWindowFlags nodeControlFlags=
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin(
            "##SovereignNodeFullscreenControl",
            nullptr,
            nodeControlFlags
        );

        if (gNodeEditorFullscreen)
        {
            ImGui::PushStyleColor(
                ImGuiCol_Button,
                ImVec4(
                    0.12f,
                    0.42f,
                    0.78f,
                    1.0f
                )
            );

            ImGui::PushStyleColor(
                ImGuiCol_ButtonHovered,
                ImVec4(
                    0.15f,
                    0.48f,
                    0.86f,
                    1.0f
                )
            );

            ImGui::PushStyleColor(
                ImGuiCol_ButtonActive,
                ImVec4(
                    0.10f,
                    0.36f,
                    0.69f,
                    1.0f
                )
            );
        }

        const bool toggleNodeFullscreen=
            ImGui::Button(
                gNodeEditorFullscreen
                    ? "EXIT FULL SCREEN"
                    : "FULL SCREEN",
                ImVec2(
                    nodeControlWidth,
                    34.0f
                )
            );

        if (gNodeEditorFullscreen)
        {
            ImGui::PopStyleColor(3);
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "%s Node Editor | F11 toggle | Esc exit",
                gNodeEditorFullscreen
                    ? "Exit full screen:"
                    : "Full screen:"
            );
        }

        ImGui::End();
        ImGui::PopStyleVar();

        if (toggleNodeFullscreen)
        {
            setNodeEditorFullscreen(
                !gNodeEditorFullscreen
            );
        }
    }

}
static void drawTimeline()
{
    static int currentFrame=0;
    static int selectedFrame=0;
    static bool playing=false;
    static bool snapToFrames=true;
    static bool followPlayhead=true;
    static float playbackSpeed=1.0f;
    static float zoom=1.0f;
    static float playbackAccumulator=0.0f;

    static std::array<int,8> userMarkers{
        -1,-1,-1,-1,-1,-1,-1,-1
    };

    static int userMarkerCount=0;

    constexpr int totalFrames=240;
    constexpr int fps=30;

    // The phase boundaries are editor defaults until case-derived
    // reconstruction timing is wired in.
    constexpr int preImpactEnd=90;
    constexpr int impactEnd=120;

    if (playing)
    {
        playbackAccumulator +=
            ImGui::GetIO().DeltaTime *
            playbackSpeed *
            static_cast<float>(fps);

        while (playbackAccumulator>=1.0f)
        {
            currentFrame++;

            if (currentFrame>totalFrames)
            {
                currentFrame=0;
            }

            selectedFrame=currentFrame;
            playbackAccumulator-=1.0f;
        }
    }

    ImGui::Begin(
        "Timeline",
        &gEditorShell.showTimeline,
        ImGuiWindowFlags_NoMove
    );
    beginEditorContextHeader(
        "##TimelineContextHeader"
    );

    ImGui::TextDisabled("TIMELINE");

    editorContextSeparator();

    ImGui::Text(
        "Frame %d",
        currentFrame
    );

    editorContextSeparator();

    const char* timelineHeaderPhase=
        currentFrame<preImpactEnd
            ? "Pre-impact"
            : (currentFrame<impactEnd
                ? "Impact"
                : "Post-impact");

    ImGui::Text(
        "%s",
        timelineHeaderPhase
    );

    editorContextSeparator();

    ImGui::TextDisabled(
        playing
            ? "Playing"
            : "Paused"
    );

    editorContextSeparator();

    ImGui::TextDisabled(
        snapToFrames
            ? "Snap On"
            : "Snap Off"
    );

    ImGui::SameLine(0.0f,14.0f);

    ImGui::TextDisabled(
        "Space Play/Pause"
    );

    endEditorContextHeader();
    // ========================================================
    // TIMELINE KEYBOARD SHORTCUTS
    // ========================================================

    {
        const ImGuiIO& io=ImGui::GetIO();

        const bool timelineFocused=
            ImGui::IsWindowFocused(
                ImGuiFocusedFlags_RootAndChildWindows
            );

        if (timelineFocused &&
            !io.WantTextInput)
        {
            const auto key =
                [](ImGuiKey k)
            {
                return ImGui::IsKeyPressed(
                    k,
                    false
                );
            };

            if (!io.KeyCtrl &&
                !io.KeyAlt &&
                !io.KeyShift)
            {
                if (key(ImGuiKey_Space))
                {
                    playing=!playing;
                    playbackAccumulator=0.0f;
                }

                if (key(ImGuiKey_LeftArrow))
                {
                    currentFrame=
                        std::max(
                            0,
                            currentFrame-1
                        );

                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_RightArrow))
                {
                    currentFrame=
                        std::min(
                            totalFrames,
                            currentFrame+1
                        );

                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_Home))
                {
                    currentFrame=0;
                    selectedFrame=0;
                    playing=false;
                }

                if (key(ImGuiKey_End))
                {
                    currentFrame=totalFrames;
                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_1))
                {
                    currentFrame=0;
                    selectedFrame=0;
                    playing=false;
                }

                if (key(ImGuiKey_2))
                {
                    currentFrame=preImpactEnd;
                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_3))
                {
                    currentFrame=impactEnd;
                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_M))
                {
                    if (userMarkerCount<
                        static_cast<int>(
                            userMarkers.size()
                        ))
                    {
                        userMarkers[
                            static_cast<size_t>(
                                userMarkerCount
                            )
                        ]=currentFrame;

                        userMarkerCount++;
                    }
                }

                if (key(ImGuiKey_S))
                    snapToFrames=!snapToFrames;

                if (key(ImGuiKey_F))
                    followPlayhead=!followPlayhead;
            }

            if (io.KeyShift &&
                !io.KeyCtrl &&
                !io.KeyAlt)
            {
                if (key(ImGuiKey_LeftArrow))
                {
                    currentFrame=
                        std::max(
                            0,
                            currentFrame-10
                        );

                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_RightArrow))
                {
                    currentFrame=
                        std::min(
                            totalFrames,
                            currentFrame+10
                        );

                    selectedFrame=currentFrame;
                    playing=false;
                }
            }

            if (io.KeyCtrl &&
                io.KeyShift &&
                !io.KeyAlt &&
                key(ImGuiKey_M))
            {
                userMarkers.fill(-1);
                userMarkerCount=0;
            }
        }
    }

    // ========================================================
    // TRANSPORT / TIMELINE TOOLBAR
    // ========================================================

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(10.0f,5.0f)
    );

    if (editorButton("FIRST",38.0f))
    {
        currentFrame=0;
        selectedFrame=currentFrame;
        playing=false;
    }

    ImGui::SameLine(0.0f,5.0f);

    if (editorButton("STEP BACK",34.0f))
    {
        currentFrame=
            std::max(
                0,
                currentFrame-1
            );

        selectedFrame=currentFrame;
        playing=false;
    }

    ImGui::SameLine(0.0f,7.0f);

    if (!playing)
    {
        if (ImGui::Button(
            "PLAY",
            ImVec2(64.0f,30.0f)))
        {
            playing=true;
            playbackAccumulator=0.0f;
        }
    }
    else
    {
        if (ImGui::Button(
            "PAUSE",
            ImVec2(64.0f,30.0f)))
        {
            playing=false;
        }
    }

    ImGui::SameLine(0.0f,5.0f);

    if (ImGui::Button(
        "STOP",
        ImVec2(62.0f,30.0f)))
    {
        playing=false;
        currentFrame=0;
        selectedFrame=0;
        playbackAccumulator=0.0f;
    }

    ImGui::SameLine(0.0f,7.0f);

    if (editorButton("STEP NEXT",34.0f))
    {
        currentFrame=
            std::min(
                totalFrames,
                currentFrame+1
            );

        selectedFrame=currentFrame;
        playing=false;
    }

    ImGui::SameLine(0.0f,5.0f);

    if (editorButton("LAST",38.0f))
    {
        currentFrame=totalFrames;
        selectedFrame=currentFrame;
        playing=false;
    }

    ImGui::SameLine(0.0f,14.0f);

    ImGui::TextDisabled("FRAME");
    ImGui::SameLine(0.0f,6.0f);

    ImGui::SetNextItemWidth(76.0f);

    if (ImGui::InputInt(
        "##TimelineFrame",
        &currentFrame,
        1,
        10))
    {
        currentFrame=
            std::max(
                0,
                std::min(
                    totalFrames,
                    currentFrame
                )
            );

        selectedFrame=currentFrame;
        playing=false;
    }

    ImGui::SameLine(0.0f,14.0f);

    ImGui::TextDisabled("SPEED");
    ImGui::SameLine(0.0f,6.0f);

    const char* speedLabels[]={
        "0.25x",
        "0.5x",
        "1x",
        "2x"
    };

    int speedIndex=
        playbackSpeed<=0.25f
            ? 0
            : (playbackSpeed<=0.5f
                ? 1
                : (playbackSpeed<2.0f
                    ? 2
                    : 3));

    ImGui::SetNextItemWidth(76.0f);

    if (ImGui::Combo(
        "##TimelineSpeed",
        &speedIndex,
        speedLabels,
        4))
    {
        playbackSpeed=
            speedIndex==0
                ? 0.25f
                : (speedIndex==1
                    ? 0.5f
                    : (speedIndex==2
                        ? 1.0f
                        : 2.0f));
    }

    ImGui::SameLine(0.0f,14.0f);

    ImGui::Checkbox(
        "Snap",
        &snapToFrames
    );

    ImGui::SameLine(0.0f,10.0f);

    ImGui::Checkbox(
        "Follow",
        &followPlayhead
    );

    ImGui::SameLine(0.0f,14.0f);

    ImGui::TextDisabled("ZOOM");
    ImGui::SameLine(0.0f,6.0f);

    ImGui::SetNextItemWidth(118.0f);

    ImGui::SliderFloat(
        "##TimelineZoom",
        &zoom,
        0.75f,
        3.0f,
        "%.2fx"
    );

    ImGui::PopStyleVar();

    ImGui::Separator();

    // ========================================================
    // TIMELINE GEOMETRY
    // ========================================================

    const float headerW=154.0f;
    const float rulerH=34.0f;
    const float phaseH=28.0f;
    const float trackH=34.0f;
    const int trackCount=4;

    const ImVec2 canvasPos=
        ImGui::GetCursorScreenPos();

    const ImVec2 avail=
        ImGui::GetContentRegionAvail();

    const float timelineW=
        std::max(
            240.0f,
            avail.x-headerW
        );

    const float totalH=
        rulerH+
        phaseH+
        static_cast<float>(trackCount)*trackH+
        8.0f;

    ImGui::InvisibleButton(
        "##TimelineCanvas",
        ImVec2(avail.x,totalH),
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight
    );

    const bool canvasHovered=
        ImGui::IsItemHovered();

    ImDrawList* dl=
        ImGui::GetWindowDrawList();

    const ImVec2 timelineMin(
        canvasPos.x+headerW,
        canvasPos.y
    );

    const ImVec2 timelineMax(
        canvasPos.x+headerW+timelineW,
        canvasPos.y+totalH
    );

    // Visible frame range.
    const float visibleFrameCount=
        static_cast<float>(totalFrames)/
        zoom;

    const int visibleFrames=
        std::max(
            30,
            static_cast<int>(visibleFrameCount)
        );

    int viewStart=
        followPlayhead
            ? currentFrame-visibleFrames/2
            : 0;

    viewStart=
        std::max(
            0,
            std::min(
                totalFrames-visibleFrames,
                viewStart
            )
        );

    const int viewEnd=
        std::min(
            totalFrames,
            viewStart+visibleFrames
        );

    auto frameToX = [&](int frame)
    {
        const float t=
            static_cast<float>(
                frame-viewStart
            )/
            static_cast<float>(
                std::max(
                    1,
                    viewEnd-viewStart
                )
            );

        return timelineMin.x+
            t*timelineW;
    };

    auto xToFrame = [&](float x)
    {
        const float t=
            std::max(
                0.0f,
                std::min(
                    1.0f,
                    (x-timelineMin.x)/
                    timelineW
                )
            );

        int frame=
            viewStart+
            static_cast<int>(
                t*
                static_cast<float>(
                    viewEnd-viewStart
                )
            );

        if (snapToFrames)
        {
            frame=
                std::max(
                    0,
                    std::min(
                        totalFrames,
                        frame
                    )
                );
        }

        return frame;
    };

    // ========================================================
    // LEFT TRACK HEADER
    // ========================================================

    dl->AddRectFilled(
        canvasPos,
        ImVec2(
            canvasPos.x+headerW-4.0f,
            canvasPos.y+totalH
        ),
        IM_COL32(25,28,32,255)
    );

    dl->AddLine(
        ImVec2(
            canvasPos.x+headerW-4.0f,
            canvasPos.y
        ),
        ImVec2(
            canvasPos.x+headerW-4.0f,
            canvasPos.y+totalH
        ),
        toU32(colorBorder()),
        1.0f
    );

    dl->AddText(
        ImVec2(
            canvasPos.x+10.0f,
            canvasPos.y+9.0f
        ),
        toU32(colorText()),
        "RECONSTRUCTION"
    );

    dl->AddText(
        ImVec2(
            canvasPos.x+10.0f,
            canvasPos.y+rulerH+7.0f
        ),
        toU32(colorMuted()),
        "PHASE"
    );

    const char* trackNames[]={
        "Vehicle A",
        "Vehicle B",
        "Evidence",
        "Measurements"
    };

    for (int i=0;i<trackCount;++i)
    {
        const float y=
            canvasPos.y+
            rulerH+
            phaseH+
            static_cast<float>(i)*trackH;

        if ((i%2)==1)
        {
            dl->AddRectFilled(
                ImVec2(
                    canvasPos.x,
                    y
                ),
                ImVec2(
                    canvasPos.x+headerW-4.0f,
                    y+trackH
                ),
                IM_COL32(30,33,38,255)
            );
        }

        const bool linkedTrackSelected=
            (i==0 &&
                gEditorShell.selectedEntity==3) |
            (i==1 &&
                gEditorShell.selectedEntity==4) |
            (i==2 &&
                sharedSelectionIsEvidence()) |
            (i==3 &&
                sharedSelectionIsMeasurement());

        if (linkedTrackSelected)
        {
            dl->AddRectFilled(
                ImVec2(
                    canvasPos.x+2.0f,
                    y+2.0f
                ),
                ImVec2(
                    canvasPos.x+headerW-6.0f,
                    y+trackH-2.0f
                ),
                IM_COL32(45,58,76,235),
                3.0f
            );

            dl->AddRectFilled(
                ImVec2(
                    canvasPos.x+2.0f,
                    y+2.0f
                ),
                ImVec2(
                    canvasPos.x+5.0f,
                    y+trackH-2.0f
                ),
                IM_COL32(88,145,220,255)
            );
        }
        dl->AddText(
            ImVec2(
                canvasPos.x+18.0f,
                y+9.0f
            ),
            toU32(colorText()),
            trackNames[i]
        );
    }

    // ========================================================
    // RULER
    // ========================================================

    dl->AddRectFilled(
        timelineMin,
        ImVec2(
            timelineMax.x,
            timelineMin.y+rulerH
        ),
        IM_COL32(30,33,38,255)
    );

    int majorStep=30;

    if (zoom>=1.5f)
        majorStep=15;

    if (zoom>=2.5f)
        majorStep=10;

    for (int frame=viewStart;frame<=viewEnd;++frame)
    {
        const float x=
            frameToX(frame);

        const bool major=
            (frame%majorStep)==0;

        const bool medium=
            (frame%(majorStep/2))==0;

        const float tickH=
            major
                ? 14.0f
                : (medium
                    ? 9.0f
                    : 5.0f);

        dl->AddLine(
            ImVec2(
                x,
                timelineMin.y+rulerH-tickH
            ),
            ImVec2(
                x,
                timelineMin.y+rulerH
            ),
            major
                ? IM_COL32(158,164,174,210)
                : IM_COL32(88,93,101,180),
            1.0f
        );

        if (major)
        {
            char label[32]{};

            std::snprintf(
                label,
                sizeof(label),
                "%d",
                frame
            );

            dl->AddText(
                ImVec2(
                    x+4.0f,
                    timelineMin.y+5.0f
                ),
                IM_COL32(160,166,176,235),
                label
            );
        }
    }

    // ========================================================
    // PHASE BAND
    // ========================================================

    const float phaseY=
        timelineMin.y+rulerH;

    const float phaseBottom=
        phaseY+phaseH;

    auto phaseRect = [&](int start,int end,ImU32 fill,const char* label)
    {
        const int clampedStart=
            std::max(
                start,
                viewStart
            );

        const int clampedEnd=
            std::min(
                end,
                viewEnd
            );

        if (clampedEnd<=clampedStart)
            return;

        const float x0=
            frameToX(clampedStart);

        const float x1=
            frameToX(clampedEnd);

        dl->AddRectFilled(
            ImVec2(x0,phaseY),
            ImVec2(x1,phaseBottom),
            fill
        );

        const ImVec2 textSize=
            ImGui::CalcTextSize(label);

        if ((x1-x0)>textSize.x+18.0f)
        {
            dl->AddText(
                ImVec2(
                    x0+8.0f,
                    phaseY+6.0f
                ),
                IM_COL32(218,221,226,245),
                label
            );
        }
    };

    phaseRect(
        0,
        preImpactEnd,
        IM_COL32(60,68,78,235),
        "PRE-IMPACT"
    );

    phaseRect(
        preImpactEnd,
        impactEnd,
        IM_COL32(88,74,52,245),
        "IMPACT"
    );

    phaseRect(
        impactEnd,
        totalFrames,
        IM_COL32(54,62,70,235),
        "POST-IMPACT"
    );

    // Phase boundaries.
    const int phaseBoundaries[]={
        preImpactEnd,
        impactEnd
    };

    for (int boundary : phaseBoundaries)
    {
        if (boundary>=viewStart &&
            boundary<=viewEnd)
        {
            const float x=
                frameToX(boundary);

            dl->AddLine(
                ImVec2(
                    x,
                    timelineMin.y
                ),
                ImVec2(
                    x,
                    timelineMax.y
                ),
                IM_COL32(192,151,76,185),
                1.4f
            );
        }
    }

    // ========================================================
    // TRACK LANES
    // ========================================================

    const float tracksTop=
        phaseBottom;

    for (int i=0;i<trackCount;++i)
    {
        const float y0=
            tracksTop+
            static_cast<float>(i)*trackH;

        const float y1=
            y0+trackH;

        dl->AddRectFilled(
            ImVec2(
                timelineMin.x,
                y0
            ),
            ImVec2(
                timelineMax.x,
                y1
            ),
            (i%2)==0
                ? IM_COL32(22,25,29,255)
                : IM_COL32(27,30,35,255)
        );

        dl->AddLine(
            ImVec2(
                timelineMin.x,
                y1
            ),
            ImVec2(
                timelineMax.x,
                y1
            ),
            IM_COL32(54,59,66,210),
            1.0f
        );

        const char* emptyLabel=
            i<2
                ? "No vehicle events"
                : (i==2
                    ? "No evidence events"
                    : "No measurement events");

        dl->AddText(
            ImVec2(
                timelineMin.x+10.0f,
                y0+9.0f
            ),
            IM_COL32(104,110,120,190),
            emptyLabel
        );
    }

    // User markers.
    for (int i=0;i<userMarkerCount;++i)
    {
        const int markerFrame=
            userMarkers[
                static_cast<size_t>(i)
            ];

        if (markerFrame<viewStart |
            markerFrame>viewEnd)
        {
            continue;
        }

        const float x=
            frameToX(markerFrame);

        dl->AddLine(
            ImVec2(
                x,
                phaseBottom
            ),
            ImVec2(
                x,
                timelineMax.y
            ),
            IM_COL32(205,174,108,210),
            1.5f
        );

        dl->AddTriangleFilled(
            ImVec2(x,phaseBottom),
            ImVec2(x-5.0f,phaseBottom+8.0f),
            ImVec2(x+5.0f,phaseBottom+8.0f),
            IM_COL32(220,184,106,235)
        );
    }

    // ========================================================
    // PLAYHEAD
    // ========================================================

    const float playheadX=
        frameToX(currentFrame);

    if (currentFrame>=viewStart &&
        currentFrame<=viewEnd)
    {
        dl->AddLine(
            ImVec2(
                playheadX,
                timelineMin.y
            ),
            ImVec2(
                playheadX,
                timelineMax.y
            ),
            toU32(colorAccent()),
            2.0f
        );

        dl->AddTriangleFilled(
            ImVec2(
                playheadX,
                timelineMin.y+rulerH
            ),
            ImVec2(
                playheadX-7.0f,
                timelineMin.y+rulerH-10.0f
            ),
            ImVec2(
                playheadX+7.0f,
                timelineMin.y+rulerH-10.0f
            ),
            toU32(colorAccent())
        );
    }

    // Selected frame indicator.
    if (selectedFrame>=viewStart &&
        selectedFrame<=viewEnd &&
        selectedFrame!=currentFrame)
    {
        const float selectedX=
            frameToX(selectedFrame);

        dl->AddLine(
            ImVec2(
                selectedX,
                phaseBottom
            ),
            ImVec2(
                selectedX,
                timelineMax.y
            ),
            IM_COL32(150,158,170,125),
            1.0f
        );
    }

    // ========================================================
    // MOUSE INTERACTION
    // ========================================================

    if (canvasHovered)
    {
        const ImVec2 mouse=
            ImGui::GetIO().MousePos;

        const float trackHeaderTop=
            canvasPos.y+
            rulerH+
            phaseH;

        const float trackHeaderBottom=
            trackHeaderTop+
            static_cast<float>(
                trackCount
            )*
            trackH;

        if (mouse.x>=canvasPos.x &&
            mouse.x<timelineMin.x &&
            mouse.y>=trackHeaderTop &&
            mouse.y<trackHeaderBottom &&
            ImGui::IsMouseClicked(
                ImGuiMouseButton_Left))
        {
            const int trackIndex=
                std::max(
                    0,
                    std::min(
                        trackCount-1,
                        static_cast<int>(
                            (mouse.y-
                                trackHeaderTop)/
                            trackH
                        )
                    )
                );

            const int linkedEntities[]={
                3,
                4,
                5,
                8
            };

            setSharedEntitySelection(
                linkedEntities[
                    trackIndex
                ],
                "Timeline"
            );

            playing=false;
        }
        if (mouse.x>=timelineMin.x &&
            mouse.x<=timelineMax.x)
        {
            if (ImGui::IsMouseClicked(
                ImGuiMouseButton_Left))
            {
                selectedFrame=
                    xToFrame(mouse.x);

                currentFrame=
                    selectedFrame;

                playing=false;
            }

            if (ImGui::IsMouseDragging(
                ImGuiMouseButton_Left,
                0.0f))
            {
                selectedFrame=
                    xToFrame(mouse.x);

                currentFrame=
                    selectedFrame;

                playing=false;
            }
        }
    }

    // ========================================================
    // TIMELINE CONTEXT MENU
    // ========================================================

    if (ImGui::BeginPopupContextItem(
        "##TimelineContextMenu"))
    {
        ImGui::TextDisabled(
            "TIMELINE"
        );

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Add Marker at Playhead"))
        {
            if (userMarkerCount<
                static_cast<int>(
                    userMarkers.size()
                ))
            {
                userMarkers[
                    static_cast<size_t>(
                        userMarkerCount
                    )
                ]=currentFrame;

                userMarkerCount++;
            }
        }

        if (ImGui::MenuItem(
            "Clear Markers",
            nullptr,
            false,
            userMarkerCount>0))
        {
            userMarkers.fill(-1);
            userMarkerCount=0;
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Jump to Pre-impact"))
        {
            currentFrame=0;
            selectedFrame=0;
            playing=false;
        }

        if (ImGui::MenuItem(
            "Jump to Impact"))
        {
            currentFrame=preImpactEnd;
            selectedFrame=currentFrame;
            playing=false;
        }

        if (ImGui::MenuItem(
            "Jump to Post-impact"))
        {
            currentFrame=impactEnd;
            selectedFrame=currentFrame;
            playing=false;
        }

        ImGui::Separator();

        ImGui::MenuItem(
            "Snap to Frames",
            nullptr,
            &snapToFrames
        );

        ImGui::MenuItem(
            "Follow Playhead",
            nullptr,
            &followPlayhead
        );

        ImGui::EndPopup();
    }

    // ========================================================
    // FOOTER READOUT
    // ========================================================

    ImGui::Spacing();
    ImGui::Separator();

    const float seconds=
        static_cast<float>(currentFrame)/
        static_cast<float>(fps);

    const char* phaseName=
        currentFrame<preImpactEnd
            ? "Pre-impact"
            : (currentFrame<impactEnd
                ? "Impact"
                : "Post-impact");

    ImGui::TextDisabled("CURRENT");
    ImGui::SameLine(0.0f,7.0f);
    ImGui::Text("Frame %d",currentFrame);

    ImGui::SameLine(0.0f,18.0f);
    ImGui::TextDisabled("TIME");
    ImGui::SameLine(0.0f,7.0f);
    ImGui::Text("%.2f s",seconds);

    ImGui::SameLine(0.0f,18.0f);
    ImGui::TextDisabled("PHASE");
    ImGui::SameLine(0.0f,7.0f);
    ImGui::Text("%s",phaseName);

    ImGui::SameLine(0.0f,18.0f);
    ImGui::TextDisabled("FPS");
    ImGui::SameLine(0.0f,7.0f);
    ImGui::Text("%d",fps);

    ImGui::SameLine(0.0f,18.0f);
    ImGui::TextDisabled("MARKERS");
    ImGui::SameLine(0.0f,7.0f);
    ImGui::Text("%d",userMarkerCount);

    ImGui::End();
}

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

static void drawCommandPalette()
{
    if (!gEditorShell.showCommandPalette)
        return;

    static const SovereignCommand commands[]={
        {1,"Focus Case View","Open the case workspace","Ctrl+1"},
        {2,"Focus Evidence","Open the evidence workspace","Ctrl+2"},
        {3,"Focus Analysis","Open the analysis workspace","Ctrl+3"},
        {4,"Focus Viewport","Open the scene viewport","Ctrl+4"},
        {5,"Focus Timeline","Open the reconstruction timeline","Ctrl+5"},
        {6,"Focus Node Editor","Open the analysis graph","Ctrl+6"},

        {10,"Toggle Outliner","Show or hide Scene Outliner","Ctrl+Shift+O"},
        {11,"Toggle Properties","Show or hide Properties","Ctrl+Shift+I"},
        {12,"Toggle Timeline","Show or hide Timeline","Ctrl+Shift+T"},
        {13,"Toggle Node Editor","Show or hide Node Editor","Ctrl+Shift+N"},

        {20,"Reset Editor Layout","Restore the default dock layout","Ctrl+Shift+R"},
        {21,"Toggle Snap","Enable or disable snapping","Shift+Tab"},
        {22,"Show Shortcut Reference","Open app-wide keyboard shortcuts","F1"},

        {30,"Tool: Select","Activate selection tool","Q"},
        {31,"Tool: Move","Activate move tool","W"},
        {32,"Tool: Rotate","Activate rotate tool","E"},
        {33,"Tool: Scale","Activate scale tool","R"},

        {40,"Mode: Scene","Scene editing workstation mode",""},
        {41,"Mode: Evidence","Evidence placement workstation mode",""},
        {42,"Mode: Measure","Measurement workstation mode",""},
        {43,"Mode: Reconstruct","Reconstruction workstation mode",""},
        {44,"Mode: Review","Review workstation mode",""}
    };

    const ImGuiViewport* viewport=
        ImGui::GetMainViewport();

    const ImVec2 center=
        viewport
            ? viewport->GetCenter()
            : ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

    ImGui::SetNextWindowPos(
        center,
        ImGuiCond_Appearing,
        ImVec2(0.5f,0.34f)
    );

    ImGui::SetNextWindowSize(
        ImVec2(650.0f,430.0f),
        ImGuiCond_Appearing
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(12.0f,12.0f)
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(8.0f,6.0f)
    );

    const ImGuiWindowFlags flags=
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    bool open=
        gEditorShell.showCommandPalette;

    if (ImGui::Begin(
        "Command Palette",
        &open,
        flags))
    {
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();

        const bool submitted=
            ImGui::InputTextWithHint(
                "##CommandPaletteSearch",
                "Type a command...",
                gEditorShell.commandSearch,
                sizeof(
                    gEditorShell.commandSearch
                ),
                ImGuiInputTextFlags_EnterReturnsTrue
            );

        if (ImGui::IsKeyPressed(
            ImGuiKey_Escape))
        {
            open=false;
        }

        ImGui::Separator();

        int visibleCount=0;
        int selectedVisibleIndex=
            gEditorShell.commandPaletteSelection;

        // Clamp selection against this frame's result count later.
        for (const SovereignCommand& command:
             commands)
        {
            const bool matches=
                sovereignCommandMatches(
                    command.name,
                    gEditorShell.commandSearch
                ) ||
                sovereignCommandMatches(
                    command.detail,
                    gEditorShell.commandSearch
                );

            if (matches)
                visibleCount++;
        }

        if (visibleCount<=0)
        {
            gEditorShell.commandPaletteSelection=0;

            ImGui::Spacing();
            ImGui::TextDisabled(
                "No matching commands."
            );
        }
        else
        {
            if (selectedVisibleIndex<0)
                selectedVisibleIndex=0;

            if (selectedVisibleIndex>=visibleCount)
                selectedVisibleIndex=
                    visibleCount-1;

            if (ImGui::IsKeyPressed(
                ImGuiKey_DownArrow))
            {
                selectedVisibleIndex=
                    std::min(
                        visibleCount-1,
                        selectedVisibleIndex+1
                    );
            }

            if (ImGui::IsKeyPressed(
                ImGuiKey_UpArrow))
            {
                selectedVisibleIndex=
                    std::max(
                        0,
                        selectedVisibleIndex-1
                    );
            }

            gEditorShell.commandPaletteSelection=
                selectedVisibleIndex;

            int visibleIndex=0;
            int submittedId=0;

            for (const SovereignCommand& command:
                 commands)
            {
                const bool matches=
                    sovereignCommandMatches(
                        command.name,
                        gEditorShell.commandSearch
                    ) ||
                    sovereignCommandMatches(
                        command.detail,
                        gEditorShell.commandSearch
                    );

                if (!matches)
                    continue;

                const bool selected=
                    visibleIndex==
                    selectedVisibleIndex;

                ImGui::PushID(command.id);

                if (ImGui::Selectable(
                    "##CommandRow",
                    selected,
                    ImGuiSelectableFlags_AllowDoubleClick,
                    ImVec2(0.0f,46.0f)))
                {
                    submittedId=
                        command.id;
                }

                const ImVec2 rowMin=
                    ImGui::GetItemRectMin();

                const ImVec2 rowMax=
                    ImGui::GetItemRectMax();

                ImDrawList* drawList=
                    ImGui::GetWindowDrawList();

                drawList->AddText(
                    ImVec2(
                        rowMin.x+10.0f,
                        rowMin.y+6.0f
                    ),
                    toU32(colorText()),
                    command.name
                );

                drawList->AddText(
                    ImVec2(
                        rowMin.x+10.0f,
                        rowMin.y+25.0f
                    ),
                    toU32(colorMuted()),
                    command.detail
                );

                if (command.shortcut &&
                    command.shortcut[0])
                {
                    const ImVec2 shortcutSize=
                        ImGui::CalcTextSize(
                            command.shortcut
                        );

                    drawList->AddText(
                        ImVec2(
                            rowMax.x-
                                shortcutSize.x-
                                12.0f,
                            rowMin.y+15.0f
                        ),
                        toU32(colorMuted()),
                        command.shortcut
                    );
                }

                ImGui::PopID();

                if (submittedId!=0)
                {
                    executeSovereignCommand(
                        submittedId
                    );
                    break;
                }

                visibleIndex++;
            }

            if (submitted &&
                visibleCount>0)
            {
                int index=0;

                for (const SovereignCommand& command:
                     commands)
                {
                    const bool matches=
                        sovereignCommandMatches(
                            command.name,
                            gEditorShell.commandSearch
                        ) ||
                        sovereignCommandMatches(
                            command.detail,
                            gEditorShell.commandSearch
                        );

                    if (!matches)
                        continue;

                    if (index==
                        gEditorShell.commandPaletteSelection)
                    {
                        executeSovereignCommand(
                            command.id
                        );
                        break;
                    }

                    index++;
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::TextDisabled(
            "Up/Down navigate   Enter run   Esc close"
        );
    }

    ImGui::End();

    gEditorShell.showCommandPalette=open;

    ImGui::PopStyleVar(2);
}
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

static void drawShortcutToast()
{
    if (gEditorShell.shortcutToast[0]==0)
        return;

    if (ImGui::GetTime()>
        gEditorShell.shortcutToastUntil)
    {
        gEditorShell.shortcutToast[0]=0;
        return;
    }

    const ImGuiViewport* viewport=
        ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x+
                viewport->WorkSize.x-18.0f,
            viewport->WorkPos.y+
                viewport->WorkSize.y-18.0f
        ),
        ImGuiCond_Always,
        ImVec2(1.0f,1.0f)
    );

    ImGui::SetNextWindowBgAlpha(0.94f);

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(14.0f,9.0f)
    );

    if (ImGui::Begin(
        "##ShortcutToast",
        nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav))
    {
        ImGui::TextUnformatted(
            gEditorShell.shortcutToast
        );
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

static void drawShortcutReferenceWindow()
{
    if (!gEditorShell.showShortcutReference)
        return;

    ImGui::SetNextWindowSize(
        ImVec2(780.0f,620.0f),
        ImGuiCond_FirstUseEver
    );

    if (!ImGui::Begin(
        "Keyboard Shortcuts",
        &gEditorShell.showShortcutReference))
    {
        ImGui::End();
        return;
    }

    auto shortcutRow =
        [](const char* action,
           const char* key,
           const char* scope)
    {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(action);

        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("%s",key);

        ImGui::TableSetColumnIndex(2);
        ImGui::TextDisabled("%s",scope);
    };

    if (ImGui::BeginTable(
        "##ShortcutReferenceTable",
        3,
        ImGuiTableFlags_BordersInnerH |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn(
            "ACTION",
            ImGuiTableColumnFlags_WidthStretch,
            0.48f
        );

        ImGui::TableSetupColumn(
            "SHORTCUT",
            ImGuiTableColumnFlags_WidthStretch,
            0.22f
        );

        ImGui::TableSetupColumn(
            "SCOPE",
            ImGuiTableColumnFlags_WidthStretch,
            0.30f
        );

        ImGui::TableHeadersRow();

        shortcutRow(
            "Shortcut reference",
            "F1",
            "Global"
        );

        shortcutRow(
            "Command search",
            "Ctrl+Shift+P / Ctrl+K",
            "Global"
        );

        shortcutRow(
            "Case / Evidence / Analysis / Viewport",
            "Ctrl+1 / 2 / 3 / 4",
            "Workspace"
        );

        shortcutRow(
            "Timeline / Node Editor",
            "Ctrl+5 / Ctrl+6",
            "Workspace"
        );

        shortcutRow(
            "Toggle Outliner",
            "Ctrl+Shift+O",
            "Workspace"
        );

        shortcutRow(
            "Toggle Properties",
            "Ctrl+Shift+I",
            "Workspace"
        );

        shortcutRow(
            "Toggle Timeline",
            "Ctrl+Shift+T",
            "Workspace"
        );

        shortcutRow(
            "Toggle Node Editor",
            "Ctrl+Shift+N",
            "Workspace"
        );

        shortcutRow(
            "Reset workspace layout",
            "Ctrl+Shift+R",
            "Workspace"
        );

        shortcutRow(
            "Select / Move / Rotate / Scale",
            "Q / W / E / R",
            "Editor"
        );

        shortcutRow(
            "Toggle snapping",
            "Shift+Tab",
            "Editor"
        );

        shortcutRow(
            "Clear selection",
            "Shift+A",
            "Editor"
        );

        shortcutRow(
            "2D / 3D / AR viewport",
            "Alt+1 / Alt+2 / Alt+3",
            "Viewport"
        );

        shortcutRow(
            "Viewport tool Select/Move/Rotate/Scale",
            "Q / W / E / R",
            "Viewport"
        );

        shortcutRow(
            "Top / Front / Right",
            "1 / 2 / 3",
            "2D Viewport"
        );

        shortcutRow(
            "Perspective / Top / Front / Right",
            "1 / 2 / 3 / 4",
            "3D Viewport"
        );

        shortcutRow(
            "Cycle Lit/Wireframe/Analysis",
            "Z",
            "3D Viewport"
        );

        shortcutRow(
            "Grid / Axes / Bounds / Measurements / Names",
            "G / X / B / M / N",
            "Viewport"
        );

        shortcutRow(
            "Safe frame",
            "Shift+F",
            "Viewport"
        );

        shortcutRow(
            "Editor Preview / Place Anchor / Clear Anchors",
            "P / A / C",
            "AR Viewport"
        );

        shortcutRow(
            "Play / Pause",
            "Space",
            "Timeline"
        );

        shortcutRow(
            "Step frame",
            "Left / Right",
            "Timeline"
        );

        shortcutRow(
            "Step 10 frames",
            "Shift+Left / Shift+Right",
            "Timeline"
        );

        shortcutRow(
            "Start / End",
            "Home / End",
            "Timeline"
        );

        shortcutRow(
            "Pre-impact / Impact / Post-impact",
            "1 / 2 / 3",
            "Timeline"
        );

        shortcutRow(
            "Add / Clear markers",
            "M / Ctrl+Shift+M",
            "Timeline"
        );

        shortcutRow(
            "Snap / Follow playhead",
            "S / F",
            "Timeline"
        );

        shortcutRow(
            "Run graph",
            "Ctrl+Enter",
            "Node Editor"
        );

        shortcutRow(
            "Save / Load graph",
            "Ctrl+S / Ctrl+O",
            "Node Editor"
        );

        shortcutRow(
            "Undo / Redo",
            "Ctrl+Z / Ctrl+Y",
            "Node Editor"
        );

        shortcutRow(
            "Copy / Paste / Duplicate",
            "Ctrl+C / Ctrl+V / Ctrl+D",
            "Node Editor"
        );

        shortcutRow(
            "Select all / Delete",
            "Ctrl+A / Delete",
            "Node Editor"
        );

        shortcutRow(
            "Center graph",
            "Home",
            "Node Editor"
        );

        shortcutRow(
            "Grid / Snap",
            "G / Shift+Tab",
            "Node Editor"
        );

        shortcutRow(
            "Cancel link / clear selection",
            "Esc",
            "Node Editor"
        );

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::TextDisabled(
        "Shortcuts are suppressed while typing into text fields unless they use a dedicated Ctrl modifier."
    );

    ImGui::End();
}
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
static void drawSovereignStatusBar()
{
    const ImGuiViewport* viewport=
        ImGui::GetMainViewport();

    if (!viewport)
        return;

    const ImVec2 statusPos(
        viewport->WorkPos.x,
        viewport->WorkPos.y+
        viewport->WorkSize.y-
        SOVEREIGN_STATUS_BAR_HEIGHT
    );

    const ImVec2 statusSize(
        viewport->WorkSize.x,
        SOVEREIGN_STATUS_BAR_HEIGHT
    );

    ImGui::SetNextWindowPos(
        statusPos,
        ImGuiCond_Always
    );

    ImGui::SetNextWindowSize(
        statusSize,
        ImGuiCond_Always
    );

    ImGui::SetNextWindowViewport(
        viewport->ID
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(10.0f,5.0f)
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowBorderSize,
        1.0f
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowRounding,
        0.0f
    );

    ImGui::PushStyleColor(
        ImGuiCol_WindowBg,
        ImVec4(0.075f,0.079f,0.086f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.23f,0.245f,0.27f,1.0f)
    );

    const ImGuiWindowFlags flags=
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin(
        "##SovereignStatusBar",
        nullptr,
        flags))
    {
        auto separator = []()
        {
            ImGui::SameLine(0.0f,10.0f);

            ImGui::TextColored(
                ImVec4(0.31f,0.33f,0.37f,1.0f),
                "|"
            );

            ImGui::SameLine(0.0f,10.0f);
        };

        // ----------------------------------------------------
        // LEFT: application state
        // ----------------------------------------------------

        ImGui::TextColored(
            ImVec4(0.53f,0.80f,0.56f,1.0f),
            "READY"
        );

        separator();

        ImGui::TextDisabled(
            "Scene"
        );

        ImGui::SameLine(0.0f,5.0f);

        ImGui::Text(
        "%zu objects",
        roadSafeActiveSceneEntityCount()
    );

        separator();

        ImGui::TextDisabled(
            "Selected"
        );

        ImGui::SameLine(0.0f,5.0f);

        const char* selectedName=
            selectedEntityName();

        ImGui::Text(
            "%s",
            selectedName &&
            selectedName[0]
                ? selectedName
                : "None"
        );

        separator();

        ImGui::TextDisabled(
            "Snap"
        );

        ImGui::SameLine(0.0f,5.0f);

        if (gEditorShell.snapEnabled)
        {
            ImGui::Text(
                "%.2f m",
                gEditorShell.snapValue
            );
        }
        else
        {
            ImGui::TextDisabled(
                "Off"
            );
        }

        separator();

        ImGui::TextDisabled(
            "Units"
        );

        ImGui::SameLine(0.0f,5.0f);

        ImGui::Text(
            "Metric"
        );

        // ----------------------------------------------------
        // RIGHT: performance + discoverability
        // ----------------------------------------------------

        const float fps=
            ImGui::GetIO().Framerate;

        char rightText[128]{};

        std::snprintf(
            rightText,
            sizeof(rightText),
            "%.0f FPS  |  %dE %dW  |  F1 Shortcuts",
            fps,
            sovereignProblemErrorCount(),
            sovereignProblemWarningCount()
        );

        const float rightWidth=
            ImGui::CalcTextSize(
                rightText
            ).x;

        const float targetX=
            ImGui::GetWindowContentRegionMax().x-
            rightWidth;

        if (targetX>
            ImGui::GetCursorPosX()+20.0f)
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(
                targetX
            );

            ImGui::TextDisabled(
                "%s",
                rightText
            );
        }
    }

    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}
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

static void drawOutputPanel()
{
    ImGui::Begin(
        "Output",
        nullptr,
        ImGuiWindowFlags_NoMove
    );

    ImGui::TextDisabled("OUTPUT");

    ImGui::SameLine();

    ImGui::Text(
        "%d messages",
        static_cast<int>(
            gSovereignOutputEntries.size()
        )
    );

    if (gSovereignOutputEntries.empty())
    {
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextDisabled(
            "No output has been produced this session."
        );

        ImGui::TextDisabled(
            "Graph, analysis, import and AR messages will appear here."
        );

        ImGui::End();
        return;
    }

    ImGui::Separator();

    if (ImGui::BeginTable(
        "##SovereignOutputTable",
        4,
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable,
        ImVec2(0.0f,0.0f)))
    {
        ImGui::TableSetupScrollFreeze(0,1);

        ImGui::TableSetupColumn(
            "TIME",
            ImGuiTableColumnFlags_WidthFixed,
            72.0f
        );

        ImGui::TableSetupColumn(
            "LEVEL",
            ImGuiTableColumnFlags_WidthFixed,
            84.0f
        );

        ImGui::TableSetupColumn(
            "SOURCE",
            ImGuiTableColumnFlags_WidthFixed,
            120.0f
        );

        ImGui::TableSetupColumn(
            "MESSAGE",
            ImGuiTableColumnFlags_WidthStretch
        );

        ImGui::TableHeadersRow();

        for (const SovereignOutputEntry& entry:
             gSovereignOutputEntries)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::Text(
                "%07.2f",
                entry.timeSeconds
            );

            ImGui::TableSetColumnIndex(1);

            ImGui::TextColored(
                sovereignDiagnosticLevelColor(
                    entry.level
                ),
                "%s",
                sovereignDiagnosticLevelName(
                    entry.level
                )
            );

            ImGui::TableSetColumnIndex(2);

            ImGui::TextUnformatted(
                entry.source.c_str()
            );

            ImGui::TableSetColumnIndex(3);

            ImGui::TextWrapped(
                "%s",
                entry.message.c_str()
            );
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

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
static void drawProblemsPanel()
{
    ImGui::Begin(
        "Problems",
        nullptr,
        ImGuiWindowFlags_NoMove
    );

    int errors=0;
    int warnings=0;
    int info=0;

    for (const SovereignProblemEntry& entry:
         gSovereignProblemEntries)
    {
        switch (entry.level)
        {
            case SovereignDiagnosticLevel::Error:
                errors++;
                break;

            case SovereignDiagnosticLevel::Warning:
                warnings++;
                break;

            case SovereignDiagnosticLevel::Info:
                info++;
                break;
        }
    }

    ImGui::TextDisabled("PROBLEMS");

    ImGui::SameLine();

    ImGui::TextColored(
        sovereignDiagnosticLevelColor(
            SovereignDiagnosticLevel::Error
        ),
        "%d errors",
        errors
    );

    ImGui::SameLine();

    ImGui::TextDisabled("|");

    ImGui::SameLine();

    ImGui::TextColored(
        sovereignDiagnosticLevelColor(
            SovereignDiagnosticLevel::Warning
        ),
        "%d warnings",
        warnings
    );

    ImGui::SameLine();

    ImGui::TextDisabled("|");

    ImGui::SameLine();

    ImGui::Text(
        "%d info",
        info
    );

    ImGui::Separator();

    if (gSovereignProblemEntries.empty())
    {
        ImGui::Spacing();

        ImGui::TextDisabled(
            "No problems have been reported this session."
        );

        ImGui::TextDisabled(
            "Validation, graph and reconstruction diagnostics will appear here."
        );

        ImGui::End();
        return;
    }

    if (ImGui::BeginTable(
        "##SovereignProblemsTable",
        3,
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable,
        ImVec2(0.0f,0.0f)))
    {
        ImGui::TableSetupScrollFreeze(0,1);

        ImGui::TableSetupColumn(
            "SEVERITY",
            ImGuiTableColumnFlags_WidthFixed,
            92.0f
        );

        ImGui::TableSetupColumn(
            "SOURCE",
            ImGuiTableColumnFlags_WidthFixed,
            135.0f
        );

        ImGui::TableSetupColumn(
            "DESCRIPTION",
            ImGuiTableColumnFlags_WidthStretch
        );

        ImGui::TableHeadersRow();

        for (const SovereignProblemEntry& entry:
             gSovereignProblemEntries)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::TextColored(
                sovereignDiagnosticLevelColor(
                    entry.level
                ),
                "%s",
                sovereignDiagnosticLevelName(
                    entry.level
                )
            );

            ImGui::TableSetColumnIndex(1);

            ImGui::TextUnformatted(
                entry.source.c_str()
            );

            ImGui::TableSetColumnIndex(2);

            ImGui::TextWrapped(
                "%s",
                entry.message.c_str()
            );
        }

        ImGui::EndTable();
    }

    ImGui::End();
}
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









































































