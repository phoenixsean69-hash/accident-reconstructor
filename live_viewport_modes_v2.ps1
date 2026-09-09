param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - LIVE VIEWPORT MODES v2" -ForegroundColor Cyan
Write-Host " 2D / 3D / AR" -ForegroundColor DarkGray
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

function Find-ChildEnd {
    param(
        [string]$FunctionText,
        [string]$ChildMarker
    )

    $markerPos = $FunctionText.IndexOf($ChildMarker)

    if ($markerPos -lt 0) {
        throw "Could not find child marker: $ChildMarker"
    }

    $endPos = $FunctionText.IndexOf("ImGui::EndChild();", $markerPos)

    if ($endPos -lt 0) {
        throw "Could not find EndChild() after: $ChildMarker"
    }

    return $endPos
}

$info = Get-FunctionBlock `
    -Source $text `
    -Signature "static void drawViewportView()"

$func = $info.Text

if ($func.Contains("ViewportModeStrip")) {
    Write-Host "[OK] ViewportModeStrip already exists." -ForegroundColor Green
    Write-Host "Nothing changed." -ForegroundColor DarkGray
    exit 0
}

$mustExist = @(
    'ImGui::Begin("Viewport")',
    'SceneCanvas',
    '"SCENE VIEWPORT"',
    '"No scene objects loaded"'
)

foreach ($marker in $mustExist) {
    if (-not $func.Contains($marker)) {
        throw "Expected live viewport marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-viewport-modes-v2-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. STATE
# ============================================================

$openBrace = $func.IndexOf("{")

$state = @'

    static int viewportMode=0;    // 0 = 2D, 1 = 3D, 2 = AR
    static int orthoView=0;       // 0 = Top, 1 = Front, 2 = Right
    static int renderMode=0;      // 0 = Lit, 1 = Wireframe, 2 = Analysis
    static float arOpacity=0.78f;
'@

$func = $func.Insert(
    $openBrace + 1,
    $state
)

Write-Host "[OK] Added viewport mode state." -ForegroundColor Green

# ============================================================
# 2. MODE STRIP
# ============================================================

$beginMatch = [regex]::Match(
    $func,
    'ImGui::Begin\s*\(\s*"Viewport"\s*\)\s*;'
)

if (-not $beginMatch.Success) {
    throw 'Could not find ImGui::Begin("Viewport");'
}

$strip = @'

    ImGui::BeginChild(
        "ViewportModeStrip",
        ImVec2(0.0f,54.0f),
        true,
        ImGuiWindowFlags_NoScrollbar
    );

    auto viewportModeButton = [&](const char* label,int mode,float width)
    {
        const bool active=(viewportMode==mode);

        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button,colorAccentMuted());
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,colorAccentMuted());
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,colorAccentMuted());
            ImGui::PushStyleColor(ImGuiCol_Text,colorAccent());
        }

        const bool pressed=
            ImGui::Button(
                label,
                ImVec2(width,31.0f)
            );

        if (active)
            ImGui::PopStyleColor(4);

        if (pressed)
            viewportMode=mode;
    };

    viewportModeButton("2D PLAN",0,96.0f);
    ImGui::SameLine(0.0f,6.0f);

    viewportModeButton("3D SCENE",1,102.0f);
    ImGui::SameLine(0.0f,6.0f);

    viewportModeButton("AR PREVIEW",2,112.0f);
    ImGui::SameLine(0.0f,18.0f);

    if (viewportMode==0)
    {
        const char* orthoViews[]={
            "Top",
            "Front",
            "Right"
        };

        ImGui::TextDisabled("ORTHO");
        ImGui::SameLine(0.0f,7.0f);

        ImGui::SetNextItemWidth(116.0f);
        ImGui::Combo(
            "##ViewportOrthoView",
            &orthoView,
            orthoViews,
            3
        );
    }
    else if (viewportMode==1)
    {
        const char* renderModes[]={
            "Lit",
            "Wireframe",
            "Analysis"
        };

        ImGui::TextDisabled("DISPLAY");
        ImGui::SameLine(0.0f,7.0f);

        ImGui::SetNextItemWidth(132.0f);
        ImGui::Combo(
            "##ViewportRenderMode",
            &renderMode,
            renderModes,
            3
        );
    }
    else
    {
        ImGui::TextDisabled("DEVICE OFFLINE");
        ImGui::SameLine(0.0f,14.0f);

        ImGui::SetNextItemWidth(154.0f);
        ImGui::SliderFloat(
            "##ViewportAROpacity",
            &arOpacity,
            0.25f,
            1.0f,
            "Overlay %.2f"
        );
    }

    ImGui::EndChild();
    ImGui::Spacing();
'@

