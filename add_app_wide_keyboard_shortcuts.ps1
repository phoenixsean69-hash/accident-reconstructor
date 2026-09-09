param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - APP-WIDE KEYBOARD SHORTCUT SYSTEM" -ForegroundColor Cyan
Write-Host " Global / Viewport / Timeline / Node Graph" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'struct EditorShellState',
    'static void drawViewportView()',
    'static void drawTimeline()',
    'static void drawNodeEditor()',
    'static void drawMainMenuBar()',
    'static void drawInterface()',
    '##GlobalCommandSearch'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected current-repo marker not found: $marker"
    }
}

if ($text.Contains('static void handleGlobalShortcuts()')) {
    Write-Host "[OK] App-wide shortcut system already appears installed." -ForegroundColor Green
    exit 0
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-app-shortcuts-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# HELPERS
# ============================================================

function Get-FunctionBlockInfo {
    param(
        [string]$Source,
        [string]$Signature
    )

    $start = $Source.IndexOf($Signature)

    if ($start -lt 0) {
        throw "Could not find function: $Signature"
    }

    $braceStart = $Source.IndexOf("{", $start)

    if ($braceStart -lt 0) {
        throw "Could not find opening brace for: $Signature"
    }

    $depth = 0
    $inString = $false
    $inChar = $false
    $escape = $false
    $end = -1

    for ($i = $braceStart; $i -lt $Source.Length; $i++) {
        $ch = $Source[$i]

        if ($escape) {
            $escape = $false
            continue
        }

        if ($inString) {
            if ($ch -eq '\') {
                $escape = $true
                continue
            }

            if ($ch -eq '"') {
                $inString = $false
            }

            continue
        }

        if ($inChar) {
            if ($ch -eq '\') {
                $escape = $true
                continue
            }

            if ($ch -eq "'") {
                $inChar = $false
            }

            continue
        }

        if ($ch -eq '"') {
            $inString = $true
            continue
        }

        if ($ch -eq "'") {
            $inChar = $true
            continue
        }

        if ($ch -eq "{") {
            $depth++
            continue
        }

        if ($ch -eq "}") {
            $depth--

            if ($depth -eq 0) {
                $end = $i
                break
            }
        }
    }

    if ($end -lt 0) {
        throw "Could not find closing brace for: $Signature"
    }

    return @{
        Start = $start
        End = $end
        Text = $Source.Substring($start, $end - $start + 1)
    }
}

# ============================================================
# 1. EXTEND EditorShellState
# ============================================================

if (-not $text.Contains('bool showShortcutReference=false;'))
{
    $text = $text.Replace(
        'bool showNodeEditor=true;',
@'
bool showNodeEditor=true;
    bool showShortcutReference=false;
    bool focusCommandSearch=false;
'@
    )
}

if (-not $text.Contains('int shortcutFocusRequest=0;'))
{
    $text = $text.Replace(
        'int selectedEntity=0;',
@'
int selectedEntity=0;
    int shortcutFocusRequest=0;
'@
    )
}

if (-not $text.Contains('char shortcutToast[128]{};'))
{
    $text = $text.Replace(
        'char commandSearch[96]{};',
@'
char commandSearch[96]{};
    char shortcutToast[128]{};
    double shortcutToastUntil=0.0;
'@
    )
}

Write-Host "[OK] Extended editor-shell shortcut state." -ForegroundColor Green

# ============================================================
# 2. FOCUS COMMAND SEARCH ON Ctrl+Shift+P / Ctrl+K
# ============================================================

$searchPattern = '(?s)(ImGui::SetNextItemWidth\(searchW\);\s*)(ImGui::InputTextWithHint\(\s*"##GlobalCommandSearch")'

if ([regex]::IsMatch($text,$searchPattern))
{
    $text = [regex]::Replace(
        $text,
        $searchPattern,
@'
$1if (gEditorShell.focusCommandSearch)
        {
            ImGui::SetKeyboardFocusHere();
            gEditorShell.focusCommandSearch=false;
        }

        $2
'@,
        1
    )

    Write-Host "[OK] Command-search keyboard focus hook installed." -ForegroundColor Green
}
else
{
    throw "Could not locate GlobalCommandSearch input."
}

# ============================================================
# 3. MENU SHORTCUT LABELS / HELP ACTION
# ============================================================

$text = [regex]::Replace(
    $text,
    '"Scene Outliner"\s*,\s*nullptr\s*,\s*&gEditorShell\.showOutliner',
    '"Scene Outliner",`r`n            "Ctrl+Shift+O",`r`n            &gEditorShell.showOutliner',
    1
)

$text = [regex]::Replace(
    $text,
    '"Properties"\s*,\s*nullptr\s*,\s*&gEditorShell\.showProperties',
    '"Properties",`r`n            "Ctrl+Shift+I",`r`n            &gEditorShell.showProperties',
    1
)

$text = [regex]::Replace(
    $text,
    '"Timeline"\s*,\s*nullptr\s*,\s*&gEditorShell\.showTimeline',
    '"Timeline",`r`n            "Ctrl+Shift+T",`r`n            &gEditorShell.showTimeline',
    1
)

$text = [regex]::Replace(
    $text,
    '"Node Editor"\s*,\s*nullptr\s*,\s*&gEditorShell\.showNodeEditor',
    '"Node Editor",`r`n            "Ctrl+Shift+N",`r`n            &gEditorShell.showNodeEditor',
    1
)

$text = $text.Replace(
    'if (ImGui::MenuItem("Reset Workspace Layout"))',
    'if (ImGui::MenuItem("Reset Workspace Layout","Ctrl+Shift+R"))'
)

$text = [regex]::Replace(
    $text,
    '"Snapping"\s*,\s*nullptr\s*,\s*&gEditorShell\.snapEnabled',
    '"Snapping",`r`n            "Shift+Tab",`r`n            &gEditorShell.snapEnabled',
    1
)

$oldHelpShortcut = 'ImGui::MenuItem("Keyboard Shortcuts");'

if ($text.Contains($oldHelpShortcut))
{
    $text = $text.Replace(
        $oldHelpShortcut,
@'
if (ImGui::MenuItem("Keyboard Shortcuts","F1"))
            gEditorShell.showShortcutReference=true;
'@
    )
}
else
{
    throw "Could not locate Help > Keyboard Shortcuts."
}

Write-Host "[OK] Main-menu shortcut labels synchronized." -ForegroundColor Green

# ============================================================
# 4. GLOBAL SHORTCUT ENGINE + REFERENCE WINDOW + TOAST
# ============================================================

$drawInterfacePos = $text.IndexOf('static void drawInterface()')

if ($drawInterfacePos -lt 0) {
    throw "Could not locate drawInterface()."
}

$globalCode = @'
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
        setShortcutToast("Command Search");
    }

    if (shellShortcutPressed(
        ImGuiKey_K,
        true,
        false,
        false))
    {
        gEditorShell.focusCommandSearch=true;
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
        gEditorShell.selectedEntity=0;
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

'@

$text = $text.Insert(
    $drawInterfacePos,
    $globalCode
)

Write-Host "[OK] Global shortcut engine + F1 reference window installed." -ForegroundColor Green

# ============================================================
# 5. CALL GLOBAL HANDLER IN drawInterface()
# ============================================================

$interfaceInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void drawInterface()'

$interface = $interfaceInfo.Text

if (-not $interface.Contains('handleGlobalShortcuts();'))
{
    $interface = $interface.Replace(
        'drawMainMenuBar();',
@'
drawMainMenuBar();
    handleGlobalShortcuts();
'@
    )
}

$nodeDrawAnchor = @'
    if (gEditorShell.showNodeEditor)
        drawNodeEditor();
'@

if ($interface.Contains($nodeDrawAnchor))
{
    $interface = $interface.Replace(
        $nodeDrawAnchor,
@'
    if (gEditorShell.showNodeEditor)
        drawNodeEditor();

    applyShortcutFocusRequest();
    drawShortcutReferenceWindow();
    drawShortcutToast();
'@
    )
}
else
{
    throw "Could not locate Node Editor draw call in drawInterface()."
}

$text = $text.Replace(
    $interfaceInfo.Text,
    $interface
)

Write-Host "[OK] Global shortcut processing wired into frame loop." -ForegroundColor Green

# ============================================================
# 6. VIEWPORT SHORTCUTS
# ============================================================

$viewportInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void drawViewportView()'

$viewport = $viewportInfo.Text

$viewportBegin = 'ImGui::Begin("Viewport");'

if (-not $viewport.Contains($viewportBegin))
{
    throw 'Could not find ImGui::Begin("Viewport");'
}

$viewportShortcuts = @'

    // ========================================================
    // VIEWPORT KEYBOARD SHORTCUTS
    // ========================================================

    {
        const ImGuiIO& io=ImGui::GetIO();

        const bool viewportFocused=
            ImGui::IsWindowFocused(
                ImGuiFocusedFlags_RootAndChildWindows
            );

        if (viewportFocused &&
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

            // Mode switching.
            if (io.KeyAlt &&
                !io.KeyCtrl &&
                !io.KeyShift)
            {
                if (key(ImGuiKey_1))
                    viewportMode=0;

                if (key(ImGuiKey_2))
                    viewportMode=1;

                if (key(ImGuiKey_3))
                    viewportMode=2;
            }

            if (!io.KeyCtrl &&
                !io.KeyAlt &&
                !io.KeyShift)
            {
                // Unreal/Unity-style transform hotkeys.
                if (key(ImGuiKey_Q))
                    selectedTool=0;

                if (key(ImGuiKey_W))
                    selectedTool=1;

                if (key(ImGuiKey_E))
                    selectedTool=2;

                if (key(ImGuiKey_R))
                    selectedTool=3;

                // Common overlays.
                if (key(ImGuiKey_G))
                    showGrid=!showGrid;

                if (key(ImGuiKey_X))
                    showAxes=!showAxes;

                if (key(ImGuiKey_B))
                    showBounds=!showBounds;

                if (key(ImGuiKey_M))
                    showMeasurements=
                        !showMeasurements;

                if (key(ImGuiKey_N))
                    showNames=!showNames;

                if (key(ImGuiKey_Home))
                {
                    viewportZoom=1.0f;

                    if (viewportMode==1)
                    {
                        viewPreset3D=0;
                        cameraFov=60.0f;
                    }
                }

                if (viewportMode==0)
                {
                    if (key(ImGuiKey_1))
                        orthoView=0;

                    if (key(ImGuiKey_2))
                        orthoView=1;

                    if (key(ImGuiKey_3))
                        orthoView=2;
                }
                else if (viewportMode==1)
                {
                    if (key(ImGuiKey_1))
                        viewPreset3D=0;

                    if (key(ImGuiKey_2))
                        viewPreset3D=1;

                    if (key(ImGuiKey_3))
                        viewPreset3D=2;

                    if (key(ImGuiKey_4))
                        viewPreset3D=3;

                    if (key(ImGuiKey_Z))
                    {
                        renderMode=
                            (renderMode+1)%3;
                    }
                }
                else if (viewportMode==2)
                {
                    if (key(ImGuiKey_P))
                    {
                        arEditorPreview=
                            !arEditorPreview;

                        if (arEditorPreview)
                        {
                            arSessionRunning=false;
                            arRecording=false;
                        }
                    }

                    if (key(ImGuiKey_A))
                    {
                        arAnchorCount=
                            std::min(
                                8,
                                arAnchorCount+1
                            );
                    }

                    if (key(ImGuiKey_C))
                    {
                        arAnchorCount=0;
                    }

                    if (key(ImGuiKey_Escape))
                    {
                        arEditorPreview=false;
                        arRecording=false;
                    }
                }
            }

            if (io.KeyShift &&
                !io.KeyCtrl &&
                !io.KeyAlt &&
                key(ImGuiKey_F))
            {
                showSafeFrame=
                    !showSafeFrame;
            }
        }
    }
'@

$viewport = $viewport.Replace(
    $viewportBegin,
    $viewportBegin + $viewportShortcuts
)

$text = $text.Replace(
    $viewportInfo.Text,
    $viewport
)

Write-Host "[OK] Viewport keyboard layer installed." -ForegroundColor Green

# ============================================================
# 7. TIMELINE SHORTCUTS
# ============================================================

$timelineInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void drawTimeline()'

$timeline = $timelineInfo.Text

$timelineBeginMatch = [regex]::Match(
    $timeline,
    'ImGui::Begin\(\s*"Timeline"[\s\S]*?\);'
)

if (-not $timelineBeginMatch.Success)
{
    throw "Could not locate Timeline Begin()."
}

$timelineShortcuts = @'

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
'@

$timeline = $timeline.Insert(
    $timelineBeginMatch.Index +
    $timelineBeginMatch.Length,
    $timelineShortcuts
)

$text = $text.Replace(
    $timelineInfo.Text,
    $timeline
)

Write-Host "[OK] Timeline keyboard layer installed." -ForegroundColor Green

# ============================================================
# 8. COMPLETE NODE EDITOR SHORTCUTS
# ============================================================

$nodeInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void drawNodeEditor()'

$node = $nodeInfo.Text

$ctrlEnterPattern = '(?s)(if \(io\.KeyCtrl &&\s*ImGui::IsKeyPressed\(ImGuiKey_Enter\)\)\s*\{\s*executeGraph\(\);\s*\})'

if (-not [regex]::IsMatch($node,$ctrlEnterPattern))
{
    throw "Could not locate existing Node Editor Ctrl+Enter shortcut."
}

$nodeExtra = @'

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
'@

$node = [regex]::Replace(
    $node,
    $ctrlEnterPattern,
    '$1' + $nodeExtra,
    1
)

$text = $text.Replace(
    $nodeInfo.Text,
    $node
)

Write-Host "[OK] Node Editor shortcut set completed." -ForegroundColor Green

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'static void handleGlobalShortcuts()',
    'static void drawShortcutReferenceWindow()',
    'static void drawShortcutToast()',
    'Ctrl+Shift+O',
    'Ctrl+Shift+I',
    'Ctrl+Shift+T',
    'Ctrl+Shift+N',
    'Viewport keyboard shortcuts',
    'TIMELINE KEYBOARD SHORTCUTS',
    'saveGraph();',
    'loadGraph();',
    'selectedNodes.push_back(',
    'gEditorShell.focusCommandSearch',
    'Keyboard Shortcuts","F1"'
)

foreach ($check in $checks)
{
    if (-not $verify.Contains($check))
    {
        throw "Verification failed after write: $check"
    }
}

# Literal PowerShell escape corruption guard.
if ($verify.Contains('`r`n'))
{
    throw "Verification failed: literal PowerShell newline text entered main.cpp."
}

Write-Host ""
Write-Host "[DONE] App-wide keyboard shortcut system installed." -ForegroundColor Cyan
Write-Host "[OK] Global workspace/panel shortcuts." -ForegroundColor Green
Write-Host "[OK] Unreal/Unity-style Q/W/E/R transform keys." -ForegroundColor Green
Write-Host "[OK] Viewport 2D / 3D / AR keyboard controls." -ForegroundColor Green
Write-Host "[OK] Timeline transport/navigation shortcuts." -ForegroundColor Green
Write-Host "[OK] Node Editor shortcut set completed." -ForegroundColor Green
Write-Host "[OK] F1 shortcut-reference window." -ForegroundColor Green
Write-Host "[OK] Ctrl+Shift+P / Ctrl+K focuses command search." -ForegroundColor Green
Write-Host "[OK] Shortcut toast feedback." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
