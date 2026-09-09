param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - ORDER MENUS / TABS + FONT SIZE" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'static void drawMainMenuBar()',
    'ImGui::DockBuilderDockWindow("Case View",center);',
    'ImGui::DockBuilderDockWindow("Evidence",center);',
    'ImGui::DockBuilderDockWindow("Analysis",center);',
    'ImGui::DockBuilderDockWindow("Viewport",center);',
    'Rubik-Regular.ttf'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-tab-order-font-$timestamp.bak"

Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ------------------------------------------------------------
# 1. Remove visual Analysis/Analysis naming conflict.
#    The menu contains analysis utilities, so call it Tools.
# ------------------------------------------------------------

$menuOld = 'if (ImGui::BeginMenu("Analysis"))'
$menuNew = 'if (ImGui::BeginMenu("Tools"))'

if ($text.Contains($menuOld)) {
    $text = $text.Replace($menuOld, $menuNew)
    Write-Host "[OK] Top menu: Analysis -> Tools" -ForegroundColor Green
}
elseif ($text.Contains($menuNew)) {
    Write-Host "[SKIP] Top menu is already named Tools." -ForegroundColor DarkGray
}
else {
    throw "Could not find the Analysis top-menu declaration."
}

# ------------------------------------------------------------
# 2. Put center workspace tabs in workflow order:
#    Case View -> Evidence -> Viewport -> Analysis
# ------------------------------------------------------------

$oldDockOrder = @'
        ImGui::DockBuilderDockWindow("Case View",center);
        ImGui::DockBuilderDockWindow("Evidence",center);
        ImGui::DockBuilderDockWindow("Analysis",center);
        ImGui::DockBuilderDockWindow("Viewport",center);
'@

$newDockOrder = @'
        ImGui::DockBuilderDockWindow("Case View",center);
        ImGui::DockBuilderDockWindow("Evidence",center);
        ImGui::DockBuilderDockWindow("Viewport",center);
        ImGui::DockBuilderDockWindow("Analysis",center);
'@

if ($text.Contains($oldDockOrder)) {
    $text = $text.Replace($oldDockOrder, $newDockOrder)
    Write-Host "[OK] Workspace order: Case View -> Evidence -> Viewport -> Analysis" -ForegroundColor Green
}
else {
    # Compact / different whitespace fallback.
    $dockPattern = '(?ms)^[ \t]*ImGui::DockBuilderDockWindow\("Case View",center\);\s*\r?\n[ \t]*ImGui::DockBuilderDockWindow\("Evidence",center\);\s*\r?\n[ \t]*ImGui::DockBuilderDockWindow\("Analysis",center\);\s*\r?\n[ \t]*ImGui::DockBuilderDockWindow\("Viewport",center\);'

    if ($text -match $dockPattern) {
        $text = [regex]::Replace(
            $text,
            $dockPattern,
            '        ImGui::DockBuilderDockWindow("Case View",center);' + "`r`n" +
            '        ImGui::DockBuilderDockWindow("Evidence",center);' + "`r`n" +
            '        ImGui::DockBuilderDockWindow("Viewport",center);' + "`r`n" +
            '        ImGui::DockBuilderDockWindow("Analysis",center);',
            1
        )
        Write-Host "[OK] Workspace order: Case View -> Evidence -> Viewport -> Analysis" -ForegroundColor Green
    }
    else {
        throw "Could not locate the four center DockBuilderDockWindow calls."
    }
}

# ------------------------------------------------------------
# 3. Slight font increase: 17.5 -> 18.5
# ------------------------------------------------------------

$fontPattern = 'AddFontFromFileTTF\(\s*rubikPath\.string\(\)\.c_str\(\)\s*,\s*17\.5f\s*\)'

if ($text -match $fontPattern) {
    $text = [regex]::Replace(
        $text,
        $fontPattern,
        'AddFontFromFileTTF(rubikPath.string().c_str(),18.5f)',
        1
    )
    Write-Host "[OK] Rubik font: 17.5 px -> 18.5 px" -ForegroundColor Green
}
elseif ($text -match 'AddFontFromFileTTF\([^;]+18\.5f') {
    Write-Host "[SKIP] Font is already 18.5 px." -ForegroundColor DarkGray
}
else {
    throw "Could not find the current 17.5 px Rubik font load."
}

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

# ------------------------------------------------------------
# VERIFY
# ------------------------------------------------------------

$verify = Get-Content $MainCpp -Raw

if (-not $verify.Contains('ImGui::BeginMenu("Tools")')) {
    throw "Verification failed: Tools menu not found."
}

if ($verify.Contains('ImGui::BeginMenu("Analysis")')) {
    throw "Verification failed: old Analysis top-menu still exists."
}

$viewportPos = $verify.IndexOf('ImGui::DockBuilderDockWindow("Viewport",center);')
$analysisPos = $verify.IndexOf('ImGui::DockBuilderDockWindow("Analysis",center);')

if ($viewportPos -lt 0 -or $analysisPos -lt 0 -or $viewportPos -gt $analysisPos) {
    throw "Verification failed: Viewport is not before Analysis in dock order."
}

if ($verify -notmatch 'AddFontFromFileTTF\([^;]+18\.5f') {
    throw "Verification failed: 18.5 px font not written."
}

Write-Host ""
Write-Host "[DONE] Navigation cleanup installed." -ForegroundColor Cyan
Write-Host "[OK] Top menu now reads File / Edit / View / Scene / Tools / Help." -ForegroundColor Green
Write-Host "[OK] Tabs now follow Case View / Evidence / Viewport / Analysis." -ForegroundColor Green
Write-Host "[OK] Global UI font increased slightly to 18.5 px." -ForegroundColor Green
Write-Host "[OK] Palette and screen content untouched." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
