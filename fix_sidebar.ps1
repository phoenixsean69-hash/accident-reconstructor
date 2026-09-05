$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " SOVEREIGN - FINAL DARK TAB COLOR FIX" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

$source = Join-Path (Get-Location) "src\main.cpp"

if (!(Test-Path $source)) {
    Write-Host "ERROR: src\main.cpp not found." -ForegroundColor Red
    exit 1
}

$text = Get-Content $source -Raw

if ($text -notmatch "applySovereignTheme") {
    Write-Host "ERROR: Sovereign theme not found." -ForegroundColor Red
    exit 1
}

if ($text -notmatch "ImGuiCol_Tab") {
    Write-Host "ERROR: ImGui tab colors not found." -ForegroundColor Red
    exit 1
}

# ============================================================
# BACKUP
# ============================================================

$backup = "$source.before_final_dark_tabs.cpp"

if (!(Test-Path $backup)) {
    Copy-Item $source $backup
    Write-Host "[OK] Backup created:" -ForegroundColor Green
    Write-Host "     $backup"
}
else {
    Write-Host "[OK] Existing backup preserved." -ForegroundColor Yellow
}

# ============================================================
# HELPER
# ============================================================

function Replace-ImGuiColor {
    param(
        [string]$ColorName,
        [string]$R,
        [string]$G,
        [string]$B,
        [string]$A = "1.0f"
    )

    $pattern =
        '(?s)c\[' +
        [regex]::Escape($ColorName) +
        '\]\s*=\s*ImVec4\s*\([^;]*\);'

    $replacement = @"
c[$ColorName] =
    ImVec4(
        $R,
        $G,
        $B,
        $A
    );
"@

    $matches = [regex]::Matches($script:text, $pattern)

    if ($matches.Count -eq 0) {
        Write-Host "[SKIP] $ColorName not found." -ForegroundColor DarkYellow
        return
    }

    $script:text = [regex]::Replace(
        $script:text,
        $pattern,
        $replacement,
        1
    )

    Write-Host "[OK] $ColorName" -ForegroundColor Green
}

# ============================================================
# TAB STRIP
# ============================================================

Replace-ImGuiColor `
    "ImGuiCol_Tab" `
    "0.075f" `
    "0.078f" `
    "0.082f"

Replace-ImGuiColor `
    "ImGuiCol_TabHovered" `
    "0.135f" `
    "0.140f" `
    "0.148f"

Replace-ImGuiColor `
    "ImGuiCol_TabActive" `
    "0.105f" `
    "0.110f" `
    "0.118f"

# ============================================================
# ACTIVE DOCK / WINDOW CHROME
# ============================================================

Replace-ImGuiColor `
    "ImGuiCol_TitleBg" `
    "0.045f" `
    "0.047f" `
    "0.050f"

Replace-ImGuiColor `
    "ImGuiCol_TitleBgActive" `
    "0.105f" `
    "0.110f" `
    "0.118f"

# ============================================================
# HEADER STATES
# These can also affect docking-related active visuals.
# ============================================================

Replace-ImGuiColor `
    "ImGuiCol_Header" `
    "0.075f" `
    "0.078f" `
    "0.082f"

Replace-ImGuiColor `
    "ImGuiCol_HeaderHovered" `
    "0.135f" `
    "0.140f" `
    "0.148f"

Replace-ImGuiColor `
    "ImGuiCol_HeaderActive" `
    "0.105f" `
    "0.110f" `
    "0.118f"

# ============================================================
# DOCKING BACKGROUND
# ============================================================
#
# Older/newer Dear ImGui docking builds use this for empty
# docking areas. Neutralize it as well.
# ============================================================

Replace-ImGuiColor `
    "ImGuiCol_DockingEmptyBg" `
    "0.035f" `
    "0.037f" `
    "0.040f"

# ============================================================
# DOCKING PREVIEW
# Keep it subtle and neutral instead of gold.
# ============================================================

Replace-ImGuiColor `
    "ImGuiCol_DockingPreview" `
    "0.180f" `
    "0.190f" `
    "0.205f" `
    "0.60f"

# ============================================================
# SEPARATOR
#
# This prevents the gold line surrounding the tab strip from
# becoming visually dominant.
# ============================================================

Replace-ImGuiColor `
    "ImGuiCol_Separator" `
    "0.180f" `
    "0.185f" `
    "0.195f"

# ============================================================
# WRITE
# ============================================================

Set-Content `
    -Path $source `
    -Value $text `
    -Encoding UTF8

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " FINAL DARK TAB PALETTE APPLIED" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Normal       : charcoal"
Write-Host "Hovered      : slightly lighter charcoal"
Write-Host "Active       : charcoal"
Write-Host "Title active : charcoal"
Write-Host "Docking bg   : near-black"
Write-Host "Dock preview : neutral gray"
Write-Host "Separator    : neutral gray"
Write-Host ""
Write-Host "Gold is now reserved for actual application accents."
Write-Host ""
Write-Host "Backup:"
Write-Host "  $backup"
Write-Host ""
Write-Host "Rebuild:"
Write-Host "  Ctrl+Shift+B"
Write-Host ""
Write-Host "Run:"
Write-Host "  F5"
Write-Host ""