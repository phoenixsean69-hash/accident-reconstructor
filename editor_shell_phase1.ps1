param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - EDITOR SHELL PHASE 1" -ForegroundColor Cyan
Write-Host " Menus / Toolbar / Outliner / Properties" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'static void drawOutliner()',
    'static void drawProperties()',
    'static void drawTimeline()',
    'static void drawMainMenuBar()',
    'static void drawInterface()',
    'int main()',
    'drawIconBadge(',
    'drawGlyph(',
    'editorButton('
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-editor-shell-phase1-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. EDITOR-SHELL STATE + REUSABLE CONTROLS
#    Insert immediately before drawOutliner().
# ============================================================

if ($text -notmatch 'struct\s+EditorShellState') {

$helpers = @'
struct EditorShellState
{
    bool showOutliner=true;
    bool showProperties=true;
    bool showTimeline=true;
    bool snapEnabled=true;
    bool requestExit=false;
    bool resetLayoutRequested=false;

    int transformMode=0;
    int selectedEntity=0;

    float snapValue=0.10f;
    char outlinerSearch[96]{};
    char commandSearch[96]{};
};

static EditorShellState gEditorShell;

static const char* selectedEntityName()
{
    switch (gEditorShell.selectedEntity)
    {
        case 1: return "Ground Plane";
        case 2: return "Road Surface";
        case 3: return "Vehicle A";
        case 4: return "Vehicle B";
        case 5: return "Skid Mark 01";
        case 6: return "Marker 01";
        case 7: return "Debris Field 01";
        case 8: return "Distance 01";
        case 9: return "Angle 01";
        default: return "Nothing selected";
    }
}

static UiGlyph selectedEntityGlyph()
{
    switch (gEditorShell.selectedEntity)
    {
        case 1:
        case 2:
            return UiGlyph::Folder;

        case 3:
        case 4:
            return UiGlyph::Cube;

        case 5:
        case 7:
            return UiGlyph::Document;

        case 6:
            return UiGlyph::Marker;

        case 8:
        case 9:
            return UiGlyph::Ruler;

        default:
            return UiGlyph::Info;
    }
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

        dl->AddRect(
            p,
            ImVec2(p.x+size.x,p.y+size.y),
            toU32(colorAccent()),
            3.0f,
            0,
            1.0f
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

static void drawEditorToolbar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(9.0f,5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(5.0f,4.0f));

    beginSurface(
        "GlobalEditorToolbar",
        ImVec2(0.0f,48.0f),
        false,
        ImGuiWindowFlags_NoScrollbar
    );

    // Case/document controls.
    shellIconButton(
        "ToolbarNew",
        UiGlyph::Document,
        "New Case  (Ctrl+N)"
    );

    ImGui::SameLine();
    shellIconButton(
        "ToolbarOpen",
        UiGlyph::Folder,
        "Open Case  (Ctrl+O)"
    );

    ImGui::SameLine();
    shellIconButton(
        "ToolbarSave",
        UiGlyph::Document,
        "Save Case  (Ctrl+S)"
    );

    ImGui::SameLine();
    toolbarSeparator();

    ImGui::SameLine();
    if (editorButton("UNDO",62.0f))
    {
        // Hook for command history.
    }

    ImGui::SameLine();
    editorButton("REDO",62.0f,false,false);

    ImGui::SameLine();
    toolbarSeparator();

    // Add dropdown.
    ImGui::SameLine();
    if (ImGui::Button("ADD  +",ImVec2(78.0f,0.0f)))
        ImGui::OpenPopup("##AddToolbarPopup");

    if (ImGui::BeginPopup("##AddToolbarPopup"))
    {
        ImGui::TextDisabled("ADD TO SCENE");
        ImGui::Separator();
        ImGui::MenuItem("Vehicle");
        ImGui::MenuItem("Evidence");
        ImGui::MenuItem("Measurement");
        ImGui::MenuItem("Scene Marker");
        ImGui::EndPopup();
    }

    // Transform mode.
    ImGui::SameLine();
    const char* transformModes[]={
        "Select",
        "Move",
        "Rotate",
        "Scale"
    };

    ImGui::SetNextItemWidth(92.0f);
    ImGui::Combo(
        "##GlobalTransformMode",
        &gEditorShell.transformMode,
        transformModes,
        4
    );

    ImGui::SameLine();
    ImGui::Checkbox("Snap",&gEditorShell.snapEnabled);

    ImGui::SameLine();
    ImGui::BeginDisabled(!gEditorShell.snapEnabled);
    ImGui::SetNextItemWidth(82.0f);
    ImGui::DragFloat(
        "##GlobalSnapValue",
        &gEditorShell.snapValue,
        0.01f,
        0.01f,
        10.0f,
        "%.2f m"
    );
    ImGui::EndDisabled();

    // Right-side command search.
    const float searchW=220.0f;
    const float right=ImGui::GetWindowWidth()-searchW-14.0f;

    if (right>ImGui::GetCursorPosX()+20.0f)
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(right);
        ImGui::SetNextItemWidth(searchW);
        ImGui::InputTextWithHint(
            "##GlobalCommandSearch",
            "Search commands...",
            gEditorShell.commandSearch,
            sizeof(gEditorShell.commandSearch)
        );
    }

    endSurface();

    ImGui::PopStyleVar(2);
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

    ImGui::TableNextRow(ImGuiTableRowFlags_None,28.0f);

    ImGui::TableSetColumnIndex(0);
    ImGui::PushID(entityId);

    const bool selected=
        gEditorShell.selectedEntity==entityId;

    const ImVec2 rowPos=ImGui::GetCursorScreenPos();

    if (ImGui::Selectable(
        "##EntityRow",
        selected,
        ImGuiSelectableFlags_None,
        ImVec2(0.0f,25.0f)))
    {
        gEditorShell.selectedEntity=entityId;
    }

    drawGlyph(
        ImGui::GetWindowDrawList(),
        glyph,
        ImVec2(rowPos.x+12.0f,rowPos.y+12.0f),
        14.0f,
        toU32(selected ? colorAccent() : colorMuted())
    );

    ImGui::GetWindowDrawList()->AddText(
        ImVec2(rowPos.x+26.0f,rowPos.y+4.0f),
        toU32(colorText()),
        label
    );

    if (ImGui::BeginPopupContextItem("##EntityContext"))
    {
        ImGui::TextDisabled("%s",label);
        ImGui::Separator();
        ImGui::MenuItem("Rename","F2");
        ImGui::MenuItem("Duplicate","Ctrl+D");
        ImGui::MenuItem("Focus in Viewport","F");
        ImGui::Separator();
        ImGui::MenuItem("Delete","Del");
        ImGui::EndPopup();
    }

    ImGui::TableSetColumnIndex(1);
    if (ImGui::SmallButton(*visible ? "V" : "-"))
        *visible=!*visible;

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(*visible ? "Visible" : "Hidden");

    ImGui::TableSetColumnIndex(2);
    if (ImGui::SmallButton(*locked ? "L" : "-"))
        *locked=!*locked;

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(*locked ? "Locked" : "Unlocked");

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

'@

    $text = $text.Replace(
        'static void drawOutliner()',
        $helpers + 'static void drawOutliner()'
    )

    Write-Host "[OK] Added reusable editor-shell controls." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] Editor-shell state already exists." -ForegroundColor DarkGray
}

