param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - 3D + AR VIEWPORT PROFESSIONAL PASS" -ForegroundColor Cyan
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
    'ViewportSecondaryToolbar',
    'SceneCanvas',
    '##ViewportContextMenu',
    'static float viewportZoom=1.0f;'
)

foreach ($marker in $required) {
    if (-not $func.Contains($marker)) {
        throw "Expected viewport marker not found: $marker"
    }
}

if ($func.Contains("Viewport3DARProStrip")) {
    Write-Host "[OK] 3D/AR professional pass already installed." -ForegroundColor Green
    exit 0
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-3d-ar-pro-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. ADD 3D + AR STATE
# ============================================================

$stateMarker = 'static float viewportZoom=1.0f;'

$extraState = @'

    // 3D professional controls.
    static int viewPreset3D=0;       // Perspective / Top / Front / Right
    static int gizmoSpace3D=0;       // World / Local
    static float cameraFov=60.0f;
    static bool showGroundShadow=true;
    static bool showNavigationHints=true;

    // AR professional controls.
    static int arPlacementMode=0;    // Origin / Surface / Vehicle
    static int arTrackingQuality=2;  // 0 poor, 1 limited, 2 good
    static bool arOcclusion=true;
    static bool arPlaneMesh=true;
    static bool arReticle=true;
    static int arAnchorCount=0;
'@

$func = $func.Replace(
    $stateMarker,
    $stateMarker + $extraState
)

Write-Host "[OK] Added 3D + AR professional state." -ForegroundColor Green

# ============================================================
# 2. INSERT PROFESSIONAL MODE STRIP AFTER SECONDARY TOOLBAR
# ============================================================

$toolbarPos = $func.IndexOf('"ViewportSecondaryToolbar"')
if ($toolbarPos -lt 0) {
    throw "Could not find ViewportSecondaryToolbar."
}

$toolbarEnd = $func.IndexOf('ImGui::EndChild();', $toolbarPos)
if ($toolbarEnd -lt 0) {
    throw "Could not find ViewportSecondaryToolbar EndChild()."
}

$toolbarEnd += 'ImGui::EndChild();'.Length

$proStrip = @'

    if (viewportMode==1 || viewportMode==2)
    {
        ImGui::Spacing();

        ImGui::BeginChild(
            "Viewport3DARProStrip",
            ImVec2(0.0f,52.0f),
            true,
            ImGuiWindowFlags_NoScrollbar
        );

        if (viewportMode==1)
        {
            const char* viewPresets[]={
                "Perspective",
                "Top",
                "Front",
                "Right"
            };

            const char* gizmoSpaces[]={
                "World",
                "Local"
            };

            ImGui::TextDisabled("VIEW");
            ImGui::SameLine(0.0f,8.0f);

            ImGui::SetNextItemWidth(122.0f);
            ImGui::Combo(
                "##Viewport3DViewPreset",
                &viewPreset3D,
                viewPresets,
                4
            );

            ImGui::SameLine(0.0f,12.0f);
            ImGui::TextDisabled("FOV");
            ImGui::SameLine(0.0f,7.0f);

            ImGui::SetNextItemWidth(108.0f);
            ImGui::SliderFloat(
                "##Viewport3DFov",
                &cameraFov,
                25.0f,
                110.0f,
                "%.0f deg"
            );

            ImGui::SameLine(0.0f,12.0f);
            ImGui::TextDisabled("GIZMO");
            ImGui::SameLine(0.0f,7.0f);

            ImGui::SetNextItemWidth(92.0f);
            ImGui::Combo(
                "##Viewport3DGizmoSpace",
                &gizmoSpace3D,
                gizmoSpaces,
                2
            );

            ImGui::SameLine(0.0f,14.0f);
            ImGui::Checkbox(
                "Shadow",
                &showGroundShadow
            );

            ImGui::SameLine(0.0f,10.0f);
            ImGui::Checkbox(
                "Nav Hints",
                &showNavigationHints
            );
        }
        else
        {
            const char* placementModes[]={
                "Origin",
                "Surface",
                "Vehicle"
            };

            const char* trackingStates[]={
                "Poor",
                "Limited",
                "Good"
            };

            ImGui::TextDisabled("PLACEMENT");
            ImGui::SameLine(0.0f,8.0f);

            ImGui::SetNextItemWidth(110.0f);
            ImGui::Combo(
                "##ARPlacementMode",
                &arPlacementMode,
                placementModes,
                3
            );

            ImGui::SameLine(0.0f,12.0f);
            ImGui::TextDisabled("TRACKING");
            ImGui::SameLine(0.0f,7.0f);

            ImGui::SetNextItemWidth(96.0f);
            ImGui::Combo(
                "##ARTrackingQuality",
                &arTrackingQuality,
                trackingStates,
                3
            );

            ImGui::SameLine(0.0f,12.0f);
            ImGui::Checkbox(
                "Occlusion",
                &arOcclusion
            );

            ImGui::SameLine(0.0f,10.0f);
            ImGui::Checkbox(
                "Planes",
                &arPlaneMesh
            );

            ImGui::SameLine(0.0f,10.0f);
            ImGui::Checkbox(
                "Reticle",
                &arReticle
            );

            ImGui::SameLine(0.0f,12.0f);

            if (editorButton(
                "PLACE ANCHOR",
                118.0f,
                true,
                true))
            {
                arAnchorCount=
                    std::min(
                        8,
                        arAnchorCount+1
                    );
            }

            ImGui::SameLine(0.0f,6.0f);

            if (editorButton(
                "CLEAR",
                64.0f,
                false,
                arAnchorCount>0))
            {
                arAnchorCount=0;
            }
        }

        ImGui::EndChild();
    }
'@

$func = $func.Insert(
    $toolbarEnd,
    $proStrip
)

Write-Host "[OK] Added dedicated 3D / AR control strip." -ForegroundColor Green

# ============================================================
# 3. INSERT PROFESSIONAL 3D + AR VISUALS BEFORE CONTEXT MENU
# ============================================================

$contextMarker = '// VIEWPORT CONTEXT MENU'
$contextPos = $func.IndexOf($contextMarker)

if ($contextPos -lt 0) {
    # fallback to actual popup call
    $contextPos = $func.IndexOf('if (ImGui::BeginPopupContextWindow(')
}

if ($contextPos -lt 0) {
    throw "Could not find viewport context-menu insertion point."
}

$visuals = @'

    // ========================================================
    // VIEWPORT PHASE 3 - PROFESSIONAL 3D / AR VISUAL LAYER
    // ========================================================

    if (viewportMode==1)
    {
        // ----------------------------------------------------
        // 3D: ground contact shadow
        // ----------------------------------------------------

        if (showGroundShadow)
        {
            d->AddEllipseFilled(
                ImVec2(
                    center.x-22.0f,
                    center.y+116.0f
                ),
                ImVec2(112.0f,22.0f),
                IM_COL32(0,0,0,70)
            );
        }

        // ----------------------------------------------------
        // 3D: more recognisable reconstruction vehicle
        // ----------------------------------------------------

        const ImVec2 carCenter(
            center.x-24.0f,
            center.y+66.0f
        );

        const ImVec2 bodyMin(
            carCenter.x-72.0f,
            carCenter.y-18.0f
        );

        const ImVec2 bodyMax(
            carCenter.x+72.0f,
            carCenter.y+30.0f
        );

        d->AddRectFilled(
            bodyMin,
            bodyMax,
            renderMode==2
                ? IM_COL32(112,126,150,105)
                : IM_COL32(94,116,152,145),
            5.0f
        );

        d->AddRect(
            bodyMin,
            bodyMax,
            IM_COL32(176,199,226,220),
            5.0f,
            0,
            1.5f
        );

        const ImVec2 roofA(
            carCenter.x-38.0f,
            carCenter.y-18.0f
        );

        const ImVec2 roofB(
            carCenter.x-18.0f,
            carCenter.y-50.0f
        );

        const ImVec2 roofC(
            carCenter.x+34.0f,
            carCenter.y-50.0f
        );

        const ImVec2 roofD(
            carCenter.x+52.0f,
            carCenter.y-18.0f
        );

        d->AddQuadFilled(
            roofA,
            roofB,
            roofC,
            roofD,
            IM_COL32(82,102,135,145)
        );

        d->AddQuad(
            roofA,
            roofB,
            roofC,
            roofD,
            IM_COL32(178,201,228,215),
            1.4f
        );

        d->AddCircleFilled(
            ImVec2(
                carCenter.x-46.0f,
                carCenter.y+31.0f
            ),
            12.0f,
            IM_COL32(24,26,29,255)
        );

        d->AddCircle(
            ImVec2(
                carCenter.x-46.0f,
                carCenter.y+31.0f
            ),
            12.0f,
            IM_COL32(130,136,145,225),
            20,
            2.0f
        );

        d->AddCircleFilled(
            ImVec2(
                carCenter.x+48.0f,
                carCenter.y+31.0f
            ),
            12.0f,
            IM_COL32(24,26,29,255)
        );

        d->AddCircle(
            ImVec2(
                carCenter.x+48.0f,
                carCenter.y+31.0f
            ),
            12.0f,
            IM_COL32(130,136,145,225),
            20,
            2.0f
        );

        // ----------------------------------------------------
        // 3D: transform gizmo at scene object
        // ----------------------------------------------------

        const ImVec2 objectGizmo(
            carCenter.x+4.0f,
            carCenter.y-58.0f
        );

        d->AddCircleFilled(
            objectGizmo,
            5.0f,
            IM_COL32(235,180,54,255)
        );

        d->AddLine(
            objectGizmo,
            ImVec2(
                objectGizmo.x+48.0f,
                objectGizmo.y
            ),
            IM_COL32(212,82,70,255),
            2.5f
        );

        d->AddTriangleFilled(
            ImVec2(
                objectGizmo.x+54.0f,
                objectGizmo.y
            ),
            ImVec2(
                objectGizmo.x+44.0f,
                objectGizmo.y-5.0f
            ),
            ImVec2(
                objectGizmo.x+44.0f,
                objectGizmo.y+5.0f
            ),
            IM_COL32(212,82,70,255)
        );

        d->AddLine(
            objectGizmo,
            ImVec2(
                objectGizmo.x,
                objectGizmo.y-48.0f
            ),
            IM_COL32(78,182,100,255),
            2.5f
        );

        d->AddTriangleFilled(
            ImVec2(
                objectGizmo.x,
                objectGizmo.y-54.0f
            ),
            ImVec2(
                objectGizmo.x-5.0f,
                objectGizmo.y-44.0f
            ),
            ImVec2(
                objectGizmo.x+5.0f,
                objectGizmo.y-44.0f
            ),
            IM_COL32(78,182,100,255)
        );

        d->AddLine(
            objectGizmo,
            ImVec2(
                objectGizmo.x-34.0f,
                objectGizmo.y+30.0f
            ),
            IM_COL32(76,126,218,255),
            2.5f
        );

        // ----------------------------------------------------
        // 3D: camera information card
        // ----------------------------------------------------

        const ImVec2 camCardMin(
            cp.x+16.0f,
            cp.y+cs.y-118.0f
        );

        const ImVec2 camCardMax(
            cp.x+276.0f,
            cp.y+cs.y-16.0f
        );

        d->AddRectFilled(
            camCardMin,
            camCardMax,
            IM_COL32(22,25,29,225),
            5.0f
        );

        d->AddRect(
            camCardMin,
            camCardMax,
            IM_COL32(68,74,82,225),
            5.0f,
            0,
            1.0f
        );

        d->AddText(
            ImVec2(
                camCardMin.x+12.0f,
                camCardMin.y+10.0f
            ),
            IM_COL32(220,223,228,255),
            "CAMERA"
        );

        const char* presetLabel =
            viewPreset3D==0
                ? "Perspective"
                : (viewPreset3D==1
                    ? "Top"
                    : (viewPreset3D==2
                        ? "Front"
                        : "Right"));

        d->AddText(
            ImVec2(
                camCardMin.x+12.0f,
                camCardMin.y+36.0f
            ),
            IM_COL32(150,156,166,255),
            presetLabel
        );

        char fovText[64]{};
        std::snprintf(
            fovText,
            sizeof(fovText),
            "FOV %.0f deg  |  %s space",
            cameraFov,
            gizmoSpace3D==0
                ? "World"
                : "Local"
        );

        d->AddText(
            ImVec2(
                camCardMin.x+12.0f,
                camCardMin.y+60.0f
            ),
            IM_COL32(150,156,166,255),
            fovText
        );

        if (showNavigationHints)
        {
            d->AddText(
                ImVec2(
                    camCardMin.x+12.0f,
                    camCardMin.y+82.0f
                ),
                IM_COL32(115,121,130,255),
                "RMB Look   MMB Pan   Wheel Zoom   F Frame"
            );
        }
    }
    else if (viewportMode==2)
    {
        // ----------------------------------------------------
        // AR: simulated camera vignette
        // ----------------------------------------------------

        d->AddRectFilledMultiColor(
            cp,
            ImVec2(
                cp.x+cs.x,
                cp.y+cs.y
            ),
            IM_COL32(8,10,13,76),
            IM_COL32(8,10,13,76),
            IM_COL32(8,10,13,120),
            IM_COL32(8,10,13,120)
        );

        // ----------------------------------------------------
        // AR: center placement reticle
        // ----------------------------------------------------

        if (arReticle)
        {
            const float r=24.0f;

            d->AddCircle(
                center,
                r,
                IM_COL32(104,205,231,225),
                32,
                1.8f
            );

            d->AddLine(
                ImVec2(center.x-r-9.0f,center.y),
                ImVec2(center.x-r+6.0f,center.y),
                IM_COL32(104,205,231,225),
                1.8f
            );

            d->AddLine(
                ImVec2(center.x+r-6.0f,center.y),
                ImVec2(center.x+r+9.0f,center.y),
                IM_COL32(104,205,231,225),
                1.8f
            );

            d->AddCircleFilled(
                center,
                3.5f,
                IM_COL32(104,205,231,235)
            );
        }

        // ----------------------------------------------------
        // AR: detected plane mesh
        // ----------------------------------------------------

        if (arPlaneMesh)
        {
            const float planeY=
                center.y+112.0f;

            for (int i=-5;i<=5;++i)
            {
                const float x=
                    center.x+
                    static_cast<float>(i)*42.0f;

                d->AddLine(
                    ImVec2(
                        x-100.0f,
                        planeY+80.0f
                    ),
                    ImVec2(
                        center.x+
                        static_cast<float>(i)*18.0f,
                        planeY-24.0f
                    ),
                    IM_COL32(92,170,196,76),
                    1.0f
                );
            }

            for (int j=0;j<5;++j)
            {
                const float y=
                    planeY+
                    static_cast<float>(j)*18.0f;

                d->AddLine(
                    ImVec2(
                        center.x-220.0f+
                        static_cast<float>(j)*18.0f,
                        y
                    ),
                    ImVec2(
                        center.x+220.0f-
                        static_cast<float>(j)*18.0f,
                        y
                    ),
                    IM_COL32(92,170,196,70),
                    1.0f
                );
            }
        }

        // ----------------------------------------------------
        // AR: anchor points
        // ----------------------------------------------------

        for (int i=0;i<arAnchorCount;++i)
        {
            const float offsetX=
                static_cast<float>(
                    (i%4)-1
                )*82.0f;

            const float offsetY=
                static_cast<float>(
                    (i/4)
                )*58.0f;

            const ImVec2 a(
                center.x+offsetX,
                center.y+92.0f+offsetY
            );

            d->AddCircleFilled(
                a,
                6.0f,
                IM_COL32(104,205,231,235)
            );

            d->AddCircle(
                a,
                14.0f,
                IM_COL32(104,205,231,190),
                24,
                1.5f
            );

            char anchorLabel[32]{};
            std::snprintf(
                anchorLabel,
                sizeof(anchorLabel),
                "A%d",
                i+1
            );

            d->AddText(
                ImVec2(
                    a.x+18.0f,
                    a.y-8.0f
                ),
                IM_COL32(150,219,239,235),
                anchorLabel
            );
        }

        // ----------------------------------------------------
        // AR: tracking/status card
        // ----------------------------------------------------

        const ImVec2 arCardMin(
            cp.x+16.0f,
            cp.y+cs.y-132.0f
        );

        const ImVec2 arCardMax(
            cp.x+306.0f,
            cp.y+cs.y-16.0f
        );

        d->AddRectFilled(
            arCardMin,
            arCardMax,
            IM_COL32(22,25,29,228),
            5.0f
        );

        d->AddRect(
            arCardMin,
            arCardMax,
            IM_COL32(68,74,82,225),
            5.0f,
            0,
            1.0f
        );

        d->AddText(
            ImVec2(
                arCardMin.x+12.0f,
                arCardMin.y+10.0f
            ),
            IM_COL32(220,223,228,255),
            "AR SESSION"
        );

        const char* trackingLabel =
            arTrackingQuality==2
                ? "Tracking: Good"
                : (arTrackingQuality==1
                    ? "Tracking: Limited"
                    : "Tracking: Poor");

        d->AddText(
            ImVec2(
                arCardMin.x+12.0f,
                arCardMin.y+36.0f
            ),
            arTrackingQuality==2
                ? IM_COL32(120,205,140,255)
                : IM_COL32(230,185,92,255),
            trackingLabel
        );

        const char* placementLabel =
            arPlacementMode==0
                ? "Placement: Origin"
                : (arPlacementMode==1
                    ? "Placement: Surface"
                    : "Placement: Vehicle");

        d->AddText(
            ImVec2(
                arCardMin.x+12.0f,
                arCardMin.y+60.0f
            ),
            IM_COL32(150,156,166,255),
            placementLabel
        );

        char anchorCountText[64]{};
        std::snprintf(
            anchorCountText,
            sizeof(anchorCountText),
            "Anchors: %d   Occlusion: %s",
            arAnchorCount,
            arOcclusion
                ? "On"
                : "Off"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+12.0f,
                arCardMin.y+84.0f
            ),
            IM_COL32(150,156,166,255),
            anchorCountText
        );
    }

'@

$func = $func.Insert(
    $contextPos,
    $visuals
)

Write-Host "[OK] Added professional 3D + AR viewport visual layer." -ForegroundColor Green

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
    'Viewport3DARProStrip',
    '##Viewport3DViewPreset',
    '##Viewport3DFov',
    '##ARPlacementMode',
    'PLACE ANCHOR',
    'VIEWPORT PHASE 3 - PROFESSIONAL 3D / AR VISUAL LAYER',
    'CAMERA',
    'AR SESSION',
    'Tracking: Good'
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] 3D + AR professional viewport pass installed." -ForegroundColor Cyan
Write-Host "[OK] 3D: view presets / FOV / gizmo space / nav controls." -ForegroundColor Green
Write-Host "[OK] 3D: improved vehicle preview / transform gizmo / camera card." -ForegroundColor Green
Write-Host "[OK] AR: placement / tracking / occlusion / plane / reticle controls." -ForegroundColor Green
Write-Host "[OK] AR: interactive preview anchors." -ForegroundColor Green
Write-Host "[OK] AR: tracking + session HUD." -ForegroundColor Green
Write-Host "[OK] 2D viewport left untouched." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
