param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - OUTLINER ACTION BAR UPGRADE" -ForegroundColor Cyan
Write-Host " Eye / Lock / Focus / More" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'static bool outlinerMiniIconButton(',
    'static void outlinerLeafRow(',
    'SceneOutlinerTable',
    'UiGlyph::Eye',
    'UiGlyph::Lock'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected Outliner marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-outliner-actionbar-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. Add Target + More glyphs to enum
# ============================================================

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
$newEnumBlock = $enumBlock

if ($newEnumBlock -notmatch '\bTarget\b')
{
    $newEnumBlock = $newEnumBlock -replace '\bLock\b', 'Lock, Target'
}

if ($newEnumBlock -notmatch '\bMore\b')
{
    $newEnumBlock = $newEnumBlock -replace '\bTarget\b', 'Target, More'
}

if ($newEnumBlock -ne $enumBlock)
{
    $text = $text.Replace($enumBlock,$newEnumBlock)
    Write-Host "[OK] Added Target + More glyphs." -ForegroundColor Green
}
else
{
    Write-Host "[SKIP] Target + More glyphs already present." -ForegroundColor DarkGray
}

# ============================================================
# 2. Add Target + More vector rendering
# ============================================================

if ($text -notmatch 'case\s+UiGlyph::Target\s*:')
{
    $lockMarker = '        case UiGlyph::Lock:'

    if (-not $text.Contains($lockMarker)) {
        throw "Could not locate Lock glyph draw case."
    }

    $extraCases = @'
        case UiGlyph::Target:
            d->AddCircle(center,s*.34f,color,24,t);
            d->AddCircle(center,s*.17f,color,20,t);
            d->AddCircleFilled(center,s*.045f,color);

            d->AddLine(
                ImVec2(x-s*.46f,y),
                ImVec2(x-s*.28f,y),
                color,
                t
            );

            d->AddLine(
                ImVec2(x+s*.28f,y),
                ImVec2(x+s*.46f,y),
                color,
                t
            );

            d->AddLine(
                ImVec2(x,y-s*.46f),
                ImVec2(x,y-s*.28f),
                color,
                t
            );

            d->AddLine(
                ImVec2(x,y+s*.28f),
                ImVec2(x,y+s*.46f),
                color,
                t
            );
            break;

        case UiGlyph::More:
            d->AddCircleFilled(
                ImVec2(x-s*.22f,y),
                s*.055f,
                color
            );

            d->AddCircleFilled(
                ImVec2(x,y),
                s*.055f,
                color
            );

            d->AddCircleFilled(
                ImVec2(x+s*.22f,y),
                s*.055f,
                color
            );
            break;

'@

    $text = $text.Replace(
        $lockMarker,
        $extraCases + $lockMarker
    )

    Write-Host "[OK] Added Target + More vector icons." -ForegroundColor Green
}

