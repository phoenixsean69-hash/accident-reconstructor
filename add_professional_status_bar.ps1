param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - PROFESSIONAL STATUS BAR" -ForegroundColor Cyan
Write-Host " Phase 1 - Workstation shell" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'struct EditorShellState',
    'static const char* selectedEntityName()',
    'static void drawInterface()',
    'SovereignDockspace',
    'gEditorShell.snapEnabled',
    'gEditorShell.snapValue'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected current editor marker not found: $marker"
    }
}

if ($text.Contains('static void drawSovereignStatusBar()')) {
    Write-Host "[OK] Sovereign status bar already installed." -ForegroundColor Green
    exit 0
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-status-bar-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

function Get-FunctionBlockInfo {
    param(
        [string]$Source,
        [string]$Signature
    )

    $start = $Source.IndexOf($Signature)

    if ($start -lt 0) {
        throw "Could not find function: $Signature"
    }

    $braceStart = $Source.IndexOf("{", $start)

    if ($braceStart -lt 0) {
        throw "Could not find opening brace for: $Signature"
    }

    $depth = 0
    $inString = $false
    $inChar = $false
    $escape = $false
    $end = -1

    for ($i = $braceStart; $i -lt $Source.Length; $i++) {
        $ch = $Source[$i]

        if ($escape) {
            $escape = $false
            continue
        }

        if ($inString) {
            if ($ch -eq '\') {
                $escape = $true
                continue
            }

            if ($ch -eq '"') {
                $inString = $false
            }

            continue
        }

        if ($inChar) {
            if ($ch -eq '\') {
                $escape = $true
                continue
            }

            if ($ch -eq "'") {
                $inChar = $false
            }

            continue
        }

        if ($ch -eq '"') {
            $inString = $true
            continue
        }

        if ($ch -eq "'") {
            $inChar = $true
            continue
        }

        if ($ch -eq "{") {
            $depth++
            continue
        }

        if ($ch -eq "}") {
            $depth--

            if ($depth -eq 0) {
                $end = $i
                break
            }
        }
    }

    if ($end -lt 0) {
        throw "Could not find closing brace for: $Signature"
    }

    return @{
        Start = $start
        End = $end
        Text = $Source.Substring(
            $start,
            $end - $start + 1
        )
    }
}

# ============================================================
# 1. ADD STATUS BAR FUNCTION BEFORE drawInterface()
# ============================================================

$interfacePos = $text.IndexOf('static void drawInterface()')

if ($interfacePos -lt 0) {
    throw "Could not locate drawInterface()."
}

$statusCode = @'
static constexpr float SOVEREIGN_STATUS_BAR_HEIGHT=30.0f;

static void drawSovereignStatusBar()
{
    const ImGuiViewport* viewport=
        ImGui::GetMainViewport();

    if (!viewport)
        return;

    const ImVec2 statusPos(
        viewport->WorkPos.x,
        viewport->WorkPos.y+
        viewport->WorkSize.y-
        SOVEREIGN_STATUS_BAR_HEIGHT
    );

    const ImVec2 statusSize(
        viewport->WorkSize.x,
        SOVEREIGN_STATUS_BAR_HEIGHT
    );

    ImGui::SetNextWindowPos(
        statusPos,
        ImGuiCond_Always
    );

    ImGui::SetNextWindowSize(
        statusSize,
        ImGuiCond_Always
    );

    ImGui::SetNextWindowViewport(
        viewport->ID
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(10.0f,5.0f)
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowBorderSize,
        1.0f
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowRounding,
        0.0f
    );

    ImGui::PushStyleColor(
        ImGuiCol_WindowBg,
        ImVec4(0.075f,0.079f,0.086f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.23f,0.245f,0.27f,1.0f)
    );

    const ImGuiWindowFlags flags=
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin(
        "##SovereignStatusBar",
        nullptr,
        flags))
    {
        auto separator = []()
        {
            ImGui::SameLine(0.0f,10.0f);

            ImGui::TextColored(
                ImVec4(0.31f,0.33f,0.37f,1.0f),
                "|"
            );

            ImGui::SameLine(0.0f,10.0f);
        };

        // ----------------------------------------------------
        // LEFT: application state
        // ----------------------------------------------------

        ImGui::TextColored(
            ImVec4(0.53f,0.80f,0.56f,1.0f),
            "READY"
        );

        separator();

        ImGui::TextDisabled(
            "Scene"
        );

        ImGui::SameLine(0.0f,5.0f);

        ImGui::Text(
            "9 objects"
        );

        separator();

        ImGui::TextDisabled(
            "Selected"
        );

        ImGui::SameLine(0.0f,5.0f);

        const char* selectedName=
            selectedEntityName();

        ImGui::Text(
            "%s",
            selectedName &&
            selectedName[0]
                ? selectedName
                : "None"
        );

        separator();

        ImGui::TextDisabled(
            "Snap"
        );

        ImGui::SameLine(0.0f,5.0f);

        if (gEditorShell.snapEnabled)
        {
            ImGui::Text(
                "%.2f m",
                gEditorShell.snapValue
            );
        }
        else
        {
            ImGui::TextDisabled(
                "Off"
            );
        }

        separator();

        ImGui::TextDisabled(
            "Units"
        );

        ImGui::SameLine(0.0f,5.0f);

        ImGui::Text(
            "Metric"
        );

        // ----------------------------------------------------
        // RIGHT: performance + discoverability
        // ----------------------------------------------------

        const float fps=
            ImGui::GetIO().Framerate;

        char rightText[128]{};

        std::snprintf(
            rightText,
            sizeof(rightText),
            "%.0f FPS    |    F1 Shortcuts",
            fps
        );

        const float rightWidth=
            ImGui::CalcTextSize(
                rightText
            ).x;

        const float targetX=
            ImGui::GetWindowContentRegionMax().x-
            rightWidth;

        if (targetX>
            ImGui::GetCursorPosX()+20.0f)
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(
                targetX
            );

            ImGui::TextDisabled(
                "%s",
                rightText
            );
        }
    }

    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

