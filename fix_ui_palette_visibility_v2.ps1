param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - READABILITY / PALETTE PASS v2" -ForegroundColor Yellow
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$requiredMarkers = @(
    'static ImVec4 colorWindow()',
    'static void applySovereignTheme()',
    'Rubik-Regular.ttf'
)

foreach ($marker in $requiredMarkers) {
    if (-not $text.Contains($marker)) {
        throw "Expected Sovereign UI marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-palette-v2-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

function Replace-Required {
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
        throw "Could not patch required item: $Name"
    }

    Write-Host "[OK] $Name" -ForegroundColor Green
}

function Replace-Optional {
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
        Write-Host "[SKIP] $Name not present in this source." -ForegroundColor DarkGray
    }
    else {
        Write-Host "[OK] $Name" -ForegroundColor Green
    }
}

# ============================================================
# CORE PALETTE TOKENS
# VS / Unreal / Unity inspired graphite hierarchy.
# Sovereign amber remains the accent.
# ============================================================

Replace-Required `
    'static ImVec4 colorWindow\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorWindow() { return ImVec4(0.070f, 0.073f, 0.079f, 1.0f); }' `
    'Workspace background'

Replace-Required `
    'static ImVec4 colorPanel\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorPanel() { return ImVec4(0.095f, 0.100f, 0.108f, 1.0f); }' `
    'Panel background'

Replace-Required `
    'static ImVec4 colorPanelRaised\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorPanelRaised() { return ImVec4(0.125f, 0.132f, 0.143f, 1.0f); }' `
    'Raised panel background'

Replace-Required `
    'static ImVec4 colorBorder\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorBorder() { return ImVec4(0.245f, 0.260f, 0.285f, 1.0f); }' `
    'Panel borders'

Replace-Required `
    'static ImVec4 colorAccent\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorAccent() { return ImVec4(0.98f, 0.68f, 0.08f, 1.0f); }' `
    'Sovereign amber'

Replace-Required `
    'static ImVec4 colorAccentMuted\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorAccentMuted() { return ImVec4(0.245f, 0.175f, 0.055f, 1.0f); }' `
    'Muted amber surface'

Replace-Required `
    'static ImVec4 colorText\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorText() { return ImVec4(0.965f, 0.972f, 0.982f, 1.0f); }' `
    'Primary text contrast'

Replace-Required `
    'static ImVec4 colorMuted\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorMuted() { return ImVec4(0.700f, 0.725f, 0.760f, 1.0f); }' `
    'Secondary text contrast'

Replace-Required `
    'static ImVec4 colorSuccess\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorSuccess() { return ImVec4(0.48f, 0.84f, 0.46f, 1.0f); }' `
    'Success green'

Replace-Optional `
    'static ImVec4 colorDanger\(\)\s*\{\s*return ImVec4\([^;]+;\s*\}' `
    'static ImVec4 colorDanger() { return ImVec4(0.93f, 0.40f, 0.36f, 1.0f); }' `
    'Danger red'

# ============================================================
# IMGUI GLOBAL CONTROL PALETTE
# ============================================================

$theme = @(
    @('c\[ImGuiCol_FrameBg\]\s*=\s*ImVec4\([^;]+;',          'c[ImGuiCol_FrameBg] = ImVec4(0.145f, 0.152f, 0.165f, 1.0f);', 'Input background'),
    @('c\[ImGuiCol_FrameBgHovered\]\s*=\s*ImVec4\([^;]+;',   'c[ImGuiCol_FrameBgHovered] = ImVec4(0.190f, 0.200f, 0.218f, 1.0f);', 'Input hover'),
    @('c\[ImGuiCol_FrameBgActive\]\s*=\s*ImVec4\([^;]+;',    'c[ImGuiCol_FrameBgActive] = ImVec4(0.225f, 0.238f, 0.258f, 1.0f);', 'Input active'),

    @('c\[ImGuiCol_TitleBg\]\s*=\s*ImVec4\([^;]+;',          'c[ImGuiCol_TitleBg] = ImVec4(0.090f, 0.095f, 0.103f, 1.0f);', 'Dock title background'),
    @('c\[ImGuiCol_TitleBgActive\]\s*=\s*ImVec4\([^;]+;',    'c[ImGuiCol_TitleBgActive] = ImVec4(0.125f, 0.132f, 0.145f, 1.0f);', 'Active dock title'),
    @('c\[ImGuiCol_MenuBarBg\]\s*=\s*ImVec4\([^;]+;',        'c[ImGuiCol_MenuBarBg] = ImVec4(0.090f, 0.095f, 0.104f, 1.0f);', 'Menu bar'),

    @('c\[ImGuiCol_Button\]\s*=\s*ImVec4\([^;]+;',           'c[ImGuiCol_Button] = ImVec4(0.145f, 0.152f, 0.165f, 1.0f);', 'Button base'),
    @('c\[ImGuiCol_ButtonHovered\]\s*=\s*ImVec4\([^;]+;',    'c[ImGuiCol_ButtonHovered] = ImVec4(0.205f, 0.216f, 0.235f, 1.0f);', 'Button hover'),
    @('c\[ImGuiCol_ButtonActive\]\s*=\s*ImVec4\([^;]+;',     'c[ImGuiCol_ButtonActive] = ImVec4(0.250f, 0.263f, 0.286f, 1.0f);', 'Button active'),

    @('c\[ImGuiCol_Header\]\s*=\s*ImVec4\([^;]+;',           'c[ImGuiCol_Header] = ImVec4(0.135f, 0.143f, 0.156f, 1.0f);', 'Header/tree base'),
    @('c\[ImGuiCol_HeaderHovered\]\s*=\s*ImVec4\([^;]+;',    'c[ImGuiCol_HeaderHovered] = ImVec4(0.190f, 0.202f, 0.220f, 1.0f);', 'Header/tree hover'),
    @('c\[ImGuiCol_HeaderActive\]\s*=\s*ImVec4\([^;]+;',     'c[ImGuiCol_HeaderActive] = ImVec4(0.220f, 0.234f, 0.255f, 1.0f);', 'Header/tree active'),

    @('c\[ImGuiCol_Separator\]\s*=\s*ImVec4\([^;]+;',        'c[ImGuiCol_Separator] = ImVec4(0.235f, 0.250f, 0.275f, 1.0f);', 'Separators'),

    @('c\[ImGuiCol_Tab\]\s*=\s*ImVec4\([^;]+;',              'c[ImGuiCol_Tab] = ImVec4(0.105f, 0.112f, 0.122f, 1.0f);', 'Inactive tabs'),
    @('c\[ImGuiCol_TabHovered\]\s*=\s*ImVec4\([^;]+;',       'c[ImGuiCol_TabHovered] = ImVec4(0.195f, 0.205f, 0.224f, 1.0f);', 'Tab hover'),
    @('c\[ImGuiCol_TabActive\]\s*=\s*ImVec4\([^;]+;',        'c[ImGuiCol_TabActive] = ImVec4(0.158f, 0.168f, 0.184f, 1.0f);', 'Active tab'),
    @('c\[ImGuiCol_TabUnfocused\]\s*=\s*ImVec4\([^;]+;',     'c[ImGuiCol_TabUnfocused] = ImVec4(0.090f, 0.095f, 0.105f, 1.0f);', 'Unfocused tab'),
    @('c\[ImGuiCol_TabUnfocusedActive\]\s*=\s*ImVec4\([^;]+;', 'c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.125f, 0.133f, 0.145f, 1.0f);', 'Unfocused active tab'),

    @('c\[ImGuiCol_TableHeaderBg\]\s*=\s*ImVec4\([^;]+;',    'c[ImGuiCol_TableHeaderBg] = ImVec4(0.160f, 0.170f, 0.185f, 1.0f);', 'Table header'),
    @('c\[ImGuiCol_TableRowBgAlt\]\s*=\s*ImVec4\([^;]+;',    'c[ImGuiCol_TableRowBgAlt] = ImVec4(0.090f, 0.095f, 0.105f, 1.0f);', 'Alternate table rows')
)

foreach ($item in $theme) {
    Replace-Required $item[0] $item[1] $item[2]
}

# Optional global colors if present in the current ImGui setup.
Replace-Optional `
    'c\[ImGuiCol_TextDisabled\]\s*=\s*[^;]+;' `
    'c[ImGuiCol_TextDisabled] = colorMuted();' `
    'Disabled text'

