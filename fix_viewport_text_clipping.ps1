param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - FIX VIEWPORT TEXT CLIPPING" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'ViewportSecondaryToolbar',
    '"OVERLAYS  v"',
    'Selection: None',
    'Objects: 0'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected viewport Phase-2 marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-viewport-clipping-fix-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

$changes = 0

# ------------------------------------------------------------
# 1. Widen OVERLAYS popup button.
# ------------------------------------------------------------

$oldOverlayButton = @'
    if (ImGui::Button("OVERLAYS  v",ImVec2(106.0f,28.0f)))
        ImGui::OpenPopup("##ViewportOverlaysPopup");
'@

$newOverlayButton = @'
    const float overlaysButtonW =
        ImGui::CalcTextSize("OVERLAYS  v").x +
        (ImGui::GetStyle().FramePadding.x * 2.0f) +
        20.0f;

    if (ImGui::Button(
        "OVERLAYS  v",
        ImVec2(overlaysButtonW,30.0f)))
    {
        ImGui::OpenPopup("##ViewportOverlaysPopup");
    }
'@

if ($text.Contains($oldOverlayButton)) {
    $text = $text.Replace($oldOverlayButton,$newOverlayButton)
    $changes++
    Write-Host "[OK] OVERLAYS button now sizes to its text." -ForegroundColor Green
}
elseif ($text.Contains('const float overlaysButtonW')) {
    Write-Host "[SKIP] OVERLAYS button is already auto-sized." -ForegroundColor DarkGray
}
else {
    throw "Could not locate the OVERLAYS button block."
}

# ------------------------------------------------------------
# 2. Increase HUD card height.
# Current card:
#   top = cp.y+14
#   bottom = cp.y+78
# only 64px tall for 3 lines at 18.5px font.
# ------------------------------------------------------------

$oldHudMax = @'
        const ImVec2 hudMax(
            cp.x+cs.x-14.0f,
            cp.y+78.0f
        );
'@

$newHudMax = @'
        const ImVec2 hudMax(
            cp.x+cs.x-14.0f,
            cp.y+98.0f
        );
'@

if ($text.Contains($oldHudMax)) {
    $text = $text.Replace($oldHudMax,$newHudMax)
    $changes++
    Write-Host "[OK] Viewport HUD card made taller." -ForegroundColor Green
}
elseif ($text.Contains('cp.y+98.0f')) {
    Write-Host "[SKIP] HUD height already increased." -ForegroundColor DarkGray
}
else {
    throw "Could not locate viewport HUD height block."
}

# ------------------------------------------------------------
# 3. Give HUD lines more breathing room.
# ------------------------------------------------------------

$replacements = @(
    @{
        Old = '            ImVec2(hudMin.x+10.0f,hudMin.y+9.0f),'
        New = '            ImVec2(hudMin.x+10.0f,hudMin.y+10.0f),'
        Name = 'HUD title baseline'
    },
    @{
        Old = '            ImVec2(hudMin.x+10.0f,hudMin.y+31.0f),'
        New = '            ImVec2(hudMin.x+10.0f,hudMin.y+37.0f),'
        Name = 'HUD selection baseline'
    },
    @{
        Old = '            ImVec2(hudMin.x+10.0f,hudMin.y+49.0f),'
        New = '            ImVec2(hudMin.x+10.0f,hudMin.y+64.0f),'
        Name = 'HUD object-count baseline'
    }
)

foreach ($r in $replacements) {
    if ($text.Contains($r.Old)) {
        $text = $text.Replace($r.Old,$r.New)
        $changes++
        Write-Host "[OK] $($r.Name) adjusted." -ForegroundColor Green
    }
}

if ($changes -eq 0) {
    Write-Host ""
    Write-Host "[INFO] No changes were necessary." -ForegroundColor Yellow
    exit 0
}

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'const float overlaysButtonW',
    'cp.y+98.0f',
    'hudMin.y+64.0f'
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] Viewport clipping fixed." -ForegroundColor Cyan
Write-Host "[OK] OVERLAYS control now fits the larger font." -ForegroundColor Green
Write-Host "[OK] HUD card now has enough room for all 3 lines." -ForegroundColor Green
Write-Host "[OK] No palette or viewport-mode changes." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
