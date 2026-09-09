param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - OUTLINER ID + EYE/LOCK ICON FIX" -ForegroundColor Cyan
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
    'static void drawGlyph(',
    'static void outlinerLeafRow(',
    'SceneOutlinerTable'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected editor-shell marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-outliner-icon-id-fix-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. Add Eye + Lock to UiGlyph
# ============================================================

if ($text -notmatch '\bEye\b' -or $text -notmatch '\bLock\b')
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

    if ($enumBlock -notmatch '\bEye\b')
    {
        $newEnum = [regex]::Replace(
            $enumBlock,
            '\bInfo\b(?=\s*\r?\n\};)',
            'Info, Eye, Lock',
            1
        )

        if ($newEnum -eq $enumBlock) {
            throw "Could not append Eye/Lock to UiGlyph."
        }

        $text = $text.Replace($enumBlock,$newEnum)
        Write-Host "[OK] Added UiGlyph::Eye and UiGlyph::Lock." -ForegroundColor Green
    }
}

# ============================================================
# 2. Add vector-drawn Eye + Lock glyphs
# ============================================================

if ($text -notmatch 'case\s+UiGlyph::Eye\s*:')
{
    $infoMarker = '        case UiGlyph::Info:'

    if (-not $text.Contains($infoMarker)) {
        throw "Could not find UiGlyph::Info draw case."
    }

    $glyphCases = @'
        case UiGlyph::Eye:
            d->AddBezierCubic(
                ImVec2(x-s*.40f,y),
                ImVec2(x-s*.18f,y-s*.28f),
                ImVec2(x+s*.18f,y-s*.28f),
                ImVec2(x+s*.40f,y),
                color,
                t
            );
            d->AddBezierCubic(
                ImVec2(x-s*.40f,y),
                ImVec2(x-s*.18f,y+s*.28f),
                ImVec2(x+s*.18f,y+s*.28f),
                ImVec2(x+s*.40f,y),
                color,
                t
            );
            d->AddCircleFilled(center,s*.09f,color);
            break;

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

    $text = $text.Replace(
        $infoMarker,
        $glyphCases + $infoMarker
    )

    Write-Host "[OK] Added vector Eye + Lock rendering." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] Eye/Lock glyph drawing already exists." -ForegroundColor DarkGray
}

# ============================================================
# 3. Add compact icon button helper for Outliner columns
# ============================================================

