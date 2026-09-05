param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - PATCH LIVE VIEWPORT: 2D / 3D / AR" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'static void drawViewportView()',
    'static int selectedTool=0;',
    'static bool showGrid=true, showAxes=true;',
    'ImGui::Begin("Viewport");',
    'ImGui::BeginChild("SceneCanvas"',
    '"SCENE VIEWPORT"',
    '"No scene objects loaded"'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected LIVE viewport marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-live-viewport-modes-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. PATCH THE ACTUAL FUNCTION HEADER
# ============================================================

$oldHeader = @'
static void drawViewportView()
{
    static int selectedTool=0;
    static bool showGrid=true, showAxes=true;
    ImGui::Begin("Viewport");
'@

$newHeader = @'
static void drawViewportView()
{
    static int selectedTool=0;
    static bool showGrid=true, showAxes=true;

    // Viewport mode state.
    static int viewportMode=0;   // 0 = 2D, 1 = 3D, 2 = AR
    static int orthoView=0;      // 0 = Top, 1 = Front, 2 = Right
    static int renderMode=0;     // 0 = Lit, 1 = Wireframe, 2 = Analysis
    static float arOpacity=0.75f;

    ImGui::Begin("Viewport");

    // --------------------------------------------------------
    // MODE STRIP
    // --------------------------------------------------------

    ImGui::BeginChild(
        "ViewportModeStrip",
        ImVec2(0.0f,52.0f),
        true,
        ImGuiWindowFlags_NoScrollbar
    );

    auto modeButton = [&](const char* label,int mode,float width)
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
                ImVec2(width,30.0f)
            );

        if (active)
            ImGui::PopStyleColor(4);

        if (pressed)
            viewportMode=mode;
    };

    modeButton("2D PLAN",0,94.0f);
    ImGui::SameLine(0.0f,6.0f);
    modeButton("3D SCENE",1,98.0f);
    ImGui::SameLine(0.0f,6.0f);
    modeButton("AR PREVIEW",2,108.0f);

    ImGui::SameLine(0.0f,16.0f);

    if (viewportMode==0)
    {
        const char* views[]={"Top","Front","Right"};

        ImGui::TextDisabled("ORTHO");
        ImGui::SameLine(0.0f,7.0f);

        ImGui::SetNextItemWidth(112.0f);
        ImGui::Combo(
            "##ViewportOrthoView",
            &orthoView,
            views,
            3
        );
    }
    else if (viewportMode==1)
    {
        const char* modes[]={"Lit","Wireframe","Analysis"};

        ImGui::TextDisabled("VIEW");
        ImGui::SameLine(0.0f,7.0f);

        ImGui::SetNextItemWidth(126.0f);
        ImGui::Combo(
            "##ViewportRenderMode",
            &renderMode,
            modes,
            3
        );
    }
    else
    {
        ImGui::TextDisabled("AR DEVICE");
        ImGui::SameLine(0.0f,7.0f);

        ImGui::TextDisabled("Offline");

        ImGui::SameLine(0.0f,14.0f);
        ImGui::SetNextItemWidth(140.0f);
        ImGui::SliderFloat(
            "##AROverlayOpacity",
            &arOpacity,
            0.25f,
            1.0f,
            "Overlay %.2f"
        );
    }

    ImGui::EndChild();
    ImGui::Spacing();

'@

if (-not $text.Contains($oldHeader)) {
    throw "Could not match the live drawViewportView() header."
}

$text = $text.Replace($oldHeader,$newHeader)

Write-Host "[OK] Added live 2D / 3D / AR mode strip." -ForegroundColor Green

# ============================================================
# 2. REPLACE ONLY THE LIVE SceneCanvas DRAWING BODY
# ============================================================

$canvasPattern = '(?s)    const ImVec2 cp=ImGui::GetWindowPos\(\), cs=ImGui::GetWindowSize\(\);.*?    d->AddText\(ImVec2\(cp\.x\+16\.0f,cp\.y\+40\.0f\),IM_COL32\(135,139,145,255\),"No scene objects loaded"\);'

$canvasMatches = [regex]::Matches($text,$canvasPattern)

if ($canvasMatches.Count -ne 1) {
    throw "Expected exactly one live SceneCanvas drawing body, found $($canvasMatches.Count)."
}

