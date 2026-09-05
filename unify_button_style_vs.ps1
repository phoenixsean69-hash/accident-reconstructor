param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - UNIFY BUTTON STYLE (APP-WIDE)" -ForegroundColor Cyan
Write-Host " VS / Unreal-Unity inspired buttons + bolder labels" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if (-not $text.Contains('static void drawInterface()')) {
    throw "Could not find drawInterface()."
}

if (-not $text.Contains('static bool editorButton(')) {
    throw "Could not find editorButton()."
}

if ($text.Contains('static void pushUnifiedButtonTheme()')) {
    Write-Host "[OK] Unified button theme already appears installed." -ForegroundColor Green
    exit 0
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-unified-buttons-$timestamp.bak"
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
        Text = $Source.Substring($start, $end - $start + 1)
    }
}

# ============================================================
# 1) INSTALL GLOBAL BUTTON-THEME HELPERS
# ============================================================

$insertPos = $text.IndexOf('static void drawInterface()')
if ($insertPos -lt 0) {
    throw "Could not locate insertion point before drawInterface()."
}

$helperCode = @'
static void pushUnifiedButtonTheme()
{
    ImGui::PushStyleVar(
        ImGuiStyleVar_FrameRounding,
        4.0f
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

static void popUnifiedButtonTheme()
{
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(3);
}

static void drawUnifiedButtonLabel(
    ImDrawList* drawList,
    const ImVec2& minPos,
    const ImVec2& maxPos,
    const char* label,
    const ImVec4& textColor,
    bool enabled)
{
    if (!label || !label[0])
        return;

    const ImVec2 textSize=
        ImGui::CalcTextSize(label);

    const ImVec2 textPos=
        ImVec2(
            minPos.x+(maxPos.x-minPos.x-textSize.x)*0.5f,
            minPos.y+(maxPos.y-minPos.y-textSize.y)*0.5f-0.5f
        );

    ImVec4 primary=textColor;
    ImVec4 secondary=textColor;

    if (!enabled)
    {
        primary=ImVec4(
            primary.x*0.75f,
            primary.y*0.75f,
            primary.z*0.75f,
            0.85f
        );

        secondary=primary;
    }

    drawList->AddText(
        textPos,
        ImGui::ColorConvertFloat4ToU32(primary),
        label
    );

    drawList->AddText(
        ImVec2(textPos.x+0.55f,textPos.y),
        ImGui::ColorConvertFloat4ToU32(secondary),
        label
    );
}
'@

$text = $text.Insert($insertPos, $helperCode)

Write-Host "[OK] Added global button-theme helpers." -ForegroundColor Green

# ============================================================
# 2) REPLACE editorButton() WITH NEW VISUAL STYLE
# ============================================================

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
            ? ImVec4(0.12f,0.42f,0.78f,1.0f)
            : ImVec4(0.19f,0.20f,0.22f,1.0f);

    ImVec4 border=
        active
            ? ImVec4(0.34f,0.61f,0.95f,1.0f)
            : ImVec4(0.35f,0.37f,0.41f,1.0f);

    ImVec4 accent=
        active
            ? ImVec4(0.53f,0.76f,1.0f,1.0f)
            : ImVec4(0.96f,0.82f,0.18f,0.95f);

    ImVec4 textColor=
        enabled
            ? ImVec4(0.97f,0.98f,1.0f,1.0f)
            : ImVec4(0.72f,0.74f,0.78f,1.0f);

    if (!enabled)
    {
        bg=ImVec4(0.16f,0.17f,0.18f,1.0f);
        border=ImVec4(0.26f,0.27f,0.29f,1.0f);
        accent=ImVec4(0.32f,0.33f,0.35f,1.0f);
    }
    else if (held)
    {
        if (active)
        {
            bg=ImVec4(0.10f,0.36f,0.69f,1.0f);
            border=ImVec4(0.29f,0.54f,0.88f,1.0f);
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
            bg=ImVec4(0.15f,0.47f,0.84f,1.0f);
            border=ImVec4(0.48f,0.73f,1.0f,1.0f);
        }
        else
        {
            bg=ImVec4(0.23f,0.24f,0.27f,1.0f);
            border=ImVec4(0.50f,0.53f,0.58f,1.0f);
        }
    }

    const float rounding=4.0f;

    drawList->AddRectFilled(
        pos,
        ImVec2(pos.x+size.x,pos.y+size.y),
        ImGui::ColorConvertFloat4ToU32(bg),
        rounding
    );

    drawList->AddRect(
        pos,
        ImVec2(pos.x+size.x,pos.y+size.y),
        ImGui::ColorConvertFloat4ToU32(border),
        rounding,
        0,
        1.0f
    );

    drawList->AddRectFilled(
        pos,
        ImVec2(pos.x+size.x,pos.y+2.0f),
        ImGui::ColorConvertFloat4ToU32(accent),
        rounding,
        ImDrawFlags_RoundCornersTop
    );

    drawUnifiedButtonLabel(
        drawList,
        pos,
        ImVec2(pos.x+size.x,pos.y+size.y),
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

Write-Host "[OK] Replaced editorButton() with VS-style renderer." -ForegroundColor Green

# ============================================================
# 3) APPLY GLOBAL THEME AROUND WHOLE UI
# ============================================================

$interfaceInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void drawInterface()'

$interface = $interfaceInfo.Text

if (-not $interface.Contains('pushUnifiedButtonTheme();'))
{
    if (-not $interface.Contains('handleGlobalShortcuts();')) {
        throw "Could not find handleGlobalShortcuts() in drawInterface()."
    }

    $interface = $interface.Replace(
        'handleGlobalShortcuts();',
@'
handleGlobalShortcuts();
    pushUnifiedButtonTheme();
'@
    )
}

if (-not $interface.Contains('popUnifiedButtonTheme();'))
{
    $lastBrace = $interface.LastIndexOf('}')
    if ($lastBrace -lt 0) {
        throw "Could not find closing brace for drawInterface()."
    }

    $interface = $interface.Insert(
        $lastBrace,
@'

    popUnifiedButtonTheme();
'@
    )
}

$text = $text.Remove(
    $interfaceInfo.Start,
    $interfaceInfo.End - $interfaceInfo.Start + 1
).Insert(
    $interfaceInfo.Start,
    $interface
)

Write-Host "[OK] Applied unified button theme across the full UI." -ForegroundColor Green

# ============================================================
# 4) BUMP DEFAULT BUTTON TEXT A TOUCH WHERE CURRENT CODE USES
#    SMALL INLINE BUTTONS VIA STYLE
# ============================================================

if (-not $text.Contains('ImGuiStyleVar_ItemSpacing')) {
    Write-Host "[INFO] No extra inline-spacing patch needed." -ForegroundColor DarkGray
}

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'static void pushUnifiedButtonTheme()',
    'static void drawUnifiedButtonLabel(',
    'static bool editorButton(',
    'pushUnifiedButtonTheme();',
    'popUnifiedButtonTheme();',
    'ImGuiCol_ButtonActive',
    'ImGui::InvisibleButton(',
    'drawUnifiedButtonLabel(',
    'ImVec4(0.12f,0.42f,0.78f,1.0f)'
)

foreach ($check in $checks)
{
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

if ($verify.Contains('`r`n')) {
    throw "Verification failed: literal PowerShell newline text entered main.cpp."
}

Write-Host ""
Write-Host "[DONE] App-wide button restyle installed." -ForegroundColor Cyan
Write-Host "[OK] Global button palette switched to dark VS-style controls." -ForegroundColor Green
Write-Host "[OK] Emphasized buttons now use blue primary styling." -ForegroundColor Green
Write-Host "[OK] Secondary buttons now use dark charcoal styling." -ForegroundColor Green
Write-Host "[OK] Button labels render bolder in custom editor buttons." -ForegroundColor Green
Write-Host "[OK] Change applies across the app's button surfaces." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