$func = $func.Insert(
    $beginMatch.Index + $beginMatch.Length,
    $strip
)

Write-Host "[OK] Added 2D PLAN / 3D SCENE / AR PREVIEW strip." -ForegroundColor Green

# ============================================================
# 3. MODE-AWARE BACKGROUND
# ============================================================

$func = [regex]::Replace(
    $func,
    'd->AddRectFilled\s*\(\s*cp\s*,\s*ImVec2\s*\(\s*cp\.x\+cs\.x\s*,\s*cp\.y\+cs\.y\s*\)\s*,\s*IM_COL32\s*\(\s*18\s*,\s*20\s*,\s*22\s*,\s*255\s*\)\s*\)\s*;',
@'
d->AddRectFilled(
        cp,
        ImVec2(cp.x+cs.x,cp.y+cs.y),
        viewportMode==0
            ? IM_COL32(18,20,22,255)
            : (viewportMode==1
                ? IM_COL32(22,25,30,255)
                : IM_COL32(27,31,37,255))
    );
'@,
    1
)

Write-Host "[OK] Made canvas background mode-aware." -ForegroundColor Green

# ============================================================
# 4. KEEP OLD GRID/AXES ONLY IN 2D
# ============================================================

$func = $func.Replace(
    'if (showGrid)',
    'if (viewportMode==0 && showGrid)'
)

$func = $func.Replace(
    'if (showAxes)',
    'if (viewportMode==0 && showAxes)'
)

Write-Host "[OK] Existing flat grid/axes are now 2D-only." -ForegroundColor Green

# ============================================================
# 5. MODE-AWARE TITLE + SUBTITLE
# ============================================================

$func = $func.Replace(
    '"SCENE VIEWPORT"',
    'viewportMode==0 ? "2D PLAN VIEW" : (viewportMode==1 ? "3D SCENE VIEW" : "AR PREVIEW")'
)

$func = $func.Replace(
    '"No scene objects loaded"',
    'viewportMode==0 ? (orthoView==0 ? "Top orthographic reconstruction" : (orthoView==1 ? "Front orthographic reconstruction" : "Right orthographic reconstruction")) : (viewportMode==1 ? (renderMode==0 ? "Perspective lit reconstruction" : (renderMode==1 ? "Wireframe inspection" : "Analysis overlay")) : "Editor AR preview - live device integration pending")'
)

Write-Host "[OK] Added mode-specific viewport labels." -ForegroundColor Green

# ============================================================
# 6. INSERT 3D + AR OVERLAYS BEFORE SceneCanvas ENDS
# ============================================================

$sceneEnd = Find-ChildEnd `
    -FunctionText $func `
    -ChildMarker '"SceneCanvas"'

