param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - TABBED ANALYSIS WORKSPACE" -ForegroundColor Cyan
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
    'drawMetricTile(',
    'drawModuleCard(',
    'editorButton(',
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
$backup = Join-Path $ProjectRoot "src\main.cpp.before-analysis-tabs-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

$replacement = @'
static void drawAnalysisView()
{
    static bool selectOverviewOnFirstFrame = true;

    ImGui::Begin("Analysis");

    // ========================================================
    // FIXED WORKSPACE HEADER
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
            "Run reconstruction methods, review prerequisites, follow workflow and inspect results."
        );

        const float guideW=138.0f;
        ImGui::SetCursorScreenPos(ImVec2(right-guideW-12.0f,p.y+15.0f));
        editorButton("ANALYSIS GUIDE",guideW,true);
    }
    endSurface();

    ImGui::Spacing();

    // ========================================================
    // INTERNAL ANALYSIS TABS
    // ========================================================

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(14.0f,7.0f));

    if (ImGui::BeginTabBar(
        "AnalysisWorkspaceTabs",
        ImGuiTabBarFlags_FittingPolicyScroll))
    {
        const ImGuiTabItemFlags overviewFlags =
            selectOverviewOnFirstFrame
                ? ImGuiTabItemFlags_SetSelected
                : ImGuiTabItemFlags_None;

        // ====================================================
        // TAB 1: OVERVIEW
        // ====================================================

        if (ImGui::BeginTabItem("Overview",nullptr,overviewFlags))
        {
            ImGui::Spacing();

            ImGui::Text("CASE READINESS");
            ImGui::SameLine(0.0f,10.0f);
            ImGui::TextDisabled("A compact view of what is ready and what still blocks analysis.");
            ImGui::Separator();

            const float contentW=ImGui::GetContentRegionAvail().x;
            const float gap=8.0f;
            const float cardW=std::max(260.0f,(contentW-gap)/2.0f);

            drawMetricTile(
                "OverviewReady",
                UiGlyph::Check,
                "ANALYSIS READINESS",
                "NOT READY",
                "Required evidence is still missing",
                cardW,
                92.0f,
                true
            );

            ImGui::SameLine(0.0f,gap);

            drawMetricTile(
                "OverviewEvidence",
                UiGlyph::Document,
                "EVIDENCE LINKED",
                "0 OF 2",
                "No measurements linked",
                cardW,
                92.0f
            );

            ImGui::Spacing();

            drawMetricTile(
                "OverviewCase",
                UiGlyph::Folder,
                "CASE STATE",
                "UNASSIGNED",
                "Set incident date and location",
                cardW,
                92.0f
            );

            ImGui::SameLine(0.0f,gap);

            drawMetricTile(
                "OverviewUpdated",
                UiGlyph::Clock,
                "LAST UPDATED",
                "JUST NOW",
                "No analysis has been run",
                cardW,
                92.0f
            );

            ImGui::Spacing();

            beginSurface("OverviewNextStep",ImVec2(0.0f,118.0f),false,ImGuiWindowFlags_NoScrollbar);
            {
                const ImVec2 p=ImGui::GetCursorScreenPos();
                const float right=ImGui::GetWindowPos().x+ImGui::GetWindowSize().x;

                drawIconBadge(UiGlyph::Info,p,40.0f,false);

                ImGui::SetCursorScreenPos(ImVec2(p.x+52.0f,p.y+1.0f));
                ImGui::Text("NEXT REQUIRED STEP");

                ImGui::SetCursorScreenPos(ImVec2(p.x+52.0f,p.y+27.0f));
                ImGui::TextDisabled(
                    "Link evidence and measurements before running speed or momentum analysis."
                );

                ImGui::SetCursorScreenPos(ImVec2(p.x+52.0f,p.y+59.0f));
                drawStatus("BLOCKED BY MISSING EVIDENCE",StatusTone::Accent);

                const float linkW=144.0f;
                const float openW=154.0f;
                const float groupW=linkW+8.0f+openW;

                ImGui::SetCursorScreenPos(ImVec2(right-groupW-12.0f,p.y+42.0f));
                editorButton("LINK EVIDENCE",linkW,true);

                ImGui::SameLine(0.0f,8.0f);
                editorButton("OPEN EVIDENCE",openW);
            }
            endSurface();

            ImGui::EndTabItem();
        }

        // ====================================================
        // TAB 2: MODULES
        // ====================================================

        if (ImGui::BeginTabItem("Modules"))
        {
            ImGui::Spacing();

            ImGui::Text("ANALYSIS MODULES");
            ImGui::SameLine(0.0f,10.0f);
            ImGui::TextDisabled("Choose a reconstruction method based on the evidence available.");
            ImGui::Separator();

            const float contentW=ImGui::GetContentRegionAvail().x;
            const float gap=8.0f;
            const float moduleW=std::max(280.0f,(contentW-gap)/2.0f);

            drawModuleCard(
                "TabbedSkidModule",
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
                "TabbedMomentumModule",
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
                "TabbedSpeedModule",
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
                "TabbedResultsModule",
                UiGlyph::Report,
                "RECONSTRUCTION RESULTS",
                "Compile analysis outputs into the complete incident reconstruction and report.",
                "NOT STARTED",
                StatusTone::Neutral,
                "OPEN",
                moduleW
            );

            ImGui::EndTabItem();
        }

        // ====================================================
        // TAB 3: WORKFLOW
        // ====================================================

        if (ImGui::BeginTabItem("Workflow"))
        {
            ImGui::Spacing();

            ImGui::Text("ANALYSIS WORKFLOW");
            ImGui::SameLine(0.0f,10.0f);
            ImGui::TextDisabled("Move through the reconstruction process one stage at a time.");
            ImGui::Separator();

            struct WorkflowStep
            {
                const char* id;
                int number;
                UiGlyph glyph;
                const char* title;
                const char* note;
                const char* state;
                StatusTone tone;
            };

            const WorkflowStep steps[]={
                {"WorkflowCollect",1,UiGlyph::Document,"COLLECT EVIDENCE",
                 "Add skid marks, debris fields, scene markers and measurements.",
                 "CURRENT STEP",StatusTone::Accent},

                {"WorkflowLink",2,UiGlyph::Link,"LINK MEASUREMENTS",
                 "Associate the collected evidence with scene elements and vehicles.",
                 "WAITING",StatusTone::Neutral},

                {"WorkflowRun",3,UiGlyph::Bars,"RUN ANALYSIS",
                 "Configure the required reconstruction modules and execute calculations.",
                 "WAITING",StatusTone::Neutral},

                {"WorkflowReview",4,UiGlyph::Report,"REVIEW RESULTS",
                 "Validate calculated outputs and prepare reconstruction findings.",
                 "WAITING",StatusTone::Neutral}
            };

            const float contentW=ImGui::GetContentRegionAvail().x;
            const float gap=8.0f;
            const float cardW=std::max(280.0f,(contentW-gap)/2.0f);

            auto drawWorkflowCard = [](const WorkflowStep& step,float width)
            {
                beginSurface(
                    step.id,
                    ImVec2(width,148.0f),
                    true,
                    ImGuiWindowFlags_NoScrollbar
                );

                const ImVec2 p=ImGui::GetCursorScreenPos();
                ImDrawList* dl=ImGui::GetWindowDrawList();
                const ImVec4 tone=toneColor(step.tone);

                dl->AddCircle(
                    ImVec2(p.x+20.0f,p.y+20.0f),
                    18.0f,
                    toU32(tone),
                    24,
                    2.0f
                );

                char number[8]{};
                std::snprintf(number,sizeof(number),"%d",step.number);
                const ImVec2 numberSize=ImGui::CalcTextSize(number);

                dl->AddText(
                    ImVec2(
                        p.x+20.0f-numberSize.x*0.5f,
                        p.y+20.0f-numberSize.y*0.5f
                    ),
                    toU32(tone),
                    number
                );

                drawIconBadge(
                    step.glyph,
                    ImVec2(p.x+52.0f,p.y+1.0f),
                    38.0f,
                    step.tone==StatusTone::Accent
                );

                ImGui::SetCursorScreenPos(ImVec2(p.x+102.0f,p.y+3.0f));
                ImGui::Text("%s",step.title);

                ImGui::SetCursorScreenPos(ImVec2(p.x+102.0f,p.y+29.0f));
                drawStatus(step.state,step.tone);

                ImGui::SetCursorScreenPos(ImVec2(p.x+20.0f,p.y+74.0f));
                ImGui::PushTextWrapPos(p.x+width-20.0f);
                ImGui::TextDisabled("%s",step.note);
                ImGui::PopTextWrapPos();

                endSurface();
            };

            drawWorkflowCard(steps[0],cardW);
            ImGui::SameLine(0.0f,gap);
            drawWorkflowCard(steps[1],cardW);

            ImGui::Spacing();

            drawWorkflowCard(steps[2],cardW);
            ImGui::SameLine(0.0f,gap);
            drawWorkflowCard(steps[3],cardW);

            ImGui::EndTabItem();
        }

        // ====================================================
        // TAB 4: RESULTS
        // ====================================================

        if (ImGui::BeginTabItem("Results"))
        {
            ImGui::Spacing();

            ImGui::Text("RESULTS & REPORTING");
            ImGui::SameLine(0.0f,10.0f);
            ImGui::TextDisabled("Calculated reconstruction outputs will appear here.");
            ImGui::Separator();

            beginSurface("ResultsEmptyState",ImVec2(0.0f,184.0f),false,ImGuiWindowFlags_NoScrollbar);
            {
                const ImVec2 pos=ImGui::GetWindowPos();
                const ImVec2 size=ImGui::GetWindowSize();

                const ImVec2 iconPos(
                    pos.x+size.x*0.5f-21.0f,
                    pos.y+24.0f
                );

                drawIconBadge(UiGlyph::Report,iconPos,42.0f,false);

                const char* title="No reconstruction results yet";
                const char* note=
                    "Run an analysis module to generate calculated speeds, momentum values and reportable findings.";

                const float titleW=ImGui::CalcTextSize(title).x;
                const float noteW=ImGui::CalcTextSize(note).x;

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        pos.x+(size.x-titleW)*0.5f,
                        pos.y+78.0f
                    )
                );
                ImGui::Text("%s",title);

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        pos.x+std::max(18.0f,(size.x-noteW)*0.5f),
                        pos.y+107.0f
                    )
                );
                ImGui::TextDisabled("%s",note);

                const float reportW=138.0f;
                ImGui::SetCursorScreenPos(
                    ImVec2(
                        pos.x+(size.x-reportW)*0.5f,
                        pos.y+139.0f
                    )
                );
                editorButton("VIEW REPORTS",reportW,false,false);
            }
            endSurface();

            ImGui::Spacing();

            ImGui::Text("QUICK ACTIONS");
            ImGui::Separator();

            const float contentW=ImGui::GetContentRegionAvail().x;
            const float gap=8.0f;
            const float actionW=std::max(
                210.0f,
                (contentW-(gap*2.0f))/3.0f
            );

            auto drawActionCard = [](
                const char* id,
                UiGlyph glyph,
                const char* title,
                const char* note,
                const char* action,
                float width,
                bool primary,
                bool enabled)
            {
                beginSurface(
                    id,
                    ImVec2(width,126.0f),
                    true,
                    ImGuiWindowFlags_NoScrollbar
                );

                const ImVec2 p=ImGui::GetCursorScreenPos();

                drawIconBadge(glyph,p,36.0f,primary);

                ImGui::SetCursorScreenPos(ImVec2(p.x+48.0f,p.y+1.0f));
                ImGui::Text("%s",title);

                ImGui::SetCursorScreenPos(ImVec2(p.x,p.y+48.0f));
                ImGui::PushTextWrapPos(p.x+width-18.0f);
                ImGui::TextDisabled("%s",note);
                ImGui::PopTextWrapPos();

                ImGui::SetCursorPosY(84.0f);
                editorButton(
                    action,
                    ImGui::GetContentRegionAvail().x,
                    primary,
                    enabled
                );

                endSurface();
            };

            drawActionCard(
                "ResultActionSkid",
                UiGlyph::Ruler,
                "START SKID ANALYSIS",
                "Configure skid-distance and friction inputs.",
                "START",
                actionW,
                true,
                true
            );

            ImGui::SameLine(0.0f,gap);

            drawActionCard(
                "ResultActionEvidence",
                UiGlyph::Link,
                "LINK EVIDENCE",
                "Associate measurements with the reconstruction scene.",
                "OPEN EVIDENCE",
                actionW,
                false,
                true
            );

            ImGui::SameLine(0.0f,gap);

            drawActionCard(
                "ResultActionAll",
                UiGlyph::Bars,
                "RUN ALL ANALYSES",
                "Execute all configured analysis modules.",
                "RUN ALL",
                actionW,
                false,
                false
            );

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
        selectOverviewOnFirstFrame=false;
    }

    ImGui::PopStyleVar();

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
    'AnalysisWorkspaceTabs',
    'ImGui::BeginTabItem("Overview"',
    'ImGui::BeginTabItem("Modules")',
    'ImGui::BeginTabItem("Workflow")',
    'ImGui::BeginTabItem("Results")',
    'WorkflowCollect',
    'ResultsEmptyState'
)

foreach ($marker in $verifyMarkers) {
    if (-not $verify.Contains($marker)) {
        throw "Verification failed after write: $marker"
    }
}

Write-Host ""
Write-Host "[DONE] Analysis workspace converted to internal tabs." -ForegroundColor Cyan
Write-Host "[OK] Overview / Modules / Workflow / Results tabs added." -ForegroundColor Green
Write-Host "[OK] Cards used inside every tab." -ForegroundColor Green
Write-Host "[OK] Only one analysis layer is visible at a time." -ForegroundColor Green
Write-Host "[OK] Palette / global docking / other screens untouched." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
