param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - COMPACT AR TOOLBAR" -ForegroundColor Cyan
Write-Host " 4 AR rows -> 2 AR rows" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if (-not $text.Contains('static void drawViewportView()')) {
    throw "Could not find drawViewportView()."
}

if (-not $text.Contains('"ViewportModeStrip"')) {
    throw "Could not find ViewportModeStrip."
}

if (-not $text.Contains('"ViewportSecondaryToolbar"')) {
    throw "Could not find ViewportSecondaryToolbar."
}

if (-not $text.Contains('"Viewport3DARProStrip"')) {
    throw "Could not find Viewport3DARProStrip."
}

if (-not $text.Contains('"ViewportARSessionBar"')) {
    throw "Could not find ViewportARSessionBar."
}

if ($text.Contains('"ViewportARCompactTools"')) {
    Write-Host "[OK] Compact AR toolbar already installed." -ForegroundColor Green
    exit 0
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-compact-ar-toolbar-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

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

    return $Source.Substring(
        $start,
        $end - $start + 1
    )
}

function Get-ChildBlock {
    param(
        [string]$FunctionText,
        [string]$ChildId
    )

    $marker = '"' + $ChildId + '"'
    $markerIndex = $FunctionText.IndexOf($marker)

    if ($markerIndex -lt 0) {
        throw "Could not find child id: $ChildId"
    }

    $begin = $FunctionText.LastIndexOf(
        'ImGui::BeginChild(',
        $markerIndex
    )

    if ($begin -lt 0) {
        throw "Could not find BeginChild for: $ChildId"
    }

    $endMarker = 'ImGui::EndChild();'
    $end = $FunctionText.IndexOf(
        $endMarker,
        $markerIndex
    )

    if ($end -lt 0) {
        throw "Could not find EndChild for: $ChildId"
    }

    $end += $endMarker.Length

    return @{
        Start = $begin
        End = $end
        Text = $FunctionText.Substring(
            $begin,
            $end - $begin
        )
    }
}

$func = Get-FunctionBlock `
    -Source $text `
    -Signature 'static void drawViewportView()'

# ============================================================
# 1. REPLACE TOP MODE STRIP
# ============================================================

$modeBlock = Get-ChildBlock `
    -FunctionText $func `
    -ChildId 'ViewportModeStrip'

