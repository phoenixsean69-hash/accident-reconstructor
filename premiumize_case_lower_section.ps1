param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - PREMIUMIZE CASE LOWER SECTION" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$requiredMarkers = @(
    'static void drawCaseView()',
    'beginSurface("IncidentSummary"',
    'beginSurface("CaseContent"',
    'beginSurface("CaseActions"',
    'beginSurface("CaseParties"'
)

foreach ($marker in $requiredMarkers) {
    if (-not $text.Contains($marker)) {
        throw "Expected Case View marker not found: $marker"
    }
}

$pattern = 'beginSurface\("IncidentSummary"[\s\S]*?beginSurface\("CaseParties"[\s\S]*?endSurface\(\);(?=\s*\r?\n\s*\r?\n\s*ImGui::End\(\);)'

$matches = [regex]::Matches(
    $text,
    $pattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($matches.Count -ne 1) {
    throw "Expected exactly one lower Case View section, found $($matches.Count). Nothing changed."
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-case-premium-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

$replacement = @'
beginSurface("IncidentSummary",ImVec2(0.0f,82.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 p=ImGui::GetCursorScreenPos();
        const float right=ImGui::GetWindowPos().x+ImGui::GetWindowSize().x;

        drawIconBadge(UiGlyph::Document,p,40.0f,false);

        ImGui::SetCursorScreenPos(ImVec2(p.x+52.0f,p.y+3.0f));
        ImGui::Text("INCIDENT SUMMARY");

        ImGui::SetCursorScreenPos(ImVec2(p.x+52.0f,p.y+29.0f));
        ImGui::TextDisabled(
            "No incident summary has been entered. Add essential facts, scene location and reconstruction notes.");

        const float editW=132.0f;
        ImGui::SetCursorScreenPos(ImVec2(right-editW-12.0f,p.y+13.0f));
        editorButton("EDIT SUMMARY",editW);
    }
    endSurface();
    ImGui::Spacing();

    beginSurface("CaseContent",ImVec2(0.0f,206.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 p=ImGui::GetCursorScreenPos();
        ImDrawList* dl=ImGui::GetWindowDrawList();
        const float left=p.x;
        const float right=ImGui::GetWindowPos().x+ImGui::GetWindowSize().x-12.0f;

        drawIconBadge(UiGlyph::Document,p,34.0f,false);

        ImGui::SetCursorScreenPos(ImVec2(p.x+46.0f,p.y+2.0f));
        ImGui::Text("CASE CONTENT");

        const float titleW=ImGui::CalcTextSize("CASE CONTENT").x;
        const float dividerX=p.x+58.0f+titleW;
        dl->AddLine(
            ImVec2(dividerX,p.y+2.0f),
            ImVec2(dividerX,p.y+24.0f),
            toU32(colorBorder()),
            1.0f);

        ImGui::SetCursorScreenPos(ImVec2(dividerX+16.0f,p.y+3.0f));
        ImGui::TextDisabled("Items included in this case.");

        ImGui::SetCursorScreenPos(ImVec2(left,p.y+45.0f));

        if (ImGui::BeginTable(
            "CaseTablePremium",
            4,
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersInnerH |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("CATEGORY",ImGuiTableColumnFlags_WidthStretch,0.57f);
            ImGui::TableSetupColumn("COUNT",ImGuiTableColumnFlags_WidthStretch,0.14f);
            ImGui::TableSetupColumn("STATUS",ImGuiTableColumnFlags_WidthStretch,0.24f);
            ImGui::TableSetupColumn("##OPEN",ImGuiTableColumnFlags_WidthFixed,34.0f);
            ImGui::TableHeadersRow();

            struct PremiumCaseRow
            {
                UiGlyph glyph;
                const char* category;
                const char* count;
                const char* status;
                StatusTone tone;
            };

            const PremiumCaseRow rows[]={
                {UiGlyph::Cube,"Vehicles","2","READY",StatusTone::Success},
                {UiGlyph::Document,"Evidence","0","EMPTY",StatusTone::Neutral},
                {UiGlyph::Ruler,"Measurements","0","EMPTY",StatusTone::Neutral}
            };

            for (const PremiumCaseRow& row:rows)
            {
                ImGui::TableNextRow(ImGuiTableRowFlags_None,39.0f);

                ImGui::TableSetColumnIndex(0);
                {
                    const ImVec2 rp=ImGui::GetCursorScreenPos();
                    drawIconBadge(row.glyph,ImVec2(rp.x,rp.y+3.0f),28.0f,false);
                    ImGui::SetCursorScreenPos(ImVec2(rp.x+40.0f,rp.y+8.0f));
                    ImGui::Text("%s",row.category);
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+7.0f);
                ImGui::Text("%s",row.count);

                ImGui::TableSetColumnIndex(2);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+7.0f);
                drawStatus(row.status,row.tone);

                ImGui::TableSetColumnIndex(3);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+7.0f);
                ImGui::TextDisabled(">");
            }

            ImGui::EndTable();
        }
    }
    endSurface();
    ImGui::Spacing();

    beginSurface("CaseActions",ImVec2(0.0f,78.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 p=ImGui::GetCursorScreenPos();
        ImDrawList* dl=ImGui::GetWindowDrawList();
        const float right=ImGui::GetWindowPos().x+ImGui::GetWindowSize().x;

        drawIconBadge(UiGlyph::Lightning,p,38.0f,false);

        ImGui::SetCursorScreenPos(ImVec2(p.x+50.0f,p.y+2.0f));
        ImGui::Text("NEXT ACTIONS");

        ImGui::SetCursorScreenPos(ImVec2(p.x+50.0f,p.y+28.0f));
        ImGui::TextDisabled("Continue building the case with the available tools.");

        const float dividerX=p.x+355.0f;
        dl->AddLine(
            ImVec2(dividerX,p.y+3.0f),
            ImVec2(dividerX,p.y+43.0f),
            toU32(colorBorder()),
            1.0f);

        const float openW=150.0f;
        const float evidenceW=140.0f;
        const float analysisW=150.0f;
        const float exportW=132.0f;
        const float buttonGap=7.0f;
        const float groupW=openW+evidenceW+analysisW+exportW+(buttonGap*3.0f);

        float groupX=right-groupW-12.0f;
        groupX=std::max(groupX,dividerX+18.0f);

        ImGui::SetCursorScreenPos(ImVec2(groupX,p.y+11.0f));
        editorButton("OPEN VIEWPORT",openW,true);

        ImGui::SameLine(0.0f,buttonGap);
        editorButton("ADD EVIDENCE",evidenceW);

        ImGui::SameLine(0.0f,buttonGap);
        editorButton("START ANALYSIS",analysisW);

        ImGui::SameLine(0.0f,buttonGap);
        editorButton("EXPORT CASE",exportW);
    }
    endSurface();
    ImGui::Spacing();

    beginSurface("CaseParties",ImVec2(0.0f,72.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 p=ImGui::GetCursorScreenPos();
        ImDrawList* dl=ImGui::GetWindowDrawList();
        const float right=ImGui::GetWindowPos().x+ImGui::GetWindowSize().x;

        drawIconBadge(UiGlyph::People,p,38.0f,false);

        ImGui::SetCursorScreenPos(ImVec2(p.x+50.0f,p.y+2.0f));
        ImGui::Text("INVOLVED PARTIES");

        const float dividerX=p.x+205.0f;
        dl->AddLine(
            ImVec2(dividerX,p.y+2.0f),
            ImVec2(dividerX,p.y+40.0f),
            toU32(colorBorder()),
            1.0f);

        ImGui::SetCursorScreenPos(ImVec2(dividerX+16.0f,p.y+12.0f));
        ImGui::TextDisabled("No parties have been added.");

        const float partyW=112.0f;
        ImGui::SetCursorScreenPos(ImVec2(right-partyW-12.0f,p.y+9.0f));
        editorButton("ADD PARTY",partyW);
    }
    endSurface();
'@

$newText = [regex]::Replace(
    $text,
    $pattern,
    $replacement,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($newText -eq $text) {
    throw "Replacement failed. Backup exists, original source left untouched."
}

Set-Content -Path $MainCpp -Value $newText -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$verifyMarkers = @(
    'CaseTablePremium',
    'Items included in this case.',
    'Continue building the case with the available tools.',
    'ImVec2(0.0f,206.0f)'
)

foreach ($marker in $verifyMarkers) {
    if (-not $verify.Contains($marker)) {
        throw "Verification failed after write: $marker"
    }
}

Write-Host ""
Write-Host "[DONE] Premium Case lower section installed." -ForegroundColor Cyan
Write-Host "[OK] Existing palette preserved." -ForegroundColor Green
Write-Host "[OK] Global theme untouched." -ForegroundColor Green
Write-Host "[OK] Docking untouched." -ForegroundColor Green
Write-Host "[OK] Viewport / Evidence / Analysis untouched." -ForegroundColor Green
Write-Host ""
Write-Host "Premiumized:" -ForegroundColor White
Write-Host "  - Incident Summary"
Write-Host "  - Case Content"
Write-Host "  - Next Actions"
Write-Host "  - Involved Parties"
Write-Host ""
Write-Host "Rebuild and run." -ForegroundColor Cyan
Write-Host ""
