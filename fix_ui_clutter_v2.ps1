param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - UI DE-CLUTTER PASS v2" -ForegroundColor Yellow
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$markers = @(
    'static void drawMetricTile',
    'CASE OVERVIEW',
    'EVIDENCE WORKSPACE',
    'ANALYSIS WORKSPACE'
)

foreach ($marker in $markers) {
    if (-not $text.Contains($marker)) {
        throw "Expected approved UI marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-declutter-v2-$timestamp.bak"
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
# METRIC CARDS
# Fix value/note text snapping back under the icon.
# ------------------------------------------------------------

$old = @'
static void drawMetricTile(const char* id, UiGlyph glyph, const char* label, const char* value, const char* note, float width, float height=70.0f, bool accentIcon=false)
{
    beginSurface(id,ImVec2(width,height),true,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 p=ImGui::GetCursorScreenPos();
    drawIconBadge(glyph,p,38.0f,accentIcon);
    ImGui::SetCursorScreenPos(ImVec2(p.x+50.0f,p.y+1.0f));
    ImGui::TextDisabled("%s",label);
    ImGui::Text("%s",value);
    if (note && note[0]) ImGui::TextDisabled("%s",note);
    endSurface();
}
'@

$new = @'
static void drawMetricTile(const char* id, UiGlyph glyph, const char* label, const char* value, const char* note, float width, float height=76.0f, bool accentIcon=false)
{
    beginSurface(id,ImVec2(width,height),true,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 p=ImGui::GetCursorScreenPos();
    const float textX=p.x+50.0f;

    drawIconBadge(glyph,p,38.0f,accentIcon);

    ImGui::SetCursorScreenPos(ImVec2(textX,p.y+1.0f));
    ImGui::TextDisabled("%s",label);

    ImGui::SetCursorScreenPos(ImVec2(textX,p.y+23.0f));
    ImGui::Text("%s",value);

    if (note && note[0])
    {
        ImGui::SetCursorScreenPos(ImVec2(textX,p.y+45.0f));
        ImGui::TextDisabled("%s",note);
    }

    endSurface();
}
'@

Replace-Exact $old $new "Aligned all metric cards"

# ------------------------------------------------------------
# CASE VIEW HEADER
# ------------------------------------------------------------

$old = @'
    beginSurface("CaseHeader",ImVec2(0.0f,62.0f),false,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 h=ImGui::GetCursorScreenPos();
    drawIconBadge(UiGlyph::Folder,h,40.0f,true);
    ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+1.0f));
    ImGui::Text("CASE OVERVIEW");
    ImGui::TextDisabled("Build the reconstruction from one place. Review scene, evidence and analysis status.");
'@

$new = @'
    beginSurface("CaseHeader",ImVec2(0.0f,70.0f),false,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 h=ImGui::GetCursorScreenPos();
    drawIconBadge(UiGlyph::Folder,h,40.0f,true);

    ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+1.0f));
    ImGui::Text("CASE OVERVIEW");

    ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+24.0f));
    ImGui::TextDisabled("Build the reconstruction from one place. Review scene, evidence and analysis status.");
'@

Replace-Exact $old $new "De-cluttered Case View header"

# ------------------------------------------------------------
# CASE INCIDENT SUMMARY
# ------------------------------------------------------------

$old = @'
    ImGui::SetCursorScreenPos(ImVec2(s.x+46.0f,s.y));
    ImGui::Text("INCIDENT SUMMARY");
    ImGui::TextDisabled("No incident summary has been entered. Add essential facts, scene location and reconstruction notes.");
'@

$new = @'
    ImGui::SetCursorScreenPos(ImVec2(s.x+46.0f,s.y));
    ImGui::Text("INCIDENT SUMMARY");

    ImGui::SetCursorScreenPos(ImVec2(s.x+46.0f,s.y+22.0f));
    ImGui::TextDisabled("No incident summary has been entered. Add essential facts, scene location and reconstruction notes.");
'@

Replace-Exact $old $new "Aligned Incident Summary"

# ------------------------------------------------------------
# EVIDENCE HEADER
# ------------------------------------------------------------

$old = @'
    beginSurface("EvidenceHeader",ImVec2(0.0f,62.0f),false,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 h=ImGui::GetCursorScreenPos();
    drawIconBadge(UiGlyph::Image,h,40.0f,true);
    ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+1.0f));
    ImGui::Text("EVIDENCE WORKSPACE");
    ImGui::TextDisabled("Manage and review photographs, skid marks, debris, scene markers and measurements.");
'@

$new = @'
    beginSurface("EvidenceHeader",ImVec2(0.0f,70.0f),false,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 h=ImGui::GetCursorScreenPos();
    drawIconBadge(UiGlyph::Image,h,40.0f,true);

    ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+1.0f));
    ImGui::Text("EVIDENCE WORKSPACE");

    ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+24.0f));
    ImGui::TextDisabled("Manage and review photographs, skid marks, debris, scene markers and measurements.");
'@

Replace-Exact $old $new "De-cluttered Evidence header"

