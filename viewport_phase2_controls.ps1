param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - VIEWPORT PHASE 2" -ForegroundColor Cyan
Write-Host " Camera / Overlays / HUD / Context Controls" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

function Get-FunctionBlock {
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
            if ($ch -eq '\') { $escape = $true; continue }
            if ($ch -eq '"') { $inString = $false }
            continue
        }

        if ($inChar) {
            if ($ch -eq '\') { $escape = $true; continue }
            if ($ch -eq "'") { $inChar = $false }
            continue
        }

        if ($ch -eq '"') { $inString = $true; continue }
        if ($ch -eq "'") { $inChar = $true; continue }

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

$info = Get-FunctionBlock `
    -Source $text `
    -Signature "static void drawViewportView()"

$func = $info.Text

$required = @(
    'ViewportModeStrip',
    'SceneCanvas',
    'static int viewportMode=0;',
    'static int orthoView=0;',
    'static int renderMode=0;'
)

foreach ($marker in $required) {
    if (-not $func.Contains($marker)) {
        throw "Expected Phase-1 viewport marker not found: $marker"
    }
}

if ($func.Contains("ViewportSecondaryToolbar")) {
    Write-Host "[OK] Viewport Phase 2 already appears installed." -ForegroundColor Green
    exit 0
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-viewport-phase2-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. ADD PHASE-2 STATE AFTER arOpacity
# ============================================================

$stateMarker = 'static float arOpacity=0.78f;'

if (-not $func.Contains($stateMarker)) {
    throw "Could not find arOpacity state marker."
}

$extraState = @'

    static int cameraSpeed=2;
    static bool showBounds=false;
    static bool showMeasurements=true;
    static bool showSafeFrame=false;
    static bool showNames=true;
    static bool showStats=true;
    static bool arShowAnchors=true;
    static bool arShowCollisionGuide=true;
    static bool arRecord=false;
    static float viewportZoom=1.0f;
'@

$func = $func.Replace(
    $stateMarker,
    $stateMarker + $extraState
)

Write-Host "[OK] Added viewport Phase-2 state." -ForegroundColor Green

# ============================================================
# 2. INSERT SECONDARY TOOLBAR AFTER MODE STRIP ENDS
# ============================================================

$stripPos = $func.IndexOf('"ViewportModeStrip"')
if ($stripPos -lt 0) {
    throw "Could not find ViewportModeStrip."
}

$stripEnd = $func.IndexOf('ImGui::EndChild();', $stripPos)
if ($stripEnd -lt 0) {
    throw "Could not find ViewportModeStrip EndChild()."
}

$stripEnd += 'ImGui::EndChild();'.Length

$secondaryToolbar = @'

    ImGui::Spacing();

    ImGui::BeginChild(
        "ViewportSecondaryToolbar",
        ImVec2(0.0f,48.0f),
        true,
        ImGuiWindowFlags_NoScrollbar
    );

    // --------------------------------------------------------
    // MODE-SPECIFIC VIEW CONTROLS
    // --------------------------------------------------------

    if (viewportMode==0)
    {
        ImGui::TextDisabled("2D");

        ImGui::SameLine(0.0f,10.0f);

        if (ImGui::Button("FIT",ImVec2(52.0f,28.0f)))
            viewportZoom=1.0f;

        ImGui::SameLine(0.0f,6.0f);

        if (ImGui::Button("-",ImVec2(30.0f,28.0f)))
            viewportZoom=std::max(0.25f,viewportZoom-0.10f);

        ImGui::SameLine(0.0f,4.0f);

        ImGui::SetNextItemWidth(90.0f);
        ImGui::SliderFloat(
            "##ViewportZoom2D",
            &viewportZoom,
            0.25f,
            3.0f,
            "%.2fx"
        );

        ImGui::SameLine(0.0f,4.0f);

        if (ImGui::Button("+",ImVec2(30.0f,28.0f)))
            viewportZoom=std::min(3.0f,viewportZoom+0.10f);

        ImGui::SameLine(0.0f,12.0f);
        editorButton("FRAME ALL",92.0f);
    }
    else if (viewportMode==1)
    {
        ImGui::TextDisabled("CAMERA");

        ImGui::SameLine(0.0f,10.0f);

        const char* speedLabels[]={
            "Very Slow",
            "Slow",
            "Normal",
            "Fast",
            "Very Fast"
        };

        ImGui::SetNextItemWidth(112.0f);
        ImGui::Combo(
            "##ViewportCameraSpeed",
            &cameraSpeed,
            speedLabels,
            5
        );

        ImGui::SameLine(0.0f,12.0f);
        editorButton("FRAME SELECT",116.0f);

        ImGui::SameLine(0.0f,6.0f);
        editorButton("FRAME ALL",92.0f);
    }
    else
    {
        ImGui::TextDisabled("AR SESSION");

        ImGui::SameLine(0.0f,10.0f);

        editorButton(
            "PAIR DEVICE",
            100.0f,
            false,
            false
        );

        ImGui::SameLine(0.0f,6.0f);

        if (arRecord)
            editorButton("STOP RECORD",108.0f,true);
        else
            editorButton("RECORD",82.0f,false,false);

        ImGui::SameLine(0.0f,12.0f);

        if (ImGui::Button("RESET ORIGIN",ImVec2(112.0f,28.0f)))
        {
            // AR origin reset hook.
        }
    }

    // --------------------------------------------------------
    // OVERLAYS MENU
    // --------------------------------------------------------

    ImGui::SameLine(0.0f,14.0f);

    if (ImGui::Button("OVERLAYS  v",ImVec2(106.0f,28.0f)))
        ImGui::OpenPopup("##ViewportOverlaysPopup");

    if (ImGui::BeginPopup("##ViewportOverlaysPopup"))
    {
        ImGui::TextDisabled("VIEWPORT OVERLAYS");
        ImGui::Separator();

        ImGui::MenuItem(
            "Grid",
            nullptr,
            &showGrid
        );

        ImGui::MenuItem(
            "Axes",
            nullptr,
            &showAxes
        );

        ImGui::MenuItem(
            "Object Bounds",
            nullptr,
            &showBounds
        );

        ImGui::MenuItem(
            "Measurements",
            nullptr,
            &showMeasurements
        );

        ImGui::MenuItem(
            "Object Names",
            nullptr,
            &showNames
        );

        ImGui::MenuItem(
            "Statistics",
            nullptr,
            &showStats
        );

        ImGui::MenuItem(
            "Safe Frame",
            nullptr,
            &showSafeFrame
        );

        if (viewportMode==2)
        {
            ImGui::Separator();

            ImGui::MenuItem(
                "AR Anchors",
                nullptr,
                &arShowAnchors
            );

            ImGui::MenuItem(
                "Collision Guide",
                nullptr,
                &arShowCollisionGuide
            );
        }

        ImGui::EndPopup();
    }

    ImGui::EndChild();
'@

$func = $func.Insert(
    $stripEnd,
    $secondaryToolbar
)

Write-Host "[OK] Added secondary camera/overlay toolbar." -ForegroundColor Green

# ============================================================
# 3. FIND LIVE SceneCanvas AND INSERT HUD/CONTEXT BEFORE END
# ============================================================

$scenePos = $func.IndexOf('"SceneCanvas"')
if ($scenePos -lt 0) {
    throw "Could not find SceneCanvas."
}

$sceneEnd = $func.IndexOf('ImGui::EndChild();', $scenePos)
if ($sceneEnd -lt 0) {
    throw "Could not find SceneCanvas EndChild()."
}

$hudAndContext = @'

    // ========================================================
    // VIEWPORT HUD / NAVIGATION GIZMO
    // ========================================================

    if (showStats)
    {
        const ImVec2 hudMin(
            cp.x+cs.x-168.0f,
            cp.y+14.0f
        );

        const ImVec2 hudMax(
            cp.x+cs.x-14.0f,
            cp.y+78.0f
        );

        d->AddRectFilled(
            hudMin,
            hudMax,
            IM_COL32(22,25,29,220),
            5.0f
        );

        d->AddRect(
            hudMin,
            hudMax,
            IM_COL32(72,77,84,230),
            5.0f,
            0,
            1.0f
        );

        d->AddText(
            ImVec2(hudMin.x+10.0f,hudMin.y+9.0f),
            IM_COL32(215,219,225,255),
            viewportMode==0
                ? "2D PLAN"
                : (viewportMode==1
                    ? "3D SCENE"
                    : "AR PREVIEW")
        );

        d->AddText(
            ImVec2(hudMin.x+10.0f,hudMin.y+31.0f),
            IM_COL32(145,151,160,255),
            "Selection: None"
        );

        d->AddText(
            ImVec2(hudMin.x+10.0f,hudMin.y+49.0f),
            IM_COL32(145,151,160,255),
            "Objects: 0"
        );
    }

    if (showSafeFrame)
    {
        d->AddRect(
            ImVec2(cp.x+cs.x*.08f,cp.y+cs.y*.08f),
            ImVec2(cp.x+cs.x*.92f,cp.y+cs.y*.92f),
            IM_COL32(180,185,192,95),
            0.0f,
            0,
            1.0f
        );
    }

    if (showBounds && viewportMode!=2)
    {
        d->AddRect(
            ImVec2(center.x-104.0f,center.y-82.0f),
            ImVec2(center.x+104.0f,center.y+82.0f),
            IM_COL32(238,174,38,170),
            3.0f,
            0,
            1.5f
        );
    }

    if (showNames && viewportMode!=2)
    {
        d->AddText(
            ImVec2(center.x-36.0f,center.y+92.0f),
            IM_COL32(196,200,206,210),
            "Scene Origin"
        );
    }

    if (showMeasurements && viewportMode==0)
    {
        d->AddLine(
            ImVec2(center.x-118.0f,center.y+78.0f),
            ImVec2(center.x+118.0f,center.y+78.0f),
            IM_COL32(198,202,208,180),
            1.4f
        );

        d->AddText(
            ImVec2(center.x-19.0f,center.y+60.0f),
            IM_COL32(220,223,228,225),
            "4.8 m"
        );
    }

    if (viewportMode==1)
    {
        // Small orientation cube in upper-right, below HUD.
        const ImVec2 cubeCenter(
            cp.x+cs.x-72.0f,
            cp.y+126.0f
        );

        const float q=22.0f;

        d->AddRectFilled(
            ImVec2(cubeCenter.x-q,cubeCenter.y-q),
            ImVec2(cubeCenter.x+q,cubeCenter.y+q),
            IM_COL32(46,51,58,235),
            3.0f
        );

        d->AddRect(
            ImVec2(cubeCenter.x-q,cubeCenter.y-q),
            ImVec2(cubeCenter.x+q,cubeCenter.y+q),
            IM_COL32(125,132,142,235),
            3.0f,
            0,
            1.2f
        );

        d->AddText(
            ImVec2(cubeCenter.x-5.0f,cubeCenter.y-9.0f),
            IM_COL32(224,226,230,255),
            "F"
        );

        d->AddText(
            ImVec2(cubeCenter.x-4.0f,cubeCenter.y-q-18.0f),
            IM_COL32(112,220,128,255),
            "Y"
        );

        d->AddText(
            ImVec2(cubeCenter.x+q+7.0f,cubeCenter.y-8.0f),
            IM_COL32(220,110,90,255),
            "X"
        );
    }

    if (viewportMode==2 && arShowAnchors)
    {
        d->AddText(
            ImVec2(cp.x+18.0f,cp.y+cs.y-52.0f),
            IM_COL32(112,205,232,220),
            "AR Anchors: Visible"
        );
    }

    if (viewportMode==2 && arShowCollisionGuide)
    {
        d->AddRect(
            ImVec2(center.x-160.0f,center.y+116.0f),
            ImVec2(center.x+160.0f,center.y+154.0f),
            IM_COL32(238,174,38,185),
            4.0f,
            0,
            1.4f
        );

        d->AddText(
            ImVec2(center.x-107.0f,center.y+126.0f),
            IM_COL32(240,194,100,225),
            "Estimated collision corridor"
        );
    }

    // ========================================================
    // VIEWPORT CONTEXT MENU
    // ========================================================

    if (ImGui::BeginPopupContextWindow(
        "##ViewportContextMenu",
        ImGuiPopupFlags_MouseButtonRight |
        ImGuiPopupFlags_NoOpenOverItems))
    {
        ImGui::TextDisabled("VIEWPORT");
        ImGui::Separator();

        ImGui::MenuItem(
            "Frame Selection",
            "F"
        );

        ImGui::MenuItem(
            "Frame All",
            "Home"
        );

        ImGui::Separator();

        ImGui::MenuItem(
            "Grid",
            nullptr,
            &showGrid
        );

        ImGui::MenuItem(
            "Axes",
            nullptr,
            &showAxes
        );

        ImGui::MenuItem(
            "Bounds",
            nullptr,
            &showBounds
        );

        ImGui::MenuItem(
            "Measurements",
            nullptr,
            &showMeasurements
        );

        ImGui::Separator();

        if (viewportMode==0)
        {
            if (ImGui::MenuItem("Top"))
                orthoView=0;

            if (ImGui::MenuItem("Front"))
                orthoView=1;

            if (ImGui::MenuItem("Right"))
                orthoView=2;
        }
        else if (viewportMode==1)
        {
            if (ImGui::MenuItem("Lit"))
                renderMode=0;

            if (ImGui::MenuItem("Wireframe"))
                renderMode=1;

            if (ImGui::MenuItem("Analysis"))
                renderMode=2;
        }
        else
        {
            ImGui::MenuItem(
                "AR Anchors",
                nullptr,
                &arShowAnchors
            );

            ImGui::MenuItem(
                "Collision Guide",
                nullptr,
                &arShowCollisionGuide
            );
        }

        ImGui::EndPopup();
    }

'@

$func = $func.Insert(
    $sceneEnd,
    $hudAndContext
)

Write-Host "[OK] Added viewport HUD, gizmo and context menu." -ForegroundColor Green

# ============================================================
# 4. WRITE BACK
# ============================================================

$text = $text.Replace(
    $info.Text,
    $func
)

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

# ============================================================
# VERIFY
# ============================================================

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'ViewportSecondaryToolbar',
    '##ViewportOverlaysPopup',
    '##ViewportContextMenu',
    'FRAME SELECT',
    'FRAME ALL',
    'cameraSpeed',
    'showSafeFrame',
    'Selection: None',
    'Estimated collision corridor'
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] Viewport Phase 2 installed." -ForegroundColor Cyan
Write-Host "[OK] Added mode-specific secondary toolbar." -ForegroundColor Green
Write-Host "[OK] Added camera speed / zoom / frame controls." -ForegroundColor Green
Write-Host "[OK] Added Overlays popup." -ForegroundColor Green
Write-Host "[OK] Added viewport HUD." -ForegroundColor Green
Write-Host "[OK] Added orientation cube in 3D." -ForegroundColor Green
Write-Host "[OK] Added right-click viewport menu." -ForegroundColor Green
Write-Host "[OK] Added AR anchor/collision overlay controls." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
