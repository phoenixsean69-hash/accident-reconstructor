$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host " SOVEREIGN - TOOLS SIDEBAR FIX" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

$source = Join-Path (Get-Location) "src\main.cpp"

if (!(Test-Path $source)) {
    Write-Host "ERROR: src\main.cpp not found." -ForegroundColor Red
    exit 1
}

$text = Get-Content $source -Raw

Write-Host "Using:" -ForegroundColor Gray
Write-Host $source
Write-Host ""

# ---------------------------------------------------------
# BACKUP
# ---------------------------------------------------------

$backup = "$source.before_tools_fix.cpp"

if (!(Test-Path $backup)) {
    Copy-Item $source $backup
    Write-Host "[OK] Backup created:" -ForegroundColor Green
    Write-Host "     $backup"
}
else {
    Write-Host "[OK] Existing backup preserved." -ForegroundColor Yellow
}

# ---------------------------------------------------------
# FIND LEFT DOCK SPLIT
# ---------------------------------------------------------

$leftPattern = '(?s)(ImGuiDir_Left\s*,\s*)([0-9]+(?:\.[0-9]+)?f)(\s*,\s*&left\s*,\s*&center\s*\))'

$leftMatch = [regex]::Match($text, $leftPattern)

if (!$leftMatch.Success) {

    Write-Host ""
    Write-Host "ERROR: I cannot find the LEFT DockBuilder split in YOUR local main.cpp." -ForegroundColor Red
    Write-Host ""
    Write-Host "Here are the DockBuilder lines I can see:" -ForegroundColor Yellow
    Write-Host ""

    Select-String `
        -Path $source `
        -Pattern "DockBuilder|ImGuiDir_Left|ImGuiDir_Right|ImGuiDir_Down|Tools" |
        Select-Object -First 40 |
        ForEach-Object {
            Write-Host ("{0}: {1}" -f $_.LineNumber, $_.Line.Trim())
        }

    Write-Host ""
    Write-Host "NOTHING WAS CHANGED." -ForegroundColor Red
    exit 1
}

$oldRatio = $leftMatch.Groups[2].Value

Write-Host "[FOUND] Left dock split uses: $oldRatio" -ForegroundColor Green

# ---------------------------------------------------------
# INSERT TOOLS WIDTH CALCULATION
# ---------------------------------------------------------

if ($text -notmatch 'constexpr\s+float\s+TOOLS_WIDTH') {

    $anchor = '(ImGuiID\s+rightTop\s*=\s*0\s*;\s*)'

    if ($text -notmatch $anchor) {
        Write-Host ""
        Write-Host "ERROR: Could not find rightTop declaration." -ForegroundColor Red
        Write-Host "Nothing was changed."
        exit 1
    }

    $insert = @'
$1
        // Fixed-width Tools rail.
        constexpr float TOOLS_WIDTH = 280.0f;
        const float toolsRatio =
            ImClamp(
                TOOLS_WIDTH / viewport->WorkSize.x,
                0.12f,
                0.22f
            );

'@

    $text = [regex]::Replace(
        $text,
        $anchor,
        $insert,
        1
    )

    Write-Host "[OK] Added 280px Tools width calculation." -ForegroundColor Green
}
else {
    Write-Host "[OK] Tools width calculation already exists." -ForegroundColor Yellow
}

# ---------------------------------------------------------
# RE-FIND LEFT SPLIT AFTER INSERTION
# ---------------------------------------------------------

$leftMatch = [regex]::Match($text, $leftPattern)

if (!$leftMatch.Success) {
    Write-Host ""
    Write-Host "ERROR: Left split disappeared unexpectedly." -ForegroundColor Red
    exit 1
}

# Replace whatever numeric ratio is there with toolsRatio
$text = [regex]::Replace(
    $text,
    $leftPattern,
    '${1}toolsRatio${3}',
    1
)

Write-Host "[OK] Left dock now uses toolsRatio." -ForegroundColor Green

# ---------------------------------------------------------
# FIND TOOLS DOCK WINDOW
# ---------------------------------------------------------

$toolsPattern = 'ImGui::DockBuilderDockWindow\s*\(\s*"Tools"\s*,\s*left\s*\)'

if ($text -notmatch $toolsPattern) {

    Write-Host ""
    Write-Host "ERROR: Could not find Tools DockBuilder window." -ForegroundColor Red
    Write-Host ""
    Write-Host "Searching your LOCAL source for Tools references:" -ForegroundColor Yellow
    Write-Host ""

    Select-String `
        -Path $source `
        -Pattern "Tools" |
        Select-Object -First 40 |
        ForEach-Object {
            Write-Host ("{0}: {1}" -f $_.LineNumber, $_.Line.Trim())
        }

    Write-Host ""
    Write-Host "NOTHING WAS WRITTEN." -ForegroundColor Red
    exit 1
}

Write-Host "[FOUND] Tools dock window." -ForegroundColor Green

# ---------------------------------------------------------
# ADD NORESIZEX
# ---------------------------------------------------------

if ($text -notmatch 'ImGuiDockNodeFlags_NoResizeX') {

    $replacement = @'
ImGui::DockBuilderDockWindow("Tools", left);

// Prevent the Tools rail from being dragged horizontally.
if (ImGuiDockNode* toolsNode = ImGui::DockBuilderGetNode(left))
{
    toolsNode->LocalFlags |= ImGuiDockNodeFlags_NoResizeX;
}
'@

    $text = [regex]::Replace(
        $text,
        $toolsPattern,
        $replacement,
        1
    )

    Write-Host "[OK] Tools horizontal resizing disabled." -ForegroundColor Green
}
else {
    Write-Host "[OK] NoResizeX already present." -ForegroundColor Yellow
}

# ---------------------------------------------------------
# DISABLE SAVED IMGUI LAYOUT
# ---------------------------------------------------------

if ($text -notmatch 'io\.IniFilename\s*=\s*nullptr') {

    $ioPattern = 'ImGuiIO\s*&\s*io\s*=\s*ImGui::GetIO\s*\(\s*\)\s*;'

    if ($text -notmatch $ioPattern) {
        Write-Host ""
        Write-Host "ERROR: Could not find ImGuiIO initialization." -ForegroundColor Red
        Write-Host "Nothing was written."
        exit 1
    }

    $ioReplacement = @'
ImGuiIO& io = ImGui::GetIO();

// Application owns the dock layout.
// Do not restore a previously dragged ImGui layout.
io.IniFilename = nullptr;
'@

    $text = [regex]::Replace(
        $text,
        $ioPattern,
        $ioReplacement,
        1
    )

    Write-Host "[OK] Disabled saved ImGui docking layout." -ForegroundColor Green
}
else {
    Write-Host "[OK] ImGui saved layout already disabled." -ForegroundColor Yellow
}

# ---------------------------------------------------------
# WRITE
# ---------------------------------------------------------

Set-Content `
    -Path $source `
    -Value $text `
    -Encoding UTF8

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host " FIX APPLIED SUCCESSFULLY" -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Tools width : ~280px"
Write-Host "Dragging    : DISABLED"
Write-Host "Old layout  : DISABLED"
Write-Host ""
Write-Host "Backup:"
Write-Host "  $backup"
Write-Host ""
Write-Host "Now rebuild the application." -ForegroundColor Cyan
Write-Host ""