param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - SHARED SELECTION SYNC" -ForegroundColor Cyan
Write-Host " Phase 4 - Outliner / Viewport / Properties / Timeline / Node Graph" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$original = Get-Content $MainCpp -Raw
$text = $original

$required = @(
    'struct EditorShellState',
    'static EditorShellState gEditorShell;',
    'static const char* selectedEntityName()',
    'static void outlinerLeafRow(',
    'static void drawNodeEditor()',
    'static void drawTimeline()',
    '"##TimelineCanvas"',
    'static std::vector<int> selectedNodes;',
    'NodeType::EvidenceInput',
    'NodeType::VehicleInput',
    'const char* trackNames[]={'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected current editor marker not found: $marker"
    }
}

if ($text.Contains('static void setSharedEntitySelection(')) {
    Write-Host "[OK] Shared selection system already installed." -ForegroundColor Green
    exit 0
}

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
    $lineComment = $false
    $blockComment = $false
    $end = -1

    for ($i = $braceStart; $i -lt $Source.Length; $i++) {
        $ch = $Source[$i]
        $next = if ($i + 1 -lt $Source.Length) {
            $Source[$i + 1]
        } else {
            [char]0
        }

        if ($lineComment) {
            if ($ch -eq "`n") {
                $lineComment = $false
            }
            continue
        }

        if ($blockComment) {
            if ($ch -eq "*" -and $next -eq "/") {
                $blockComment = $false
                $i++
            }
            continue
        }

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

        if ($ch -eq "/" -and $next -eq "/") {
            $lineComment = $true
            $i++
            continue
        }

        if ($ch -eq "/" -and $next -eq "*") {
            $blockComment = $true
            $i++
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
# 1. SHARED SELECTION SERVICE
# ============================================================

$selectedInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static const char* selectedEntityName()'

$selectionService = @'

struct SharedSelectionState
{
    int revision=0;
    int previousEntity=0;
    char source[32]="None";
    double changedAt=0.0;
};

static SharedSelectionState gSharedSelection;

static void setSharedEntitySelection(
    int entity,
    const char* source)
{
    entity=
        std::max(
            0,
            std::min(
                9,
                entity
            )
        );

    if (gEditorShell.selectedEntity!=entity)
    {
        gSharedSelection.previousEntity=
            gEditorShell.selectedEntity;

        gEditorShell.selectedEntity=
            entity;

        gSharedSelection.revision++;
        gSharedSelection.changedAt=
            ImGui::GetTime();
    }

    std::snprintf(
        gSharedSelection.source,
        sizeof(gSharedSelection.source),
        "%s",
        source && source[0]
            ? source
            : "Editor"
    );
}

static void clearSharedEntitySelection(
    const char* source)
{
    setSharedEntitySelection(
        0,
        source
    );
}

static bool sharedSelectionIsEvidence()
{
    return
        gEditorShell.selectedEntity>=5 &&
        gEditorShell.selectedEntity<=7;
}

static bool sharedSelectionIsMeasurement()
{
    return
        gEditorShell.selectedEntity>=8 &&
        gEditorShell.selectedEntity<=9;
}

'@

$text = $text.Insert(
    $selectedInfo.End + 1,
    $selectionService
)

Write-Host "[OK] Shared scene-selection service added." -ForegroundColor Green

# Route the existing global clear-selection shortcut through the service.
$clearMatches = [regex]::Matches(
    $text,
    'gEditorShell\.selectedEntity\s*=\s*0\s*;'
).Count

if ($clearMatches -gt 0) {
    $text = [regex]::Replace(
        $text,
        'gEditorShell\.selectedEntity\s*=\s*0\s*;',
        'clearSharedEntitySelection("Shortcut");'
    )

    Write-Host "[OK] Existing clear-selection command synchronized." -ForegroundColor Green
}

# ============================================================
# 2. OUTLINER -> SHARED SELECTION
# ============================================================

$outlinerInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void outlinerLeafRow('

$outliner = $outlinerInfo.Text

$directEntityAssignments = [regex]::Matches(
    $outliner,
    'gEditorShell\.selectedEntity\s*=\s*entityId\s*;'
).Count

if ($directEntityAssignments -lt 3) {
    throw "Expected at least 3 Outliner entity-selection assignments; found $directEntityAssignments."
}

$outliner = [regex]::Replace(
    $outliner,
    'gEditorShell\.selectedEntity\s*=\s*entityId\s*;',
    'setSharedEntitySelection(entityId,"Outliner");'
)

$text = $text.Remove(
    $outlinerInfo.Start,
    $outlinerInfo.End -
    $outlinerInfo.Start + 1
).Insert(
    $outlinerInfo.Start,
    $outliner
)

Write-Host "[OK] Outliner row/focus actions now drive shared selection." -ForegroundColor Green

# ============================================================
# 3. NODE EDITOR <-> SCENE SELECTION
# ============================================================

$nodeInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void drawNodeEditor()'

$node = $nodeInfo.Text

# Add one local revision latch.
$nodeStateAnchor =
    'static bool graphMessageError=false;'

if (-not $node.Contains($nodeStateAnchor)) {
    throw "Node Editor state anchor not found."
}

$node = $node.Replace(
    $nodeStateAnchor,
@'
static bool graphMessageError=false;
    static int lastSceneSelectionRevision=-1;
'@
)

# Scene -> graph synchronization goes after initial graph creation,
# immediately before graph execution helpers.
$graphExecutionMarker = @'
    // ========================================================
    // GRAPH EXECUTION
    // ========================================================
'@

$graphExecutionPos = $node.IndexOf($graphExecutionMarker)

if ($graphExecutionPos -lt 0) {
    throw "Node Editor GRAPH EXECUTION marker not found."
}

$sceneToGraph = @'
    // ========================================================
    // SHARED SCENE SELECTION -> NODE GRAPH
    // ========================================================

    if (lastSceneSelectionRevision!=
        gSharedSelection.revision)
    {
        lastSceneSelectionRevision=
            gSharedSelection.revision;

        int targetNode=-1;

        for (const Node& candidate : nodes)
        {
            if (gEditorShell.selectedEntity==5 &&
                candidate.type==
                    NodeType::EvidenceInput)
            {
                targetNode=candidate.id;
                break;
            }

            if (gEditorShell.selectedEntity==3 &&
                candidate.type==
                    NodeType::VehicleInput &&
                candidate.title=="Vehicle A")
            {
                targetNode=candidate.id;
                break;
            }

            if (gEditorShell.selectedEntity==4 &&
                candidate.type==
                    NodeType::VehicleInput &&
                candidate.title=="Vehicle B")
            {
                targetNode=candidate.id;
                break;
            }
        }

        if (targetNode!=-1)
        {
            selectedNodes.clear();
            selectedLinks.clear();
            selectedNodes.push_back(
                targetNode
            );

            graphMessage=
                std::string("Linked selection: ")+
                selectedEntityName();

            graphMessageError=false;
        }
    }

'@

$node = $node.Insert(
    $graphExecutionPos,
    $sceneToGraph
)

# Node -> scene synchronization.
$nodeSelectionAnchor = @'
            if (headerHit &&
                isNodeSelected(node.id))
'@

$nodeSelectionPos = $node.IndexOf(
    $nodeSelectionAnchor
)

if ($nodeSelectionPos -lt 0) {
    throw "Node click-selection anchor not found."
}

$nodeToScene = @'
            // Linked input nodes select their real scene entity.
            if (node.type==
                NodeType::EvidenceInput)
            {
                setSharedEntitySelection(
                    5,
                    "Node Editor"
                );
            }
            else if (node.type==
                NodeType::VehicleInput)
            {
                if (node.title=="Vehicle A")
                {
                    setSharedEntitySelection(
                        3,
                        "Node Editor"
                    );
                }
                else if (node.title=="Vehicle B")
                {
                    setSharedEntitySelection(
                        4,
                        "Node Editor"
                    );
                }
            }

'@

$node = $node.Insert(
    $nodeSelectionPos,
    $nodeToScene
)

$text = $text.Remove(
    $nodeInfo.Start,
    $nodeInfo.End -
    $nodeInfo.Start + 1
).Insert(
    $nodeInfo.Start,
    $node
)

Write-Host "[OK] Node Graph input nodes linked to scene selection." -ForegroundColor Green

# ============================================================
# 4. TIMELINE <-> SCENE SELECTION
# ============================================================

$timelineInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static void drawTimeline()'

$timeline = $timelineInfo.Text

# Add selected-track highlighting to the existing track header loop.
$trackTextAnchor = @'
        dl->AddText(
            ImVec2(
                canvasPos.x+18.0f,
                y+9.0f
            ),
            toU32(colorText()),
            trackNames[i]
        );
'@

$trackTextPos = $timeline.IndexOf(
    $trackTextAnchor
)

if ($trackTextPos -lt 0) {
    throw "Timeline track label drawing anchor not found."
}

$trackHighlight = @'
        const bool linkedTrackSelected=
            (i==0 &&
                gEditorShell.selectedEntity==3) ||
            (i==1 &&
                gEditorShell.selectedEntity==4) ||
            (i==2 &&
                sharedSelectionIsEvidence()) ||
            (i==3 &&
                sharedSelectionIsMeasurement());

        if (linkedTrackSelected)
        {
            dl->AddRectFilled(
                ImVec2(
                    canvasPos.x+2.0f,
                    y+2.0f
                ),
                ImVec2(
                    canvasPos.x+headerW-6.0f,
                    y+trackH-2.0f
                ),
                IM_COL32(45,58,76,235),
                3.0f
            );

            dl->AddRectFilled(
                ImVec2(
                    canvasPos.x+2.0f,
                    y+2.0f
                ),
                ImVec2(
                    canvasPos.x+5.0f,
                    y+trackH-2.0f
                ),
                IM_COL32(88,145,220,255)
            );
        }

'@

$timeline = $timeline.Insert(
    $trackTextPos,
    $trackHighlight
)

# Track-header click -> shared scene selection.
$mouseAnchor = @'
        if (mouse.x>=timelineMin.x &&
            mouse.x<=timelineMax.x)
'@

$mousePos = $timeline.IndexOf(
    $mouseAnchor
)

if ($mousePos -lt 0) {
    throw "Timeline mouse-interaction anchor not found."
}

$timelineClickSync = @'
        const float trackHeaderTop=
            canvasPos.y+
            rulerH+
            phaseH;

        const float trackHeaderBottom=
            trackHeaderTop+
            static_cast<float>(
                trackCount
            )*
            trackH;

        if (mouse.x>=canvasPos.x &&
            mouse.x<timelineMin.x &&
            mouse.y>=trackHeaderTop &&
            mouse.y<trackHeaderBottom &&
            ImGui::IsMouseClicked(
                ImGuiMouseButton_Left))
        {
            const int trackIndex=
                std::max(
                    0,
                    std::min(
                        trackCount-1,
                        static_cast<int>(
                            (mouse.y-
                                trackHeaderTop)/
                            trackH
                        )
                    )
                );

            const int linkedEntities[]={
                3,
                4,
                5,
                8
            };

            setSharedEntitySelection(
                linkedEntities[
                    trackIndex
                ],
                "Timeline"
            );

            playing=false;
        }

'@

$timeline = $timeline.Insert(
    $mousePos,
    $timelineClickSync
)

$text = $text.Remove(
    $timelineInfo.Start,
    $timelineInfo.End -
    $timelineInfo.Start + 1
).Insert(
    $timelineInfo.Start,
    $timeline
)

Write-Host "[OK] Timeline tracks now select and highlight linked scene entities." -ForegroundColor Green

# ============================================================
# 5. PRE-WRITE VALIDATION
# ============================================================

$checks = @(
    'struct SharedSelectionState',
    'static void setSharedEntitySelection(',
    'static void clearSharedEntitySelection(',
    'setSharedEntitySelection(entityId,"Outliner");',
    'lastSceneSelectionRevision',
    '"Linked selection: "',
    '"Node Editor"',
    'sharedSelectionIsEvidence()',
    'sharedSelectionIsMeasurement()',
    'linkedTrackSelected',
    '"Timeline"'
)

foreach ($check in $checks) {
    if (-not $text.Contains($check)) {
        throw "Pre-write verification failed: $check"
    }
}

if ([regex]::Matches(
    $text,
    'static void setSharedEntitySelection\('
).Count -ne 1)
{
    throw "Pre-write verification failed: shared selection setter definition count is not 1."
}

if ([regex]::Matches(
    $text,
    'static void drawNodeEditor\(\)'
).Count -ne 1)
{
    throw "Pre-write verification failed: drawNodeEditor definition count changed."
}

if ([regex]::Matches(
    $text,
    'static void drawTimeline\(\)'
).Count -ne 1)
{
    throw "Pre-write verification failed: drawTimeline definition count changed."
}

if ([regex]::Matches(
    $text,
    'static void outlinerLeafRow\('
).Count -ne 1)
{
    throw "Pre-write verification failed: outlinerLeafRow definition count changed."
}

$badPair =
    [string]([char]96) +
    "r" +
    [char]96 +
    "n"

if ($text.Contains($badPair)) {
    throw "Pre-write verification failed: literal PowerShell newline text detected."
}

# ============================================================
# 6. BACKUP + WRITE
# ============================================================

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-shared-selection-$timestamp.bak"

Copy-Item $MainCpp $backup -Force

Write-Host "[OK] All pre-write checks passed." -ForegroundColor Green
Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

Set-Content `
    -Path $MainCpp `
    -Value $text `
    -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        Copy-Item $backup $MainCpp -Force
        throw "Post-write verification failed; backup automatically restored: $check"
    }
}

Write-Host ""
Write-Host "[DONE] Shared selection synchronization installed." -ForegroundColor Cyan
Write-Host ""
Write-Host "[SYNC]" -ForegroundColor Green
Write-Host "  Outliner -> Properties / Viewport / Timeline / Node Graph"
Write-Host "  Timeline Vehicle A -> Vehicle A"
Write-Host "  Timeline Vehicle B -> Vehicle B"
Write-Host "  Timeline Evidence -> Skid Mark 01"
Write-Host "  Timeline Measurements -> Distance 01"
Write-Host "  Node Vehicle A -> Vehicle A"
Write-Host "  Node Vehicle B -> Vehicle B"
Write-Host "  Node Skid Evidence -> Skid Mark 01"
Write-Host "  External scene selection -> linked Node input"
Write-Host ""
Write-Host "[NOTE]" -ForegroundColor Yellow
Write-Host "  Analysis nodes do not pretend to be scene objects."
Write-Host "  Viewport reflects the shared selected entity through its existing context header."
Write-Host ""
Write-Host "Rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
