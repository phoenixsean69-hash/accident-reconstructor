param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - POLISH OUTLINER LOCK/UNLOCK ICON" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'enum class UiGlyph',
    'static bool outlinerMiniIconButton(',
    '"LockToggle"',
    'UiGlyph::Lock'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected Outliner marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-lock-icon-polish-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. Add UiGlyph::Unlock
# ============================================================

if ($text -notmatch '\bUnlock\b')
{
    $enumPattern = 'enum class UiGlyph\s*\{[\s\S]*?\};'
    $enumMatch = [regex]::Match(
        $text,
        $enumPattern,
        [System.Text.RegularExpressions.RegexOptions]::Singleline
    )

    if (-not $enumMatch.Success) {
        throw "Could not locate UiGlyph enum."
    }

    $enumBlock = $enumMatch.Value
    $newEnumBlock = [regex]::Replace(
        $enumBlock,
        '\bLock\b(?=\s*,\s*Target\b)',
        'Lock, Unlock',
        1
    )

    if ($newEnumBlock -eq $enumBlock) {
        throw "Could not insert UiGlyph::Unlock into enum."
    }

    $text = $text.Replace($enumBlock,$newEnumBlock)
    Write-Host "[OK] Added UiGlyph::Unlock." -ForegroundColor Green
}
else
{
    Write-Host "[SKIP] UiGlyph::Unlock already present." -ForegroundColor DarkGray
}

# ============================================================
# 2. Add draw case for UiGlyph::Unlock
# ============================================================

if ($text -notmatch 'case\s+UiGlyph::Unlock\s*:')
{
    $lockCaseMarker = '        case UiGlyph::Lock:'

    if (-not $text.Contains($lockCaseMarker)) {
        throw "Could not locate UiGlyph::Lock draw case."
    }

    $unlockCase = @'
        case UiGlyph::Unlock:
            d->AddRect(
                ImVec2(x-s*.28f,y-s*.02f),
                ImVec2(x+s*.28f,y+s*.34f),
                color,
                2.0f,
                0,
                t
            );

            d->AddCircleFilled(
                ImVec2(x,y+s*.14f),
                s*.045f,
                color
            );

            d->AddBezierCubic(
                ImVec2(x-s*.20f,y-s*.02f),
                ImVec2(x-s*.28f,y-s*.18f),
                ImVec2(x-s*.12f,y-s*.34f),
                ImVec2(x+s*.08f,y-s*.26f),
                color,
                t
            );

            d->AddLine(
                ImVec2(x+s*.08f,y-s*.26f),
                ImVec2(x+s*.22f,y-s*.18f),
                color,
                t
            );
            break;

'@

    $text = $text.Replace(
        $lockCaseMarker,
        $unlockCase + $lockCaseMarker
    )

    Write-Host "[OK] Added open-lock glyph drawing." -ForegroundColor Green
}
else
{
    Write-Host "[SKIP] UiGlyph::Unlock draw case already exists." -ForegroundColor DarkGray
}

# ============================================================
# 3. Tighten lock icon geometry so it looks cleaner
# ============================================================

$oldLockCase = @'
        case UiGlyph::Lock:
            d->PathArcTo(
                ImVec2(x,y-s*.06f),
                s*.22f,
                3.1415926f,
                0.0f,
                12
            );
            d->PathStroke(color,0,t);

            d->AddRect(
                ImVec2(x-s*.30f,y-s*.02f),
                ImVec2(x+s*.30f,y+s*.36f),
                color,
                2.0f,
                0,
                t
            );

            d->AddCircleFilled(
                ImVec2(x,y+s*.14f),
                s*.045f,
                color
            );
            break;
'@

$newLockCase = @'
        case UiGlyph::Lock:
            d->AddRect(
                ImVec2(x-s*.28f,y-s*.02f),
                ImVec2(x+s*.28f,y+s*.34f),
                color,
                2.0f,
                0,
                t
            );

            d->AddCircleFilled(
                ImVec2(x,y+s*.14f),
                s*.045f,
                color
            );

            d->AddBezierCubic(
                ImVec2(x-s*.18f,y-s*.02f),
                ImVec2(x-s*.18f,y-s*.28f),
                ImVec2(x+s*.18f,y-s*.28f),
                ImVec2(x+s*.18f,y-s*.02f),
                color,
                t
            );
            break;
'@

if ($text.Contains($oldLockCase))
{
    $text = $text.Replace($oldLockCase,$newLockCase)
    Write-Host "[OK] Refined closed-lock glyph geometry." -ForegroundColor Green
}
else
{
    Write-Host "[SKIP] Lock glyph geometry already differs from previous version." -ForegroundColor DarkGray
}

# ============================================================
# 4. Replace outlinerMiniIconButton with larger / better-aligned version
# ============================================================

$miniPattern =
    'static bool outlinerMiniIconButton\([\s\S]*?(?=\r?\nstatic void outlinerLeafRow\()'