$newModeStrip = @'
ImGui::BeginChild(
        "ViewportModeStrip",
        ImVec2(0.0f,52.0f),
        true,
        ImGuiWindowFlags_NoScrollbar
    );

    if (editorButton(
        "2D PLAN",
        96.0f,
        viewportMode==0,
        true))
    {
        viewportMode=0;
    }

    ImGui::SameLine(0.0f,6.0f);

    if (editorButton(
        "3D SCENE",
        104.0f,
        viewportMode==1,
        true))
    {
        viewportMode=1;
    }

    ImGui::SameLine(0.0f,6.0f);

    if (editorButton(
        "AR PREVIEW",
        112.0f,
        viewportMode==2,
        true))
    {
        viewportMode=2;
    }

    if (viewportMode==2)
    {
        ImGui::SameLine(0.0f,18.0f);

        if (arEditorPreview)
        {
            ImGui::TextColored(
                ImVec4(0.92f,0.69f,0.18f,1.0f),
                "● EDITOR PREVIEW"
            );
        }
        else if (arDeviceConnected && arSessionRunning)
        {
            ImGui::TextColored(
                ImVec4(0.48f,0.84f,0.46f,1.0f),
                "● LIVE"
            );
        }
        else
        {
            ImGui::TextDisabled("● OFFLINE");
        }

        ImGui::SameLine(0.0f,14.0f);

        if (editorButton(
            arEditorPreview
                ? "EXIT PREVIEW"
                : "EDITOR PREVIEW",
            arEditorPreview
                ? 110.0f
                : 126.0f,
            arEditorPreview,
            true))
        {
            arEditorPreview=!arEditorPreview;

            if (arEditorPreview)
            {
                arSessionRunning=false;
                arRecording=false;
            }
            else
            {
                arRecording=false;
            }
        }

        ImGui::SameLine(0.0f,6.0f);

        if (editorButton(
            "CONNECT DEVICE",
            126.0f,
            false,
            true))
        {
            ImGui::OpenPopup(
                "##CompactARDevicePopup"
            );
        }

        if (ImGui::BeginPopup(
            "##CompactARDevicePopup"))
        {
            ImGui::Text("AR DEVICE");
            ImGui::Separator();

            ImGui::TextDisabled(
                "Live device transport is not wired yet."
            );

            const char* profiles[]={
                "Android Companion",
                "iPhone / iPad",
                "External AR Camera"
            };

            ImGui::SetNextItemWidth(220.0f);

            ImGui::Combo(
                "Device profile",
                &arDeviceProfile,
                profiles,
                3
            );

            ImGui::Spacing();
            ImGui::Separator();

            if (ImGui::MenuItem(
                "Use Editor Preview"))
            {
                arEditorPreview=true;
                arDeviceConnected=false;
                arSessionRunning=false;
                arRecording=false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine(0.0f,14.0f);
        ImGui::TextDisabled("OPACITY");
        ImGui::SameLine(0.0f,6.0f);

        ImGui::SetNextItemWidth(108.0f);

        ImGui::SliderFloat(
            "##AROpacityCompact",
            &arOpacity,
            0.10f,
            1.00f,
            "%.2f"
        );
    }
    else if (viewportMode==1)
    {
        ImGui::SameLine(0.0f,18.0f);
        ImGui::TextDisabled("DISPLAY");
        ImGui::SameLine(0.0f,6.0f);

        const char* renderModes[]={
            "Lit",
            "Wireframe",
            "Analysis"
        };

        ImGui::SetNextItemWidth(118.0f);

        ImGui::Combo(
            "##CompactRenderMode",
            &renderMode,
            renderModes,
            3
        );
    }
    else
    {
        ImGui::SameLine(0.0f,18.0f);
        ImGui::TextDisabled("VIEW");
        ImGui::SameLine(0.0f,6.0f);

        const char* orthoModes[]={
            "Top",
            "Front",
            "Right"
        };

        ImGui::SetNextItemWidth(112.0f);

        ImGui::Combo(
            "##CompactOrthoMode",
            &orthoView,
            orthoModes,
            3
        );
    }

    ImGui::EndChild();
'@

$func = $func.Remove(
    $modeBlock.Start,
    $modeBlock.End - $modeBlock.Start
).Insert(
    $modeBlock.Start,
    $newModeStrip
)

Write-Host "[OK] Rebuilt compact top mode/session row." -ForegroundColor Green

# ============================================================
# 2. ADD COMPACT AR TOOLS ROW AFTER MODE STRIP
# ============================================================

$modeBlock2 = Get-ChildBlock `
    -FunctionText $func `
    -ChildId 'ViewportModeStrip'

$compactTools = @'

    if (viewportMode==2)
    {
        ImGui::Spacing();

        ImGui::BeginChild(
            "ViewportARCompactTools",
            ImVec2(0.0f,52.0f),
            true,
            ImGuiWindowFlags_NoScrollbar
        );

        ImGui::TextDisabled("PLACEMENT");
        ImGui::SameLine(0.0f,7.0f);

        const char* placementModes[]={
            "Origin",
            "Surface",
            "Vehicle"
        };

        ImGui::SetNextItemWidth(112.0f);

        ImGui::Combo(
            "##CompactARPlacement",
            &arPlacementMode,
            placementModes,
            3
        );

        ImGui::SameLine(0.0f,14.0f);
        ImGui::TextDisabled("TRACKING");
        ImGui::SameLine(0.0f,7.0f);

        const char* trackingModes[]={
            "Poor",
            "Limited",
            "Good"
        };

        ImGui::SetNextItemWidth(104.0f);

        ImGui::Combo(
            "##CompactARTracking",
            &arTrackingQuality,
            trackingModes,
            3
        );

        ImGui::SameLine(0.0f,12.0f);

        ImGui::Checkbox(
            "Occlusion",
            &arOcclusion
        );

        ImGui::SameLine(0.0f,9.0f);

        ImGui::Checkbox(
            "Planes",
            &arPlaneMesh
        );

        ImGui::SameLine(0.0f,9.0f);

        ImGui::Checkbox(
            "Reticle",
            &arReticle
        );

        ImGui::SameLine(0.0f,12.0f);

        const bool canPlaceAnchor=
            arEditorPreview ||
            (arDeviceConnected &&
             arSessionRunning);

        if (editorButton(
            "PLACE ANCHOR",
            118.0f,
            true,
            canPlaceAnchor))
        {
            arAnchorCount=
                std::min(
                    8,
                    arAnchorCount+1
                );
        }

        ImGui::SameLine(0.0f,7.0f);

        if (editorButton(
            "MORE  v",
            86.0f,
            false,
            true))
        {
            ImGui::OpenPopup(
                "##CompactARMorePopup"
            );
        }

        if (ImGui::BeginPopup(
            "##CompactARMorePopup"))
        {
            ImGui::TextDisabled(
                "AR SESSION"
            );

            ImGui::Separator();

            if (ImGui::MenuItem(
                "Reset Origin"))
            {
                arPlacementMode=0;
                arAnchorCount=0;
            }

            if (ImGui::MenuItem(
                "Clear Anchors",
                nullptr,
                false,
                arAnchorCount>0))
            {
                arAnchorCount=0;
            }

            ImGui::Separator();

            if (arDeviceConnected)
            {
                if (!arSessionRunning)
                {
                    if (ImGui::MenuItem(
                        "Start Session"))
                    {
                        arSessionRunning=true;
                        arEditorPreview=false;
                    }
                }
                else
                {
                    if (ImGui::MenuItem(
                        "Stop Session"))
                    {
                        arSessionRunning=false;
                        arRecording=false;
                    }
                }
            }
            else
            {
                ImGui::MenuItem(
                    "Start Session",
                    nullptr,
                    false,
                    false
                );
            }

            const bool canRecord=
                arEditorPreview ||
                (arDeviceConnected &&
                 arSessionRunning);

            if (ImGui::MenuItem(
                arRecording
                    ? "Stop Recording"
                    : "Record",
                nullptr,
                false,
                canRecord))
            {
                arRecording=!arRecording;
            }

            ImGui::Separator();

            if (ImGui::BeginMenu(
                "Overlays"))
            {
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

                ImGui::EndMenu();
            }

            ImGui::Separator();

            ImGui::TextDisabled(
                "Shortcuts: P Preview   A Anchor   C Clear"
            );

            ImGui::EndPopup();
        }

        ImGui::EndChild();
    }
'@

$insertAt = $modeBlock2.End

$func = $func.Insert(
    $insertAt,
    $compactTools
)

Write-Host "[OK] Added compact AR tools row." -ForegroundColor Green

# ============================================================
# 3. HIDE OLD SECONDARY TOOLBAR IN AR MODE
# ============================================================

$secondary = Get-ChildBlock `
    -FunctionText $func `
    -ChildId 'ViewportSecondaryToolbar'

$wrappedSecondary = @'
if (viewportMode!=2)
    {
'@ + $secondary.Text + @'

    }
'@

$func = $func.Remove(
    $secondary.Start,
    $secondary.End - $secondary.Start
).Insert(
    $secondary.Start,
    $wrappedSecondary
)

Write-Host "[OK] Old secondary toolbar hidden in AR mode." -ForegroundColor Green

# ============================================================
# 4. HIDE OLD 3D/AR PRO STRIP IN AR MODE
#    3D keeps its existing pro strip.
# ============================================================

$pro = Get-ChildBlock `
    -FunctionText $func `
    -ChildId 'Viewport3DARProStrip'

$wrappedPro = @'
if (viewportMode!=2)
    {
'@ + $pro.Text + @'

    }
'@

$func = $func.Remove(
    $pro.Start,
    $pro.End - $pro.Start
).Insert(
    $pro.Start,
    $wrappedPro
)

Write-Host "[OK] Old AR pro strip hidden; 3D pro strip preserved." -ForegroundColor Green

# ============================================================
# 5. REMOVE OLD AR SESSION BAR
# ============================================================

$session = Get-ChildBlock `
    -FunctionText $func `
    -ChildId 'ViewportARSessionBar'

$replacement = @'
// Compact AR toolbar owns session/device/record controls.
'@

$func = $func.Remove(
    $session.Start,
    $session.End - $session.Start
).Insert(
    $session.Start,
    $replacement
)

Write-Host "[OK] Removed duplicate AR session row." -ForegroundColor Green

# ============================================================
# 6. WRITE FUNCTION BACK
# ============================================================

$oldFunc = Get-FunctionBlock `
    -Source $text `
    -Signature 'static void drawViewportView()'

$text = $text.Replace(
    $oldFunc,
    $func
)

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

# ============================================================
# VERIFY
# ============================================================

$verify = Get-Content $MainCpp -Raw

$checks = @(
    '"ViewportARCompactTools"',
    '"##CompactARMorePopup"',
    '"##CompactARDevicePopup"',
    '"##AROpacityCompact"',
    '"MORE  v"',
    'if (viewportMode!=2)',
    'Compact AR toolbar owns session/device/record controls.'
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

if ($verify.Contains('`r`n')) {
    throw "Verification failed: literal PowerShell newline text entered main.cpp."
}

Write-Host ""
Write-Host "[DONE] AR toolbar compacted." -ForegroundColor Cyan
Write-Host "[OK] AR now uses 2 top rows instead of 4." -ForegroundColor Green
Write-Host "[OK] Duplicate session controls removed." -ForegroundColor Green
Write-Host "[OK] Device / preview / opacity moved to row 1." -ForegroundColor Green
Write-Host "[OK] Placement / tracking / overlays moved to row 2." -ForegroundColor Green
Write-Host "[OK] Reset / clear / session / record moved under MORE." -ForegroundColor Green
Write-Host "[OK] Existing 2D and 3D toolbar behavior preserved." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
