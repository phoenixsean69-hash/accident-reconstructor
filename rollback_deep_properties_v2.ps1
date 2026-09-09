param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - ROLLBACK BROKEN DEEP PROPERTIES V2" -ForegroundColor Cyan
Write-Host " Restore exact pre-V2 main.cpp" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$SrcDir = Join-Path $ProjectRoot "src"
$MainCpp = Join-Path $SrcDir "main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$backups = Get-ChildItem `
    -Path $SrcDir `
    -Filter "main.cpp.before-deep-properties-v2-*.bak" `
    -File |
    Sort-Object LastWriteTime -Descending

if (-not $backups -or $backups.Count -eq 0) {
    throw @"
Could not find a V2 Properties backup.

Expected something like:
  src\main.cpp.before-deep-properties-v2-YYYYMMDD-HHMMSS.bak

Do NOT manually edit main.cpp yet.
"@
}

$restore = $backups[0]

Write-Host "[FOUND] Latest pre-V2 backup:" -ForegroundColor Green
Write-Host "        $($restore.FullName)"
Write-Host ""

$backupText = Get-Content $restore.FullName -Raw

# Sanity checks: this must look like the editor file from immediately before V2.
$required = @(
    "static void drawViewportView()",
    "static void drawTimeline()",
    "static void drawNodeEditor()",
    "struct EditorShellState",
    "selectedEntityName()"
)

foreach ($marker in $required) {
    if (-not $backupText.Contains($marker)) {
        throw "Backup sanity check failed. Missing marker: $marker"
    }
}

# It should NOT contain the V2 deep inspector payload.
if ($backupText.Contains('"VEHICLE PHYSICS"') -and
    $backupText.Contains('"EVIDENCE METADATA"') -and
    $backupText.Contains('"MEASUREMENT SETTINGS"'))
{
    throw "Selected backup already contains the V2 deep inspector. Refusing to restore the wrong file."
}

# Preserve the currently broken file for diagnosis.
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$brokenCopy = Join-Path $SrcDir "main.cpp.broken-after-deep-properties-v2-$timestamp.cpp"

Copy-Item $MainCpp $brokenCopy -Force

Write-Host "[OK] Broken current file preserved:" -ForegroundColor Yellow
Write-Host "     $brokenCopy"
Write-Host ""

# Restore exact pre-patch bytes.
Copy-Item $restore.FullName $MainCpp -Force

$verify = Get-Content $MainCpp -Raw

foreach ($marker in $required) {
    if (-not $verify.Contains($marker)) {
        throw "Restore verification failed. Missing marker: $marker"
    }
}

if ($verify.Contains('"VEHICLE PHYSICS"') -and
    $verify.Contains('"EVIDENCE METADATA"') -and
    $verify.Contains('"MEASUREMENT SETTINGS"'))
{
    throw "Restore verification failed: V2 payload still appears present."
}

Write-Host "[DONE] main.cpp restored to the exact state before Deep Properties V2." -ForegroundColor Cyan
Write-Host ""
Write-Host "[PRESERVED]" -ForegroundColor Green
Write-Host "  - status bar"
Write-Host "  - contextual editor headers"
Write-Host "  - keyboard shortcuts"
Write-Host "  - compact AR toolbar"
Write-Host "  - node editor"
Write-Host "  - timeline"
Write-Host "  - all work that existed BEFORE the V2 Properties patch"
Write-Host ""
Write-Host "Rebuild now:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