if ($text -notmatch 'static\s+bool\s+outlinerMiniIconButton\s*\(')
{
    $leafMarker = 'static void outlinerLeafRow('

    if (-not $text.Contains($leafMarker)) {
        throw "Could not find outlinerLeafRow() insertion point."
    }

    $helper = @'
static bool outlinerMiniIconButton(
    const char* id,
    UiGlyph glyph,
    bool active,
    const char* tooltip)
{
    ImGui::PushID(id);

    const ImVec2 p=ImGui::GetCursorScreenPos();
    const ImVec2 size(24.0f,22.0f);

    const bool pressed=
        ImGui::InvisibleButton(
            "##StateIcon",
            size
        );

    const bool hovered=ImGui::IsItemHovered();

    ImDrawList* dl=ImGui::GetWindowDrawList();

    if (hovered)
    {
        dl->AddRectFilled(
            p,
            ImVec2(p.x+size.x,p.y+size.y),
            toU32(colorPanelRaised()),
            3.0f
        );
    }

    const ImU32 iconColor=
        toU32(
            active
                ? colorText()
                : colorMuted()
        );

    drawGlyph(
        dl,
        glyph,
        ImVec2(
            p.x+size.x*0.5f,
            p.y+size.y*0.5f
        ),
        14.0f,
        iconColor
    );

    // Hidden state: slash the eye instead of replacing it with text.
    if (glyph==UiGlyph::Eye && !active)
    {
        dl->AddLine(
            ImVec2(p.x+5.0f,p.y+17.0f),
            ImVec2(p.x+19.0f,p.y+5.0f),
            toU32(colorMuted()),
            1.5f
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

    $text = $text.Replace(
        $leafMarker,
        $helper + $leafMarker
    )

    Write-Host "[OK] Added compact Outliner icon-button control." -ForegroundColor Green
}

# ============================================================
# 4. Replace conflicting V/L/- SmallButtons
# ============================================================

$oldButtons = @'
    ImGui::TableSetColumnIndex(1);
    if (ImGui::SmallButton(*visible ? "V" : "-"))
        *visible=!*visible;

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(*visible ? "Visible" : "Hidden");

    ImGui::TableSetColumnIndex(2);
    if (ImGui::SmallButton(*locked ? "L" : "-"))
        *locked=!*locked;

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(*locked ? "Locked" : "Unlocked");
'@

$newButtons = @'
    ImGui::TableSetColumnIndex(1);

    if (outlinerMiniIconButton(
        "VisibilityToggle",
        UiGlyph::Eye,
        *visible,
        *visible
            ? "Visible - click to hide"
            : "Hidden - click to show"))
    {
        *visible=!*visible;
    }

    ImGui::TableSetColumnIndex(2);

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

if ($text.Contains($oldButtons))
{
    $text = $text.Replace(
        $oldButtons,
        $newButtons
    )

    Write-Host "[OK] Removed conflicting text-button IDs." -ForegroundColor Green
}
elseif ($text.Contains('outlinerMiniIconButton(') -and
        $text.Contains('"VisibilityToggle"'))
{
    Write-Host "[SKIP] Outliner row icon buttons already patched." -ForegroundColor DarkGray
}
else
{
    throw "Could not locate the V/L Outliner button block."
}

# ============================================================
# 5. Replace V/L table headers with Eye/Lock icons
# ============================================================

$oldHeader = @'
        ImGui::TableSetupColumn(
            "V",
            ImGuiTableColumnFlags_WidthFixed,
            28.0f
        );

        ImGui::TableSetupColumn(
            "L",
            ImGuiTableColumnFlags_WidthFixed,
            28.0f
        );

        ImGui::TableHeadersRow();
'@

$newHeader = @'
        ImGui::TableSetupColumn(
            "##VisibilityColumn",
            ImGuiTableColumnFlags_WidthFixed,
            30.0f
        );

        ImGui::TableSetupColumn(
            "##LockColumn",
            ImGuiTableColumnFlags_WidthFixed,
            30.0f
        );

        ImGui::TableNextRow(
            ImGuiTableRowFlags_Headers,
            26.0f
        );

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("OBJECT");

        ImGui::TableSetColumnIndex(1);
        {
            const ImVec2 hp=ImGui::GetCursorScreenPos();

            drawGlyph(
                ImGui::GetWindowDrawList(),
                UiGlyph::Eye,
                ImVec2(hp.x+12.0f,hp.y+10.0f),
                14.0f,
                toU32(colorMuted())
            );

            ImGui::Dummy(ImVec2(24.0f,20.0f));

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Visibility");
        }

        ImGui::TableSetColumnIndex(2);
        {
            const ImVec2 hp=ImGui::GetCursorScreenPos();

            drawGlyph(
                ImGui::GetWindowDrawList(),
                UiGlyph::Lock,
                ImVec2(hp.x+12.0f,hp.y+10.0f),
                14.0f,
                toU32(colorMuted())
            );

            ImGui::Dummy(ImVec2(24.0f,20.0f));

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Lock state");
        }
'@

if ($text.Contains($oldHeader))
{
    $text = $text.Replace(
        $oldHeader,
        $newHeader
    )

    Write-Host "[OK] Replaced V/L headers with Eye/Lock icons." -ForegroundColor Green
}
elseif ($text.Contains('##VisibilityColumn') -and
        $text.Contains('##LockColumn'))
{
    Write-Host "[SKIP] Icon headers already installed." -ForegroundColor DarkGray
}
else
{
    throw "Could not locate the Outliner V/L header block."
}

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'UiGlyph::Eye',
    'UiGlyph::Lock',
    'outlinerMiniIconButton',
    '"VisibilityToggle"',
    '"LockToggle"',
    '##VisibilityColumn',
    '##LockColumn'
)

foreach ($check in $checks)
{
    if (-not $verify.Contains($check))
    {
        throw "Verification failed after write: $check"
    }
}

if ($verify.Contains('ImGui::SmallButton(*visible ? "V" : "-")') -or
    $verify.Contains('ImGui::SmallButton(*locked ? "L" : "-")'))
{
    throw "Verification failed: old V/L SmallButtons still remain."
}

Write-Host ""
Write-Host "[DONE] Outliner control fix installed." -ForegroundColor Cyan
Write-Host "[OK] Conflicting ImGui IDs removed." -ForegroundColor Green
Write-Host "[OK] Visibility uses an eye icon." -ForegroundColor Green
Write-Host "[OK] Hidden state uses a slashed eye." -ForegroundColor Green
Write-Host "[OK] Lock state uses a lock icon." -ForegroundColor Green
Write-Host "[OK] V / L column labels removed." -ForegroundColor Green
Write-Host "[OK] Existing Outliner selection/context menus preserved." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
