param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - REBALANCE AR VIEWPORT LAYOUT" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'NO AR SESSION',
    '// AR: tracking/status card',
    'Objects: 0',
    'AR PREVIEW'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected AR layout marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-ar-rebalance-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

$changes = 0

# ------------------------------------------------------------
# 1. Hide the generic top-right mini stats card in AR mode.
#    It crowds the top-right and doesn't help balance.
# ------------------------------------------------------------

if ($text.Contains('if (showStats)')) {
    $text = $text.Replace(
        'if (showStats)',
        'if (showStats && viewportMode!=2)'
    )
    $changes++
    Write-Host "[OK] Generic viewport HUD suppressed in AR mode." -ForegroundColor Green
}
elseif ($text.Contains('if (showStats && viewportMode!=2)')) {
    Write-Host "[SKIP] Generic viewport HUD already suppressed in AR." -ForegroundColor DarkGray
}
else {
    throw "Could not locate generic showStats HUD condition."
}

# ------------------------------------------------------------
# 2. Shift the central NO AR SESSION panel left-of-center so the
#    right side can carry the overview panel.
# ------------------------------------------------------------

$oldPanelMin = @'
            const ImVec2 panelMin(
                center.x-panelW*0.5f,
                center.y-panelH*0.5f
            );

            const ImVec2 panelMax(
                center.x+panelW*0.5f,
                center.y+panelH*0.5f
            );
'@

$newPanelMin = @'
            const ImVec2 panelMin(
                cp.x + cs.x*0.34f - panelW*0.5f,
                center.y - panelH*0.5f
            );

            const ImVec2 panelMax(
                cp.x + cs.x*0.34f + panelW*0.5f,
                center.y + panelH*0.5f
            );
'@

if ($text.Contains($oldPanelMin)) {
    $text = $text.Replace($oldPanelMin,$newPanelMin)
    $changes++
    Write-Host "[OK] NO AR SESSION panel shifted left-of-center." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] Did not find exact centered NO AR SESSION panel block." -ForegroundColor DarkGray
}

# ------------------------------------------------------------
# 3. Replace the old bottom-left AR tracking/status card with a
#    proper right-side overview panel.
# ------------------------------------------------------------

$oldArCard = @'
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
'@

