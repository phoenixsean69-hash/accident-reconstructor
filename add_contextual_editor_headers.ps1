param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - CONTEXTUAL EDITOR HEADERS" -ForegroundColor Cyan
Write-Host " Phase 2 - Editor identity + active context" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'static void drawViewportView()',
    'static void drawTimeline()',
    'static void drawNodeEditor()',
    '"Outliner"',
    '"Properties"',
    'selectedEntityName()'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected editor marker not found: $marker"
    }
}

if ($text.Contains('static void beginEditorContextHeader(')) {
    Write-Host "[OK] Contextual editor headers already installed." -ForegroundColor Green
    exit 0
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-context-headers-$timestamp.bak"
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

function Insert-AfterBegin {
    param(
        [string]$FunctionText,
        [string]$WindowName,
        [string]$InsertText
    )

    $escaped = [regex]::Escape($WindowName)

    $pattern =
        'ImGui::Begin\(\s*"' +
        $escaped +
        '"[\s\S]*?\);'

    $match = [regex]::Match(
        $FunctionText,
        $pattern
    )

    if (-not $match.Success) {
        throw "Could not find ImGui::Begin for window: $WindowName"
    }

    return $FunctionText.Insert(
        $match.Index + $match.Length,
        $InsertText
    )
}

# ============================================================
# 1. SHARED HEADER HELPERS
# ============================================================

$helperInsert = $text.IndexOf('static void drawViewportView()')

if ($helperInsert -lt 0) {
    throw "Could not locate helper insertion point."
}

$helpers = @'
static void beginEditorContextHeader(
    const char* id)
{
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(10.0f,5.0f)
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(7.0f,4.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_ChildBg,
        ImVec4(0.086f,0.091f,0.099f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.22f,0.235f,0.26f,1.0f)
    );

    ImGui::BeginChild(
        id,
        ImVec2(0.0f,34.0f),
        true,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );
}

static void editorContextSeparator()
{
    ImGui::SameLine(0.0f,9.0f);

    ImGui::TextColored(
        ImVec4(0.31f,0.33f,0.37f,1.0f),
        "|"
    );

    ImGui::SameLine(0.0f,9.0f);
}

static void endEditorContextHeader()
{
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::Spacing();
}

'@

$text = $text.Insert(
    $helperInsert,
    $helpers
)

Write-Host "[OK] Added shared contextual-header renderer." -ForegroundColor Green

# ============================================================
# 2. VIEWPORT HEADER
# ============================================================

$viewportInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void drawViewportView()'

$viewport = $viewportInfo.Text

$viewportHeader = @'

    beginEditorContextHeader(
        "##ViewportContextHeader"
    );

    ImGui::TextDisabled("VIEWPORT");

    editorContextSeparator();

    ImGui::Text(
        "%s",
        viewportMode==0
            ? "2D Plan"
            : (viewportMode==1
                ? "3D Scene"
                : "AR Preview")
    );

    editorContextSeparator();

    const char* contextToolNames[]={
        "Select",
        "Move",
        "Rotate",
        "Scale"
    };

    ImGui::TextDisabled("Tool");
    ImGui::SameLine(0.0f,5.0f);

    ImGui::Text(
        "%s",
        contextToolNames[
            std::max(
                0,
                std::min(
                    3,
                    selectedTool
                )
            )
        ]
    );

    editorContextSeparator();

    ImGui::TextDisabled("Snap");
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
        ImGui::TextDisabled("Off");
    }

    editorContextSeparator();

    ImGui::TextDisabled("Selected");
    ImGui::SameLine(0.0f,5.0f);

    ImGui::Text(
        "%s",
        gEditorShell.selectedEntity!=0
            ? selectedEntityName()
            : "None"
    );

    endEditorContextHeader();
'@

