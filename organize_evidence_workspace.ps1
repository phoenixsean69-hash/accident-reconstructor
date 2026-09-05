param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - ORGANIZE EVIDENCE WORKSPACE" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if (-not $text.Contains('static void drawEvidenceView()')) {
    throw "Could not find drawEvidenceView()."
}

if (-not $text.Contains('static void drawAnalysisView()')) {
    throw "Could not find drawAnalysisView()."
}

$pattern = 'static void drawEvidenceView\(\)\s*\{[\s\S]*?(?=\r?\nstatic void drawAnalysisView\(\))'

$matches = [regex]::Matches(
    $text,
    $pattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($matches.Count -ne 1) {
    throw "Expected exactly one drawEvidenceView() function, found $($matches.Count). Nothing changed."
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-evidence-organize-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

$replacement = @'
static void drawEvidenceView()
{
    ImGui::Begin("Evidence");

    // ========================================================
    // HEADER
    // ========================================================

    beginSurface("EvidenceHeader",ImVec2(0.0f,74.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 h=ImGui::GetCursorScreenPos();
        const float right=ImGui::GetWindowPos().x+ImGui::GetWindowSize().x;

        drawIconBadge(UiGlyph::Image,h,40.0f,true);

        ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+2.0f));
        ImGui::Text("EVIDENCE WORKSPACE");

        ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+27.0f));
        ImGui::TextDisabled(
            "Manage photographs, skid marks, debris, scene markers and measurements.");

        const float addW=136.0f;
        const float importW=142.0f;
        const float gap=8.0f;
        const float groupW=addW+gap+importW;

        ImGui::SetCursorScreenPos(ImVec2(right-groupW-12.0f,h.y+13.0f));
        editorButton("ADD EVIDENCE",addW,true);

        ImGui::SameLine(0.0f,gap);
        editorButton("IMPORT PHOTOS",importW);
    }
    endSurface();

    ImGui::Spacing();

    // ========================================================
    // FILTER / SEARCH TOOLBAR
    // ========================================================

    beginSurface("EvidenceToolbar",ImVec2(0.0f,52.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const float right=ImGui::GetWindowPos().x+ImGui::GetWindowSize().x;
        const float searchW=230.0f;
        const float filterGap=6.0f;

        editorButton("ALL",62.0f,true);
        ImGui::SameLine(0.0f,filterGap);
        editorButton("PHOTOS",76.0f);
        ImGui::SameLine(0.0f,filterGap);
        editorButton("SKID MARKS",106.0f);
        ImGui::SameLine(0.0f,filterGap);
        editorButton("DEBRIS",76.0f);
        ImGui::SameLine(0.0f,filterGap);
        editorButton("MEASUREMENTS",126.0f);
        ImGui::SameLine(0.0f,filterGap);
        editorButton("MARKERS",84.0f);

        static char evidenceSearch[128]="";

        ImGui::SetCursorScreenPos(
            ImVec2(
                right-searchW-12.0f,
                ImGui::GetWindowPos().y+11.0f
            )
        );

        ImGui::SetNextItemWidth(searchW);
        ImGui::InputTextWithHint(
            "##EvidenceSearch",
            "Search evidence...",
            evidenceSearch,
            sizeof(evidenceSearch)
        );
    }
    endSurface();

    ImGui::Spacing();

    // ========================================================
    // SUMMARY METRICS
    // ========================================================

    const float contentW=ImGui::GetContentRegionAvail().x;
    const float metricGap=8.0f;
    const float metricW=std::max(
        150.0f,
        (contentW-(metricGap*3.0f))/4.0f
    );

    drawMetricTile(
        "EvidenceTotal",
        UiGlyph::Document,
        "TOTAL EVIDENCE",
        "0 items",
        "No evidence collected",
        metricW,
        82.0f
    );

    ImGui::SameLine(0.0f,metricGap);

    drawMetricTile(
        "EvidenceReviewed",
        UiGlyph::Check,
        "REVIEWED",
        "0 items",
        "0% reviewed",
        metricW,
        82.0f
    );

    ImGui::SameLine(0.0f,metricGap);

    drawMetricTile(
        "EvidenceLinked",
        UiGlyph::Link,
        "LINKED TO SCENE",
        "0 items",
        "0% linked",
        metricW,
        82.0f
    );

    ImGui::SameLine(0.0f,metricGap);

    drawMetricTile(
        "EvidenceLastAdded",
        UiGlyph::Clock,
        "LAST ADDED",
        "NONE",
        "No evidence yet",
        metricW,
        82.0f
    );

    ImGui::Spacing();

    // ========================================================
    // EVIDENCE LIBRARY
    // ========================================================

    sectionLabel("EVIDENCE LIBRARY","Collected case material appears here.");
    ImGui::Separator();

    beginSurface("EvidenceLibrary",ImVec2(0.0f,210.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 pos=ImGui::GetWindowPos();
        const ImVec2 size=ImGui::GetWindowSize();

        const ImVec2 iconCenter(
            pos.x+size.x*0.5f,
            pos.y+61.0f
        );

        drawGlyph(
            ImGui::GetWindowDrawList(),
            UiGlyph::Image,
            iconCenter,
            54.0f,
            toU32(colorMuted())
        );

        const char* title="No evidence added yet";
        const char* body=
            "Add photographs, skid marks, debris, markers or measurements to begin building the evidence library.";

        const float titleW=ImGui::CalcTextSize(title).x;
        const float bodyW=ImGui::CalcTextSize(body).x;

        ImGui::SetCursorScreenPos(
            ImVec2(
                pos.x+(size.x-titleW)*0.5f,
                pos.y+101.0f
            )
        );
        ImGui::Text("%s",title);

        ImGui::SetCursorScreenPos(
            ImVec2(
                pos.x+std::max(18.0f,(size.x-bodyW)*0.5f),
                pos.y+127.0f
            )
        );
        ImGui::TextDisabled("%s",body);

        const float addW=130.0f;
        const float importW=140.0f;
        const float gap=8.0f;
        const float actionW=addW+gap+importW;

        ImGui::SetCursorScreenPos(
            ImVec2(
                pos.x+(size.x-actionW)*0.5f,
                pos.y+157.0f
            )
        );

        editorButton("ADD EVIDENCE",addW,true);
        ImGui::SameLine(0.0f,gap);
        editorButton("IMPORT PHOTOS",importW);
    }
    endSurface();

    ImGui::Spacing();

    // ========================================================
    // CATEGORIES
    // 3 + 2 grid so titles/descriptions do not clip.
    // ========================================================

    sectionLabel("EVIDENCE CATEGORIES","Browse by evidence type.");
    ImGui::Separator();

    struct EvidenceCategory
    {
        const char* id;
        UiGlyph glyph;
        const char* title;
        const char* note;
    };

    const EvidenceCategory categories[]={
        {"EvidencePhotos",UiGlyph::Image,"Photographs","Scene photos and documentation"},
        {"EvidenceSkids",UiGlyph::Ruler,"Skid Marks","Tire marks and friction evidence"},
        {"EvidenceDebris",UiGlyph::Document,"Debris Fields","Vehicle debris and fragments"},
        {"EvidenceMarkers",UiGlyph::Marker,"Scene Markers","Reference points and markers"},
        {"EvidenceMeasurements",UiGlyph::Ruler,"Measurements","Distances, angles and dimensions"}
    };

    const float rowGap=8.0f;
    const float threeW=std::max(
        180.0f,
        (contentW-(rowGap*2.0f))/3.0f
    );

    auto drawCategoryCard=[](
        const EvidenceCategory& category,
        float width
    )
    {
        beginSurface(
            category.id,
            ImVec2(width,100.0f),
            true,
            ImGuiWindowFlags_NoScrollbar
        );

        const ImVec2 p=ImGui::GetCursorScreenPos();

        drawIconBadge(
            category.glyph,
            p,
            34.0f,
            true
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x+46.0f,
                p.y+1.0f
            )
        );
        ImGui::Text("%s",category.title);

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x+46.0f,
                p.y+24.0f
            )
        );
        ImGui::TextDisabled("0 items");

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x,
                p.y+59.0f
            )
        );

        ImGui::PushTextWrapPos(p.x+width-18.0f);
        ImGui::TextDisabled("%s",category.note);
        ImGui::PopTextWrapPos();

        endSurface();
    };

    drawCategoryCard(categories[0],threeW);
    ImGui::SameLine(0.0f,rowGap);
    drawCategoryCard(categories[1],threeW);
    ImGui::SameLine(0.0f,rowGap);
    drawCategoryCard(categories[2],threeW);

    ImGui::Spacing();

    const float twoW=std::max(
        240.0f,
        (contentW-rowGap)/2.0f
    );

    drawCategoryCard(categories[3],twoW);
    ImGui::SameLine(0.0f,rowGap);
    drawCategoryCard(categories[4],twoW);

    ImGui::Spacing();

    // ========================================================
    // INSPECTOR FOOTER
    // ========================================================

    beginSurface("EvidenceInspector",ImVec2(0.0f,80.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 p=ImGui::GetCursorScreenPos();

        drawIconBadge(
            UiGlyph::Info,
            p,
            36.0f,
            false
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x+48.0f,
                p.y+1.0f
            )
        );
        ImGui::TextDisabled("EVIDENCE INSPECTOR");

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x+48.0f,
                p.y+23.0f
            )
        );
        ImGui::Text("SELECT AN ITEM TO INSPECT");

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x+48.0f,
                p.y+46.0f
            )
        );
        ImGui::TextDisabled(
            "Select evidence to review metadata, links and analysis readiness."
        );
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
    throw "Replacement failed. Original source left untouched."
}

Set-Content -Path $MainCpp -Value $newText -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$verifyMarkers = @(
    'EvidenceToolbar',
    'MEASUREMENTS",126.0f',
    'Collected case material appears here.',
    'Browse by evidence type.',
    'const float threeW',
    'const float twoW'
)

foreach ($marker in $verifyMarkers) {
    if (-not $verify.Contains($marker)) {
        throw "Verification failed after write: $marker"
    }
}

Write-Host ""
Write-Host "[DONE] Evidence workspace reorganized." -ForegroundColor Cyan
Write-Host "[OK] Clipped filter/header controls fixed." -ForegroundColor Green
Write-Host "[OK] Categories reorganized into a 3 + 2 grid." -ForegroundColor Green
Write-Host "[OK] Palette unchanged." -ForegroundColor Green
Write-Host "[OK] Other screens unchanged." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild and run." -ForegroundColor Cyan
Write-Host ""