Replace-Optional `
    'c\[ImGuiCol_ChildBg\]\s*=\s*[^;]+;' `
    'c[ImGuiCol_ChildBg] = colorPanel();' `
    'Child panel background'

Replace-Optional `
    'c\[ImGuiCol_PopupBg\]\s*=\s*[^;]+;' `
    'c[ImGuiCol_PopupBg] = colorPanelRaised();' `
    'Popup background'

# ============================================================
# FONT + CONTROL DENSITY
# ============================================================

$fontBefore = $text

$text = [regex]::Replace(
    $text,
    'AddFontFromFileTTF\(\s*rubikPath\.string\(\)\.c_str\(\)\s*,\s*(?:16\.0f|17\.5f)\s*\)',
    'AddFontFromFileTTF(rubikPath.string().c_str(),17.5f)'
)

$text = $text -replace `
    'rubikPath\.string\(\)\.c_str\(\),(?:16\.0f|17\.5f)', `
    'rubikPath.string().c_str(),17.5f'

if ($text -eq $fontBefore) {
    Write-Host "[SKIP] Font already uses another size; leaving it unchanged." -ForegroundColor DarkGray
}
else {
    Write-Host "[OK] Rubik font -> 17.5 px" -ForegroundColor Green
}

Replace-Optional `
    'style\.FramePadding\s*=\s*ImVec2\([^;]+;' `
    'style.FramePadding = ImVec2(9.0f, 6.0f);' `
    'Frame padding'

Replace-Optional `
    'style\.ItemSpacing\s*=\s*ImVec2\([^;]+;' `
    'style.ItemSpacing = ImVec2(8.0f, 7.0f);' `
    'Item spacing'

Replace-Optional `
    'style\.CellPadding\s*=\s*ImVec2\([^;]+;' `
    'style.CellPadding = ImVec2(9.0f, 6.0f);' `
    'Table cell padding'

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

if ($verify -notmatch 'colorText\(\).*0\.965f') {
    throw "Verification failed: primary text palette was not written."
}

if ($verify -notmatch 'ImGuiCol_TabActive') {
    throw "Verification failed: tab palette missing."
}

Write-Host ""
Write-Host "[DONE] Palette/readability pass v2 installed." -ForegroundColor Cyan
Write-Host "[OK] Text contrast increased." -ForegroundColor Green
Write-Host "[OK] Panels now have stronger visual hierarchy." -ForegroundColor Green
Write-Host "[OK] Controls/tabs use distinct graphite levels." -ForegroundColor Green
Write-Host "[OK] Sovereign amber retained as the accent." -ForegroundColor Green
Write-Host "[OK] Layout and docking were not redesigned." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild now." -ForegroundColor Cyan
Write-Host ""
