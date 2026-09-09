param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - FIX editorButton SIGNATURE" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if (-not $text.Contains('static bool editorButton(')) {
    throw "Could not find editorButton() in main.cpp"
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-editorButton-signature-fix-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# Replace the complete helper, stopping immediately before drawMetricTile().
$pattern = 'static bool editorButton\([\s\S]*?(?=\r?\nstatic void drawMetricTile\()'

$matches = [regex]::Matches(
    $text,
    $pattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($matches.Count -ne 1) {
    throw "Expected exactly one editorButton() block before drawMetricTile(); found $($matches.Count)."
}

$replacement = @'
static bool editorButton(
    const char* label,
    float width=0.0f,
    bool primary=false,
    bool enabled=true)
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Measure the visible label and guarantee enough horizontal room.
    const float textW = ImGui::CalcTextSize(label, nullptr, true).x;
    const float minW =
        textW +
        (style.FramePadding.x * 2.0f) +
        18.0f;

    const float finalW =
        (width > 0.0f)
            ? std::max(width, minW)
            : minW;

    if (!enabled)
        ImGui::BeginDisabled();

    if (primary)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, colorAccentMuted());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorAccentMuted());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorAccentMuted());
        ImGui::PushStyleColor(ImGuiCol_Text, colorAccent());
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, colorPanelRaised());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorPanelRaised());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorPanelRaised());
        ImGui::PushStyleColor(ImGuiCol_Text, colorText());
    }

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(12.0f, 7.0f)
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_FrameRounding,
        4.0f
    );

    const bool pressed =
        ImGui::Button(
            label,
            ImVec2(finalW, 0.0f)
        );

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    if (!enabled)
        ImGui::EndDisabled();

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
    throw "Replacement made no changes."
}

Set-Content -Path $MainCpp -Value $newText -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

if (-not $verify.Contains('bool enabled=true')) {
    throw "Verification failed: enabled parameter is missing."
}

if (-not $verify.Contains('std::max(width, minW)')) {
    throw "Verification failed: auto-width protection is missing."
}

Write-Host "[OK] Restored editorButton(..., enabled) fourth parameter." -ForegroundColor Green
Write-Host "[OK] Kept automatic button-width protection." -ForegroundColor Green
Write-Host "[OK] Disabled buttons now use balanced BeginDisabled/EndDisabled." -ForegroundColor Green
Write-Host "[OK] Palette and screen layouts were not changed." -ForegroundColor Green
Write-Host ""
Write-Host "IMPORTANT:" -ForegroundColor Yellow
Write-Host "  Close the currently running old sfe.exe before rebuilding."
Write-Host ""
Write-Host "Then rebuild the SAME configuration you run in Visual Studio:" -ForegroundColor Cyan
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
