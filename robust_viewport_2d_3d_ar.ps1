param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - ROBUST VIEWPORT 2D / 3D / AR PATCH" -ForegroundColor Cyan
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

$blockInfo = Get-FunctionBlock -Source $text -Signature "static void drawViewportView()"
$func = $blockInfo.Text

if ($func.Contains("ViewportModeStrip")) {
    Write-Host "[OK] Viewport mode strip already exists. Nothing to patch." -ForegroundColor Green
    exit 0
}

$requiredInFunction = @(
    'ImGui::Begin("Viewport")',
    'SceneCanvas'
)

foreach ($marker in $requiredInFunction) {
    if (-not $func.Contains($marker)) {
        throw "Live drawViewportView() does not contain expected marker: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-robust-viewport-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. ADD VIEWPORT MODE STATE
# ============================================================

$stateBlock = @'
    static int viewportMode=0;   // 0 = 2D, 1 = 3D, 2 = AR
    static int orthoView=0;      // 0 = Top, 1 = Front, 2 = Right
    static int renderMode=0;     // 0 = Lit, 1 = Wireframe, 2 = Analysis
    static float arOpacity=0.75f;

'@

$openingBrace = $func.IndexOf("{")
if ($openingBrace -lt 0) {
    throw "Could not find drawViewportView() opening brace."
}

$insertPos = $openingBrace + 1
$func = $func.Insert($insertPos, "`r`n" + $stateBlock)

Write-Host "[OK] Added 2D / 3D / AR viewport state." -ForegroundColor Green

# ============================================================
# 2. INSERT MODE STRIP AFTER ImGui::Begin("Viewport");
# ============================================================

$beginPattern = 'ImGui::Begin\s*\(\s*"Viewport"\s*\)\s*;'
$beginMatch = [regex]::Match($func, $beginPattern)

if (-not $beginMatch.Success) {
    throw 'Could not locate ImGui::Begin("Viewport");'
}

$modeStrip = @'

    // ========================================================
    // VIEWPORT MODE STRIP
    // ========================================================

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
            ImGui::Button(label,ImVec2(width,31.0f));

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
        const char* orthoViews[]={"Top","Front","Right"};

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
        const char* renderModes[]={"Lit","Wireframe","Analysis"};

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

$afterBegin = $beginMatch.Index + $beginMatch.Length
$func = $func.Insert($afterBegin, $modeStrip)

Write-Host "[OK] Added visible 2D PLAN / 3D SCENE / AR PREVIEW strip." -ForegroundColor Green

# ============================================================
# 3. MAKE THE LIVE CANVAS VISIBLY MODE-AWARE
# ============================================================

# Background color.
$bgPattern = 'd->AddRectFilled\s*\(\s*cp\s*,\s*ImVec2\s*\(\s*cp\.x\+cs\.x\s*,\s*cp\.y\+cs\.y\s*\)\s*,\s*IM_COL32\s*\(\s*18\s*,\s*20\s*,\s*22\s*,\s*255\s*\)\s*\)\s*;'

if ([regex]::IsMatch($func,$bgPattern)) {
    $bgReplacement = @'
d->AddRectFilled(
        cp,
        ImVec2(cp.x+cs.x,cp.y+cs.y),
        viewportMode==0
            ? IM_COL32(18,20,22,255)
            : (viewportMode==1
                ? IM_COL32(22,25,30,255)
                : IM_COL32(27,31,37,255))
    );
'@

    $func = [regex]::Replace($func,$bgPattern,$bgReplacement,1)
    Write-Host "[OK] Canvas background now changes by mode." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] Exact old canvas background line not found." -ForegroundColor DarkGray
}

# Existing SCENE VIEWPORT title.
$titlePattern = 'd->AddText\s*\(\s*ImVec2\s*\(\s*cp\.x\+16\.0f\s*,\s*cp\.y\+16\.0f\s*\)\s*,\s*IM_COL32\s*\(\s*225\s*,\s*226\s*,\s*228\s*,\s*255\s*\)\s*,\s*"SCENE VIEWPORT"\s*\)\s*;'

$titleReplacement = @'
d->AddText(
        ImVec2(cp.x+16.0f,cp.y+16.0f),
        IM_COL32(225,226,228,255),
        viewportMode==0
            ? "2D PLAN VIEW"
            : (viewportMode==1
                ? "3D SCENE VIEW"
                : "AR PREVIEW")
    );
'@

if ([regex]::IsMatch($func,$titlePattern)) {
    $func = [regex]::Replace($func,$titlePattern,$titleReplacement,1)
    Write-Host "[OK] Viewport title changes by mode." -ForegroundColor Green
}
else {
    # Looser fallback.
    $func = $func.Replace(
        '"SCENE VIEWPORT"',
        'viewportMode==0 ? "2D PLAN VIEW" : (viewportMode==1 ? "3D SCENE VIEW" : "AR PREVIEW")'
    )
    Write-Host "[OK] Viewport title patched using fallback." -ForegroundColor Green
}

# Existing subtitle.
$subtitlePattern = 'd->AddText\s*\(\s*ImVec2\s*\(\s*cp\.x\+16\.0f\s*,\s*cp\.y\+40\.0f\s*\)\s*,\s*IM_COL32\s*\(\s*135\s*,\s*139\s*,\s*145\s*,\s*255\s*\)\s*,\s*"No scene objects loaded"\s*\)\s*;'

$subtitleReplacement = @'
d->AddText(
        ImVec2(cp.x+16.0f,cp.y+40.0f),
        IM_COL32(135,139,145,255),
        viewportMode==0
            ? (orthoView==0
                ? "Top orthographic reconstruction"
                : (orthoView==1
                    ? "Front orthographic reconstruction"
                    : "Right orthographic reconstruction"))
            : (viewportMode==1
                ? (renderMode==0
                    ? "Perspective lit reconstruction"
                    : (renderMode==1
                        ? "Wireframe inspection"
                        : "Analysis overlay"))
                : "Editor AR preview - device integration pending")
    );
'@

if ([regex]::IsMatch($func,$subtitlePattern)) {
    $func = [regex]::Replace($func,$subtitlePattern,$subtitleReplacement,1)
    Write-Host "[OK] Viewport subtitle changes by mode." -ForegroundColor Green
}
else {
    $func = $func.Replace(
        '"No scene objects loaded"',
        'viewportMode==0 ? "Orthographic reconstruction" : (viewportMode==1 ? "Perspective reconstruction" : "Editor AR preview - device integration pending")'
    )
    Write-Host "[OK] Viewport subtitle patched using fallback." -ForegroundColor Green
}

# ============================================================
# 4. ADD DISTINCT 3D + AR OVERLAYS BEFORE THE MODE TITLE
# ============================================================

$overlayAnchor = 'viewportMode==0 ? "2D PLAN VIEW"'

if (-not $func.Contains($overlayAnchor)) {
    throw "Could not find mode-aware viewport title after patch."
}

$anchorPos = $func.IndexOf('d->AddText(', $func.IndexOf($overlayAnchor) - 200)
if ($anchorPos -lt 0) {
    throw "Could not locate insertion point for viewport mode overlays."
}

$overlayCode = @'
    if (viewportMode==1)
    {
        const float horizon=cp.y+cs.y*0.34f;

        d->AddLine(
            ImVec2(cp.x+20.0f,horizon),
            ImVec2(cp.x+cs.x-20.0f,horizon),
            IM_COL32(75,79,85,170),
            1.0f
        );

        const ImVec2 vanish(center.x,horizon+18.0f);

        for (int i=0;i<=12;++i)
        {
            const float t=static_cast<float>(i)/12.0f;
            const float x=cp.x+24.0f+(cs.x-48.0f)*t;

            d->AddLine(
                ImVec2(x,cp.y+cs.y-20.0f),
                vanish,
                IM_COL32(61,65,70,150),
                1.0f
            );
        }

        const ImVec2 boxMin(center.x-82.0f,center.y+36.0f);
        const ImVec2 boxMax(center.x+28.0f,center.y+112.0f);

        if (renderMode==1)
        {
            d->AddRect(
                boxMin,
                boxMax,
                IM_COL32(185,205,230,220),
                4.0f,
                0,
                2.0f
            );
        }
        else
        {
            d->AddRectFilled(
                boxMin,
                boxMax,
                IM_COL32(105,125,160,105),
                4.0f
            );

            d->AddRect(
                boxMin,
                boxMax,
                IM_COL32(180,205,235,220),
                4.0f,
                0,
                1.7f
            );
        }

        if (renderMode==2)
        {
            d->AddLine(
                ImVec2(center.x-145.0f,center.y+125.0f),
                ImVec2(center.x+150.0f,center.y+94.0f),
                IM_COL32(238,174,38,225),
                2.0f
            );
        }
    }
    else if (viewportMode==2)
    {
        const ImVec2 frameMin(cp.x+34.0f,cp.y+30.0f);
        const ImVec2 frameMax(cp.x+cs.x-34.0f,cp.y+cs.y-30.0f);

        d->AddRect(
            frameMin,
            frameMax,
            IM_COL32(90,98,108,220),
            12.0f,
            0,
            1.4f
        );

        const ImVec2 targetMin(center.x-132.0f,center.y-72.0f);
        const ImVec2 targetMax(center.x+132.0f,center.y+72.0f);

        const int arAlpha=
            static_cast<int>(220.0f*arOpacity);

        d->AddRect(
            targetMin,
            targetMax,
            IM_COL32(95,195,225,arAlpha),
            6.0f,
            0,
            2.0f
        );

        d->AddCircle(
            ImVec2(center.x,center.y+100.0f),
            17.0f,
            IM_COL32(95,195,225,210),
            24,
            2.0f
        );

        d->AddText(
            ImVec2(center.x+25.0f,center.y+91.0f),
            IM_COL32(120,205,232,230),
            "Origin anchor"
        );

        d->AddRectFilled(
            ImVec2(frameMax.x-174.0f,frameMin.y+14.0f),
            ImVec2(frameMax.x-18.0f,frameMin.y+42.0f),
            IM_COL32(55,61,69,220),
            14.0f
        );

        d->AddText(
            ImVec2(frameMax.x-154.0f,frameMin.y+20.0f),
            IM_COL32(218,222,228,255),
            "DEVICE OFFLINE"
        );
    }

'@

$func = $func.Insert($anchorPos,$overlayCode)
Write-Host "[OK] Added distinct 3D and AR visual overlays." -ForegroundColor Green

# ============================================================
# 5. WRITE FUNCTION BACK INTO FILE
# ============================================================

$oldFunc = $blockInfo.Text
$text = $text.Replace($oldFunc,$func)

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'ViewportModeStrip',
    '"2D PLAN"',
    '"3D SCENE"',
    '"AR PREVIEW"',
    '"DEVICE OFFLINE"',
    'static int viewportMode=0;'
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] Robust live viewport patch installed." -ForegroundColor Cyan
Write-Host "[OK] No exact whitespace/header match was required." -ForegroundColor Green
Write-Host "[OK] 2D / 3D / AR strip is now in drawViewportView()." -ForegroundColor Green
Write-Host "[OK] 3D and AR have visibly different canvas overlays." -ForegroundColor Green
Write-Host ""
Write-Host "VERIFY BEFORE BUILD:" -ForegroundColor Yellow
Write-Host '  findstr /C:"ViewportModeStrip" .\src\main.cpp'
Write-Host '  findstr /C:"DEVICE OFFLINE" .\src\main.cpp'
Write-Host ""
Write-Host "Then rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
