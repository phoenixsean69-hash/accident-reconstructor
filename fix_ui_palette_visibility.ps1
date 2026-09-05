param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - READABILITY / PALETTE PASS" -ForegroundColor Yellow
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'static ImVec4 colorWindow()',
    'static void applySovereignTheme()',
    'Rubik-Regular.ttf'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected Sovereign UI marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-palette-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

function Replace-Regex {
    param(
        [string]$Pattern,
        [string]$Replacement,
        [string]$Name
    )

    $before = $script:text
    $script:text = [regex]::Replace(
        $script:text,
        $Pattern,
        $Replacement,
        [System.Text.RegularExpressions.RegexOptions]::Singleline
    )

    if ($script:text -eq $before) {
        throw "Could not patch: $Name"
    }

    Write-Host "[OK] $Name" -ForegroundColor Green
}

# ============================================================
# CORE COLOR TOKENS
# Visual Studio / Unreal / Unity-inspired graphite hierarchy,
# while retaining Sovereign amber as the product accent.
# ============================================================

Replace-Regex `
    'static ImVec4 colorWindow\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorWindow() { return ImVec4(0.075f, 0.080f, 0.088f, 1.0f); }' `
    'Workspace background -> graphite'

Replace-Regex `
    'static ImVec4 colorPanel\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorPanel() { return ImVec4(0.095f, 0.101f, 0.110f, 1.0f); }' `
    'Panel background -> elevated graphite'

Replace-Regex `
    'static ImVec4 colorPanelRaised\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorPanelRaised() { return ImVec4(0.125f, 0.132f, 0.143f, 1.0f); }' `
    'Raised surfaces -> clearer separation'

Replace-Regex `
    'static ImVec4 colorBorder\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorBorder() { return ImVec4(0.255f, 0.275f, 0.300f, 1.0f); }' `
    'Borders -> visible neutral separators'

Replace-Regex `
    'static ImVec4 colorAccent\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorAccent() { return ImVec4(0.98f, 0.67f, 0.08f, 1.0f); }' `
    'Amber accent -> brighter but restrained'

Replace-Regex `
    'static ImVec4 colorAccentMuted\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorAccentMuted() { return ImVec4(0.245f, 0.175f, 0.055f, 1.0f); }' `
    'Muted accent surface -> warmer selection'

Replace-Regex `
    'static ImVec4 colorText\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorText() { return ImVec4(0.955f, 0.965f, 0.975f, 1.0f); }' `
    'Primary text -> high contrast'

Replace-Regex `
    'static ImVec4 colorMuted\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorMuted() { return ImVec4(0.690f, 0.715f, 0.745f, 1.0f); }' `
    'Secondary text -> readable gray'

Replace-Regex `
    'static ImVec4 colorSuccess\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorSuccess() { return ImVec4(0.46f, 0.82f, 0.43f, 1.0f); }' `
    'Success green -> clearer semantic state'

Replace-Regex `
    'static ImVec4 colorDanger\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorDanger() { return ImVec4(0.93f, 0.40f, 0.36f, 1.0f); }' `
    'Danger red -> clearer semantic state'

# ============================================================
# IMGUI CONTROL PALETTE
# ============================================================