$viewport = Insert-AfterBegin `
    -FunctionText $viewport `
    -WindowName 'Viewport' `
    -InsertText $viewportHeader

$text = $text.Remove(
    $viewportInfo.Start,
    $viewportInfo.End -
    $viewportInfo.Start + 1
).Insert(
    $viewportInfo.Start,
    $viewport
)

Write-Host "[OK] Viewport contextual header added." -ForegroundColor Green

# ============================================================
# 3. TIMELINE HEADER
# ============================================================

$timelineInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void drawTimeline()'

$timeline = $timelineInfo.Text

$timelineHeader = @'

    beginEditorContextHeader(
        "##TimelineContextHeader"
    );

    ImGui::TextDisabled("TIMELINE");

    editorContextSeparator();

    ImGui::Text(
        "Frame %d",
        currentFrame
    );

    editorContextSeparator();

    const char* timelineHeaderPhase=
        currentFrame<preImpactEnd
            ? "Pre-impact"
            : (currentFrame<impactEnd
                ? "Impact"
                : "Post-impact");

    ImGui::Text(
        "%s",
        timelineHeaderPhase
    );

    editorContextSeparator();

    ImGui::TextDisabled(
        playing
            ? "Playing"
            : "Paused"
    );

    editorContextSeparator();

    ImGui::TextDisabled(
        snapToFrames
            ? "Snap On"
            : "Snap Off"
    );

    ImGui::SameLine(0.0f,14.0f);

    ImGui::TextDisabled(
        "Space Play/Pause"
    );

    endEditorContextHeader();
'@

$timeline = Insert-AfterBegin `
    -FunctionText $timeline `
    -WindowName 'Timeline' `
    -InsertText $timelineHeader

$text = $text.Remove(
    $timelineInfo.Start,
    $timelineInfo.End -
    $timelineInfo.Start + 1
).Insert(
    $timelineInfo.Start,
    $timeline
)

Write-Host "[OK] Timeline contextual header added." -ForegroundColor Green

# ============================================================
# 4. NODE EDITOR HEADER
# ============================================================

$nodeInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void drawNodeEditor()'

$node = $nodeInfo.Text

$nodeHeader = @'

    beginEditorContextHeader(
        "##NodeEditorContextHeader"
    );

    ImGui::TextDisabled("NODE GRAPH");

    editorContextSeparator();

    ImGui::Text(
        "%d nodes",
        static_cast<int>(
            nodes.size()
        )
    );

    editorContextSeparator();

    ImGui::Text(
        "%d links",
        static_cast<int>(
            links.size()
        )
    );

    editorContextSeparator();

    if (!selectedNodes.empty())
    {
        ImGui::Text(
            "%d selected",
            static_cast<int>(
                selectedNodes.size()
            )
        );
    }
    else if (!selectedLinks.empty())
    {
        ImGui::Text(
            "%d link selected",
            static_cast<int>(
                selectedLinks.size()
            )
        );
    }
    else
    {
        ImGui::TextDisabled(
            "Nothing selected"
        );
    }

    editorContextSeparator();

    if (graphMessageError)
    {
        ImGui::TextColored(
            ImVec4(0.90f,0.58f,0.45f,1.0f),
            "%s",
            graphMessage.c_str()
        );
    }
    else
    {
        ImGui::TextDisabled(
            "%s",
            graphMessage.c_str()
        );
    }

    endEditorContextHeader();
'@

