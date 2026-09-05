param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - RESTORE SAFE EVIDENCE VIEW" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$srcDir = Join-Path $ProjectRoot "src"

$backup = Get-ChildItem `
    -Path $srcDir `
    -Filter "main.cpp.before-evidence-organize-*.bak" `
    -File `
    -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if (-not $backup) {
    throw "Could not find an Evidence backup in $srcDir"
}

Write-Host "[OK] Using backup:" -ForegroundColor Green
Write-Host "     $($backup.FullName)"

$current = Get-Content $MainCpp -Raw
$old = Get-Content $backup.FullName -Raw

$pattern = 'static void drawEvidenceView\(\)\s*\{[\s\S]*?(?=\r?\nstatic void drawAnalysisView\(\))'

$currentMatches = [regex]::Matches(
    $current,
    $pattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

$backupMatches = [regex]::Matches(
    $old,
    $pattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($currentMatches.Count -ne 1) {
    throw "Current main.cpp does not contain exactly one drawEvidenceView()."
}

if ($backupMatches.Count -ne 1) {
    throw "Backup does not contain exactly one drawEvidenceView()."
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$safety = Join-Path $srcDir "main.cpp.before-evidence-restore-$timestamp.bak"
Copy-Item $MainCpp $safety -Force

Write-Host "[OK] Current file backed up:" -ForegroundColor Green
Write-Host "     $safety"

$replacement = $backupMatches[0].Value

$fixed = [regex]::Replace(
    $current,
    $pattern,
    [System.Text.RegularExpressions.MatchEvaluator]{
        param($m)
        return $replacement
    },
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($fixed -eq $current) {
    throw "Evidence restore made no changes."
}

Set-Content -Path $MainCpp -Value $fixed -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

if ($verify -match 'EvidenceToolbar') {
    throw "Verification failed: rewritten Evidence function still appears present."
}

Write-Host ""
Write-Host "[DONE] Restored the last known-good Evidence function." -ForegroundColor Cyan
Write-Host "[OK] Case View changes preserved." -ForegroundColor Green
Write-Host "[OK] Palette preserved." -ForegroundColor Green
Write-Host "[OK] Analysis and Viewport preserved." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild and run now." -ForegroundColor Cyan
Write-Host ""
