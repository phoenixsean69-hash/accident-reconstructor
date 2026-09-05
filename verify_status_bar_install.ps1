param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - VERIFY STATUS BAR INSTALL" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$checks = @(
    'SOVEREIGN_STATUS_BAR_HEIGHT',
    'static void drawSovereignStatusBar()',
    '"##SovereignStatusBar"',
    '"READY"',
    '"9 objects"',
    '"Metric"',
    'F1 Shortcuts',
    'drawSovereignStatusBar();',
    'viewport->WorkSize.y-',
    'gEditorShell.snapValue',
    'selectedEntityName()'
)

$failed = @()

foreach ($check in $checks)
{
    if (-not $text.Contains($check))
    {
        $failed += $check
    }
    else
    {
        Write-Host "[OK] $check" -ForegroundColor Green
    }
}

$badPair =
    [string]([char]96) +
    "r" +
    [char]96 +
    "n"

if ($text.Contains($badPair))
{
    $failed += 'literal PowerShell `r`n text'
}

if ($failed.Count -gt 0)
{
    Write-Host ""
    Write-Host "[ERROR] Status bar installation is incomplete." -ForegroundColor Red
    Write-Host "Missing:" -ForegroundColor Yellow

    foreach ($item in $failed)
    {
        Write-Host "  - $item"
    }

    throw "Status bar verification failed."
}

Write-Host ""
Write-Host "[DONE] Status bar installation is valid." -ForegroundColor Cyan
Write-Host "[OK] The earlier failure was only the verifier." -ForegroundColor Green
Write-Host ""
Write-Host "Now rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
