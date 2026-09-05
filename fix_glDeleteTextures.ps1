param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - FIX glDeleteTextures BUILD ERROR" -ForegroundColor Yellow
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if ($text -notmatch 'glDeleteTextures') {
    Write-Host "[OK] glDeleteTextures is not present. Nothing to change." -ForegroundColor Green
    exit 0
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-glDeleteTextures-fix-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# Replace only the unsupported deletion statement.
# OpenGL context teardown will release these resources when the app exits.
$text = [regex]::Replace(
    $text,
    'glDeleteTextures\s*\(\s*1\s*,\s*&texture\s*\)\s*;',
    '/* texture released when OpenGL context is destroyed */'
)

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

if ((Get-Content $MainCpp -Raw) -match 'glDeleteTextures') {
    throw "Patch did not remove all glDeleteTextures references."
}

Write-Host "[OK] Removed unsupported glDeleteTextures call." -ForegroundColor Green
Write-Host "[OK] No UI/layout code was changed." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild now." -ForegroundColor Cyan
Write-Host ""