# ============================================================
# 3. Make mini icon buttons larger + perfectly centered
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
    const ImVec2 size(30.0f,28.0f);

    const bool pressed=
        ImGui::InvisibleButton(
            "##StateIcon",
            size
        );

    const bool hovered=ImGui::IsItemHovered();

    ImDrawList* dl=ImGui::GetWindowDrawList();

    if (active)
    {
        dl->AddRectFilled(
            p,
            ImVec2(p.x+size.x,p.y+size.y),
            toU32(colorPanelRaised()),
            3.0f
        );
    }
    else if (hovered)
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
        19.0f,
        iconColor
    );

    if (glyph==UiGlyph::Eye && !active)
    {
        dl->AddLine(
            ImVec2(p.x+6.0f,p.y+21.0f),
            ImVec2(p.x+24.0f,p.y+7.0f),
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

Write-Host "[OK] Enlarged and centered Outliner action icons." -ForegroundColor Green

# ============================================================
# 4. Upgrade row from 2 actions -> 4 actions
# ============================================================

$rowPattern =
    'static void outlinerLeafRow\([\s\S]*?(?=\r?\nstatic void propertyVec3Row\()'

$rowMatches = [regex]::Matches(
    $text,
    $rowPattern,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if ($rowMatches.Count -ne 1) {
    throw "Expected exactly one outlinerLeafRow() helper."
}

$rowReplacement = @'
static void outlinerLeafRow(
    const char* label,
    UiGlyph glyph,
    int entityId,
    bool* visible,
    bool* locked)
{
    if (!shellLabelMatches(label))
        return;

    ImGui::TableNextRow(
        ImGuiTableRowFlags_None,
        32.0f
    );

    ImGui::PushID(entityId);

    // --------------------------------------------------------
    // OBJECT
    // --------------------------------------------------------

    ImGui::TableSetColumnIndex(0);

    const bool selected=
        gEditorShell.selectedEntity==entityId;

    const ImVec2 rowPos=ImGui::GetCursorScreenPos();

    if (ImGui::Selectable(
        "##EntityRow",
        selected,
        ImGuiSelectableFlags_None,
        ImVec2(0.0f,29.0f)))
    {
        gEditorShell.selectedEntity=entityId;
    }

    drawGlyph(
        ImGui::GetWindowDrawList(),
        glyph,
        ImVec2(
            rowPos.x+13.0f,
            rowPos.y+14.0f
        ),
        16.0f,
        toU32(
            selected
                ? colorAccent()
                : colorMuted()
        )
    );

    ImGui::GetWindowDrawList()->AddText(
        ImVec2(
            rowPos.x+30.0f,
            rowPos.y+5.0f
        ),
        toU32(colorText()),
        label
    );

    // Full-row context menu.
    if (ImGui::BeginPopupContextItem("##EntityContext"))
    {
        ImGui::TextDisabled("%s",label);
        ImGui::Separator();

        ImGui::MenuItem("Rename","F2");
        ImGui::MenuItem("Duplicate","Ctrl+D");

        if (ImGui::MenuItem("Focus in Viewport","F"))
            gEditorShell.selectedEntity=entityId;

        ImGui::Separator();
        ImGui::MenuItem("Delete","Del");

        ImGui::EndPopup();
    }

    // --------------------------------------------------------
    // VISIBILITY
    // --------------------------------------------------------

    ImGui::TableSetColumnIndex(1);
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX()+2.0f
    );

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

    // --------------------------------------------------------
    // LOCK
    // --------------------------------------------------------

    ImGui::TableSetColumnIndex(2);
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX()+2.0f
    );

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

    // --------------------------------------------------------
    // FOCUS
    // --------------------------------------------------------

    ImGui::TableSetColumnIndex(3);
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX()+2.0f
    );

    if (outlinerMiniIconButton(
        "FocusAction",
        UiGlyph::Target,
        false,
        "Focus object in Viewport"))
    {
        gEditorShell.selectedEntity=entityId;
        // Hook viewport framing here later.
    }

    // --------------------------------------------------------
    // MORE
    // --------------------------------------------------------

    ImGui::TableSetColumnIndex(4);
    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX()+2.0f
    );

    if (outlinerMiniIconButton(
        "MoreAction",
        UiGlyph::More,
        false,
        "More object actions"))
    {
        ImGui::OpenPopup("##EntityMorePopup");
    }

    if (ImGui::BeginPopup("##EntityMorePopup"))
    {
        ImGui::TextDisabled("%s",label);
        ImGui::Separator();

        ImGui::MenuItem("Rename","F2");
        ImGui::MenuItem("Duplicate","Ctrl+D");

        if (ImGui::MenuItem("Focus in Viewport","F"))
            gEditorShell.selectedEntity=entityId;

        ImGui::Separator();

        ImGui::MenuItem(
            *visible ? "Hide" : "Show"
        );

        ImGui::MenuItem(
            *locked ? "Unlock" : "Lock"
        );

        ImGui::Separator();
        ImGui::MenuItem("Delete","Del");

        ImGui::EndPopup();
    }

    ImGui::PopID();
}