# ------------------------------------------------------------
# EVIDENCE CATEGORY CARDS
# Keep title/count firmly to the right of each icon.
# ------------------------------------------------------------

$old = @'
        ImGui::SetCursorScreenPos(ImVec2(p.x+43.0f,p.y)); ImGui::Text("%s",cats[i].name); ImGui::TextDisabled("0 items");
        ImGui::SetCursorPosY(67.0f); ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+cw-22.0f); ImGui::TextDisabled("%s",cats[i].note); ImGui::PopTextWrapPos();
'@

$new = @'
        ImGui::SetCursorScreenPos(ImVec2(p.x+43.0f,p.y));
        ImGui::Text("%s",cats[i].name);

        ImGui::SetCursorScreenPos(ImVec2(p.x+43.0f,p.y+22.0f));
        ImGui::TextDisabled("0 items");

        ImGui::SetCursorPosY(67.0f);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+cw-22.0f);
        ImGui::TextDisabled("%s",cats[i].note);
        ImGui::PopTextWrapPos();
'@

Replace-Exact $old $new "Aligned Evidence category cards"

# ------------------------------------------------------------
# EVIDENCE INSPECTOR
# ------------------------------------------------------------

$old = @'
    beginSurface("EvidenceInspector",ImVec2(0.0f,64.0f),false,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 p=ImGui::GetCursorScreenPos();
    drawIconBadge(UiGlyph::Info,p,34.0f,false);
    ImGui::SetCursorScreenPos(ImVec2(p.x+46.0f,p.y)); ImGui::TextDisabled("EVIDENCE INSPECTOR"); ImGui::Text("SELECT AN ITEM TO INSPECT");
    ImGui::SameLine(0.0f,12.0f); ImGui::TextDisabled("Choose an evidence item to view details, metadata and scene links.");
    endSurface();
'@

$new = @'
    beginSurface("EvidenceInspector",ImVec2(0.0f,82.0f),false,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 p=ImGui::GetCursorScreenPos();
    drawIconBadge(UiGlyph::Info,p,34.0f,false);

    ImGui::SetCursorScreenPos(ImVec2(p.x+46.0f,p.y));
    ImGui::TextDisabled("EVIDENCE INSPECTOR");

    ImGui::SetCursorScreenPos(ImVec2(p.x+46.0f,p.y+21.0f));
    ImGui::Text("SELECT AN ITEM TO INSPECT");

    ImGui::SetCursorScreenPos(ImVec2(p.x+46.0f,p.y+43.0f));
    ImGui::TextDisabled("Choose an evidence item to view details, metadata and scene links.");

    endSurface();
'@

Replace-Exact $old $new "De-cluttered Evidence Inspector"

# ------------------------------------------------------------
# ANALYSIS HEADER
# ------------------------------------------------------------

$old = @'
    beginSurface("AnalysisHeader",ImVec2(0.0f,62.0f),false,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 h=ImGui::GetCursorScreenPos();
    drawIconBadge(UiGlyph::Bars,h,40.0f,true);
    ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+1.0f)); ImGui::Text("ANALYSIS WORKSPACE");
    ImGui::TextDisabled("Perform technical analysis using case evidence to reconstruct vehicle dynamics and incident sequence.");
'@

$new = @'
    beginSurface("AnalysisHeader",ImVec2(0.0f,70.0f),false,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 h=ImGui::GetCursorScreenPos();
    drawIconBadge(UiGlyph::Bars,h,40.0f,true);

    ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+1.0f));
    ImGui::Text("ANALYSIS WORKSPACE");

    ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+24.0f));
    ImGui::TextDisabled("Perform technical analysis using case evidence to reconstruct vehicle dynamics and incident sequence.");
'@

Replace-Exact $old $new "De-cluttered Analysis header"

# ------------------------------------------------------------
# ANALYSIS RESULTS PANEL
# ------------------------------------------------------------

$old = @'
    ImGui::SetCursorScreenPos(ImVec2(rp.x+50.0f,rp.y)); ImGui::Text("RESULTS & INSIGHTS");
    ImGui::TextDisabled("No analysis results yet. Link evidence and run an analysis module to generate outputs.");
'@

$new = @'
    ImGui::SetCursorScreenPos(ImVec2(rp.x+50.0f,rp.y));
    ImGui::Text("RESULTS & INSIGHTS");

    ImGui::SetCursorScreenPos(ImVec2(rp.x+50.0f,rp.y+23.0f));
    ImGui::TextDisabled("No analysis results yet. Link evidence and run an analysis module to generate outputs.");
'@

Replace-Exact $old $new "Aligned Analysis results panel"

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

Write-Host ""
Write-Host "[DONE] UI de-clutter pass v2 installed." -ForegroundColor Cyan
Write-Host "[OK] Docking unchanged." -ForegroundColor Green
Write-Host "[OK] Viewport unchanged." -ForegroundColor Green
Write-Host "[OK] Screen structure unchanged." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild now." -ForegroundColor Cyan
Write-Host ""
