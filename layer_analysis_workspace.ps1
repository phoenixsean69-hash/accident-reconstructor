param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - LAYER ANALYSIS WORKSPACE" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'static void drawAnalysisView()',
    'static void drawRailButton',
    'drawModuleCard(',
    'beginSurface('
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected marker not found: $marker"
    }
}

$pattern = 'static void drawAnalysisView\(\)\s*\{[\s\S]*?(?=\r?\nstatic void drawRailButton)'

$matches = [regex]::Matches(
    $text,
    $pattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($matches.Count -ne 1) {
    throw "Expected exactly one drawAnalysisView() function, found $($matches.Count)."
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-analysis-layering-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

$replacement = @'
static void drawAnalysisView()
{
    ImGui::Begin("Analysis");

    // ========================================================
    // WORKSPACE HEADER
    // ========================================================

    beginSurface("AnalysisHeader",ImVec2(0.0f,76.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 p=ImGui::GetCursorScreenPos();
        const float right=ImGui::GetWindowPos().x+ImGui::GetWindowSize().x;

        drawIconBadge(UiGlyph::Bars,p,42.0f,true);

        ImGui::SetCursorScreenPos(ImVec2(p.x+54.0f,p.y+2.0f));
        ImGui::Text("ANALYSIS WORKSPACE");

        ImGui::SetCursorScreenPos(ImVec2(p.x+54.0f,p.y+29.0f));
        ImGui::TextDisabled(
            "Perform technical analysis using case evidence to reconstruct vehicle dynamics and incident sequence."
        );

        const float guideW=138.0f;
        ImGui::SetCursorScreenPos(ImVec2(right-guideW-12.0f,p.y+15.0f));
        editorButton("ANALYSIS GUIDE",guideW,true);
    }
    endSurface();

    ImGui::Spacing();

    // ========================================================
    // READINESS / PREREQUISITES BAND
    // ========================================================

    beginSurface("AnalysisReadinessBand",ImVec2(0.0f,112.0f),true,ImGuiWindowFlags_NoScrollbar);
    {
        ImGui::Text("READINESS & PREREQUISITES");
        ImGui::SameLine(0.0f,10.0f);
        ImGui::TextDisabled("Complete the required case inputs before running analysis.");
        ImGui::Separator();

        if (ImGui::BeginTable(
            "AnalysisReadinessTable",
            4,
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("##Readiness",ImGuiTableColumnFlags_WidthStretch,0.34f);
            ImGui::TableSetupColumn("##Evidence",ImGuiTableColumnFlags_WidthStretch,0.22f);
            ImGui::TableSetupColumn("##Case",ImGuiTableColumnFlags_WidthStretch,0.22f);
            ImGui::TableSetupColumn("##Updated",ImGuiTableColumnFlags_WidthStretch,0.22f);

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            {
                const ImVec2 p=ImGui::GetCursorScreenPos();
                drawIconBadge(UiGlyph::Check,p,34.0f,true);

                ImGui::SetCursorScreenPos(ImVec2(p.x+46.0f,p.y));
                ImGui::TextDisabled("ANALYSIS READINESS");

                ImGui::SetCursorScreenPos(ImVec2(p.x+46.0f,p.y+23.0f));
                drawStatus("NOT READY",StatusTone::Accent);

                ImGui::SetCursorScreenPos(ImVec2(p.x+46.0f,p.y+46.0f));
                ImGui::TextDisabled("Link required evidence and measurements.");
            }

            ImGui::TableSetColumnIndex(1);
            {
                const ImVec2 p=ImGui::GetCursorScreenPos();
                drawIconBadge(UiGlyph::Document,p,30.0f,false);

                ImGui::SetCursorScreenPos(ImVec2(p.x+42.0f,p.y));
                ImGui::TextDisabled("EVIDENCE LINKED");

                ImGui::SetCursorScreenPos(ImVec2(p.x+42.0f,p.y+24.0f));
                ImGui::Text("0 OF 2");

                ImGui::SetCursorScreenPos(ImVec2(p.x+42.0f,p.y+47.0f));
                ImGui::TextDisabled("No measurements linked");
            }

            ImGui::TableSetColumnIndex(2);
            {
                const ImVec2 p=ImGui::GetCursorScreenPos();
                drawIconBadge(UiGlyph::Folder,p,30.0f,false);

                ImGui::SetCursorScreenPos(ImVec2(p.x+42.0f,p.y));
                ImGui::TextDisabled("CASE STATE");

                ImGui::SetCursorScreenPos(ImVec2(p.x+42.0f,p.y+24.0f));
                ImGui::Text("UNASSIGNED");

                ImGui::SetCursorScreenPos(ImVec2(p.x+42.0f,p.y+47.0f));
                ImGui::TextDisabled("Set date and location");
            }

            ImGui::TableSetColumnIndex(3);
            {
                const ImVec2 p=ImGui::GetCursorScreenPos();
                drawIconBadge(UiGlyph::Clock,p,30.0f,false);

                ImGui::SetCursorScreenPos(ImVec2(p.x+42.0f,p.y));
                ImGui::TextDisabled("LAST UPDATED");

                ImGui::SetCursorScreenPos(ImVec2(p.x+42.0f,p.y+24.0f));
                ImGui::Text("JUST NOW");

                ImGui::SetCursorScreenPos(ImVec2(p.x+42.0f,p.y+47.0f));
                ImGui::TextDisabled("No analysis has been run");
            }

            ImGui::EndTable();
        }
    }
    endSurface();

    ImGui::Spacing();

    // ========================================================
    // ANALYSIS MODULES - 2 x 2
    // ========================================================

    ImGui::Text("ANALYSIS MODULES");
    ImGui::SameLine(0.0f,10.0f);
    ImGui::TextDisabled("Choose a method based on the evidence available.");
    ImGui::Separator();

    const float contentW=ImGui::GetContentRegionAvail().x;
    const float gap=8.0f;
    const float moduleW=std::max(260.0f,(contentW-gap)/2.0f);

    drawModuleCard(
        "SkidModule",
        UiGlyph::Ruler,
        "SKID ANALYSIS",
        "Calculate vehicle speed from skid distance, friction and road-surface data.",
        "NOT STARTED",
        StatusTone::Neutral,
        "CONFIGURE",
        moduleW
    );

    ImGui::SameLine(0.0f,gap);

    drawModuleCard(
        "MomentumModule",
        UiGlyph::Momentum,
        "MOMENTUM ANALYSIS",
        "Analyze vehicle motion using conservation of momentum and collision dynamics.",
        "AWAITING EVIDENCE",
        StatusTone::Accent,
        "OPEN",
        moduleW
    );

    ImGui::Spacing();

    drawModuleCard(
        "SpeedModule",
        UiGlyph::Speed,
        "SPEED ANALYSIS",
        "Determine vehicle speed from crush, throw distance and simulation evidence.",
        "AWAITING EVIDENCE",
        StatusTone::Accent,
        "OPEN",
        moduleW
    );

    ImGui::SameLine(0.0f,gap);

    drawModuleCard(
        "ResultsModule",
        UiGlyph::Report,
        "RECONSTRUCTION RESULTS",
        "Compile analysis outputs into the complete incident reconstruction and technical report.",
        "NOT STARTED",
        StatusTone::Neutral,
        "OPEN",
        moduleW
    );

    ImGui::Spacing();

    // ========================================================
    // PIPELINE
    // ========================================================

    beginSurface("AnalysisPipeline",ImVec2(0.0f,124.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        ImGui::Text("ANALYSIS PIPELINE");
        ImGui::SameLine(0.0f,10.0f);
        ImGui::TextDisabled("Follow the workflow from evidence collection through reporting.");
        ImGui::Separator();

        struct Step
        {
            int number;
            const char* title;
            const char* note;
            StatusTone tone;
        };

        const Step steps[]={
            {1,"Collect Evidence","Add skid marks, debris and measurements.",StatusTone::Accent},
            {2,"Link Measurements","Associate evidence with scene elements.",StatusTone::Neutral},
            {3,"Run Analysis","Configure and execute analysis modules.",StatusTone::Neutral},
            {4,"Review Results","Validate outputs and generate reports.",StatusTone::Neutral}
        };

        const float pipelineW=ImGui::GetContentRegionAvail().x;
        const float stepW=pipelineW/4.0f;
        const ImVec2 start=ImGui::GetCursorScreenPos();
        ImDrawList* dl=ImGui::GetWindowDrawList();

        for (int i=0;i<4;++i)
        {
            const float x=start.x+(stepW*i);
            const ImVec4 tone=toneColor(steps[i].tone);
            const ImVec2 circle(x+18.0f,start.y+28.0f);

            dl->AddCircle(circle,16.0f,toU32(tone),24,2.0f);

            char number[8]{};
            std::snprintf(number,sizeof(number),"%d",steps[i].number);
            const ImVec2 numberSize=ImGui::CalcTextSize(number);

            dl->AddText(
                ImVec2(
                    circle.x-numberSize.x*0.5f,
                    circle.y-numberSize.y*0.5f
                ),
                toU32(tone),
                number
            );

            ImGui::SetCursorScreenPos(ImVec2(x+44.0f,start.y+7.0f));
            ImGui::Text("%s",steps[i].title);

            ImGui::SetCursorScreenPos(ImVec2(x+44.0f,start.y+35.0f));
            ImGui::PushTextWrapPos(x+stepW-22.0f);
            ImGui::TextDisabled("%s",steps[i].note);
            ImGui::PopTextWrapPos();

            if (i<3)
            {
                const float arrowX=x+stepW-20.0f;

                dl->AddLine(
                    ImVec2(arrowX-10.0f,start.y+28.0f),
                    ImVec2(arrowX+7.0f,start.y+28.0f),
                    toU32(colorMuted()),
                    1.5f
                );

                dl->AddTriangleFilled(
                    ImVec2(arrowX+10.0f,start.y+28.0f),
                    ImVec2(arrowX+3.0f,start.y+23.0f),
                    ImVec2(arrowX+3.0f,start.y+33.0f),
                    toU32(colorMuted())
                );
            }
        }
    }
    endSurface();

    ImGui::Spacing();

    // ========================================================
    // RESULTS + ACTIONS
    // ========================================================

    beginSurface("AnalysisFooter",ImVec2(0.0f,132.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 p=ImGui::GetCursorScreenPos();
        const float right=ImGui::GetWindowPos().x+ImGui::GetWindowSize().x;

        drawIconBadge(UiGlyph::Report,p,38.0f,false);

        ImGui::SetCursorScreenPos(ImVec2(p.x+50.0f,p.y));
        ImGui::Text("RESULTS & INSIGHTS");

        ImGui::SetCursorScreenPos(ImVec2(p.x+50.0f,p.y+25.0f));
        ImGui::TextDisabled(
            "No analysis results yet. Link evidence and run a module to generate outputs."
        );

        const float reportsW=126.0f;
        ImGui::SetCursorScreenPos(ImVec2(right-reportsW-12.0f,p.y+8.0f));
        editorButton("VIEW REPORTS",reportsW,false,false);

        ImGui::SetCursorScreenPos(ImVec2(p.x,p.y+64.0f));
        ImGui::Separator();

        ImGui::SetCursorScreenPos(ImVec2(p.x,p.y+79.0f));
        ImGui::Text("QUICK ACTIONS");

        ImGui::SameLine(0.0f,18.0f);
        editorButton("START SKID ANALYSIS",188.0f,true);

        ImGui::SameLine(0.0f,8.0f);
        editorButton("LINK EVIDENCE",144.0f);

        ImGui::SameLine(0.0f,8.0f);
        editorButton("RUN ALL ANALYSES",164.0f,false,false);
    }
    endSurface();

    ImGui::End();
}

'@

$newText = [regex]::Replace(
    $text,
    $pattern,
    $replacement,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($newText -eq $text) {
    throw "Replacement failed. Original source was not changed."
}

Set-Content -Path $MainCpp -Value $newText -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$verifyMarkers = @(
    'AnalysisReadinessBand',
    'AnalysisReadinessTable',
    'const float moduleW',
    'AnalysisPipeline',
    'AnalysisFooter',
    'Choose a method based on the evidence available.'
)

foreach ($marker in $verifyMarkers) {
    if (-not $verify.Contains($marker)) {
        throw "Verification failed after write: $marker"
    }
}

Write-Host ""
Write-Host "[DONE] Analysis workspace layered and cleaned." -ForegroundColor Cyan
Write-Host "[OK] Readiness consolidated into one prerequisite band." -ForegroundColor Green
Write-Host "[OK] Analysis modules reorganized into a 2 x 2 grid." -ForegroundColor Green
Write-Host "[OK] Pipeline kept as a distinct workflow layer." -ForegroundColor Green
Write-Host "[OK] Results + quick actions combined into one footer." -ForegroundColor Green
Write-Host "[OK] Palette / docking / other screens untouched." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
