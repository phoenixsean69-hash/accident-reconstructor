param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - GLOBAL BUTTON SHAPE / TOP-LINE FIX" -ForegroundColor Cyan
Write-Host " All buttons: 4-corner rounding + remove top accent lines" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

function Get-FunctionBlockInfo {
    param(
        [string]$Source,
        [string]$Signature
    )

    $start = $Source.IndexOf($Signature)

    if ($start -lt 0) {
        throw "Could not find function signature: $Signature"
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
        End   = $end
        Text  = $Source.Substring($start, $end - $start + 1)
    }
}

if (-not $text.Contains('static void pushUnifiedButtonTheme()')) {
    throw "Could not find pushUnifiedButtonTheme(). Run this on the current patched UI source."
}

if (-not $text.Contains('static bool editorButton(')) {
    throw "Could not find editorButton()."
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-button-shape-fix-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ------------------------------------------------------------
# 1) Replace global unified button theme
# ------------------------------------------------------------
$themeInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void pushUnifiedButtonTheme()'

$newTheme = @'
static void pushUnifiedButtonTheme()
{
    ImGui::PushStyleVar(
        ImGuiStyleVar_FrameRounding,
        5.0f
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_FrameBorderSize,
        1.0f
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(12.0f,7.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(0.18f,0.19f,0.21f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(0.23f,0.25f,0.28f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.14f,0.36f,0.67f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.34f,0.37f,0.42f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.95f,0.96f,0.98f,1.0f)
    );
}
'@

$text = $text.Remove(
    $themeInfo.Start,
    $themeInfo.End - $themeInfo.Start + 1
).Insert(
    $themeInfo.Start,
    $newTheme
)

Write-Host "[OK] Updated global button rounding theme." -ForegroundColor Green

# ------------------------------------------------------------
# 2) Replace custom editorButton renderer
#    - no top accent line
#    - all 4 corners rounded
#    - slightly tighter, cleaner professional shape
# ------------------------------------------------------------
$editorInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static bool editorButton('

$newEditorButton = @'
static bool editorButton(
    const char* label,
    float width,
    bool active,
    bool enabled=true)
{
    ImGuiWindow* window=
        ImGui::GetCurrentWindow();

    if (window->SkipItems)
        return false;

    const ImGuiStyle& style=
        ImGui::GetStyle();

    const ImVec2 textSize=
        ImGui::CalcTextSize(label);

    const float buttonHeight=
        ImMax(
            ImGui::GetFrameHeight()+2.0f,
            textSize.y+style.FramePadding.y*2.0f+2.0f
        );

    ImVec2 size(
        width>0.0f
            ? width
            : textSize.x+style.FramePadding.x*2.0f+18.0f,
        buttonHeight
    );

    const ImVec2 pos=
        ImGui::GetCursorScreenPos();

    ImGui::PushID(
        static_cast<int>(pos.x)
    );
    ImGui::PushID(
        static_cast<int>(pos.y)
    );

    if (!enabled)
        ImGui::BeginDisabled();

    const bool pressed=
        ImGui::InvisibleButton(
            "##editorButton",
            size
        );

    const bool hovered=
        ImGui::IsItemHovered();

    const bool held=
        ImGui::IsItemActive();

    if (!enabled)
        ImGui::EndDisabled();

    ImGui::PopID();
    ImGui::PopID();

    ImDrawList* drawList=
        ImGui::GetWindowDrawList();

    ImVec4 bg=
        active
            ? ImVec4(0.13f,0.40f,0.76f,1.0f)
            : ImVec4(0.19f,0.20f,0.22f,1.0f);

    ImVec4 border=
        active
            ? ImVec4(0.35f,0.60f,0.93f,1.0f)
            : ImVec4(0.35f,0.37f,0.41f,1.0f);

    ImVec4 textColor=
        enabled
            ? ImVec4(0.97f,0.98f,1.0f,1.0f)
            : ImVec4(0.72f,0.74f,0.78f,1.0f);

    if (!enabled)
    {
        bg=ImVec4(0.16f,0.17f,0.18f,1.0f);
        border=ImVec4(0.26f,0.27f,0.29f,1.0f);
    }
    else if (held)
    {
        if (active)
        {
            bg=ImVec4(0.11f,0.34f,0.64f,1.0f);
            border=ImVec4(0.28f,0.50f,0.84f,1.0f);
        }
        else
        {
            bg=ImVec4(0.15f,0.16f,0.18f,1.0f);
            border=ImVec4(0.43f,0.46f,0.51f,1.0f);
        }
    }
    else if (hovered)
    {
        if (active)
        {
            bg=ImVec4(0.16f,0.46f,0.84f,1.0f);
            border=ImVec4(0.47f,0.71f,0.98f,1.0f);
        }
        else
        {
            bg=ImVec4(0.23f,0.24f,0.27f,1.0f);
            border=ImVec4(0.50f,0.53f,0.58f,1.0f);
        }
    }

    const float rounding=5.0f;
    const ImVec2 maxPos=
        ImVec2(pos.x+size.x,pos.y+size.y);

    drawList->AddRectFilled(
        pos,
        maxPos,
        ImGui::ColorConvertFloat4ToU32(bg),
        rounding
    );

    drawList->AddRect(
        pos,
        maxPos,
        ImGui::ColorConvertFloat4ToU32(border),
        rounding,
        0,
        1.0f
    );

    drawUnifiedButtonLabel(
        drawList,
        pos,
        maxPos,
        label,
        textColor,
        enabled
    );

    return enabled && pressed;
}
'@

$text = $text.Remove(
    $editorInfo.Start,
    $editorInfo.End - $editorInfo.Start + 1
).Insert(
    $editorInfo.Start,
    $newEditorButton
)

Write-Host "[OK] Replaced custom button renderer." -ForegroundColor Green

# ------------------------------------------------------------
# 3) Safety cleanup for any leftover top-accent snippets
# ------------------------------------------------------------
$leftoverPatterns = @(
    'ImDrawFlags_RoundCornersTop',
    'drawList->AddRectFilled(' + "`r`n" + '        pos,' + "`r`n" + '        ImVec2(pos.x+size.x,pos.y+2.0f),'
)

foreach ($pattern in $leftoverPatterns) {
    if ($text.Contains($pattern)) {
        Write-Host "[INFO] Leftover top-accent signature detected elsewhere; leaving untouched unless centralized." -ForegroundColor DarkYellow
    }
}

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'static void pushUnifiedButtonTheme()',
    'ImGuiStyleVar_FrameRounding',
    '5.0f',
    'static bool editorButton(',
    'drawUnifiedButtonLabel('
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

if ($verify.Contains('ImDrawFlags_RoundCornersTop') -and $verify.Contains('static bool editorButton(')) {
    $editorVerify = (Get-FunctionBlockInfo -Source $verify -Signature 'static bool editorButton(').Text
    if ($editorVerify.Contains('ImDrawFlags_RoundCornersTop')) {
        throw "Verification failed: editorButton() still contains top-corner accent rendering."
    }
}

if ($verify.Contains('pos.y+2.0f') -and $verify.Contains('static bool editorButton(')) {
    $editorVerify = (Get-FunctionBlockInfo -Source $verify -Signature 'static bool editorButton(').Text
    if ($editorVerify.Contains('pos.y+2.0f')) {
        throw "Verification failed: editorButton() still appears to draw a top accent strip."
    }
}

Write-Host ""
Write-Host "[DONE] Global button shape patch installed." -ForegroundColor Cyan
Write-Host "[OK] All unified/editor buttons now use subtle 4-corner rounding." -ForegroundColor Green
Write-Host "[OK] Top accent lines removed from the custom button renderer." -ForegroundColor Green
Write-Host "[OK] Active/selected buttons keep the blue VS-like styling without the top stripe." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild your app after running this script." -ForegroundColor DarkGray