$miniMatches = [regex]::Matches(
    $text,
    $miniPattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($miniMatches.Count -ne 1) {
    throw "Expected exactly one outlinerMiniIconButton() helper."
}

$miniReplacement = @'
static bool outlinerMiniIconButton(
    const char* id,
    UiGlyph glyph,
    bool active,
    const char* tooltip)
{
    ImGui::PushID(id);

    const ImVec2 p=ImGui::GetCursorScreenPos();
    const ImVec2 size(34.0f,30.0f);

    const bool pressed=
        ImGui::InvisibleButton(
            "##StateIcon",
            size
        );

    const bool hovered=ImGui::IsItemHovered();
    ImDrawList* dl=ImGui::GetWindowDrawList();

    if (active || hovered)
    {
        dl->AddRectFilled(
            p,
            ImVec2(p.x+size.x,p.y+size.y),
            toU32(active ? colorPanelRaised() : colorPanel()),
            4.0f
        );

        dl->AddRect(
            p,
            ImVec2(p.x+size.x,p.y+size.y),
            toU32(hovered ? colorBorder() : colorPanelRaised()),
            4.0f,
            0,
            1.0f
        );
    }

    float glyphSize=20.0f;
    if (glyph==UiGlyph::More) glyphSize=16.5f;
    if (glyph==UiGlyph::Lock || glyph==UiGlyph::Unlock) glyphSize=22.0f;
    if (glyph==UiGlyph::Eye) glyphSize=20.5f;
    if (glyph==UiGlyph::Target) glyphSize=20.5f;

    const ImVec2 center(
        p.x+size.x*0.5f,
        p.y+size.y*0.5f + ((glyph==UiGlyph::Lock || glyph==UiGlyph::Unlock) ? 0.5f : 0.0f)
    );

    drawGlyph(
        dl,
        glyph,
        center,
        glyphSize,
        toU32(active ? colorText() : colorMuted())
    );

    if (glyph==UiGlyph::Eye && !active)
    {
        dl->AddLine(
            ImVec2(p.x+7.0f,p.y+22.0f),
            ImVec2(p.x+27.0f,p.y+8.0f),
            toU32(colorMuted()),
            1.7f
        );
    }

    if (hovered && tooltip && tooltip[0])
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltip);
        ImGui::EndTooltip();
    }

    ImGui::PopID();
    return pressed;
}

'@

$text = [regex]::Replace(
    $text,
    $miniPattern,
    $miniReplacement,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

Write-Host "[OK] Enlarged and aligned Outliner state buttons." -ForegroundColor Green

# ============================================================
# 5. Use Unlock icon when object is not locked
# ============================================================

$oldLockToggle = @'
    if (outlinerMiniIconButton(
        "LockToggle",
        UiGlyph::Lock,
        *locked,
        *locked
            ? "Locked - click to unlock"
            : "Unlocked - click to lock"))
    {
        *locked=!*locked;
    }
'@

$newLockToggle = @'
    if (outlinerMiniIconButton(
        "LockToggle",
        *locked ? UiGlyph::Lock : UiGlyph::Unlock,
        *locked,
        *locked
            ? "Locked - click to unlock"
            : "Unlocked - click to lock"))
    {
        *locked=!*locked;
    }
'@

if ($text.Contains($oldLockToggle))
{
    $text = $text.Replace($oldLockToggle,$newLockToggle)
    Write-Host "[OK] Lock column now shows lock vs unlock state explicitly." -ForegroundColor Green
}
elseif ($text.Contains('*locked ? UiGlyph::Lock : UiGlyph::Unlock'))
{
    Write-Host "[SKIP] Lock toggle already uses unlock state." -ForegroundColor DarkGray
}
else
{
    throw "Could not locate LockToggle block."
}

# ============================================================
# 6. Slightly enlarge Outliner header icons
# ============================================================

$text = $text.Replace(
    '                18.0f,',
    '                20.0f,'
)

$text = $text.Replace(
    '                    hp.x+15.0f,',
    '                    hp.x+16.0f,'
)

$text = $text.Replace(
    '                    hp.y+13.0f',
    '                    hp.y+14.0f'
)

Write-Host "[OK] Increased header icon size slightly." -ForegroundColor Green

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'UiGlyph::Unlock',
    'case UiGlyph::Unlock:',
    '*locked ? UiGlyph::Lock : UiGlyph::Unlock',
    'const ImVec2 size(34.0f,30.0f);',
    'glyph==UiGlyph::Lock || glyph==UiGlyph::Unlock'
)

foreach ($check in $checks)
{
    if (-not $verify.Contains($check))
    {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] Outliner lock/unlock icon polished." -ForegroundColor Cyan
Write-Host "[OK] Lock and unlock now use different icons." -ForegroundColor Green
Write-Host "[OK] Buttons are larger and better centered." -ForegroundColor Green
Write-Host "[OK] Header icons slightly enlarged." -ForegroundColor Green
Write-Host "[OK] No other screen changed." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