'@

$text = [regex]::Replace(
    $text,
    $rowPattern,
    $rowReplacement,
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

Write-Host "[OK] Added Focus + More actions to each Outliner row." -ForegroundColor Green

# ============================================================
# 5. Upgrade table columns + headers from 3 -> 5
# ============================================================

$text = $text.Replace(
    '            "SceneOutlinerTable",`r`n            3,',
    '            "SceneOutlinerTable",`r`n            5,'
)

# Fallback for LF or compact formatting
$text = [regex]::Replace(
    $text,
    '"SceneOutlinerTable"\s*,\s*3\s*,',
    '"SceneOutlinerTable",`r`n            5,',
    1
)

$oldColumns = @'
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

$newColumns = @'
        ImGui::TableSetupColumn(
            "##VisibilityColumn",
            ImGuiTableColumnFlags_WidthFixed,
            36.0f
        );

        ImGui::TableSetupColumn(
            "##LockColumn",
            ImGuiTableColumnFlags_WidthFixed,
            36.0f
        );

        ImGui::TableSetupColumn(
            "##FocusColumn",
            ImGuiTableColumnFlags_WidthFixed,
            36.0f
        );

        ImGui::TableSetupColumn(
            "##MoreColumn",
            ImGuiTableColumnFlags_WidthFixed,
            36.0f
        );

        ImGui::TableNextRow(
            ImGuiTableRowFlags_Headers,
            31.0f
        );

        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("OBJECT");

        auto drawOutlinerHeaderIcon = [](
            UiGlyph glyph,
            const char* tooltip)
        {
            const ImVec2 hp=ImGui::GetCursorScreenPos();

            drawGlyph(
                ImGui::GetWindowDrawList(),
                glyph,
                ImVec2(
                    hp.x+15.0f,
                    hp.y+13.0f
                ),
                18.0f,
                toU32(colorMuted())
            );

            ImGui::Dummy(
                ImVec2(30.0f,26.0f)
            );

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s",tooltip);
        };

        ImGui::TableSetColumnIndex(1);
        drawOutlinerHeaderIcon(
            UiGlyph::Eye,
            "Visibility"
        );

        ImGui::TableSetColumnIndex(2);
        drawOutlinerHeaderIcon(
            UiGlyph::Lock,
            "Lock state"
        );

        ImGui::TableSetColumnIndex(3);
        drawOutlinerHeaderIcon(
            UiGlyph::Target,
            "Focus in Viewport"
        );

        ImGui::TableSetColumnIndex(4);
        drawOutlinerHeaderIcon(
            UiGlyph::More,
            "More actions"
        );
'@

if ($text.Contains($oldColumns))
{
    $text = $text.Replace(
        $oldColumns,
        $newColumns
    )
}
elseif (-not $text.Contains('##FocusColumn'))
{
    throw "Could not locate the existing Outliner icon-header block."
}

Write-Host "[OK] Enlarged and aligned all Outliner header icons." -ForegroundColor Green

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'UiGlyph::Target',
    'UiGlyph::More',
    '"FocusAction"',
    '"MoreAction"',
    '##FocusColumn',
    '##MoreColumn',
    'Focus object in Viewport',
    'More object actions'
)

foreach ($check in $checks)
{
    if (-not $verify.Contains($check))
    {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] Outliner action bar upgraded." -ForegroundColor Cyan
Write-Host "[OK] Icons increased to editor-scale size." -ForegroundColor Green
Write-Host "[OK] Headers and row controls aligned." -ForegroundColor Green
Write-Host "[OK] Eye = visibility." -ForegroundColor Green
Write-Host "[OK] Lock = lock state." -ForegroundColor Green
Write-Host "[OK] Target = focus in viewport." -ForegroundColor Green
Write-Host "[OK] More = object actions popup." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