$newCanvas = @'
    const ImVec2 cp=ImGui::GetWindowPos(), cs=ImGui::GetWindowSize();
    ImDrawList* d=ImGui::GetWindowDrawList();

    d->AddRectFilled(
        cp,
        ImVec2(cp.x+cs.x,cp.y+cs.y),
        IM_COL32(18,20,22,255)
    );

    const ImVec2 center(
        cp.x+cs.x*.5f,
        cp.y+cs.y*.5f
    );

    if (viewportMode==0)
    {
        // ====================================================
        // 2D PLAN
        // ====================================================

        if (showGrid)
        {
            const float gs=32.0f;

            for (float x=cp.x;x<cp.x+cs.x;x+=gs)
                d->AddLine(
                    ImVec2(x,cp.y),
                    ImVec2(x,cp.y+cs.y),
                    IM_COL32(49,52,55,255)
                );

            for (float y=cp.y;y<cp.y+cs.y;y+=gs)
                d->AddLine(
                    ImVec2(cp.x,y),
                    ImVec2(cp.x+cs.x,y),
                    IM_COL32(49,52,55,255)
                );
        }

        if (showAxes)
        {
            d->AddCircleFilled(
                center,
                7.0f,
                IM_COL32(238,174,38,255)
            );

            d->AddLine(
                center,
                ImVec2(center.x+100.0f,center.y),
                IM_COL32(190,65,55,255),
                2.0f
            );

            d->AddLine(
                center,
                ImVec2(center.x,center.y-100.0f),
                IM_COL32(70,145,80,255),
                2.0f
            );

            d->AddText(
                ImVec2(center.x+105.0f,center.y-10.0f),
                IM_COL32(220,90,75,255),
                "X"
            );

            d->AddText(
                ImVec2(center.x+7.0f,center.y-120.0f),
                IM_COL32(100,190,110,255),
                "Y"
            );
        }

        // A subtle road corridor makes the 2D mode read as a plan.
        d->AddRectFilled(
            ImVec2(center.x-72.0f,cp.y+18.0f),
            ImVec2(center.x+72.0f,cp.y+cs.y-18.0f),
            IM_COL32(70,76,84,36)
        );

        d->AddText(
            ImVec2(cp.x+16.0f,cp.y+16.0f),
            IM_COL32(225,226,228,255),
            "2D PLAN VIEW"
        );

        d->AddText(
            ImVec2(cp.x+16.0f,cp.y+40.0f),
            IM_COL32(135,139,145,255),
            orthoView==0
                ? "Top orthographic reconstruction"
                : (orthoView==1
                    ? "Front orthographic reconstruction"
                    : "Right orthographic reconstruction")
        );
    }
    else if (viewportMode==1)
    {
        // ====================================================
        // 3D SCENE
        // ====================================================

        const float horizon=
            cp.y+cs.y*0.34f;

        d->AddRectFilled(
            cp,
            ImVec2(cp.x+cs.x,horizon),
            IM_COL32(31,35,40,255)
        );

        d->AddRectFilled(
            ImVec2(cp.x,horizon),
            ImVec2(cp.x+cs.x,cp.y+cs.y),
            IM_COL32(22,24,27,255)
        );

        if (showGrid)
        {
            const ImVec2 vanish(
                center.x,
                horizon+22.0f
            );

            for (int i=0;i<=14;++i)
            {
                const float t=
                    static_cast<float>(i)/14.0f;

                const float x=
                    cp.x+22.0f+
                    (cs.x-44.0f)*t;

                d->AddLine(
                    ImVec2(x,cp.y+cs.y-18.0f),
                    vanish,
                    IM_COL32(66,69,73,160)
                );
            }

            for (int i=0;i<10;++i)
            {
                const float t=
                    static_cast<float>(i)/9.0f;

                const float y=
                    horizon+30.0f+
                    (t*t)*(cs.y*.55f);

                d->AddLine(
                    ImVec2(cp.x+26.0f,y),
                    ImVec2(cp.x+cs.x-26.0f,y),
                    IM_COL32(62,65,69,155)
                );
            }
        }

        // Placeholder scene object.
        const ImVec2 boxMin(
            center.x-78.0f,
            center.y+28.0f
        );

        const ImVec2 boxMax(
            center.x+24.0f,
            center.y+104.0f
        );

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
                renderMode==2
                    ? IM_COL32(125,145,185,88)
                    : IM_COL32(105,125,160,115),
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

        if (showAxes)
        {
            const ImVec2 gizmo(
                cp.x+78.0f,
                cp.y+cs.y-68.0f
            );

            d->AddLine(
                gizmo,
                ImVec2(gizmo.x+46.0f,gizmo.y),
                IM_COL32(190,65,55,255),
                2.0f
            );

            d->AddLine(
                gizmo,
                ImVec2(gizmo.x,gizmo.y-46.0f),
                IM_COL32(70,145,80,255),
                2.0f
            );

            d->AddLine(
                gizmo,
                ImVec2(gizmo.x-28.0f,gizmo.y+22.0f),
                IM_COL32(70,110,190,255),
                2.0f
            );

            d->AddText(
                ImVec2(gizmo.x+50.0f,gizmo.y-8.0f),
                IM_COL32(220,90,75,255),
                "X"
            );

            d->AddText(
                ImVec2(gizmo.x+6.0f,gizmo.y-58.0f),
                IM_COL32(100,190,110,255),
                "Y"
            );

            d->AddText(
                ImVec2(gizmo.x-40.0f,gizmo.y+18.0f),
                IM_COL32(105,150,225,255),
                "Z"
            );
        }

        if (renderMode==2)
        {
            d->AddLine(
                ImVec2(center.x-150.0f,center.y+126.0f),
                ImVec2(center.x+154.0f,center.y+94.0f),
                IM_COL32(238,174,38,220),
                2.0f
            );

            d->AddText(
                ImVec2(center.x+20.0f,center.y+102.0f),
                IM_COL32(240,194,100,230),
                "analysis vector"
            );
        }

        d->AddText(
            ImVec2(cp.x+16.0f,cp.y+16.0f),
            IM_COL32(225,226,228,255),
            "3D SCENE VIEW"
        );

        d->AddText(
            ImVec2(cp.x+16.0f,cp.y+40.0f),
            IM_COL32(135,139,145,255),
            renderMode==0
                ? "Perspective lit reconstruction"
                : (renderMode==1
                    ? "Wireframe inspection"
                    : "Analysis overlay")
        );
    }
    else
    {
        // ====================================================
        // AR PREVIEW
        // ====================================================

        const ImVec2 frameMin(
            cp.x+34.0f,
            cp.y+28.0f
        );

        const ImVec2 frameMax(
            cp.x+cs.x-34.0f,
            cp.y+cs.y-28.0f
        );

        d->AddRectFilled(
            frameMin,
            frameMax,
            IM_COL32(36,40,46,255),
            12.0f
        );

        d->AddRect(
            frameMin,
            frameMax,
            IM_COL32(92,99,108,255),
            12.0f,
            0,
            1.2f
        );

        const ImVec2 targetMin(
            center.x-130.0f,
            center.y-72.0f
        );

        const ImVec2 targetMax(
            center.x+130.0f,
            center.y+72.0f
        );

        const int alpha=
            static_cast<int>(
                220.0f*arOpacity
            );

        d->AddRect(
            targetMin,
            targetMax,
            IM_COL32(95,195,225,alpha),
            6.0f,
            0,
            2.0f
        );

        d->AddCircle(
            ImVec2(
                center.x,
                center.y+102.0f
            ),
            17.0f,
            IM_COL32(95,195,225,210),
            24,
            2.0f
        );

        d->AddText(
            ImVec2(
                center.x+26.0f,
                center.y+92.0f
            ),
            IM_COL32(120,205,232,230),
            "Origin anchor"
        );

        d->AddRectFilled(
            ImVec2(
                frameMax.x-174.0f,
                frameMin.y+14.0f
            ),
            ImVec2(
                frameMax.x-18.0f,
                frameMin.y+42.0f
            ),
            IM_COL32(54,60,68,220),
            14.0f
        );

        d->AddText(
            ImVec2(
                frameMax.x-154.0f,
                frameMin.y+20.0f
            ),
            IM_COL32(218,222,228,255),
            "DEVICE OFFLINE"
        );

        d->AddText(
            ImVec2(frameMin.x+16.0f,frameMin.y+16.0f),
            IM_COL32(225,226,228,255),
            "AR PREVIEW"
        );

        d->AddText(
            ImVec2(frameMin.x+16.0f,frameMin.y+40.0f),
            IM_COL32(135,139,145,255),
            "Editor preview - live AR device integration comes next."
        );
    }
