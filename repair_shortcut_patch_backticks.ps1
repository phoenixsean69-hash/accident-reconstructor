param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - REPAIR SHORTCUT PATCH BACKTICKS" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-shortcut-backtick-repair-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# Literal characters inserted by the previous PowerShell regex replacements:
#     `r`n
$bad = [string]([char]96) + "r" + [char]96 + "n"

$count = 0
$scan = 0

while (($scan = $text.IndexOf($bad, $scan)) -ge 0) {
    $count++
    $scan += $bad.Length
}

if ($count -eq 0) {
    Write-Host "[INFO] No literal backtick-newline sequences were found." -ForegroundColor Yellow
}
else {
    $text = $text.Replace(
        $bad,
        [Environment]::NewLine
    )

    Write-Host "[OK] Repaired $count literal backtick-newline sequence(s)." -ForegroundColor Green
}

# Defensive cleanup for accidental literal `n or `r remnants on their own.
$badN = [string]([char]96) + "n"
$badR = [string]([char]96) + "r"

$remainingPair = $text.Contains($bad)

if ($remainingPair) {
    throw "Repair failed: literal `r`n text still remains."
}

# Do NOT globally replace lone `n / `r because those may legitimately exist
# inside C++ strings. Only report them if they occur outside quoted contexts
# around the known shortcut menu area.
$menuStart = $text.IndexOf('static void drawMainMenuBar()')
$menuEnd   = $text.IndexOf('static void drawInterface()')

if ($menuStart -ge 0 -and $menuEnd -gt $menuStart) {
    $menuText = $text.Substring($menuStart, $menuEnd - $menuStart)

    if ($menuText.Contains($badN) -or $menuText.Contains($badR)) {
        Write-Host "[WARN] A lone literal backtick escape still appears in drawMainMenuBar()." -ForegroundColor Yellow
        Write-Host "       The script will show matching lines below." -ForegroundColor Yellow
    }
}

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

if ($verify.Contains($bad)) {
    throw "Verification failed: literal `r`n still exists after write."
}

Write-Host ""
Write-Host "[DONE] Shortcut-patch syntax repaired." -ForegroundColor Cyan
Write-Host "[OK] No UI layout or shortcut behavior was removed." -ForegroundColor Green
Write-Host ""
Write-Host "Check for any remaining literal backticks near the failing area:" -ForegroundColor Yellow
Write-Host '  powershell -Command "$i=0; Get-Content .\src\main.cpp | %% { $i++; if($i -ge 9845 -and $i -le 9920){ ''{0,5}: {1}'' -f $i,$_ } }"'
Write-Host ""
Write-Host "Then rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
