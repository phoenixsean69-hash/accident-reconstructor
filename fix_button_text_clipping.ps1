param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - FIX BUTTON TEXT CLIPPING" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if (-not $text.Contains('static bool editorButton(')) {
    throw "Could not find editorButton() helper."
}

$pattern = 'static bool editorButton\([^)]*\)\s*\{[\s\S]*?\r?\n\}'

$matches = [regex]::Matches(
    $text,
    $pattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($matches.Count -ne 1) {
    throw "Expected exactly one editorButton() helper, found $($matches.Count)."
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-button-clipping-fix-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

$replacement = @'
static bool editorButton(const char* label, float width, bool primary=false)
{
    ImGuiStyle& style = ImGui::GetStyle();

    const float textW = ImGui::CalcTextSize(label).x;
    const float minW = textW + (style.FramePadding.x * 2.0f) + 18.0f;
    const float finalW = (width > minW) ? width : minW;

    if (primary)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, colorAccentMuted());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorAccent());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorAccent());
        ImGui::PushStyleColor(ImGuiCol_Text, colorAccent());
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, colorPanelRaised());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorPanelRaised());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorPanelRaised());
        ImGui::PushStyleColor(ImGuiCol_Text, colorText());
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 7.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    const bool pressed = ImGui::Button(label, ImVec2(finalW, 0.0f));

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    return pressed;
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
    'const float minW = textW + (style.FramePadding.x * 2.0f) + 18.0f;',
    'const float finalW = (width > minW) ? width : minW;',
    'ImGui::Button(label, ImVec2(finalW, 0.0f));'
)

foreach ($marker in $verifyMarkers) {
    if (-not $verify.Contains($marker)) {
        throw "Verification failed after write: $marker"
    }
}

Write-Host "[OK] editorButton() now auto-expands to fit its label." -ForegroundColor Green
Write-Host "[OK] This clears clipped button text across the UI." -ForegroundColor Green
Write-Host "[OK] Palette unchanged." -ForegroundColor Green
Write-Host "[OK] Layout unchanged." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild and run." -ForegroundColor Cyan
Write-Host ""