# ============================================================
# 2. REPLACE OUTLINER
# ============================================================

$outlinerPattern =
    'static void drawOutliner\(\)\s*\{[\s\S]*?(?=\r?\nstatic void drawProperties\(\))'

$outlinerMatches = [regex]::Matches(
    $text,
    $outlinerPattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($outlinerMatches.Count -ne 1) {
    throw "Expected exactly one drawOutliner() function, found $($outlinerMatches.Count)."
}

$outlinerReplacement = @'
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
        ImGui::MenuItem("Vehicle");
        ImGui::MenuItem("Evidence");
        ImGui::MenuItem("Measurement");
        ImGui::MenuItem("Scene Marker");
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
        3,
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
            "V",
            ImGuiTableColumnFlags_WidthFixed,
            28.0f
        );

        ImGui::TableSetupColumn(
            "L",
            ImGuiTableColumnFlags_WidthFixed,
            28.0f
        );

        ImGui::TableHeadersRow();

        const bool envMatches =
            shellLabelMatches("Ground Plane") ||
            shellLabelMatches("Road Surface");

        if (envMatches)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            const bool open=ImGui::TreeNodeEx(
                "Environment",
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth
            );

            if (open)
            {
                outlinerLeafRow(
                    "Ground Plane",
                    UiGlyph::Folder,
                    1,
                    &groundVisible,
                    &groundLocked
                );

                outlinerLeafRow(
                    "Road Surface",
                    UiGlyph::Folder,
                    2,
                    &roadVisible,
                    &roadLocked
                );

                ImGui::TreePop();
            }
        }

        const bool vehicleMatches =
            shellLabelMatches("Vehicle A") ||
            shellLabelMatches("Vehicle B");

        if (vehicleMatches)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            const bool open=ImGui::TreeNodeEx(
                "Vehicles",
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth
            );

            if (open)
            {
                outlinerLeafRow(
                    "Vehicle A",
                    UiGlyph::Cube,
                    3,
                    &vehicleAVisible,
                    &vehicleALocked
                );

                outlinerLeafRow(
                    "Vehicle B",
                    UiGlyph::Cube,
                    4,
                    &vehicleBVisible,
                    &vehicleBLocked
                );

                ImGui::TreePop();
            }
        }

        const bool evidenceMatches =
            shellLabelMatches("Skid Mark 01") ||
            shellLabelMatches("Marker 01") ||
            shellLabelMatches("Debris Field 01");

        if (evidenceMatches)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            const bool open=ImGui::TreeNodeEx(
                "Evidence",
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth
            );

            if (open)
            {
                outlinerLeafRow(
                    "Skid Mark 01",
                    UiGlyph::Document,
                    5,
                    &skidVisible,
                    &skidLocked
                );

                outlinerLeafRow(
                    "Marker 01",
                    UiGlyph::Marker,
                    6,
                    &markerVisible,
                    &markerLocked
                );

                outlinerLeafRow(
                    "Debris Field 01",
                    UiGlyph::Document,
                    7,
                    &debrisVisible,
                    &debrisLocked
                );

                ImGui::TreePop();
            }
        }

        const bool measurementMatches =
            shellLabelMatches("Distance 01") ||
            shellLabelMatches("Angle 01");

        if (measurementMatches)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            const bool open=ImGui::TreeNodeEx(
                "Measurements",
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth
            );

            if (open)
            {
                outlinerLeafRow(
                    "Distance 01",
                    UiGlyph::Ruler,
                    8,
                    &distanceVisible,
                    &distanceLocked
                );

                outlinerLeafRow(
                    "Angle 01",
                    UiGlyph::Ruler,
                    9,
                    &angleVisible,
                    &angleLocked
                );

                ImGui::TreePop();
            }
        }

        ImGui::EndTable();
    }

    // Bottom selection summary.
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("SELECTION");

    if (gEditorShell.selectedEntity==0)
        ImGui::TextDisabled("No scene object selected.");
    else
        ImGui::Text("%s",selectedEntityName());

    ImGui::End();
}

