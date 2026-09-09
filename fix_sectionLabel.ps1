param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - FIX sectionLabel BUILD ERROR" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if ($text -match 'static\s+void\s+sectionLabel\s*\(') {
    Write-Host "[OK] sectionLabel helper already exists. Nothing to change." -ForegroundColor Green
    exit 0
}

$marker = 'static void drawEvidenceView()'

if (-not $text.Contains($marker)) {
    throw "Could not find drawEvidenceView() insertion point."
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-sectionLabel-fix-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

$helper = @'
static void sectionLabel(const char* title, const char* subtitle=nullptr)
{
    ImGui::Text("%s", title);

    if (subtitle && subtitle[0] != '\0')
    {
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::TextDisabled("%s", subtitle);
    }
}

'@

$text = $text.Replace($marker, $helper + $marker)

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

if ($verify -notmatch 'static\s+void\s+sectionLabel\s*\(') {
    throw "Verification failed: helper was not added."
}

Write-Host "[OK] Added sectionLabel helper." -ForegroundColor Green
Write-Host "[OK] Evidence layout unchanged." -ForegroundColor Green
Write-Host "[OK] Palette unchanged." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild now." -ForegroundColor Cyan
Write-Host ""