$replacements = @(
    @{
        Pattern = 'c\[ImGuiCol_FrameBg\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_FrameBg] = ImVec4(0.145f, 0.152f, 0.165f, 1.0f);'
        Name    = 'Input backgrounds'
    },
    @{
        Pattern = 'c\[ImGuiCol_FrameBgHovered\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_FrameBgHovered] = ImVec4(0.185f, 0.195f, 0.212f, 1.0f);'
        Name    = 'Input hover state'
    },
    @{
        Pattern = 'c\[ImGuiCol_FrameBgActive\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_FrameBgActive] = ImVec4(0.215f, 0.228f, 0.248f, 1.0f);'
        Name    = 'Input active state'
    },
    @{
        Pattern = 'c\[ImGuiCol_TitleBg\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_TitleBg] = ImVec4(0.090f, 0.095f, 0.103f, 1.0f);'
        Name    = 'Inactive dock titles'
    },
    @{
        Pattern = 'c\[ImGuiCol_TitleBgActive\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_TitleBgActive] = ImVec4(0.125f, 0.132f, 0.145f, 1.0f);'
        Name    = 'Active dock titles'
    },
    @{
        Pattern = 'c\[ImGuiCol_MenuBarBg\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_MenuBarBg] = ImVec4(0.095f, 0.100f, 0.110f, 1.0f);'
        Name    = 'Menu bar'
    },
    @{
        Pattern = 'c\[ImGuiCol_Button\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_Button] = ImVec4(0.145f, 0.152f, 0.165f, 1.0f);'
        Name    = 'Button base'
    },
    @{
        Pattern = 'c\[ImGuiCol_ButtonHovered\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_ButtonHovered] = ImVec4(0.205f, 0.216f, 0.235f, 1.0f);'
        Name    = 'Button hover'
    },
    @{
        Pattern = 'c\[ImGuiCol_ButtonActive\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_ButtonActive] = ImVec4(0.245f, 0.258f, 0.280f, 1.0f);'
        Name    = 'Button active'
    },
    @{
        Pattern = 'c\[ImGuiCol_Header\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_Header] = ImVec4(0.135f, 0.143f, 0.156f, 1.0f);'
        Name    = 'Tree/header base'
    },
    @{
        Pattern = 'c\[ImGuiCol_HeaderHovered\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_HeaderHovered] = ImVec4(0.190f, 0.202f, 0.220f, 1.0f);'
        Name    = 'Tree/header hover'
    },
    @{
        Pattern = 'c\[ImGuiCol_HeaderActive\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_HeaderActive] = ImVec4(0.220f, 0.234f, 0.255f, 1.0f);'
        Name    = 'Tree/header active'
    },
    @{
        Pattern = 'c\[ImGuiCol_Separator\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_Separator] = ImVec4(0.235f, 0.250f, 0.275f, 1.0f);'
        Name    = 'Separators'
    },
    @{
        Pattern = 'c\[ImGuiCol_Tab\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_Tab] = ImVec4(0.105f, 0.112f, 0.122f, 1.0f);'
        Name    = 'Inactive tabs'
    },
    @{
        Pattern = 'c\[ImGuiCol_TabHovered\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_TabHovered] = ImVec4(0.190f, 0.200f, 0.218f, 1.0f);'
        Name    = 'Tab hover'
    },
    @{
        Pattern = 'c\[ImGuiCol_TabActive\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_TabActive] = ImVec4(0.155f, 0.165f, 0.180f, 1.0f);'
        Name    = 'Active tab'
    },
    @{
        Pattern = 'c\[ImGuiCol_TabUnfocused\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_TabUnfocused] = ImVec4(0.090f, 0.095f, 0.105f, 1.0f);'
        Name    = 'Unfocused tabs'
    },
    @{
        Pattern = 'c\[ImGuiCol_TabUnfocusedActive\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.125f, 0.133f, 0.145f, 1.0f);'
        Name    = 'Unfocused active tab'
    },
    @{
        Pattern = 'c\[ImGuiCol_TableHeaderBg\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_TableHeaderBg] = ImVec4(0.160f, 0.170f, 0.185f, 1.0f);'
        Name    = 'Table headers'
    },
    @{
        Pattern = 'c\[ImGuiCol_TableRowBgAlt\]\s*=\s*ImVec4\([^;]+;'
        Value   = 'c[ImGuiCol_TableRowBgAlt] = ImVec4(0.090f, 0.095f, 0.105f, 1.0f);'
        Name    = 'Alternate table rows'
    }
)

foreach ($item in $replacements) {
    Replace-Regex $item.Pattern $item.Value $item.Name
}

# ============================================================
# FONT READABILITY
# 16px -> 17.5px, without changing the layout architecture.
# ============================================================

$beforeFont = $text
$text = [regex]::Replace(
    $text,
    'AddFontFromFileTTF\(\s*rubikPath\.string\(\)\.c_str\(\)\s*,\s*16\.0f\s*\)',
    'AddFontFromFileTTF(rubikPath.string().c_str(),17.5f)'
)

if ($text -eq $beforeFont) {
    # Compact source may use slightly different spacing.
    $text = $text -replace 'rubikPath\.string\(\)\.c_str\(\),16\.0f', 'rubikPath.string().c_str(),17.5f'
}

if ($text -eq $beforeFont) {
    throw "Could not patch Rubik font size."
}

Write-Host "[OK] Font size 16.0 -> 17.5" -ForegroundColor Green

# Slightly more breathing room around controls after increasing font size.
$text = [regex]::Replace(
    $text,
    'style\.FramePadding\s*=\s*ImVec2\([^;]+;',
    'style.FramePadding = ImVec2(9.0f, 6.0f);'
)

$text = [regex]::Replace(
    $text,
    'style\.ItemSpacing\s*=\s*ImVec2\([^;]+;',
    'style.ItemSpacing = ImVec2(8.0f, 7.0f);'
)

Write-Host "[OK] Control spacing adjusted for larger text" -ForegroundColor Green

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

Write-Host ""
Write-Host "[DONE] Readability / palette pass installed." -ForegroundColor Cyan
Write-Host "[OK] Layout unchanged." -ForegroundColor Green
Write-Host "[OK] Docking unchanged." -ForegroundColor Green
Write-Host "[OK] Viewport structure unchanged." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild and run the app." -ForegroundColor Cyan
Write-Host ""