'@

$text = [regex]::Replace(
    $text,
    $outlinerPattern,
    $outlinerReplacement,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

Write-Host "[OK] Upgraded Outliner." -ForegroundColor Green

# ============================================================
# 3. REPLACE PROPERTIES / INSPECTOR
# ============================================================

$propertiesPattern =
    'static void drawProperties\(\)\s*\{[\s\S]*?(?=\r?\nstatic void drawTimeline\(\))'

$propertiesMatches = [regex]::Matches(
    $text,
    $propertiesPattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($propertiesMatches.Count -ne 1) {
    throw "Expected exactly one drawProperties() function, found $($propertiesMatches.Count)."
}

$propertiesReplacement = @'
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

    ImGui::Text("INSPECTOR");
    ImGui::Separator();

    // Selection header.
    beginSurface(
        "InspectorSelectionHeader",
        ImVec2(0.0f,72.0f),
        false,
        ImGuiWindowFlags_NoScrollbar
    );

    {
        const ImVec2 p=ImGui::GetCursorScreenPos();

        drawIconBadge(
            selectedEntityGlyph(),
            p,
            38.0f,
            gEditorShell.selectedEntity!=0
        );

        ImGui::SetCursorScreenPos(
            ImVec2(p.x+50.0f,p.y+1.0f)
        );

        if (gEditorShell.selectedEntity==0)
            ImGui::TextDisabled("NO SELECTION");
        else
            ImGui::Text("%s",selectedEntityName());

        ImGui::SetCursorScreenPos(
            ImVec2(p.x+50.0f,p.y+27.0f)
        );

        if (gEditorShell.selectedEntity==0)
            ImGui::TextDisabled("Select an object in the Scene Outliner.");
        else
            ImGui::TextDisabled("Scene Entity");
    }

    endSurface();

    ImGui::Spacing();

    if (gEditorShell.selectedEntity==0)
    {
        ImGui::TextDisabled(
            "Properties will appear when a scene object is selected."
        );
        ImGui::End();
        return;
    }

    // Transform.
    if (ImGui::CollapsingHeader(
        "Transform",
        ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginTable(
            "TransformPropertyGrid",
            2,
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_BordersInnerH))
        {
            ImGui::TableSetupColumn(
                "Property",
                ImGuiTableColumnFlags_WidthStretch,
                0.34f
            );

            ImGui::TableSetupColumn(
                "Value",
                ImGuiTableColumnFlags_WidthStretch,
                0.66f
            );

            propertyVec3Row(
                "Position",
                "##InspectorPosition",
                position,
                0.10f
            );

            propertyVec3Row(
                "Rotation",
                "##InspectorRotation",
                rotation,
                1.00f
            );

            propertyVec3Row(
                "Scale",
                "##InspectorScale",
                scale,
                0.01f
            );

            ImGui::EndTable();
        }

        ImGui::Spacing();

        if (editorButton("RESET TRANSFORM",138.0f))
        {
            position[0]=position[1]=position[2]=0.0f;
            rotation[0]=rotation[1]=rotation[2]=0.0f;
            scale[0]=scale[1]=scale[2]=1.0f;
        }
    }

    // Object metadata.
    if (ImGui::CollapsingHeader(
        "Object",
        ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (objectName[0]=='\0' ||
            std::string(objectName)=="Untitled Object")
        {
            std::snprintf(
                objectName,
                sizeof(objectName),
                "%s",
                selectedEntityName()
            );
        }

        if (ImGui::BeginTable(
            "ObjectPropertyGrid",
            2,
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_BordersInnerH))
        {
            ImGui::TableSetupColumn(
                "Property",
                ImGuiTableColumnFlags_WidthStretch,
                0.34f
            );

            ImGui::TableSetupColumn(
                "Value",
                ImGuiTableColumnFlags_WidthStretch,
                0.66f
            );

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Name");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText(
                "##InspectorName",
                objectName,
                sizeof(objectName)
            );

            propertyTextRow(
                "Type",
                "Scene Entity"
            );

            propertyTextRow(
                "Units",
                "Meters"
            );

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Visible");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox(
                "##InspectorVisible",
                &objectVisible
            );

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Locked");

            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox(
                "##InspectorLocked",
                &objectLocked
            );

            ImGui::EndTable();
        }
    }

    // Analysis metadata.
    if (ImGui::CollapsingHeader(
        "Analysis",
        ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginTable(
            "AnalysisPropertyGrid",
            2,
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_BordersInnerH))
        {
            ImGui::TableSetupColumn(
                "Property",
                ImGuiTableColumnFlags_WidthStretch,
                0.40f
            );

            ImGui::TableSetupColumn(
                "Value",
                ImGuiTableColumnFlags_WidthStretch,
                0.60f
            );

            propertyTextRow(
                "Analysis state",
                "Not calculated"
            );

            propertyTextRow(
                "Evidence links",
                "0"
            );

            propertyTextRow(
                "Confidence",
                "N/A"
            );

            ImGui::EndTable();
        }

        ImGui::Spacing();
        editorButton(
            "OPEN ANALYSIS",
            ImGui::GetContentRegionAvail().x,
            false,
            false
        );
    }

    ImGui::End();
}