$overlays = @'

    if (viewportMode==1)
    {
        const float horizon=
            cp.y+cs.y*0.34f;

        d->AddLine(
            ImVec2(cp.x+22.0f,horizon),
            ImVec2(cp.x+cs.x-22.0f,horizon),
            IM_COL32(77,81,87,190),
            1.0f
        );

        const ImVec2 vanish(
            center.x,
            horizon+20.0f
        );

        for (int i=0;i<=14;++i)
        {
            const float t=
                static_cast<float>(i)/14.0f;

            const float x=
                cp.x+26.0f+
                (cs.x-52.0f)*t;

            d->AddLine(
                ImVec2(
                    x,
                    cp.y+cs.y-22.0f
                ),
                vanish,
                IM_COL32(62,66,72,155),
                1.0f
            );
        }

        for (int i=0;i<9;++i)
        {
            const float t=
                static_cast<float>(i)/8.0f;

            const float y=
                horizon+
                30.0f+
                (t*t)*(cs.y*.54f);

            d->AddLine(
                ImVec2(cp.x+30.0f,y),
                ImVec2(cp.x+cs.x-30.0f,y),
                IM_COL32(62,66,72,145),
                1.0f
            );
        }

        const ImVec2 vehicleMin(
            center.x-82.0f,
            center.y+32.0f
        );

        const ImVec2 vehicleMax(
            center.x+30.0f,
            center.y+108.0f
        );

        if (renderMode==1)
        {
            d->AddRect(
                vehicleMin,
                vehicleMax,
                IM_COL32(185,205,230,230),
                4.0f,
                0,
                2.0f
            );
        }
        else
        {
            d->AddRectFilled(
                vehicleMin,
                vehicleMax,
                IM_COL32(106,126,162,112),
                4.0f
            );

            d->AddRect(
                vehicleMin,
                vehicleMax,
                IM_COL32(180,205,235,225),
                4.0f,
                0,
                1.7f
            );
        }

        if (renderMode==2)
        {
            d->AddLine(
                ImVec2(
                    center.x-145.0f,
                    center.y+126.0f
                ),
                ImVec2(
                    center.x+150.0f,
                    center.y+94.0f
                ),
                IM_COL32(238,174,38,225),
                2.0f
            );

            d->AddText(
                ImVec2(
                    center.x+20.0f,
                    center.y+102.0f
                ),
                IM_COL32(240,194,100,235),
                "analysis vector"
            );
        }

        if (showAxes)
        {
            const ImVec2 gizmo(
                cp.x+82.0f,
                cp.y+cs.y-76.0f
            );

            d->AddLine(
                gizmo,
                ImVec2(gizmo.x+44.0f,gizmo.y),
                IM_COL32(190,65,55,255),
                2.0f
            );

            d->AddLine(
                gizmo,
                ImVec2(gizmo.x,gizmo.y-44.0f),
                IM_COL32(70,145,80,255),
                2.0f
            );

            d->AddLine(
                gizmo,
                ImVec2(gizmo.x-28.0f,gizmo.y+22.0f),
                IM_COL32(74,116,195,255),
                2.0f
            );

            d->AddText(
                ImVec2(gizmo.x+49.0f,gizmo.y-8.0f),
                IM_COL32(220,90,75,255),
                "X"
            );

            d->AddText(
                ImVec2(gizmo.x+5.0f,gizmo.y-56.0f),
                IM_COL32(100,190,110,255),
                "Y"
            );

            d->AddText(
                ImVec2(gizmo.x-40.0f,gizmo.y+18.0f),
                IM_COL32(110,155,230,255),
                "Z"
            );
        }
    }
    else if (viewportMode==2)
    {
        const ImVec2 frameMin(
            cp.x+36.0f,
            cp.y+30.0f
        );

        const ImVec2 frameMax(
            cp.x+cs.x-36.0f,
            cp.y+cs.y-30.0f
        );

        d->AddRect(
            frameMin,
            frameMax,
            IM_COL32(93,100,110,225),
            12.0f,
            0,
            1.5f
        );

        const ImVec2 targetMin(
            center.x-132.0f,
            center.y-72.0f
        );

        const ImVec2 targetMax(
            center.x+132.0f,
            center.y+72.0f
        );

        const int arAlpha=
            static_cast<int>(
                220.0f*arOpacity
            );

        d->AddRect(
            targetMin,
            targetMax,
            IM_COL32(95,195,225,arAlpha),
            6.0f,
            0,
            2.0f
        );

        d->AddCircle(
            ImVec2(
                center.x,
                center.y+101.0f
            ),
            17.0f,
            IM_COL32(95,195,225,215),
            24,
            2.0f
        );

        d->AddText(
            ImVec2(
                center.x+25.0f,
                center.y+92.0f
            ),
            IM_COL32(120,205,232,235),
            "Origin anchor"
        );

        d->AddRectFilled(
            ImVec2(
                frameMax.x-177.0f,
                frameMin.y+14.0f
            ),
            ImVec2(
                frameMax.x-18.0f,
                frameMin.y+42.0f
            ),
            IM_COL32(55,61,69,225),
            14.0f
        );

        d->AddText(
            ImVec2(
                frameMax.x-156.0f,
                frameMin.y+20.0f
            ),
            IM_COL32(218,222,228,255),
            "DEVICE OFFLINE"
        );
    }

'@

$func = $func.Insert(
    $sceneEnd,
    $overlays
)

Write-Host "[OK] Added 3D perspective and AR overlays." -ForegroundColor Green

# ============================================================
# 7. WRITE BACK ONLY THIS FUNCTION
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
    'ViewportModeStrip',
    'static int viewportMode=0;',
    '"2D PLAN"',
    '"3D SCENE"',
    '"AR PREVIEW"',
    '"DEVICE OFFLINE"',
    'if (viewportMode==1)',
    'if (viewportMode==0 && showGrid)'
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] Live viewport modes v2 installed." -ForegroundColor Cyan
Write-Host "[OK] 2D uses the existing orthographic grid." -ForegroundColor Green
Write-Host "[OK] 3D gets perspective-grid / scene-object treatment." -ForegroundColor Green
Write-Host "[OK] AR gets editor preview framing / target / anchor state." -ForegroundColor Green
Write-Host "[OK] Existing rail / Outliner / Properties / Timeline preserved." -ForegroundColor Green
Write-Host ""
Write-Host "Verify source:" -ForegroundColor Yellow
Write-Host '  findstr /C:"ViewportModeStrip" .\src\main.cpp'
Write-Host '  findstr /C:"DEVICE OFFLINE" .\src\main.cpp'
Write-Host ""
Write-Host "Then rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