'@

$text = [regex]::Replace(
    $text,
    $canvasPattern,
    [System.Text.RegularExpressions.MatchEvaluator]{
        param($m)
        return $newCanvas
    },
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

Write-Host "[OK] SceneCanvas now changes visibly between 2D / 3D / AR." -ForegroundColor Green

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'ViewportModeStrip',
    '"2D PLAN"',
    '"3D SCENE"',
    '"AR PREVIEW"',
    '"2D PLAN VIEW"',
    '"3D SCENE VIEW"',
    '"DEVICE OFFLINE"'
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] LIVE viewport function patched." -ForegroundColor Cyan
Write-Host "[OK] This patch targets the same SceneCanvas visible in your screenshot." -ForegroundColor Green
Write-Host "[OK] 2D / 3D / AR switcher added above the existing rail." -ForegroundColor Green
Write-Host "[OK] Each mode has a visibly different canvas." -ForegroundColor Green
Write-Host "[OK] Existing tool rail / Outliner / Properties / Timeline preserved." -ForegroundColor Green
Write-Host ""
Write-Host "VERIFY SOURCE BEFORE BUILDING:" -ForegroundColor Yellow
Write-Host '  findstr /C:"ViewportModeStrip" .\src\main.cpp'
Write-Host '  findstr /C:"AR PREVIEW" .\src\main.cpp'
Write-Host ""
Write-Host "Then rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
