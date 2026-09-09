param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - REMOVE FAKE 3D SCENE CONTENT" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if (-not $text.Contains('static void drawViewportView()')) {
    throw "Could not find drawViewportView()."
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-remove-fake-3d-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

$changes = 0

# ============================================================
# 1. REMOVE THE EARLIER PHASE-2 PLACEHOLDER VEHICLE BLOCK
# ============================================================

$phase2Pattern = '(?s)\s*const ImVec2 vehicleMin\([\s\S]*?(?=\s*if \(showAxes\))'

if ([regex]::IsMatch($text,$phase2Pattern)) {
    $text = [regex]::Replace(
        $text,
        $phase2Pattern,
        "`r`n",
        1
    )
    $changes++
    Write-Host "[OK] Removed Phase-2 placeholder vehicle." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] Phase-2 placeholder vehicle block not found." -ForegroundColor DarkGray
}

# ============================================================
# 2. REMOVE PHASE-3 FAKE CAR + OBJECT GIZMO + SHADOW
#    Keep the camera information card that follows it.
# ============================================================

$phase3Pattern = '(?s)\s*// ----------------------------------------------------\s*\r?\n\s*// 3D: ground contact shadow[\s\S]*?(?=\s*// ----------------------------------------------------\s*\r?\n\s*// 3D: camera information card)'

if ([regex]::IsMatch($text,$phase3Pattern)) {
    $text = [regex]::Replace(
        $text,
        $phase3Pattern,
        "`r`n",
        1
    )
    $changes++
    Write-Host "[OK] Removed fake 3D car, shadow and object gizmo." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] Phase-3 fake-car block not found." -ForegroundColor DarkGray
}

# ============================================================
# 3. REMOVE ANY LEFTOVER FAKE ANALYSIS VECTOR
# ============================================================

$analysisVectorPattern = '(?s)\s*if \(renderMode==2\)\s*\{\s*d->AddLine\([\s\S]*?"analysis vector"\s*\);\s*\}'

if ([regex]::IsMatch($text,$analysisVectorPattern)) {
    $text = [regex]::Replace(
        $text,
        $analysisVectorPattern,
        "`r`n",
        1
    )
    $changes++
    Write-Host "[OK] Removed fake analysis vector." -ForegroundColor Green
}

# ============================================================
# 4. ADD A CLEAN EMPTY-STATE MESSAGE TO 3D
# ============================================================

$cameraCardMarker = '// 3D: camera information card'

if ($text.Contains($cameraCardMarker) -and
    -not $text.Contains('"No 3D scene objects loaded"'))
{
    $emptyState = @'
        // Clean empty-state. Real scene objects should be rendered by the
        // actual scene/renderer path, not fabricated here in ImGui.
        {
            const char* emptyTitle="No 3D scene objects loaded";
            const char* emptyNote="Add or import scene objects to begin reconstruction.";

            const ImVec2 titleSize=
                ImGui::CalcTextSize(emptyTitle);

            const ImVec2 noteSize=
                ImGui::CalcTextSize(emptyNote);

            d->AddText(
                ImVec2(
                    center.x-titleSize.x*0.5f,
                    center.y-18.0f
                ),
                IM_COL32(205,209,215,215),
                emptyTitle
            );

            d->AddText(
                ImVec2(
                    center.x-noteSize.x*0.5f,
                    center.y+10.0f
                ),
                IM_COL32(132,138,147,210),
                emptyNote
            );
        }

'@

    $text = $text.Replace(
        $cameraCardMarker,
        $emptyState + '        ' + $cameraCardMarker
    )

    $changes++
    Write-Host "[OK] Added clean 3D empty-state message." -ForegroundColor Green
}

# ============================================================
# SAVE + VERIFY
# ============================================================

if ($changes -eq 0) {
    Write-Host ""
    Write-Host "[INFO] No fake 3D content was found to remove." -ForegroundColor Yellow
    exit 0
}

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

if ($verify.Contains('const ImVec2 carCenter(')) {
    throw "Verification failed: fake Phase-3 car still exists."
}

if ($verify.Contains('const ImVec2 vehicleMin(')) {
    throw "Verification failed: fake Phase-2 vehicle still exists."
}

if (-not $verify.Contains('"No 3D scene objects loaded"')) {
    throw "Verification failed: clean 3D empty-state was not installed."
}

Write-Host ""
Write-Host "[DONE] Fake 3D scene content removed." -ForegroundColor Cyan
Write-Host "[OK] Perspective grid preserved." -ForegroundColor Green
Write-Host "[OK] Camera controls / HUD preserved." -ForegroundColor Green
Write-Host "[OK] Orientation gizmo preserved." -ForegroundColor Green
Write-Host "[OK] Real scene objects can be wired in later." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