'@

$text = $text.Insert(
    $interfacePos,
    $statusCode
)

Write-Host "[OK] Added persistent status-bar renderer." -ForegroundColor Green

# ============================================================
# 2. RESERVE STATUS-BAR HEIGHT FROM WORKSPACE HOST
# ============================================================

$interfaceInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void drawInterface()'

$interface = $interfaceInfo.Text

# Handle the common ImGui host patterns:
#   SetNextWindowSize(viewport->WorkSize);
#   SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
# with arbitrary whitespace/newlines.

$sizePattern =
    'ImGui::SetNextWindowSize\s*\(\s*viewport->WorkSize\s*(,\s*ImGuiCond_Always\s*)?\);'

$sizeReplacement = @'
ImGui::SetNextWindowSize(
        ImVec2(
            viewport->WorkSize.x,
            std::max(
                100.0f,
                viewport->WorkSize.y-
                SOVEREIGN_STATUS_BAR_HEIGHT
            )
        ),
        ImGuiCond_Always
    );
'@

if ([regex]::IsMatch(
    $interface,
    $sizePattern))
{
    $interface = [regex]::Replace(
        $interface,
        $sizePattern,
        $sizeReplacement,
        1
    )

    Write-Host "[OK] Reserved 30 px below dockspace for status bar." -ForegroundColor Green
}
else
{
    # Alternative local variable pattern.
    $workSizePattern =
        'ImVec2\s+workSize\s*=\s*viewport->WorkSize\s*;'

    if ([regex]::IsMatch(
        $interface,
        $workSizePattern))
    {
        $interface = [regex]::Replace(
            $interface,
            $workSizePattern,
@'
ImVec2 workSize=viewport->WorkSize;
    workSize.y=std::max(
        100.0f,
        workSize.y-
        SOVEREIGN_STATUS_BAR_HEIGHT
    );
'@,
            1
        )

        Write-Host "[OK] Reduced workspace workSize to reserve status bar." -ForegroundColor Green
    }
    else
    {
        throw "Could not locate workspace SetNextWindowSize/workSize reservation point."
    }
}

# ============================================================
# 3. DRAW STATUS BAR EVERY FRAME
# ============================================================

if (-not $interface.Contains(
    'drawSovereignStatusBar();'))
{
    # Put it late in drawInterface, but before unified theme is popped
    # when that newer styling patch exists.

    if ($interface.Contains(
        'popUnifiedButtonTheme();'))
    {
        $interface = $interface.Replace(
            'popUnifiedButtonTheme();',
@'
drawSovereignStatusBar();

    popUnifiedButtonTheme();
'@
        )
    }
    else
    {
        $lastBrace=
            $interface.LastIndexOf('}')

        if ($lastBrace -lt 0) {
            throw "Could not locate end of drawInterface()."
        }

        $interface = $interface.Insert(
            $lastBrace,
@'

    drawSovereignStatusBar();
'@
        )
    }
}

$text = $text.Remove(
    $interfaceInfo.Start,
    $interfaceInfo.End -
    $interfaceInfo.Start + 1
).Insert(
    $interfaceInfo.Start,
    $interface
)

Write-Host "[OK] Status bar wired into application frame." -ForegroundColor Green

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content `
    -Path $MainCpp `
    -Value $text `
    -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'SOVEREIGN_STATUS_BAR_HEIGHT',
    'static void drawSovereignStatusBar()',
    '"##SovereignStatusBar"',
    '"READY"',
    '"9 objects"',
    '"Metric"',
    '"F1 Shortcuts"',
    'drawSovereignStatusBar();',
    'viewport->WorkSize.y-',
    'gEditorShell.snapValue',
    'selectedEntityName()'
)

foreach ($check in $checks)
{
    if (-not $verify.Contains($check))
    {
        throw "Verification failed after write: $check"
    }
}

$badPair=
    [string]([char]96)+
    "r"+
    [char]96+
    "n"

if ($verify.Contains($badPair))
{
    throw "Verification failed: literal PowerShell newline text entered main.cpp."
}

Write-Host ""
Write-Host "[DONE] Professional status bar installed." -ForegroundColor Cyan
Write-Host "[OK] Workspace reserves its own bottom strip." -ForegroundColor Green
Write-Host "[OK] READY / scene count / selection." -ForegroundColor Green
Write-Host "[OK] Snap value / metric units." -ForegroundColor Green
Write-Host "[OK] Live FPS / F1 shortcut hint." -ForegroundColor Green
Write-Host "[OK] No extra toolbar or dashboard clutter." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
