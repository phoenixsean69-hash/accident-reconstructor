param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - NODE EDITOR" -ForegroundColor Cyan
Write-Host " Docked beside Timeline" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if (-not $text.Contains('#include <cmath>')) {
    if ($text.Contains('#include <cstdio>')) {
        $text = $text.Replace(
            '#include <cstdio>',
            "#include <cstdio>`r`n#include <cmath>"
        )
    }
    else {
        throw "Could not find include insertion point for <cmath>."
    }
}


function Get-FunctionBlock {
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
            if ($ch -eq '\') { $escape = $true; continue }
            if ($ch -eq '"') { $inString = $false }
            continue
        }

        if ($inChar) {
            if ($ch -eq '\') { $escape = $true; continue }
            if ($ch -eq "'") { $inChar = $false }
            continue
        }

        if ($ch -eq '"') { $inString = $true; continue }
        if ($ch -eq "'") { $inChar = $true; continue }

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
        Text = $Source.Substring($start, $end - $start + 1)
    }
}

$required = @(
    'struct EditorShellState',
    'static void drawTimeline()',
    'static void drawInterface()',
    'DockBuilderDockWindow',
    'gEditorShell.showTimeline'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected editor marker not found: $marker"
    }
}

if ($text.Contains('static void drawNodeEditor()')) {
    Write-Host "[OK] Node Editor already appears installed." -ForegroundColor Green
    exit 0
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-node-editor-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. ADD showNodeEditor TO EditorShellState
# ============================================================

if ($text.Contains('bool showTimeline=true;') -and
    -not $text.Contains('bool showNodeEditor=true;'))
{
    $text = $text.Replace(
        'bool showTimeline=true;',
        "bool showTimeline=true;`r`n    bool showNodeEditor=true;"
    )

    Write-Host "[OK] Added Node Editor visibility state." -ForegroundColor Green
}

# ============================================================
# 2. ADD VIEW MENU TOGGLE
# ============================================================

$viewMenuAnchor = @'
        ImGui::MenuItem(
            "Timeline",
            nullptr,
            &gEditorShell.showTimeline
        );
'@

$viewMenuInsert = @'
        ImGui::MenuItem(
            "Timeline",
            nullptr,
            &gEditorShell.showTimeline
        );

        ImGui::MenuItem(
            "Node Editor",
            nullptr,
            &gEditorShell.showNodeEditor
        );
'@

if ($text.Contains($viewMenuAnchor)) {
    $text = $text.Replace(
        $viewMenuAnchor,
        $viewMenuInsert
    )
    Write-Host "[OK] Added View > Node Editor toggle." -ForegroundColor Green
}
else {
    Write-Host "[INFO] View-menu Timeline block not matched; editor will still dock/open." -ForegroundColor Yellow
}

# ============================================================
# 3. INSERT NODE EDITOR FUNCTION BEFORE drawTimeline()
# ============================================================

$timelineSignature = 'static void drawTimeline()'
$timelinePos = $text.IndexOf($timelineSignature)

if ($timelinePos -lt 0) {
    throw "Could not find drawTimeline() insertion point."
}

$nodeEditor = @'
static void drawNodeEditor()
{
    struct Node
    {
        int id;
        const char* title;
        const char* subtitle;
        ImVec2 pos;
        ImVec2 size;
        int inputCount;
        int outputCount;
        bool enabled;
    };

    struct Link
    {
        int fromNode;
        int fromPin;
        int toNode;
        int toPin;
    };

    static bool initialized=false;
    static bool showGrid=true;
    static bool snapToGrid=true;
    static float zoom=1.0f;
    static ImVec2 pan(0.0f,0.0f);
    static int selectedNode=-1;
    static int draggingNode=-1;
    static ImVec2 dragOffset(0.0f,0.0f);

    static std::vector<Node> nodes;
    static std::vector<Link> links;

    if (!initialized)
    {
        nodes={
            {
                1,
                "Evidence Input",
                "Scene evidence source",
                ImVec2(110.0f,90.0f),
                ImVec2(220.0f,126.0f),
                0,
                2,
                true
            },
            {
                2,
                "Skid Analysis",
                "Speed from skid evidence",
                ImVec2(430.0f,58.0f),
                ImVec2(230.0f,138.0f),
                2,
                1,
                true
            },
            {
                3,
                "Momentum Analysis",
                "Collision momentum solve",
                ImVec2(430.0f,242.0f),
                ImVec2(230.0f,138.0f),
                2,
                1,
                true
            },
            {
                4,
                "Reconstruction Result",
                "Final reconstruction output",
                ImVec2(770.0f,145.0f),
                ImVec2(250.0f,142.0f),
                2,
                0,
                true
            }
        };

        links={
            {1,0,2,0},
            {1,1,3,0},
            {2,0,4,0},
            {3,0,4,1}
        };

        initialized=true;
    }

    ImGui::Begin(
        "Node Editor",
        &gEditorShell.showNodeEditor,
        ImGuiWindowFlags_NoMove
    );

    // ========================================================
    // TOOLBAR
    // ========================================================

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(9.0f,5.0f)
    );

    if (ImGui::Button(
        "ADD NODE  +",
        ImVec2(104.0f,30.0f)))
    {
        ImGui::OpenPopup("##NodeEditorAddPopup");
    }

    if (ImGui::BeginPopup("##NodeEditorAddPopup"))
    {
        ImGui::TextDisabled("ADD NODE");
        ImGui::Separator();

        if (ImGui::MenuItem("Evidence Input"))
        {
            const int id=
                nodes.empty()
                    ? 1
                    : nodes.back().id+1;

            nodes.push_back({
                id,
                "Evidence Input",
                "Scene evidence source",
                ImVec2(160.0f-pan.x,120.0f-pan.y),
                ImVec2(220.0f,126.0f),
                0,
                2,
                true
            });
        }

        if (ImGui::MenuItem("Skid Analysis"))
        {
            const int id=
                nodes.empty()
                    ? 1
                    : nodes.back().id+1;

            nodes.push_back({
                id,
                "Skid Analysis",
                "Speed from skid evidence",
                ImVec2(420.0f-pan.x,120.0f-pan.y),
                ImVec2(230.0f,138.0f),
                2,
                1,
                true
            });
        }

        if (ImGui::MenuItem("Momentum Analysis"))
        {
            const int id=
                nodes.empty()
                    ? 1
                    : nodes.back().id+1;

            nodes.push_back({
                id,
                "Momentum Analysis",
                "Collision momentum solve",
                ImVec2(420.0f-pan.x,250.0f-pan.y),
                ImVec2(230.0f,138.0f),
                2,
                1,
                true
            });
        }

        if (ImGui::MenuItem("Result Output"))
        {
            const int id=
                nodes.empty()
                    ? 1
                    : nodes.back().id+1;

            nodes.push_back({
                id,
                "Reconstruction Result",
                "Final reconstruction output",
                ImVec2(760.0f-pan.x,170.0f-pan.y),
                ImVec2(250.0f,142.0f),
                2,
                0,
                true
            });
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine(0.0f,7.0f);

    if (ImGui::Button(
        "CENTER",
        ImVec2(76.0f,30.0f)))
    {
        pan=ImVec2(0.0f,0.0f);
        zoom=1.0f;
    }

    ImGui::SameLine(0.0f,10.0f);

    ImGui::Checkbox(
        "Grid",
        &showGrid
    );

    ImGui::SameLine(0.0f,10.0f);

    ImGui::Checkbox(
        "Snap",
        &snapToGrid
    );

    ImGui::SameLine(0.0f,12.0f);

    ImGui::TextDisabled("ZOOM");
    ImGui::SameLine(0.0f,6.0f);

    ImGui::SetNextItemWidth(116.0f);

    ImGui::SliderFloat(
        "##NodeEditorZoom",
        &zoom,
        0.65f,
        1.75f,
        "%.2fx"
    );

    if (selectedNode!=-1)
    {
        ImGui::SameLine(0.0f,14.0f);

        if (ImGui::Button(
            "DELETE SELECTED",
            ImVec2(132.0f,30.0f)))
        {
            links.erase(
                std::remove_if(
                    links.begin(),
                    links.end(),
                    [&](const Link& link)
                    {
                        return
                            link.fromNode==selectedNode ||
                            link.toNode==selectedNode;
                    }
                ),
                links.end()
            );

            nodes.erase(
                std::remove_if(
                    nodes.begin(),
                    nodes.end(),
                    [&](const Node& node)
                    {
                        return node.id==selectedNode;
                    }
                ),
                nodes.end()
            );

            selectedNode=-1;
        }
    }

    ImGui::PopStyleVar();

    ImGui::Separator();

    // ========================================================
    // CANVAS
    // ========================================================

    const ImVec2 canvasPos=
        ImGui::GetCursorScreenPos();

    const ImVec2 canvasSize=
        ImGui::GetContentRegionAvail();

    ImGui::InvisibleButton(
        "##NodeCanvas",
        canvasSize,
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonMiddle |
        ImGuiButtonFlags_MouseButtonRight
    );

    const bool canvasHovered=
        ImGui::IsItemHovered();

    ImDrawList* dl=
        ImGui::GetWindowDrawList();

    dl->PushClipRect(
        canvasPos,
        ImVec2(
            canvasPos.x+canvasSize.x,
            canvasPos.y+canvasSize.y
        ),
        true
    );

    dl->AddRectFilled(
        canvasPos,
        ImVec2(
            canvasPos.x+canvasSize.x,
            canvasPos.y+canvasSize.y
        ),
        IM_COL32(20,23,27,255)
    );

    // --------------------------------------------------------
    // GRID
    // --------------------------------------------------------

    if (showGrid)
    {
        const float minor=
            24.0f*zoom;

        const float major=
            minor*4.0f;

        const float startMinorX=
            canvasPos.x+
            std::fmod(
                pan.x*zoom,
                minor
            );

        const float startMinorY=
            canvasPos.y+
            std::fmod(
                pan.y*zoom,
                minor
            );

        for (
            float x=startMinorX;
            x<canvasPos.x+canvasSize.x;
            x+=minor)
        {
            dl->AddLine(
                ImVec2(x,canvasPos.y),
                ImVec2(
                    x,
                    canvasPos.y+canvasSize.y
                ),
                IM_COL32(44,48,54,135),
                1.0f
            );
        }

        for (
            float y=startMinorY;
            y<canvasPos.y+canvasSize.y;
            y+=minor)
        {
            dl->AddLine(
                ImVec2(canvasPos.x,y),
                ImVec2(
                    canvasPos.x+canvasSize.x,
                    y
                ),
                IM_COL32(44,48,54,135),
                1.0f
            );
        }

        const float startMajorX=
            canvasPos.x+
            std::fmod(
                pan.x*zoom,
                major
            );

        const float startMajorY=
            canvasPos.y+
            std::fmod(
                pan.y*zoom,
                major
            );

        for (
            float x=startMajorX;
            x<canvasPos.x+canvasSize.x;
            x+=major)
        {
            dl->AddLine(
                ImVec2(x,canvasPos.y),
                ImVec2(
                    x,
                    canvasPos.y+canvasSize.y
                ),
                IM_COL32(60,65,73,155),
                1.0f
            );
        }

        for (
            float y=startMajorY;
            y<canvasPos.y+canvasSize.y;
            y+=major)
        {
            dl->AddLine(
                ImVec2(canvasPos.x,y),
                ImVec2(
                    canvasPos.x+canvasSize.x,
                    y
                ),
                IM_COL32(60,65,73,155),
                1.0f
            );
        }
    }

    auto nodeScreenPos = [&](const Node& node)
    {
        return ImVec2(
            canvasPos.x+
            (node.pos.x+pan.x)*zoom,
            canvasPos.y+
            (node.pos.y+pan.y)*zoom
        );
    };

    auto inputPinPos = [&](const Node& node,int pin)
    {
        const ImVec2 p=
            nodeScreenPos(node);

        const float nodeH=
            node.size.y*zoom;

        const float spacing=
            nodeH/
            static_cast<float>(
                node.inputCount+1
            );

        return ImVec2(
            p.x,
            p.y+
            spacing*
            static_cast<float>(pin+1)
        );
    };

    auto outputPinPos = [&](const Node& node,int pin)
    {
        const ImVec2 p=
            nodeScreenPos(node);

        const float nodeH=
            node.size.y*zoom;

        const float spacing=
            nodeH/
            static_cast<float>(
                node.outputCount+1
            );

        return ImVec2(
            p.x+
            node.size.x*zoom,
            p.y+
            spacing*
            static_cast<float>(pin+1)
        );
    };

    auto findNode = [&](int id) -> Node*
    {
        for (auto& node : nodes)
        {
            if (node.id==id)
                return &node;
        }

        return nullptr;
    };

    // --------------------------------------------------------
    // LINKS
    // --------------------------------------------------------

    for (const Link& link : links)
    {
        Node* from=
            findNode(link.fromNode);

        Node* to=
            findNode(link.toNode);

        if (!from || !to)
            continue;

        if (link.fromPin>=from->outputCount ||
            link.toPin>=to->inputCount)
        {
            continue;
        }

        const ImVec2 a=
            outputPinPos(
                *from,
                link.fromPin
            );

        const ImVec2 b=
            inputPinPos(
                *to,
                link.toPin
            );

        const float tangent=
            std::max(
                70.0f*zoom,
                std::fabs(b.x-a.x)*0.45f
            );

        dl->AddBezierCubic(
            a,
            ImVec2(
                a.x+tangent,
                a.y
            ),
            ImVec2(
                b.x-tangent,
                b.y
            ),
            b,
            IM_COL32(130,151,178,225),
            2.2f
        );
    }

    // --------------------------------------------------------
    // NODES
    // --------------------------------------------------------

    for (Node& node : nodes)
    {
        const ImVec2 p=
            nodeScreenPos(node);

        const ImVec2 size(
            node.size.x*zoom,
            node.size.y*zoom
        );

        const ImVec2 max(
            p.x+size.x,
            p.y+size.y
        );

        const bool selected=
            selectedNode==node.id;

        const ImVec2 mouse=
            ImGui::GetIO().MousePos;

        const bool hovered=
            mouse.x>=p.x &&
            mouse.x<=max.x &&
            mouse.y>=p.y &&
            mouse.y<=max.y;

        dl->AddRectFilled(
            p,
            max,
            selected
                ? IM_COL32(42,47,54,255)
                : IM_COL32(32,36,42,255),
            6.0f
        );

        dl->AddRect(
            p,
            max,
            selected
                ? toU32(colorAccent())
                : IM_COL32(78,85,95,245),
            6.0f,
            0,
            selected
                ? 1.8f
                : 1.0f
        );

        const float titleH=
            34.0f*zoom;

        dl->AddRectFilled(
            p,
            ImVec2(
                max.x,
                p.y+titleH
            ),
            selected
                ? IM_COL32(61,55,41,255)
                : IM_COL32(43,48,56,255),
            6.0f,
            ImDrawFlags_RoundCornersTop
        );

        dl->AddText(
            ImVec2(
                p.x+12.0f*zoom,
                p.y+9.0f*zoom
            ),
            toU32(colorText()),
            node.title
        );

        dl->AddText(
            ImVec2(
                p.x+12.0f*zoom,
                p.y+48.0f*zoom
            ),
            toU32(colorMuted()),
            node.subtitle
        );

        dl->AddText(
            ImVec2(
                p.x+12.0f*zoom,
                max.y-28.0f*zoom
            ),
            node.enabled
                ? IM_COL32(165,172,182,225)
                : IM_COL32(100,105,113,190),
            node.enabled
                ? "Enabled"
                : "Disabled"
        );

        for (int pin=0;pin<node.inputCount;++pin)
        {
            const ImVec2 pinPos=
                inputPinPos(
                    node,
                    pin
                );

            dl->AddCircleFilled(
                pinPos,
                5.5f*zoom,
                IM_COL32(110,137,170,255)
            );

            dl->AddCircle(
                pinPos,
                7.5f*zoom,
                IM_COL32(170,188,210,220),
                18,
                1.2f
            );
        }

        for (int pin=0;pin<node.outputCount;++pin)
        {
            const ImVec2 pinPos=
                outputPinPos(
                    node,
                    pin
                );

            dl->AddCircleFilled(
                pinPos,
                5.5f*zoom,
                IM_COL32(160,170,184,255)
            );

            dl->AddCircle(
                pinPos,
                7.5f*zoom,
                IM_COL32(205,210,220,225),
                18,
                1.2f
            );
        }

        // Selection / dragging starts on title/header region.
        const bool headerHovered=
            hovered &&
            mouse.y<=p.y+titleH;

        if (headerHovered &&
            ImGui::IsMouseClicked(
                ImGuiMouseButton_Left))
        {
            selectedNode=node.id;
            draggingNode=node.id;

            dragOffset=
                ImVec2(
                    mouse.x-p.x,
                    mouse.y-p.y
                );
        }

        if (draggingNode==node.id &&
            ImGui::IsMouseDown(
                ImGuiMouseButton_Left))
        {
            const ImVec2 newScreen(
                mouse.x-dragOffset.x,
                mouse.y-dragOffset.y
            );

            ImVec2 newPos(
                (newScreen.x-canvasPos.x)/
                    zoom-
                    pan.x,
                (newScreen.y-canvasPos.y)/
                    zoom-
                    pan.y
            );

            if (snapToGrid)
            {
                const float snap=24.0f;

                newPos.x=
                    std::round(
                        newPos.x/snap
                    )*snap;

                newPos.y=
                    std::round(
                        newPos.y/snap
                    )*snap;
            }

            node.pos=newPos;
        }

        if (draggingNode==node.id &&
            ImGui::IsMouseReleased(
                ImGuiMouseButton_Left))
        {
            draggingNode=-1;
        }
    }

    // --------------------------------------------------------
    // CANVAS PAN
    // --------------------------------------------------------

    if (canvasHovered &&
        ImGui::IsMouseDragging(
            ImGuiMouseButton_Middle,
            0.0f))
    {
        const ImVec2 delta=
            ImGui::GetIO().MouseDelta;

        pan.x += delta.x/zoom;
        pan.y += delta.y/zoom;
    }

    // --------------------------------------------------------
    // RIGHT CLICK MENU
    // --------------------------------------------------------

    if (ImGui::BeginPopupContextItem(
        "##NodeCanvasContext"))
    {
        ImGui::TextDisabled("NODE EDITOR");
        ImGui::Separator();

        if (ImGui::MenuItem("Add Evidence Input"))
        {
            const int id=
                nodes.empty()
                    ? 1
                    : nodes.back().id+1;

            nodes.push_back({
                id,
                "Evidence Input",
                "Scene evidence source",
                ImVec2(220.0f-pan.x,140.0f-pan.y),
                ImVec2(220.0f,126.0f),
                0,
                2,
                true
            });
        }

        if (ImGui::MenuItem("Add Analysis Node"))
        {
            const int id=
                nodes.empty()
                    ? 1
                    : nodes.back().id+1;

            nodes.push_back({
                id,
                "Analysis Node",
                "Reconstruction analysis stage",
                ImVec2(440.0f-pan.x,180.0f-pan.y),
                ImVec2(230.0f,138.0f),
                2,
                1,
                true
            });
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Center View"))
        {
            pan=ImVec2(0.0f,0.0f);
            zoom=1.0f;
        }

        ImGui::MenuItem(
            "Grid",
            nullptr,
            &showGrid
        );

        ImGui::MenuItem(
            "Snap to Grid",
            nullptr,
            &snapToGrid
        );

        ImGui::EndPopup();
    }

    dl->PopClipRect();

    ImGui::End();
}

'@

$text = $text.Insert(
    $timelinePos,
    $nodeEditor
)

Write-Host "[OK] Added native Dear ImGui node editor." -ForegroundColor Green

# ============================================================
# 4. DRAW NODE EDITOR IN INTERFACE
# ============================================================

$drawAnchor = @'
    if (gEditorShell.showTimeline)
        drawTimeline();
'@

$drawInsert = @'
    if (gEditorShell.showTimeline)
        drawTimeline();

    if (gEditorShell.showNodeEditor)
        drawNodeEditor();
'@

if ($text.Contains($drawAnchor)) {
    $text = $text.Replace(
        $drawAnchor,
        $drawInsert
    )
    Write-Host "[OK] Node Editor added to draw loop." -ForegroundColor Green
}
else {
    throw "Could not locate drawTimeline() call in drawInterface()."
}

# ============================================================
# 5. DOCK NODE EDITOR BESIDE TIMELINE
# ============================================================

$dockAnchor = @'
        ImGui::DockBuilderDockWindow(
            "Timeline",
            bottom
        );
'@

$dockInsert = @'
        ImGui::DockBuilderDockWindow(
            "Timeline",
            bottom
        );

        ImGui::DockBuilderDockWindow(
            "Node Editor",
            bottom
        );
'@

if ($text.Contains($dockAnchor)) {
    $text = $text.Replace(
        $dockAnchor,
        $dockInsert
    )
    Write-Host "[OK] Node Editor docked into Timeline node." -ForegroundColor Green
}
else {
    throw "Could not locate Timeline DockBuilder block."
}

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'static void drawNodeEditor()',
    '"Node Editor"',
    '##NodeCanvas',
    'ADD NODE  +',
    'Evidence Input',
    'Skid Analysis',
    'Momentum Analysis',
    'Reconstruction Result',
    'DockBuilderDockWindow(',
    'gEditorShell.showNodeEditor'
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] Node Editor installed." -ForegroundColor Cyan
Write-Host "[OK] Docked beside Timeline as a tab." -ForegroundColor Green
Write-Host "[OK] Pan / zoom / grid / snapping." -ForegroundColor Green
Write-Host "[OK] Draggable nodes." -ForegroundColor Green
Write-Host "[OK] Input/output pins and curved links." -ForegroundColor Green
Write-Host "[OK] Add-node popup and context menu." -ForegroundColor Green
Write-Host "[OK] View-menu visibility toggle." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