$newArCard = @'
        // ----------------------------------------------------
        // AR: right-side overview panel
        // ----------------------------------------------------

        const ImVec2 arCardMin(
            cp.x+cs.x-324.0f,
            cp.y+150.0f
        );

        const ImVec2 arCardMax(
            cp.x+cs.x-18.0f,
            cp.y+430.0f
        );

        d->AddRectFilled(
            arCardMin,
            arCardMax,
            IM_COL32(22,25,29,228),
            6.0f
        );

        d->AddRect(
            arCardMin,
            arCardMax,
            IM_COL32(68,74,82,225),
            6.0f,
            0,
            1.0f
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+12.0f
            ),
            IM_COL32(220,223,228,255),
            "AR OVERVIEW"
        );

        d->AddLine(
            ImVec2(arCardMin.x+14.0f,arCardMin.y+36.0f),
            ImVec2(arCardMax.x-14.0f,arCardMin.y+36.0f),
            IM_COL32(56,61,69,220),
            1.0f
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+52.0f
            ),
            IM_COL32(145,151,160,255),
            "Mode"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+52.0f
            ),
            IM_COL32(214,217,222,255),
            arEditorPreview
                ? "Editor Preview"
                : (arDeviceConnected && arSessionRunning
                    ? "Live Session"
                    : "Offline")
        );

        const char* trackingLabel =
            arTrackingQuality==2
                ? "Good"
                : (arTrackingQuality==1
                    ? "Limited"
                    : "Poor");

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+82.0f
            ),
            IM_COL32(145,151,160,255),
            "Tracking"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+82.0f
            ),
            arTrackingQuality==2
                ? IM_COL32(120,205,140,255)
                : IM_COL32(230,185,92,255),
            trackingLabel
        );

        const char* placementLabel =
            arPlacementMode==0
                ? "Origin"
                : (arPlacementMode==1
                    ? "Surface"
                    : "Vehicle");

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+112.0f
            ),
            IM_COL32(145,151,160,255),
            "Placement"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+112.0f
            ),
            IM_COL32(214,217,222,255),
            placementLabel
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+142.0f
            ),
            IM_COL32(145,151,160,255),
            "Anchors"
        );

        char anchorsText[32]{};
        std::snprintf(
            anchorsText,
            sizeof(anchorsText),
            "%d",
            arAnchorCount
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+142.0f
            ),
            IM_COL32(214,217,222,255),
            anchorsText
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+172.0f
            ),
            IM_COL32(145,151,160,255),
            "Occlusion"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+172.0f
            ),
            IM_COL32(214,217,222,255),
            arOcclusion ? "On" : "Off"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+202.0f
            ),
            IM_COL32(145,151,160,255),
            "Planes"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+202.0f
            ),
            IM_COL32(214,217,222,255),
            arPlaneMesh ? "Visible" : "Hidden"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+232.0f
            ),
            IM_COL32(145,151,160,255),
            "Reticle"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+232.0f
            ),
            IM_COL32(214,217,222,255),
            arReticle ? "Enabled" : "Disabled"
        );

        d->AddLine(
            ImVec2(arCardMin.x+14.0f,arCardMin.y+260.0f),
            ImVec2(arCardMax.x-14.0f,arCardMin.y+260.0f),
            IM_COL32(56,61,69,220),
            1.0f
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+276.0f
            ),
            IM_COL32(145,151,160,255),
            "READY STATE"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+304.0f
            ),
            IM_COL32(115,121,130,255),
            "Use Editor Preview for layout and placement validation."
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+330.0f
            ),
            IM_COL32(115,121,130,255),
            "Connect a device later for live camera and tracking."
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+360.0f
            ),
            IM_COL32(98,179,205,235),
            "Right side now carries live AR overview information."
        );
'@

if ($text.Contains($oldArCard)) {
    $text = $text.Replace($oldArCard,$newArCard)
    $changes++
    Write-Host "[OK] Old bottom-left AR card replaced with right-side overview panel." -ForegroundColor Green
}
else {
    throw "Could not locate the old AR tracking/status card block."
}

# ------------------------------------------------------------
# 4. Add a subtle mid-right guide caption if not already present.
# ------------------------------------------------------------

if (-not $text.Contains('AR PLACEMENT / SESSION OVERVIEW')) {
    $marker = '        // AR: right-side overview panel'
    $insert = @'
        d->AddText(
            ImVec2(
                cp.x+cs.x-266.0f,
                cp.y+126.0f
            ),
            IM_COL32(116,122,131,210),
            "AR PLACEMENT / SESSION OVERVIEW"
        );

'@
    if ($text.Contains($marker)) {
        $text = $text.Replace($marker, $insert + $marker)
        $changes++
        Write-Host "[OK] Added right-side section caption." -ForegroundColor Green
    }
}

if ($changes -eq 0) {
    Write-Host ""
    Write-Host "[INFO] No AR rebalance changes were applied." -ForegroundColor Yellow
    exit 0
}

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'if (showStats && viewportMode!=2)',
    'AR OVERVIEW',
    'AR PLACEMENT / SESSION OVERVIEW',
    'cp.x+cs.x-324.0f',
    'cp.x + cs.x*0.34f - panelW*0.5f'
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] AR viewport layout rebalanced." -ForegroundColor Cyan
Write-Host "[OK] Top-right AR clutter reduced." -ForegroundColor Green
Write-Host "[OK] Mid-right viewport now contains a proper overview panel." -ForegroundColor Green
Write-Host "[OK] Empty-state panel shifted left for balance." -ForegroundColor Green
Write-Host "[OK] AR viewport should now feel less left-biased." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
