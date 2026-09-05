param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - FIX PREMIUM SECTION GLYPHS" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-glyph-fix-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

$changes = 0

if ($text.Contains('UiGlyph::Lightning')) {
    $text = $text.Replace('UiGlyph::Lightning', 'UiGlyph::Bars')
    $changes++
    Write-Host "[OK] Next Actions glyph: Lightning -> Bars" -ForegroundColor Green
}
else {
    Write-Host "[SKIP] UiGlyph::Lightning not present." -ForegroundColor DarkGray
}

if ($text.Contains('UiGlyph::People')) {
    $text = $text.Replace('UiGlyph::People', 'UiGlyph::Info')
    $changes++
    Write-Host "[OK] Involved Parties glyph: People -> Info" -ForegroundColor Green
}
else {
    Write-Host "[SKIP] UiGlyph::People not present." -ForegroundColor DarkGray
}

if ($changes -eq 0) {
    Write-Host ""
    Write-Host "[OK] No unsupported premium glyphs remain." -ForegroundColor Green
    exit 0
}

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

if ($verify.Contains('UiGlyph::Lightning') -or $verify.Contains('UiGlyph::People')) {
    throw "Verification failed: unsupported glyph references still remain."
}

Write-Host ""
Write-Host "[DONE] Premium section glyph fix installed." -ForegroundColor Cyan
Write-Host "[OK] Layout unchanged." -ForegroundColor Green
Write-Host "[OK] Palette unchanged." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild now." -ForegroundColor Cyan
Write-Host ""