$node = Insert-AfterBegin `
    -FunctionText $node `
    -WindowName 'Node Editor' `
    -InsertText $nodeHeader

$text = $text.Remove(
    $nodeInfo.Start,
    $nodeInfo.End -
    $nodeInfo.Start + 1
).Insert(
    $nodeInfo.Start,
    $node
)

Write-Host "[OK] Node Editor contextual header added." -ForegroundColor Green

# ============================================================
# 5. OUTLINER HEADER
# ============================================================

$outlinerPattern =
    'ImGui::Begin\(\s*"Outliner"[\s\S]*?\);'

$outlinerMatch = [regex]::Match(
    $text,
    $outlinerPattern
)

if (-not $outlinerMatch.Success) {
    throw 'Could not find ImGui::Begin("Outliner").'
}

$outlinerHeader = @'

    beginEditorContextHeader(
        "##OutlinerContextHeader"
    );

    ImGui::TextDisabled("SCENE");

    editorContextSeparator();

    ImGui::Text("9 objects");

    editorContextSeparator();

    ImGui::TextDisabled("Selected");
    ImGui::SameLine(0.0f,5.0f);

    ImGui::Text(
        "%s",
        gEditorShell.selectedEntity!=0
            ? selectedEntityName()
            : "None"
    );

    endEditorContextHeader();
'@

$text = $text.Insert(
    $outlinerMatch.Index +
    $outlinerMatch.Length,
    $outlinerHeader
)

# Remove old redundant title if still present.
$text = [regex]::Replace(
    $text,
    'ImGui::Text\(\s*"SCENE OUTLINER"\s*\);\s*ImGui::Separator\(\s*\);',
    '',
    1
)

Write-Host "[OK] Outliner contextual header added." -ForegroundColor Green

# ============================================================
# 6. PROPERTIES HEADER
# ============================================================

$propertiesPattern =
    'ImGui::Begin\(\s*"Properties"[\s\S]*?\);'

$propertiesMatch = [regex]::Match(
    $text,
    $propertiesPattern
)

if (-not $propertiesMatch.Success) {
    throw 'Could not find ImGui::Begin("Properties").'
}

$propertiesHeader = @'

    beginEditorContextHeader(
        "##PropertiesContextHeader"
    );

    ImGui::TextDisabled("INSPECTOR");

    editorContextSeparator();

    ImGui::TextDisabled("Context");
    ImGui::SameLine(0.0f,5.0f);

    ImGui::Text(
        "%s",
        gEditorShell.selectedEntity!=0
            ? selectedEntityName()
            : "Scene"
    );

    editorContextSeparator();

    ImGui::TextDisabled(
        gEditorShell.selectedEntity!=0
            ? "Object Properties"
            : "No object selected"
    );

    endEditorContextHeader();
'@

$text = $text.Insert(
    $propertiesMatch.Index +
    $propertiesMatch.Length,
    $propertiesHeader
)

$text = [regex]::Replace(
    $text,
    'ImGui::Text\(\s*"INSPECTOR"\s*\);\s*ImGui::Separator\(\s*\);',
    '',
    1
)

Write-Host "[OK] Properties contextual header added." -ForegroundColor Green

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content `
    -Path $MainCpp `
    -Value $text `
    -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'static void beginEditorContextHeader(',
    '"##ViewportContextHeader"',
    '"##TimelineContextHeader"',
    '"##NodeEditorContextHeader"',
    '"##OutlinerContextHeader"',
    '"##PropertiesContextHeader"',
    '"Space Play/Pause"',
    '"NODE GRAPH"',
    '"Object Properties"'
)

foreach ($check in $checks)
{
    if (-not $verify.Contains($check))
    {
        throw "Verification failed after write: $check"
    }
}

$badPair =
    [string]([char]96) +
    "r" +
    [char]96 +
    "n"

if ($verify.Contains($badPair))
{
    throw "Verification failed: literal PowerShell newline text entered main.cpp."
}

Write-Host ""
Write-Host "[DONE] Contextual editor headers installed." -ForegroundColor Cyan
Write-Host "[OK] Viewport context header." -ForegroundColor Green
Write-Host "[OK] Timeline context header." -ForegroundColor Green
Write-Host "[OK] Node Graph context header." -ForegroundColor Green
Write-Host "[OK] Outliner context header." -ForegroundColor Green
Write-Host "[OK] Properties context header." -ForegroundColor Green
Write-Host "[OK] Redundant Outliner/Inspector titles removed." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
