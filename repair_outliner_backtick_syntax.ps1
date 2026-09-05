param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - REPAIR LITERAL BACKTICK NEWLINES" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-backtick-repair-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# The previous patch accidentally inserted the literal characters:
#     `r`n
# into the C++ source instead of a real CRLF.
$literal = '`r`n'

$count = ([regex]::Matches(
    $text,
    [regex]::Escape($literal)
)).Count

if ($count -eq 0) {
    Write-Host "[INFO] No literal `r`n sequences were found." -ForegroundColor Yellow
}
else {
    $text = $text.Replace($literal, "`r`n")
    Write-Host "[OK] Replaced $count literal backtick-newline sequence(s) with real newlines." -ForegroundColor Green
}

# Also repair the SceneOutlinerTable declaration explicitly in case whitespace
# around it was partially mangled.
$pattern = '"SceneOutlinerTable"\s*,\s*5\s*,'
if ($text -match $pattern) {
    $text = [regex]::Replace(
        $text,
        $pattern,
        '"SceneOutlinerTable",' + "`r`n" + '            5,',
        1
    )

    Write-Host "[OK] Normalized SceneOutlinerTable column declaration." -ForegroundColor Green
}
else {
    throw "Could not verify SceneOutlinerTable is configured for 5 columns."
}

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

if ($verify.Contains($literal)) {
    throw "Verification failed: literal backtick-newline text still remains."
}

if ($verify -notmatch '"SceneOutlinerTable"\s*,\s*5\s*,') {
    throw "Verification failed: SceneOutlinerTable 5-column declaration is invalid."
}

Write-Host ""
Write-Host "[DONE] Source syntax corruption repaired." -ForegroundColor Cyan
Write-Host "[OK] No UI design/layout changes were made." -ForegroundColor Green
Write-Host "[OK] Eye / Lock / Focus / More work remains installed." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug now:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
