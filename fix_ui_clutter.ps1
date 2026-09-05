param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - UI ALIGNMENT / DE-CLUTTER PASS" -ForegroundColor Yellow
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'static void drawMetricTile',
    'CASE OVERVIEW',
    'EVIDENCE WORKSPACE',
    'ANALYSIS WORKSPACE'
)

foreach ($needle in $required) {
    if ($text -notlike "*$needle*") {
        throw "Expected approved UI marker not found: $needle"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-declutter-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

function Replace-Exact {
    param(
        [string]$Old,
        [string]$New,
        [string]$Name
    )

    if (-not $script:text.Contains($Old)) {
        throw "Could not find expected block: $Name"
    }

    $script:text = $script:text.Replace($Old, $New)
    Write-Host "[OK] $Name" -ForegroundColor Green
}

# ------------------------------------------------------------
# 1. Metric cards:
#    ImGui resets X after each text line, so explicitly anchor
#    label/value/note to the text column every time.
# ------------------------------------------------------------

$old = @'
    ImGui::SetCursorScreenPos(ImVec2(start.x+50.0f,start.y+1.0f));
    ImGui::TextDisabled("%s", label);
    ImGui::Text("%s", value);

    if (note && note[0] != '\0')
        ImGui::TextDisabled("%s", note);
'@

$new = @'
    const float textX = start.x + 50.0f;

    ImGui::SetCursorScreenPos(ImVec2(textX,start.y+1.0f));
    ImGui::TextDisabled("%s", label);

    ImGui::SetCursorScreenPos(ImVec2(textX,start.y+22.0f));
    ImGui::Text("%s", value);

    if (note && note[0] != '\0')
    {
        ImGui::SetCursorScreenPos(ImVec2(textX,start.y+42.0f));
        ImGui::TextDisabled("%s", note);
    }
'@

Replace-Exact $old $new "Aligned metric-card label/value/note columns"

# ------------------------------------------------------------
# 2. Case header subtitle
# ------------------------------------------------------------

$old = @'
    ImGui::SetCursorScreenPos(ImVec2(headerStart.x+52.0f,headerStart.y+1.0f));
    ImGui::Text("CASE OVERVIEW");
    ImGui::TextDisabled("Build the reconstruction from one place. Review scene, evidence and analysis status.");
'@

$new = @'
    ImGui::SetCursorScreenPos(ImVec2(headerStart.x+52.0f,headerStart.y+1.0f));
    ImGui::Text("CASE OVERVIEW");

    ImGui::SetCursorScreenPos(ImVec2(headerStart.x+52.0f,headerStart.y+23.0f));
    ImGui::TextDisabled("Build the reconstruction from one place. Review scene, evidence and analysis status.");
'@

Replace-Exact $old $new "Aligned Case View header subtitle"

# ------------------------------------------------------------
# 3. Incident summary text
# ------------------------------------------------------------

$old = @'
    ImGui::SetCursorScreenPos(ImVec2(summaryStart.x+46.0f,summaryStart.y));
    ImGui::Text("INCIDENT SUMMARY");
    ImGui::TextDisabled(
        "No incident summary has been entered. Add essential facts, scene location and reconstruction notes.");
'@

$new = @'
    ImGui::SetCursorScreenPos(ImVec2(summaryStart.x+46.0f,summaryStart.y));
    ImGui::Text("INCIDENT SUMMARY");

    ImGui::SetCursorScreenPos(ImVec2(summaryStart.x+46.0f,summaryStart.y+22.0f));
    ImGui::TextDisabled(
        "No incident summary has been entered. Add essential facts, scene location and reconstruction notes.");
'@

Replace-Exact $old $new "Aligned Incident Summary text"

# ------------------------------------------------------------
# 4. Evidence header subtitle
# ------------------------------------------------------------

$old = @'
    ImGui::SetCursorScreenPos(ImVec2(headerStart.x+52.0f,headerStart.y+1.0f));
    ImGui::Text("EVIDENCE WORKSPACE");
    ImGui::TextDisabled(
        "Manage and review photographs, skid marks, debris, scene markers and measurements.");
'@

$new = @'
    ImGui::SetCursorScreenPos(ImVec2(headerStart.x+52.0f,headerStart.y+1.0f));
    ImGui::Text("EVIDENCE WORKSPACE");

    ImGui::SetCursorScreenPos(ImVec2(headerStart.x+52.0f,headerStart.y+23.0f));
    ImGui::TextDisabled(
        "Manage and review photographs, skid marks, debris, scene markers and measurements.");
'@

Replace-Exact $old $new "Aligned Evidence header subtitle"

# ------------------------------------------------------------
# 5. Evidence category count alignment
# ------------------------------------------------------------

$old = @'
        ImGui::SetCursorScreenPos(ImVec2(start.x+43.0f,start.y));
        ImGui::Text("%s",categories[i].name);
        ImGui::TextDisabled("0 items");

        ImGui::SetCursorPosY(67.0f);
'@

$new = @'
        ImGui::SetCursorScreenPos(ImVec2(start.x+43.0f,start.y));
        ImGui::Text("%s",categories[i].name);

        ImGui::SetCursorScreenPos(ImVec2(start.x+43.0f,start.y+21.0f));
        ImGui::TextDisabled("0 items");

        ImGui::SetCursorPosY(67.0f);
'@

Replace-Exact $old $new "Aligned Evidence category counts"

# ------------------------------------------------------------
# 6. Evidence inspector: give it a real 3-line hierarchy
# ------------------------------------------------------------

$old = @'
    beginSurface("EvidenceInspector",ImVec2(0.0f,64.0f),false,ImGuiWindowFlags_NoScrollbar);

    const ImVec2 inspectorStart=ImGui::GetCursorScreenPos();
    drawIconBadge(UiGlyph::Info,inspectorStart,34.0f,false);

    ImGui::SetCursorScreenPos(ImVec2(inspectorStart.x+46.0f,inspectorStart.y));
    ImGui::TextDisabled("EVIDENCE INSPECTOR");
    ImGui::Text("SELECT AN ITEM TO INSPECT");
    ImGui::SameLine(0.0f,12.0f);
    ImGui::TextDisabled("Choose an evidence item to view details, metadata and scene links.");

    endSurface();
'@

$new = @'
    beginSurface("EvidenceInspector",ImVec2(0.0f,78.0f),false,ImGuiWindowFlags_NoScrollbar);

    const ImVec2 inspectorStart=ImGui::GetCursorScreenPos();
    drawIconBadge(UiGlyph::Info,inspectorStart,34.0f,false);

    ImGui::SetCursorScreenPos(ImVec2(inspectorStart.x+46.0f,inspectorStart.y));
    ImGui::TextDisabled("EVIDENCE INSPECTOR");

    ImGui::SetCursorScreenPos(ImVec2(inspectorStart.x+46.0f,inspectorStart.y+20.0f));
    ImGui::Text("SELECT AN ITEM TO INSPECT");

    ImGui::SetCursorScreenPos(ImVec2(inspectorStart.x+46.0f,inspectorStart.y+41.0f));
    ImGui::TextDisabled("Choose an evidence item to view details, metadata and scene links.");

    endSurface();
'@

Replace-Exact $old $new "De-cluttered Evidence Inspector"

# ------------------------------------------------------------
# 7. Analysis header subtitle
# ------------------------------------------------------------

$old = @'
    ImGui::SetCursorScreenPos(ImVec2(headerStart.x+52.0f,headerStart.y+1.0f));
    ImGui::Text("ANALYSIS WORKSPACE");
    ImGui::TextDisabled(
        "Perform technical analysis using case evidence to reconstruct vehicle dynamics and incident sequence.");
'@

$new = @'
    ImGui::SetCursorScreenPos(ImVec2(headerStart.x+52.0f,headerStart.y+1.0f));
    ImGui::Text("ANALYSIS WORKSPACE");

    ImGui::SetCursorScreenPos(ImVec2(headerStart.x+52.0f,headerStart.y+23.0f));
    ImGui::TextDisabled(
        "Perform technical analysis using case evidence to reconstruct vehicle dynamics and incident sequence.");
'@

Replace-Exact $old $new "Aligned Analysis header subtitle"

# ------------------------------------------------------------
# 8. Analysis results subtitle
# ------------------------------------------------------------

$old = @'
    ImGui::SetCursorScreenPos(ImVec2(resultsStart.x+50.0f,resultsStart.y));
    ImGui::Text("RESULTS & INSIGHTS");
    ImGui::TextDisabled(
        "No analysis results yet. Link evidence and run an analysis module to generate outputs.");
'@

$new = @'
    ImGui::SetCursorScreenPos(ImVec2(resultsStart.x+50.0f,resultsStart.y));
    ImGui::Text("RESULTS & INSIGHTS");

    ImGui::SetCursorScreenPos(ImVec2(resultsStart.x+50.0f,resultsStart.y+23.0f));
    ImGui::TextDisabled(
        "No analysis results yet. Link evidence and run an analysis module to generate outputs.");
'@

Replace-Exact $old $new "Aligned Analysis Results text"

# ------------------------------------------------------------
# 9. Give the 3 workspace headers a little more room
# ------------------------------------------------------------

$text = $text.Replace(
    'beginSurface("CaseHeader", ImVec2(0.0f,62.0f)',
    'beginSurface("CaseHeader", ImVec2(0.0f,68.0f)'
)

$text = $text.Replace(
    'beginSurface("EvidenceHeader",ImVec2(0.0f,62.0f)',
    'beginSurface("EvidenceHeader",ImVec2(0.0f,68.0f)'
)

$text = $text.Replace(
    'beginSurface("AnalysisHeader",ImVec2(0.0f,62.0f)',
    'beginSurface("AnalysisHeader",ImVec2(0.0f,68.0f)'
)

Write-Host "[OK] Increased workspace header breathing room" -ForegroundColor Green

# ------------------------------------------------------------
# 10. Case metrics: slight height increase for cleaner baselines
# ------------------------------------------------------------

$text = $text.Replace(
    'static void drawMetricTile(',
    'static void drawMetricTile('
)

# Case cards rely on default height, so make default 74 instead of 70.
$text = $text.Replace(
    'float height=70.0f,',
    'float height=74.0f,'
)

Write-Host "[OK] Increased metric-card vertical spacing" -ForegroundColor Green

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

Write-Host ""
Write-Host "[DONE] Alignment/de-clutter pass installed." -ForegroundColor Cyan
Write-Host "[INFO] No docking layout or Viewport design was changed." -ForegroundColor DarkGray
Write-Host ""
Write-Host "Rebuild and run the app, then compare the same 3 screens." -ForegroundColor Cyan
Write-Host ""