'@

$text = [regex]::Replace(
    $text,
    $propertiesPattern,
    $propertiesReplacement,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

Write-Host "[OK] Upgraded Properties inspector." -ForegroundColor Green

# ============================================================
# 4. REPLACE GLOBAL MENU
# ============================================================

$menuPattern =
    'static void drawMainMenuBar\(\)\s*\{[\s\S]*?(?=\r?\nstatic void drawInterface\(\))'

$menuMatches = [regex]::Matches(
    $text,
    $menuPattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($menuMatches.Count -ne 1) {
    throw "Expected exactly one drawMainMenuBar() function, found $($menuMatches.Count)."
}

$menuReplacement = @'
static void drawMainMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
        return;

    // FILE
    if (ImGui::BeginMenu("File"))
    {
        ImGui::MenuItem("New Case","Ctrl+N");
        ImGui::MenuItem("Open Case...","Ctrl+O");

        if (ImGui::BeginMenu("Open Recent"))
        {
            ImGui::MenuItem("No recent cases",nullptr,false,false);
            ImGui::EndMenu();
        }

        ImGui::Separator();
        ImGui::MenuItem("Save","Ctrl+S");
        ImGui::MenuItem("Save As...","Ctrl+Shift+S");

        ImGui::Separator();

        if (ImGui::BeginMenu("Import"))
        {
            ImGui::MenuItem("Evidence...");
            ImGui::MenuItem("Scene Data...");
            ImGui::MenuItem("Vehicle Data...");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Export"))
        {
            ImGui::MenuItem("Case Package...");
            ImGui::MenuItem("Report...");
            ImGui::MenuItem("Scene Snapshot...");
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit","Alt+F4"))
            gEditorShell.requestExit=true;

        ImGui::EndMenu();
    }

    // EDIT
    if (ImGui::BeginMenu("Edit"))
    {
        ImGui::MenuItem("Undo","Ctrl+Z");
        ImGui::MenuItem("Redo","Ctrl+Y",false,false);

        ImGui::Separator();
        ImGui::MenuItem("Cut","Ctrl+X");
        ImGui::MenuItem("Copy","Ctrl+C");
        ImGui::MenuItem("Paste","Ctrl+V");

        ImGui::Separator();
        ImGui::MenuItem("Duplicate","Ctrl+D");
        ImGui::MenuItem("Delete","Del");

        ImGui::Separator();
        ImGui::MenuItem("Preferences...");
        ImGui::EndMenu();
    }

    // VIEW
    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem(
            "Scene Outliner",
            nullptr,
            &gEditorShell.showOutliner
        );

        ImGui::MenuItem(
            "Properties",
            nullptr,
            &gEditorShell.showProperties
        );

        ImGui::MenuItem(
            "Timeline",
            nullptr,
            &gEditorShell.showTimeline
        );

        ImGui::Separator();

        if (ImGui::MenuItem("Reset Workspace Layout"))
            gEditorShell.resetLayoutRequested=true;

        ImGui::Separator();

        if (ImGui::BeginMenu("Viewport"))
        {
            ImGui::MenuItem("Perspective");
            ImGui::MenuItem("Top");
            ImGui::MenuItem("Front");
            ImGui::MenuItem("Right");
            ImGui::Separator();
            ImGui::MenuItem("Frame Selection","F");
            ImGui::MenuItem("Frame All","Home");
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    // SCENE
    if (ImGui::BeginMenu("Scene"))
    {
        if (ImGui::BeginMenu("Add"))
        {
            ImGui::MenuItem("Vehicle");
            ImGui::MenuItem("Evidence");
            ImGui::MenuItem("Measurement");
            ImGui::MenuItem("Scene Marker");
            ImGui::EndMenu();
        }

        ImGui::Separator();
        ImGui::MenuItem("Focus Selection","F");
        ImGui::MenuItem("Select All","Ctrl+A");
        ImGui::MenuItem("Deselect All","Alt+A");

        ImGui::Separator();
        ImGui::MenuItem(
            "Snapping",
            nullptr,
            &gEditorShell.snapEnabled
        );

        ImGui::EndMenu();
    }

    // TOOLS
    if (ImGui::BeginMenu("Tools"))
    {
        if (ImGui::BeginMenu("Analysis"))
        {
            ImGui::MenuItem("Skid Analysis");
            ImGui::MenuItem("Momentum Analysis");
            ImGui::MenuItem("Speed Analysis");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Measurement"))
        {
            ImGui::MenuItem("Distance Tool");
            ImGui::MenuItem("Angle Tool");
            ImGui::MenuItem("Reference Marker");
            ImGui::EndMenu();
        }

        ImGui::Separator();
        ImGui::MenuItem("Validate Case");
        ImGui::MenuItem("Command Palette...","Ctrl+Shift+P");

        ImGui::EndMenu();
    }

    // HELP
    if (ImGui::BeginMenu("Help"))
    {
        ImGui::MenuItem("Documentation");
        ImGui::MenuItem("Keyboard Shortcuts");
        ImGui::Separator();
        ImGui::MenuItem("About Sovereign");
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

'@

$text = [regex]::Replace(
    $text,
    $menuPattern,
    $menuReplacement,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

Write-Host "[OK] Upgraded global menus." -ForegroundColor Green

# ============================================================
# 5. REPLACE DRAW INTERFACE TO ADD GLOBAL TOOLBAR
# ============================================================

$interfacePattern =
    'static void drawInterface\(\)\s*\{[\s\S]*?(?=\r?\nint main\(\))'

$interfaceMatches = [regex]::Matches(
    $text,
    $interfacePattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($interfaceMatches.Count -ne 1) {
    throw "Expected exactly one drawInterface() function, found $($interfaceMatches.Count)."
}

$interfaceReplacement = @'
static void drawInterface()
{
    static bool layoutBuilt=false;
    static ImGuiID rightDockNodeId=0;

    drawMainMenuBar();

    if (gEditorShell.resetLayoutRequested)
    {
        layoutBuilt=false;
        gEditorShell.resetLayoutRequested=false;
    }

    ImGuiViewport* viewport=ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
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

        ImGui::DockBuilderFinish(dockspace);
        layoutBuilt=true;
    }

    ImGui::End();
    ImGui::PopStyleVar();

    if (rightDockNodeId)
        enforceRightPanelBounds(rightDockNodeId);

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
}

'@

$text = [regex]::Replace(
    $text,
    $interfacePattern,
    $interfaceReplacement,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

Write-Host "[OK] Added persistent editor toolbar to workspace shell." -ForegroundColor Green

# ============================================================
# 6. MAKE FILE > EXIT ACTUALLY CLOSE THE GLFW WINDOW
# ============================================================

if ($text -notmatch 'gEditorShell\.requestExit\s*\)\s*glfwSetWindowShouldClose') {

    $drawInterfaceCallPattern = '(\s*drawInterface\(\);\s*)'

    $drawInterfaceCallMatches = [regex]::Matches(
        $text,
        $drawInterfaceCallPattern
    )

    if ($drawInterfaceCallMatches.Count -lt 1) {
        throw "Could not find drawInterface() call in main loop."
    }

    $text = [regex]::Replace(
        $text,
        $drawInterfaceCallPattern,
        '$1' + "`r`n        if (gEditorShell.requestExit) glfwSetWindowShouldClose(window,GLFW_TRUE);`r`n",
        1
    )

    Write-Host "[OK] Wired File > Exit to GLFW." -ForegroundColor Green
}

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$verifyMarkers = @(
    'struct EditorShellState',
    'drawEditorToolbar()',
    'SceneOutlinerTable',
    'TransformPropertyGrid',
    'Command Palette...',
    'Reset Workspace Layout',
    'GlobalEditorToolbar',
    'gEditorShell.requestExit'
)

foreach ($marker in $verifyMarkers) {
    if (-not $verify.Contains($marker)) {
        throw "Verification failed after write: $marker"
    }
}

Write-Host ""
Write-Host "[DONE] Editor Shell Phase 1 installed." -ForegroundColor Cyan
Write-Host "[OK] Premium global menu structure." -ForegroundColor Green
Write-Host "[OK] Persistent editor toolbar." -ForegroundColor Green
Write-Host "[OK] Searchable/selectable Outliner with visibility + lock controls." -ForegroundColor Green
Write-Host "[OK] Right-click Outliner context menus." -ForegroundColor Green
Write-Host "[OK] Two-column Properties inspector." -ForegroundColor Green
Write-Host "[OK] Selection-aware Properties panel." -ForegroundColor Green
Write-Host "[OK] View menu can show/hide core panels." -ForegroundColor Green
Write-Host "[OK] Reset Workspace Layout command." -ForegroundColor Green
Write-Host "[OK] File > Exit is wired." -ForegroundColor Green
Write-Host "[OK] Existing workspace screens and palette were preserved." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
