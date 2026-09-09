param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - FULL NODE GRAPH ENGINE" -ForegroundColor Cyan
Write-Host " Links / Types / Execution / History / Selection" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if (-not $text.Contains('static void drawNodeEditor()')) {
    throw "Existing Node Editor not found. Apply add_node_editor_tab.ps1 first."
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-full-node-graph-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# ENSURE STANDARD LIBRARY INCLUDES
# ============================================================

$neededIncludes = @(
    '#include <vector>',
    '#include <string>',
    '#include <array>',
    '#include <algorithm>',
    '#include <cmath>',
    '#include <functional>',
    '#include <fstream>',
    '#include <sstream>',
    '#include <iomanip>',
    '#include <utility>',
    '#include <cstdio>'
)

$missing = @()

foreach ($inc in $neededIncludes) {
    if (-not $text.Contains($inc)) {
        $missing += $inc
    }
}

if ($missing.Count -gt 0) {
    $matches = [regex]::Matches(
        $text,
        '(?m)^#include[^\r\n]*'
    )

    if ($matches.Count -eq 0) {
        throw "Could not find include block."
    }

    $last = $matches[$matches.Count - 1]
    $insertAt = $last.Index + $last.Length

    $includeText = "`r`n" + ($missing -join "`r`n")

    $text = $text.Insert(
        $insertAt,
        $includeText
    )

    Write-Host "[OK] Added required standard-library includes." -ForegroundColor Green
}

# ============================================================
# FIND FUNCTION STRUCTURALLY
# ============================================================

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

    return $Source.Substring(
        $start,
        $end - $start + 1
    )
}

$oldNodeEditor = Get-FunctionBlock `
    -Source $text `
    -Signature 'static void drawNodeEditor()'

# ============================================================
# FULL NODE EDITOR
# ============================================================

$newNodeEditor = @'
static void drawNodeEditor()
{
    enum class PinType
    {
        EvidenceDistance=0,
        VehicleState,
        Scalar
    };

    enum class NodeType
    {
        EvidenceInput=0,
        VehicleInput,
        SkidAnalysis,
        SpeedAnalysis,
        MomentumAnalysis,
        ResultOutput
    };

    struct Node
    {
        int id=0;
        NodeType type=NodeType::EvidenceInput;
        std::string title;
        ImVec2 pos{0.0f,0.0f};
        bool enabled=true;

        // Generic editable parameters:
        // Evidence: p0 = distance m
        // Vehicle:  p0 = mass kg, p1 = velocity m/s
        // Skid:     p0 = friction coefficient
        // Speed:    p0 = multiplier
        float p0=0.0f;
        float p1=0.0f;

        float result=0.0f;
        float result2=0.0f;
        bool resultValid=false;
        std::string error;
    };

    struct Link
    {
        int id=0;
        int fromNode=0;
        int fromPin=0;
        int toNode=0;
        int toPin=0;
    };

    struct EvalValue
    {
        PinType type=PinType::Scalar;
        bool valid=false;
        float a=0.0f;
        float b=0.0f;
        std::string error;
    };

    static bool initialized=false;

    static std::vector<Node> nodes;
    static std::vector<Link> links;

    static std::vector<int> selectedNodes;
    static std::vector<int> selectedLinks;

    static int nextNodeId=1;
    static int nextLinkId=1;

    static bool showGrid=true;
    static bool snapToGrid=true;
    static float zoom=1.0f;
    static ImVec2 pan(0.0f,0.0f);

    static bool linkDragActive=false;
    static int linkDragNode=-1;
    static int linkDragPin=-1;
    static PinType linkDragType=PinType::Scalar;

    static bool boxSelecting=false;
    static ImVec2 boxStart(0.0f,0.0f);
    static ImVec2 boxEnd(0.0f,0.0f);

    static bool nodeDragging=false;
    static ImVec2 dragMouseStart(0.0f,0.0f);
    static std::vector<std::pair<int,ImVec2>> dragStartPositions;

    static std::vector<std::string> undoStack;
    static std::vector<std::string> redoStack;

    static std::string internalClipboard;

    static char graphPath[260]="node_graph.sargraph";

    static std::string graphMessage="Graph ready";
    static bool graphMessageError=false;

    static int lastExecutedNodes=0;
    static int lastExecutionErrors=0;

    static std::string propertyEditBefore;
    static bool propertyEditChanged=false;

    auto defaultTitle = [](NodeType type) -> const char*
    {
        switch (type)
        {
            case NodeType::EvidenceInput:
                return "Evidence Input";

            case NodeType::VehicleInput:
                return "Vehicle State";

            case NodeType::SkidAnalysis:
                return "Skid Analysis";

            case NodeType::SpeedAnalysis:
                return "Speed Analysis";

            case NodeType::MomentumAnalysis:
                return "Momentum Analysis";

            case NodeType::ResultOutput:
                return "Reconstruction Result";
        }

        return "Node";
    };

    auto pinTypeLabel = [](PinType type) -> const char*
    {
        switch (type)
        {
            case PinType::EvidenceDistance:
                return "Evidence Distance";

            case PinType::VehicleState:
                return "Vehicle State";

            case PinType::Scalar:
                return "Scalar";
        }

        return "Unknown";
    };

    auto inputCount = [](NodeType type) -> int
    {
        switch (type)
        {
            case NodeType::EvidenceInput:
            case NodeType::VehicleInput:
                return 0;

            case NodeType::SkidAnalysis:
            case NodeType::SpeedAnalysis:
                return 1;

            case NodeType::MomentumAnalysis:
            case NodeType::ResultOutput:
                return 2;
        }

        return 0;
    };

    auto outputCount = [](NodeType type) -> int
    {
        return type==NodeType::ResultOutput
            ? 0
            : 1;
    };

    auto inputType = [](NodeType type,int pin) -> PinType
    {
        switch (type)
        {
            case NodeType::SkidAnalysis:
            case NodeType::SpeedAnalysis:
                return PinType::EvidenceDistance;

            case NodeType::MomentumAnalysis:
                return PinType::VehicleState;

            case NodeType::ResultOutput:
                return PinType::Scalar;

            default:
                break;
        }

        return PinType::Scalar;
    };

    auto outputType = [](NodeType type,int) -> PinType
    {
        switch (type)
        {
            case NodeType::EvidenceInput:
                return PinType::EvidenceDistance;

            case NodeType::VehicleInput:
                return PinType::VehicleState;

            case NodeType::SkidAnalysis:
            case NodeType::SpeedAnalysis:
            case NodeType::MomentumAnalysis:
                return PinType::Scalar;

            case NodeType::ResultOutput:
                break;
        }

        return PinType::Scalar;
    };

    auto inputLabel = [](NodeType type,int pin) -> const char*
    {
        switch (type)
        {
            case NodeType::SkidAnalysis:
            case NodeType::SpeedAnalysis:
                return "Evidence";

            case NodeType::MomentumAnalysis:
                return pin==0
                    ? "Vehicle A"
                    : "Vehicle B";

            case NodeType::ResultOutput:
                return pin==0
                    ? "Result A"
                    : "Result B";

            default:
                break;
        }

        return "Input";
    };

    auto outputLabel = [](NodeType type,int) -> const char*
    {
        switch (type)
        {
            case NodeType::EvidenceInput:
                return "Distance";

            case NodeType::VehicleInput:
                return "State";

            case NodeType::SkidAnalysis:
            case NodeType::SpeedAnalysis:
                return "Speed";

            case NodeType::MomentumAnalysis:
                return "Momentum";

            default:
                break;
        }

        return "Output";
    };

    auto nodeSize = [](NodeType type) -> ImVec2
    {
        if (type==NodeType::ResultOutput)
            return ImVec2(270.0f,154.0f);

        return ImVec2(252.0f,154.0f);
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

    auto isNodeSelected = [&](int id)
    {
        return std::find(
            selectedNodes.begin(),
            selectedNodes.end(),
            id
        ) != selectedNodes.end();
    };

    auto isLinkSelected = [&](int id)
    {
        return std::find(
            selectedLinks.begin(),
            selectedLinks.end(),
            id
        ) != selectedLinks.end();
    };

    auto clearSelection = [&]()
    {
        selectedNodes.clear();
        selectedLinks.clear();
    };

    auto serializeGraph = [&]() -> std::string
    {
        std::ostringstream out;

        out << "SAR_NODE_GRAPH_V2\n";

        for (const Node& node : nodes)
        {
            out
                << "N "
                << node.id << " "
                << static_cast<int>(node.type) << " "
                << std::quoted(node.title) << " "
                << node.pos.x << " "
                << node.pos.y << " "
                << (node.enabled ? 1 : 0) << " "
                << node.p0 << " "
                << node.p1
                << "\n";
        }

        for (const Link& link : links)
        {
            out
                << "L "
                << link.id << " "
                << link.fromNode << " "
                << link.fromPin << " "
                << link.toNode << " "
                << link.toPin
                << "\n";
        }

        return out.str();
    };

    auto restoreGraph = [&](const std::string& data) -> bool
    {
        std::istringstream in(data);

        std::string header;

        if (!std::getline(in,header) ||
            header!="SAR_NODE_GRAPH_V2")
        {
            return false;
        }

        std::vector<Node> restoredNodes;
        std::vector<Link> restoredLinks;

        std::string line;

        int maxNodeId=0;
        int maxLinkId=0;

        while (std::getline(in,line))
        {
            if (line.empty())
                continue;

            std::istringstream row(line);

            char kind=0;
            row >> kind;

            if (kind=='N')
            {
                Node node;
                int typeInt=0;
                int enabledInt=1;

                row
                    >> node.id
                    >> typeInt
                    >> std::quoted(node.title)
                    >> node.pos.x
                    >> node.pos.y
                    >> enabledInt
                    >> node.p0
                    >> node.p1;

                if (!row)
                    return false;

                if (typeInt<0 ||
                    typeInt>
                    static_cast<int>(
                        NodeType::ResultOutput
                    ))
                {
                    return false;
                }

                node.type=
                    static_cast<NodeType>(
                        typeInt
                    );

                node.enabled=
                    enabledInt!=0;

                restoredNodes.push_back(node);

                maxNodeId=
                    std::max(
                        maxNodeId,
                        node.id
                    );
            }
            else if (kind=='L')
            {
                Link link;

                row
                    >> link.id
                    >> link.fromNode
                    >> link.fromPin
                    >> link.toNode
                    >> link.toPin;

                if (!row)
                    return false;

                restoredLinks.push_back(link);

                maxLinkId=
                    std::max(
                        maxLinkId,
                        link.id
                    );
            }
        }

        nodes=std::move(restoredNodes);
        links=std::move(restoredLinks);

        nextNodeId=maxNodeId+1;
        nextLinkId=maxLinkId+1;

        clearSelection();

        for (Node& node : nodes)
        {
            node.resultValid=false;
            node.result=0.0f;
            node.result2=0.0f;
            node.error.clear();
        }

        return true;
    };

    auto pushSnapshot = [&](const std::string& snapshot)
    {
        if (!undoStack.empty() &&
            undoStack.back()==snapshot)
        {
            return;
        }

        undoStack.push_back(snapshot);

        if (undoStack.size()>64)
        {
            undoStack.erase(
                undoStack.begin()
            );
        }

        redoStack.clear();
    };

    auto pushUndo = [&]()
    {
        pushSnapshot(
            serializeGraph()
        );
    };

    auto doUndo = [&]()
    {
        if (undoStack.empty())
            return;

        const std::string current=
            serializeGraph();

        redoStack.push_back(current);

        const std::string target=
            undoStack.back();

        undoStack.pop_back();

        if (restoreGraph(target))
        {
            graphMessage="Undo";
            graphMessageError=false;
        }
    };

    auto doRedo = [&]()
    {
        if (redoStack.empty())
            return;

        const std::string current=
            serializeGraph();

        undoStack.push_back(current);

        const std::string target=
            redoStack.back();

        redoStack.pop_back();

        if (restoreGraph(target))
        {
            graphMessage="Redo";
            graphMessageError=false;
        }
    };

    auto addNode = [&](NodeType type,ImVec2 pos)
    {
        pushUndo();

        Node node;
        node.id=nextNodeId++;
        node.type=type;
        node.title=defaultTitle(type);
        node.pos=pos;

        switch (type)
        {
            case NodeType::EvidenceInput:
                node.p0=24.0f;
                break;

            case NodeType::VehicleInput:
                node.p0=1500.0f;
                node.p1=12.0f;
                break;

            case NodeType::SkidAnalysis:
                node.p0=0.70f;
                break;

            case NodeType::SpeedAnalysis:
                node.p0=1.0f;
                break;

            case NodeType::MomentumAnalysis:
                break;

            case NodeType::ResultOutput:
                break;
        }

        nodes.push_back(node);

        clearSelection();
        selectedNodes.push_back(node.id);
    };

    auto deleteSelection = [&]()
    {
        if (selectedNodes.empty() &&
            selectedLinks.empty())
        {
            return;
        }

        pushUndo();

        links.erase(
            std::remove_if(
                links.begin(),
                links.end(),
                [&](const Link& link)
                {
                    if (isLinkSelected(link.id))
                        return true;

                    return
                        isNodeSelected(link.fromNode) ||
                        isNodeSelected(link.toNode);
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
                    return isNodeSelected(node.id);
                }
            ),
            nodes.end()
        );

        clearSelection();

        graphMessage="Selection deleted";
        graphMessageError=false;
    };

    auto copySelection = [&]()
    {
        if (selectedNodes.empty())
            return;

        std::ostringstream out;
        out << "SAR_NODE_CLIP_V1\n";

        for (const Node& node : nodes)
        {
            if (!isNodeSelected(node.id))
                continue;

            out
                << "N "
                << node.id << " "
                << static_cast<int>(node.type) << " "
                << std::quoted(node.title) << " "
                << node.pos.x << " "
                << node.pos.y << " "
                << (node.enabled ? 1 : 0) << " "
                << node.p0 << " "
                << node.p1
                << "\n";
        }

        for (const Link& link : links)
        {
            if (isNodeSelected(link.fromNode) &&
                isNodeSelected(link.toNode))
            {
                out
                    << "L "
                    << link.id << " "
                    << link.fromNode << " "
                    << link.fromPin << " "
                    << link.toNode << " "
                    << link.toPin
                    << "\n";
            }
        }

        internalClipboard=out.str();

        ImGui::SetClipboardText(
            internalClipboard.c_str()
        );

        graphMessage="Copied selection";
        graphMessageError=false;
    };

    auto pasteFromText = [&](const std::string& data)
    {
        std::istringstream in(data);

        std::string header;

        if (!std::getline(in,header) ||
            header!="SAR_NODE_CLIP_V1")
        {
            graphMessage=
                "Clipboard does not contain Sovereign nodes";

            graphMessageError=true;
            return;
        }

        struct ClipNode
        {
            int oldId=0;
            Node node;
        };

        std::vector<ClipNode> clipNodes;
        std::vector<Link> clipLinks;

        std::string line;

        while (std::getline(in,line))
        {
            if (line.empty())
                continue;

            std::istringstream row(line);

            char kind=0;
            row >> kind;

            if (kind=='N')
            {
                ClipNode item;
                int typeInt=0;
                int enabledInt=1;

                row
                    >> item.oldId
                    >> typeInt
                    >> std::quoted(item.node.title)
                    >> item.node.pos.x
                    >> item.node.pos.y
                    >> enabledInt
                    >> item.node.p0
                    >> item.node.p1;

                if (!row)
                    continue;

                item.node.type=
                    static_cast<NodeType>(
                        typeInt
                    );

                item.node.enabled=
                    enabledInt!=0;

                clipNodes.push_back(item);
            }
            else if (kind=='L')
            {
                Link link;

                row
                    >> link.id
                    >> link.fromNode
                    >> link.fromPin
                    >> link.toNode
                    >> link.toPin;

                if (row)
                    clipLinks.push_back(link);
            }
        }

        if (clipNodes.empty())
            return;

        pushUndo();

        std::vector<std::pair<int,int>> idMap;

        clearSelection();

        for (ClipNode& item : clipNodes)
        {
            const int newId=
                nextNodeId++;

            idMap.push_back(
                {item.oldId,newId}
            );

            item.node.id=newId;

            item.node.pos.x += 32.0f;
            item.node.pos.y += 32.0f;

            item.node.resultValid=false;
            item.node.error.clear();

            nodes.push_back(item.node);

            selectedNodes.push_back(newId);
        }

        auto remapId = [&](int oldId) -> int
        {
            for (const auto& pair : idMap)
            {
                if (pair.first==oldId)
                    return pair.second;
            }

            return -1;
        };

        for (const Link& oldLink : clipLinks)
        {
            const int from=
                remapId(oldLink.fromNode);

            const int to=
                remapId(oldLink.toNode);

            if (from<0 || to<0)
                continue;

            links.push_back({
                nextLinkId++,
                from,
                oldLink.fromPin,
                to,
                oldLink.toPin
            });
        }

        graphMessage="Pasted selection";
        graphMessageError=false;
    };

    auto pasteSelection = [&]()
    {
        const char* clipboard=
            ImGui::GetClipboardText();

        if (clipboard &&
            std::string(clipboard).find(
                "SAR_NODE_CLIP_V1"
            )==0)
        {
            pasteFromText(clipboard);
            return;
        }

        if (!internalClipboard.empty())
        {
            pasteFromText(
                internalClipboard
            );
        }
    };

    auto duplicateSelection = [&]()
    {
        if (selectedNodes.empty())
            return;

        copySelection();

        pasteFromText(
            internalClipboard
        );

        graphMessage="Duplicated selection";
        graphMessageError=false;
    };

    auto compatible = [&](PinType from,PinType to)
    {
        return from==to;
    };

    auto saveGraph = [&]()
    {
        std::ofstream out(
            graphPath,
            std::ios::binary
        );

        if (!out)
        {
            graphMessage=
                "Could not open graph file for saving";

            graphMessageError=true;
            return;
        }

        const std::string data=
            serializeGraph();

        out.write(
            data.data(),
            static_cast<std::streamsize>(
                data.size()
            )
        );

        graphMessage=
            std::string("Saved: ")+
            graphPath;

        graphMessageError=false;
    };

    auto loadGraph = [&]()
    {
        std::ifstream in(
            graphPath,
            std::ios::binary
        );

        if (!in)
        {
            graphMessage=
                std::string("File not found: ")+
                graphPath;

            graphMessageError=true;
            return;
        }

        std::ostringstream buffer;
        buffer << in.rdbuf();

        const std::string before=
            serializeGraph();

        if (!restoreGraph(
            buffer.str()
        ))
        {
            graphMessage=
                "Invalid Sovereign graph file";

            graphMessageError=true;
            return;
        }

        pushSnapshot(before);

        graphMessage=
            std::string("Loaded: ")+
            graphPath;

        graphMessageError=false;
    };

    if (!initialized)
    {
        Node evidence;
        evidence.id=nextNodeId++;
        evidence.type=NodeType::EvidenceInput;
        evidence.title="Skid Evidence";
        evidence.pos=ImVec2(100.0f,88.0f);
        evidence.p0=24.0f;

        Node vehicleA;
        vehicleA.id=nextNodeId++;
        vehicleA.type=NodeType::VehicleInput;
        vehicleA.title="Vehicle A";
        vehicleA.pos=ImVec2(100.0f,286.0f);
        vehicleA.p0=1500.0f;
        vehicleA.p1=12.0f;

        Node vehicleB;
        vehicleB.id=nextNodeId++;
        vehicleB.type=NodeType::VehicleInput;
        vehicleB.title="Vehicle B";
        vehicleB.pos=ImVec2(100.0f,470.0f);
        vehicleB.p0=1250.0f;
        vehicleB.p1=-8.0f;

        Node skid;
        skid.id=nextNodeId++;
        skid.type=NodeType::SkidAnalysis;
        skid.title="Skid Analysis";
        skid.pos=ImVec2(430.0f,88.0f);
        skid.p0=0.70f;

        Node momentum;
        momentum.id=nextNodeId++;
        momentum.type=NodeType::MomentumAnalysis;
        momentum.title="Momentum Analysis";
        momentum.pos=ImVec2(430.0f,350.0f);

        Node output;
        output.id=nextNodeId++;
        output.type=NodeType::ResultOutput;
        output.title="Reconstruction Result";
        output.pos=ImVec2(770.0f,210.0f);

        nodes={
            evidence,
            vehicleA,
            vehicleB,
            skid,
            momentum,
            output
        };

        links={
            {
                nextLinkId++,
                evidence.id,
                0,
                skid.id,
                0
            },
            {
                nextLinkId++,
                vehicleA.id,
                0,
                momentum.id,
                0
            },
            {
                nextLinkId++,
                vehicleB.id,
                0,
                momentum.id,
                1
            },
            {
                nextLinkId++,
                skid.id,
                0,
                output.id,
                0
            },
            {
                nextLinkId++,
                momentum.id,
                0,
                output.id,
                1
            }
        };

        initialized=true;
    }

    // ========================================================
    // GRAPH EXECUTION
    // ========================================================

    auto executeGraph = [&]()
    {
        for (Node& node : nodes)
        {
            node.result=0.0f;
            node.result2=0.0f;
            node.resultValid=false;
            node.error.clear();
        }

        lastExecutedNodes=0;
        lastExecutionErrors=0;

        std::function<EvalValue(
            int,
            int,
            std::vector<int>&
        )> evalOutput;

        auto resolveInput =
            [&](int nodeId,
                int inputPin,
                PinType expected,
                std::vector<int>& visiting)
                -> EvalValue
        {
            for (const Link& link : links)
            {
                if (link.toNode==nodeId &&
                    link.toPin==inputPin)
                {
                    Node* source=
                        findNode(link.fromNode);

                    if (!source)
                    {
                        return {
                            expected,
                            false,
                            0.0f,
                            0.0f,
                            "Missing source node"
                        };
                    }

                    const PinType sourceType=
                        outputType(
                            source->type,
                            link.fromPin
                        );

                    if (!compatible(
                        sourceType,
                        expected))
                    {
                        return {
                            expected,
                            false,
                            0.0f,
                            0.0f,
                            "Incompatible pin type"
                        };
                    }

                    return evalOutput(
                        link.fromNode,
                        link.fromPin,
                        visiting
                    );
                }
            }

            return {
                expected,
                false,
                0.0f,
                0.0f,
                "Input not connected"
            };
        };

        evalOutput =
            [&](int nodeId,
                int outputPin,
                std::vector<int>& visiting)
                -> EvalValue
        {
            Node* node=
                findNode(nodeId);

            if (!node)
            {
                return {
                    PinType::Scalar,
                    false,
                    0.0f,
                    0.0f,
                    "Node missing"
                };
            }

            if (!node->enabled)
            {
                return {
                    outputType(
                        node->type,
                        outputPin
                    ),
                    false,
                    0.0f,
                    0.0f,
                    "Node disabled"
                };
            }

            if (std::find(
                visiting.begin(),
                visiting.end(),
                nodeId
            )!=visiting.end())
            {
                node->error=
                    "Cycle detected";

                return {
                    outputType(
                        node->type,
                        outputPin
                    ),
                    false,
                    0.0f,
                    0.0f,
                    "Cycle detected"
                };
            }

            visiting.push_back(nodeId);

            EvalValue result;

            switch (node->type)
            {
                case NodeType::EvidenceInput:
                {
                    result={
                        PinType::EvidenceDistance,
                        node->p0>=0.0f,
                        std::max(
                            0.0f,
                            node->p0
                        ),
                        0.0f,
                        node->p0>=0.0f
                            ? ""
                            : "Distance must be non-negative"
                    };
                    break;
                }

                case NodeType::VehicleInput:
                {
                    result={
                        PinType::VehicleState,
                        node->p0>0.0f,
                        node->p0,
                        node->p1,
                        node->p0>0.0f
                            ? ""
                            : "Mass must be positive"
                    };
                    break;
                }

                case NodeType::SkidAnalysis:
                {
                    EvalValue evidence=
                        resolveInput(
                            nodeId,
                            0,
                            PinType::EvidenceDistance,
                            visiting
                        );

                    if (!evidence.valid)
                    {
                        result={
                            PinType::Scalar,
                            false,
                            0.0f,
                            0.0f,
                            evidence.error
                        };
                        break;
                    }

                    if (node->p0<=0.0f)
                    {
                        result={
                            PinType::Scalar,
                            false,
                            0.0f,
                            0.0f,
                            "Friction must be positive"
                        };
                        break;
                    }

                    constexpr float g=
                        9.80665f;

                    const float speed=
                        std::sqrt(
                            std::max(
                                0.0f,
                                2.0f*
                                node->p0*
                                g*
                                evidence.a
                            )
                        );

                    node->result=speed;
                    node->resultValid=true;

                    result={
                        PinType::Scalar,
                        true,
                        speed,
                        0.0f,
                        ""
                    };
                    break;
                }

                case NodeType::SpeedAnalysis:
                {
                    EvalValue evidence=
                        resolveInput(
                            nodeId,
                            0,
                            PinType::EvidenceDistance,
                            visiting
                        );

                    if (!evidence.valid)
                    {
                        result={
                            PinType::Scalar,
                            false,
                            0.0f,
                            0.0f,
                            evidence.error
                        };
                        break;
                    }

                    constexpr float g=
                        9.80665f;

                    const float baseSpeed=
                        std::sqrt(
                            std::max(
                                0.0f,
                                2.0f*
                                g*
                                evidence.a
                            )
                        );

                    const float speed=
                        baseSpeed*
                        std::max(
                            0.0f,
                            node->p0
                        );

                    node->result=speed;
                    node->resultValid=true;

                    result={
                        PinType::Scalar,
                        true,
                        speed,
                        0.0f,
                        ""
                    };
                    break;
                }

                case NodeType::MomentumAnalysis:
                {
                    EvalValue a=
                        resolveInput(
                            nodeId,
                            0,
                            PinType::VehicleState,
                            visiting
                        );

                    EvalValue b=
                        resolveInput(
                            nodeId,
                            1,
                            PinType::VehicleState,
                            visiting
                        );

                    if (!a.valid || !b.valid)
                    {
                        result={
                            PinType::Scalar,
                            false,
                            0.0f,
                            0.0f,
                            !a.valid
                                ? a.error
                                : b.error
                        };
                        break;
                    }

                    const float momentum=
                        a.a*a.b+
                        b.a*b.b;

                    node->result=momentum;
                    node->resultValid=true;

                    result={
                        PinType::Scalar,
                        true,
                        momentum,
                        0.0f,
                        ""
                    };
                    break;
                }

                case NodeType::ResultOutput:
                {
                    result={
                        PinType::Scalar,
                        false,
                        0.0f,
                        0.0f,
                        "Result nodes have no output"
                    };
                    break;
                }
            }

            visiting.pop_back();

            if (!result.valid)
            {
                node->error=result.error;
            }

            return result;
        };

        for (Node& node : nodes)
        {
            if (node.type==
                NodeType::ResultOutput)
            {
                std::vector<int> visiting;

                EvalValue a=
                    resolveInput(
                        node.id,
                        0,
                        PinType::Scalar,
                        visiting
                    );

                visiting.clear();

                EvalValue b=
                    resolveInput(
                        node.id,
                        1,
                        PinType::Scalar,
                        visiting
                    );

                if (a.valid || b.valid)
                {
                    node.result=
                        a.valid
                            ? a.a
                            : 0.0f;

                    node.result2=
                        b.valid
                            ? b.a
                            : 0.0f;

                    node.resultValid=true;
                    node.error.clear();
                }
                else
                {
                    node.resultValid=false;
                    node.error=
                        "No valid result inputs";
                }

                continue;
            }

            if (outputCount(node.type)>0)
            {
                std::vector<int> visiting;

                EvalValue value=
                    evalOutput(
                        node.id,
                        0,
                        visiting
                    );

                if (node.type==
                        NodeType::SkidAnalysis ||
                    node.type==
                        NodeType::SpeedAnalysis ||
                    node.type==
                        NodeType::MomentumAnalysis)
                {
                    lastExecutedNodes++;

                    if (!value.valid)
                        lastExecutionErrors++;
                }
            }
        }

        if (lastExecutionErrors==0)
        {
            graphMessage=
                "Graph executed successfully";

            graphMessageError=false;
        }
        else
        {
            graphMessage=
                "Graph executed with errors";

            graphMessageError=true;
        }
    };

    // ========================================================
    // WINDOW
    // ========================================================

    ImGui::Begin(
        "Node Editor",
        &gEditorShell.showNodeEditor,
        ImGuiWindowFlags_NoMove
    );

    // ========================================================
    // SHORTCUTS
    // ========================================================

    const bool focused=
        ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows
        );

    if (focused)
    {
        const ImGuiIO& io=
            ImGui::GetIO();

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            doUndo();
        }

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_Y))
        {
            doRedo();
        }

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_C))
        {
            copySelection();
        }

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_V))
        {
            pasteSelection();
        }

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_D))
        {
            duplicateSelection();
        }

        if (ImGui::IsKeyPressed(
            ImGuiKey_Delete))
        {
            deleteSelection();
        }

        if (io.KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_Enter))
        {
            executeGraph();
        }
    }

    // ========================================================
    // TOOLBAR ROW 1 - GRAPH COMMANDS
    // ========================================================

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(8.0f,5.0f)
    );

    if (ImGui::Button(
        "FILE  v",
        ImVec2(74.0f,30.0f)))
    {
        ImGui::OpenPopup(
            "##NodeGraphFilePopup"
        );
    }

    if (ImGui::BeginPopup(
        "##NodeGraphFilePopup"))
    {
        ImGui::TextDisabled(
            "GRAPH FILE"
        );

        ImGui::Separator();

        ImGui::SetNextItemWidth(
            260.0f
        );

        ImGui::InputText(
            "##NodeGraphPath",
            graphPath,
            sizeof(graphPath)
        );

        ImGui::Spacing();

        if (ImGui::MenuItem(
            "Save Graph",
            "Ctrl+S"))
        {
            saveGraph();
        }

        if (ImGui::MenuItem(
            "Load Graph",
            "Ctrl+O"))
        {
            loadGraph();
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine(0.0f,6.0f);

    if (ImGui::Button(
        "ADD NODE  +",
        ImVec2(104.0f,30.0f)))
    {
        ImGui::OpenPopup(
            "##NodeAddPopup"
        );
    }

    if (ImGui::BeginPopup(
        "##NodeAddPopup"))
    {
        ImGui::TextDisabled(
            "ADD NODE"
        );

        ImGui::Separator();

        const ImVec2 base(
            220.0f-pan.x,
            120.0f-pan.y
        );

        if (ImGui::MenuItem(
            "Evidence Input"))
        {
            addNode(
                NodeType::EvidenceInput,
                base
            );
        }

        if (ImGui::MenuItem(
            "Vehicle State"))
        {
            addNode(
                NodeType::VehicleInput,
                base
            );
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Skid Analysis"))
        {
            addNode(
                NodeType::SkidAnalysis,
                base
            );
        }

        if (ImGui::MenuItem(
            "Speed Analysis"))
        {
            addNode(
                NodeType::SpeedAnalysis,
                base
            );
        }

        if (ImGui::MenuItem(
            "Momentum Analysis"))
        {
            addNode(
                NodeType::MomentumAnalysis,
                base
            );
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Result Output"))
        {
            addNode(
                NodeType::ResultOutput,
                base
            );
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine(0.0f,8.0f);

    if (editorButton(
        "RUN GRAPH",
        104.0f,
        true,
        true))
    {
        executeGraph();
    }

    ImGui::SameLine(0.0f,12.0f);

    ImGui::BeginDisabled(
        undoStack.empty()
    );

    if (ImGui::Button(
        "UNDO",
        ImVec2(62.0f,30.0f)))
    {
        doUndo();
    }

    ImGui::EndDisabled();

    ImGui::SameLine(0.0f,5.0f);

    ImGui::BeginDisabled(
        redoStack.empty()
    );

    if (ImGui::Button(
        "REDO",
        ImVec2(62.0f,30.0f)))
    {
        doRedo();
    }

    ImGui::EndDisabled();

    ImGui::SameLine(0.0f,12.0f);

    ImGui::BeginDisabled(
        selectedNodes.empty()
    );

    if (ImGui::Button(
        "COPY",
        ImVec2(62.0f,30.0f)))
    {
        copySelection();
    }

    ImGui::SameLine(0.0f,5.0f);

    if (ImGui::Button(
        "DUPLICATE",
        ImVec2(94.0f,30.0f)))
    {
        duplicateSelection();
    }

    ImGui::EndDisabled();

    ImGui::SameLine(0.0f,5.0f);

    if (ImGui::Button(
        "PASTE",
        ImVec2(66.0f,30.0f)))
    {
        pasteSelection();
    }

    ImGui::SameLine(0.0f,5.0f);

    ImGui::BeginDisabled(
        selectedNodes.empty() &&
        selectedLinks.empty()
    );

    if (ImGui::Button(
        "DELETE",
        ImVec2(72.0f,30.0f)))
    {
        deleteSelection();
    }

    ImGui::EndDisabled();

    // ========================================================
    // TOOLBAR ROW 2 - VIEW
    // ========================================================

    ImGui::Spacing();

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

    ImGui::SetNextItemWidth(
        118.0f
    );

    ImGui::SliderFloat(
        "##NodeGraphZoom",
        &zoom,
        0.70f,
        1.55f,
        "%.2fx"
    );

    ImGui::SameLine(0.0f,10.0f);

    if (ImGui::Button(
        "CENTER",
        ImVec2(72.0f,28.0f)))
    {
        pan=ImVec2(0.0f,0.0f);
        zoom=1.0f;
    }

    ImGui::SameLine(0.0f,14.0f);

    if (graphMessageError)
    {
        ImGui::TextColored(
            ImVec4(
                0.90f,
                0.58f,
                0.45f,
                1.0f
            ),
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

    ImGui::PopStyleVar();

    ImGui::Separator();

    // ========================================================
    // CANVAS + INSPECTOR SPLIT
    // ========================================================

    const float inspectorW=
        std::min(
            330.0f,
            std::max(
                260.0f,
                ImGui::GetContentRegionAvail().x*
                0.24f
            )
        );

    ImGui::BeginChild(
        "NodeGraphCanvasPane",
        ImVec2(
            -inspectorW-8.0f,
            0.0f
        ),
        false,
        ImGuiWindowFlags_NoScrollbar
    );

    const ImVec2 canvasPos=
        ImGui::GetCursorScreenPos();

    const ImVec2 canvasSize=
        ImGui::GetContentRegionAvail();

    ImGui::InvisibleButton(
        "##NodeGraphCanvas",
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

        float startX=
            canvasPos.x+
            std::fmod(
                pan.x*zoom,
                minor
            );

        float startY=
            canvasPos.y+
            std::fmod(
                pan.y*zoom,
                minor
            );

        for (
            float x=startX;
            x<canvasPos.x+canvasSize.x;
            x+=minor)
        {
            dl->AddLine(
                ImVec2(x,canvasPos.y),
                ImVec2(
                    x,
                    canvasPos.y+canvasSize.y
                ),
                IM_COL32(43,47,53,135),
                1.0f
            );
        }

        for (
            float y=startY;
            y<canvasPos.y+canvasSize.y;
            y+=minor)
        {
            dl->AddLine(
                ImVec2(canvasPos.x,y),
                ImVec2(
                    canvasPos.x+canvasSize.x,
                    y
                ),
                IM_COL32(43,47,53,135),
                1.0f
            );
        }

        startX=
            canvasPos.x+
            std::fmod(
                pan.x*zoom,
                major
            );

        startY=
            canvasPos.y+
            std::fmod(
                pan.y*zoom,
                major
            );

        for (
            float x=startX;
            x<canvasPos.x+canvasSize.x;
            x+=major)
        {
            dl->AddLine(
                ImVec2(x,canvasPos.y),
                ImVec2(
                    x,
                    canvasPos.y+canvasSize.y
                ),
                IM_COL32(59,64,72,155),
                1.0f
            );
        }

        for (
            float y=startY;
            y<canvasPos.y+canvasSize.y;
            y+=major)
        {
            dl->AddLine(
                ImVec2(canvasPos.x,y),
                ImVec2(
                    canvasPos.x+canvasSize.x,
                    y
                ),
                IM_COL32(59,64,72,155),
                1.0f
            );
        }
    }

    auto screenPos = [&](const Node& node)
    {
        return ImVec2(
            canvasPos.x+
            (node.pos.x+pan.x)*zoom,
            canvasPos.y+
            (node.pos.y+pan.y)*zoom
        );
    };

    auto inputPinPos =
        [&](const Node& node,int pin)
    {
        const ImVec2 p=
            screenPos(node);

        const ImVec2 size=
            nodeSize(node.type);

        const int count=
            inputCount(node.type);

        const float bodyTop=
            48.0f*zoom;

        const float bodyHeight=
            size.y*zoom-
            bodyTop-
            18.0f*zoom;

        const float spacing=
            bodyHeight/
            static_cast<float>(
                count+1
            );

        return ImVec2(
            p.x,
            p.y+
            bodyTop+
            spacing*
            static_cast<float>(pin+1)
        );
    };

    auto outputPinPos =
        [&](const Node& node,int pin)
    {
        const ImVec2 p=
            screenPos(node);

        const ImVec2 size=
            nodeSize(node.type);

        const int count=
            outputCount(node.type);

        const float bodyTop=
            48.0f*zoom;

        const float bodyHeight=
            size.y*zoom-
            bodyTop-
            18.0f*zoom;

        const float spacing=
            bodyHeight/
            static_cast<float>(
                count+1
            );

        return ImVec2(
            p.x+
            size.x*zoom,
            p.y+
            bodyTop+
            spacing*
            static_cast<float>(pin+1)
        );
    };

    const ImVec2 mouse=
        ImGui::GetIO().MousePos;

    int hoveredInputNode=-1;
    int hoveredInputPin=-1;
    int hoveredOutputNode=-1;
    int hoveredOutputPin=-1;

    const float pinHitRadius=
        11.0f;

    for (const Node& node : nodes)
    {
        for (
            int pin=0;
            pin<inputCount(node.type);
            ++pin)
        {
            const ImVec2 p=
                inputPinPos(node,pin);

            const float dx=
                mouse.x-p.x;

            const float dy=
                mouse.y-p.y;

            if (dx*dx+dy*dy <=
                pinHitRadius*
                pinHitRadius)
            {
                hoveredInputNode=node.id;
                hoveredInputPin=pin;
            }
        }

        for (
            int pin=0;
            pin<outputCount(node.type);
            ++pin)
        {
            const ImVec2 p=
                outputPinPos(node,pin);

            const float dx=
                mouse.x-p.x;

            const float dy=
                mouse.y-p.y;

            if (dx*dx+dy*dy <=
                pinHitRadius*
                pinHitRadius)
            {
                hoveredOutputNode=node.id;
                hoveredOutputPin=pin;
            }
        }
    }

    auto bezierPoint =
        [](float t,
           ImVec2 a,
           ImVec2 c1,
           ImVec2 c2,
           ImVec2 b)
    {
        const float u=
            1.0f-t;

        const float tt=
            t*t;

        const float uu=
            u*u;

        const float uuu=
            uu*u;

        const float ttt=
            tt*t;

        return ImVec2(
            uuu*a.x+
            3.0f*uu*t*c1.x+
            3.0f*u*tt*c2.x+
            ttt*b.x,

            uuu*a.y+
            3.0f*uu*t*c1.y+
            3.0f*u*tt*c2.y+
            ttt*b.y
        );
    };

    auto pointSegmentDistance =
        [](ImVec2 p,ImVec2 a,ImVec2 b)
    {
        const float vx=
            b.x-a.x;

        const float vy=
            b.y-a.y;

        const float wx=
            p.x-a.x;

        const float wy=
            p.y-a.y;

        const float vv=
            vx*vx+vy*vy;

        float t=
            vv>0.0001f
                ? (wx*vx+wy*vy)/vv
                : 0.0f;

        t=
            std::max(
                0.0f,
                std::min(
                    1.0f,
                    t
                )
            );

        const float px=
            a.x+t*vx;

        const float py=
            a.y+t*vy;

        const float dx=
            p.x-px;

        const float dy=
            p.y-py;

        return std::sqrt(
            dx*dx+dy*dy
        );
    };

    // --------------------------------------------------------
    // LINKS + LINK HIT TEST
    // --------------------------------------------------------

    int hoveredLink=-1;
    float hoveredLinkDistance=8.0f;

    for (const Link& link : links)
    {
        Node* from=
            findNode(link.fromNode);

        Node* to=
            findNode(link.toNode);

        if (!from || !to)
            continue;

        if (link.fromPin>=
                outputCount(from->type) ||
            link.toPin>=
                inputCount(to->type))
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
                std::fabs(
                    b.x-a.x
                )*0.45f
            );

        const ImVec2 c1(
            a.x+tangent,
            a.y
        );

        const ImVec2 c2(
            b.x-tangent,
            b.y
        );

        const PinType fromType=
            outputType(
                from->type,
                link.fromPin
            );

        const PinType toType=
            inputType(
                to->type,
                link.toPin
            );

        const bool validType=
            compatible(
                fromType,
                toType
            );

        const bool selected=
            isLinkSelected(
                link.id
            );

        dl->AddBezierCubic(
            a,
            c1,
            c2,
            b,
            !validType
                ? IM_COL32(205,95,88,235)
                : (selected
                    ? toU32(colorAccent())
                    : IM_COL32(120,145,178,225)),
            selected
                ? 3.4f
                : 2.2f
        );

        if (canvasHovered)
        {
            ImVec2 previous=a;

            for (int i=1;i<=24;++i)
            {
                const float t=
                    static_cast<float>(i)/
                    24.0f;

                const ImVec2 current=
                    bezierPoint(
                        t,
                        a,
                        c1,
                        c2,
                        b
                    );

                const float distance=
                    pointSegmentDistance(
                        mouse,
                        previous,
                        current
                    );

                if (distance<
                    hoveredLinkDistance)
                {
                    hoveredLinkDistance=
                        distance;

                    hoveredLink=
                        link.id;
                }

                previous=current;
            }
        }
    }

    // --------------------------------------------------------
    // NODES
    // --------------------------------------------------------

    int hoveredNode=-1;
    bool interactionHandled=false;

    for (Node& node : nodes)
    {
        const ImVec2 p=
            screenPos(node);

        const ImVec2 baseSize=
            nodeSize(node.type);

        const ImVec2 size(
            baseSize.x*zoom,
            baseSize.y*zoom
        );

        const ImVec2 max(
            p.x+size.x,
            p.y+size.y
        );

        const bool selected=
            isNodeSelected(
                node.id
            );

        const bool hovered=
            mouse.x>=p.x &&
            mouse.x<=max.x &&
            mouse.y>=p.y &&
            mouse.y<=max.y;

        if (hovered)
            hoveredNode=node.id;

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
            36.0f*zoom;

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
            node.title.c_str()
        );

        char bodyText[160]{};

        switch (node.type)
        {
            case NodeType::EvidenceInput:
                std::snprintf(
                    bodyText,
                    sizeof(bodyText),
                    "Distance %.2f m",
                    node.p0
                );
                break;

            case NodeType::VehicleInput:
                std::snprintf(
                    bodyText,
                    sizeof(bodyText),
                    "%.0f kg  |  %.2f m/s",
                    node.p0,
                    node.p1
                );
                break;

            case NodeType::SkidAnalysis:
                if (node.resultValid)
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "mu %.2f  |  %.2f m/s",
                        node.p0,
                        node.result
                    );
                }
                else
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "mu %.2f",
                        node.p0
                    );
                }
                break;

            case NodeType::SpeedAnalysis:
                if (node.resultValid)
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "Scale %.2f  |  %.2f m/s",
                        node.p0,
                        node.result
                    );
                }
                else
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "Scale %.2f",
                        node.p0
                    );
                }
                break;

            case NodeType::MomentumAnalysis:
                if (node.resultValid)
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "%.2f kg m/s",
                        node.result
                    );
                }
                else
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "Awaiting vehicle states"
                    );
                }
                break;

            case NodeType::ResultOutput:
                if (node.resultValid)
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "A %.2f   |   B %.2f",
                        node.result,
                        node.result2
                    );
                }
                else
                {
                    std::snprintf(
                        bodyText,
                        sizeof(bodyText),
                        "Awaiting analysis results"
                    );
                }
                break;
        }

        dl->AddText(
            ImVec2(
                p.x+12.0f*zoom,
                p.y+48.0f*zoom
            ),
            toU32(colorMuted()),
            bodyText
        );

        if (!node.error.empty())
        {
            dl->AddText(
                ImVec2(
                    p.x+12.0f*zoom,
                    max.y-26.0f*zoom
                ),
                IM_COL32(222,125,112,230),
                node.error.c_str()
            );
        }
        else
        {
            dl->AddText(
                ImVec2(
                    p.x+12.0f*zoom,
                    max.y-26.0f*zoom
                ),
                node.enabled
                    ? IM_COL32(150,158,169,220)
                    : IM_COL32(105,110,118,190),
                node.enabled
                    ? "Enabled"
                    : "Disabled"
            );
        }

        for (
            int pin=0;
            pin<inputCount(node.type);
            ++pin)
        {
            const ImVec2 pinPos=
                inputPinPos(node,pin);

            const bool pinHovered=
                hoveredInputNode==node.id &&
                hoveredInputPin==pin;

            dl->AddCircleFilled(
                pinPos,
                pinHovered
                    ? 7.0f
                    : 5.5f,
                IM_COL32(102,137,178,255)
            );

            dl->AddCircle(
                pinPos,
                8.0f,
                IM_COL32(177,197,220,225),
                20,
                1.2f
            );

            dl->AddText(
                ImVec2(
                    pinPos.x+12.0f,
                    pinPos.y-8.0f
                ),
                IM_COL32(158,164,174,225),
                inputLabel(
                    node.type,
                    pin
                )
            );
        }

        for (
            int pin=0;
            pin<outputCount(node.type);
            ++pin)
        {
            const ImVec2 pinPos=
                outputPinPos(node,pin);

            const bool pinHovered=
                hoveredOutputNode==node.id &&
                hoveredOutputPin==pin;

            dl->AddCircleFilled(
                pinPos,
                pinHovered
                    ? 7.0f
                    : 5.5f,
                IM_COL32(168,177,190,255)
            );

            dl->AddCircle(
                pinPos,
                8.0f,
                IM_COL32(213,218,227,230),
                20,
                1.2f
            );

            const char* label=
                outputLabel(
                    node.type,
                    pin
                );

            const float labelW=
                ImGui::CalcTextSize(
                    label
                ).x;

            dl->AddText(
                ImVec2(
                    pinPos.x-
                    labelW-
                    12.0f,
                    pinPos.y-8.0f
                ),
                IM_COL32(175,181,191,225),
                label
            );
        }
    }

    // --------------------------------------------------------
    // PIN TOOLTIPS
    // --------------------------------------------------------

    if (canvasHovered &&
        hoveredInputNode!=-1)
    {
        Node* node=
            findNode(
                hoveredInputNode
            );

        if (node)
        {
            ImGui::BeginTooltip();

            ImGui::Text(
                "%s input",
                inputLabel(
                    node->type,
                    hoveredInputPin
                )
            );

            ImGui::TextDisabled(
                "%s",
                pinTypeLabel(
                    inputType(
                        node->type,
                        hoveredInputPin
                    )
                )
            );

            ImGui::EndTooltip();
        }
    }
    else if (
        canvasHovered &&
        hoveredOutputNode!=-1)
    {
        Node* node=
            findNode(
                hoveredOutputNode
            );

        if (node)
        {
            ImGui::BeginTooltip();

            ImGui::Text(
                "%s output",
                outputLabel(
                    node->type,
                    hoveredOutputPin
                )
            );

            ImGui::TextDisabled(
                "%s",
                pinTypeLabel(
                    outputType(
                        node->type,
                        hoveredOutputPin
                    )
                )
            );

            ImGui::EndTooltip();
        }
    }

    // --------------------------------------------------------
    // LINK DRAG PREVIEW
    // --------------------------------------------------------

    if (linkDragActive)
    {
        Node* source=
            findNode(
                linkDragNode
            );

        if (source)
        {
            const ImVec2 a=
                outputPinPos(
                    *source,
                    linkDragPin
                );

            const ImVec2 b=
                mouse;

            const float tangent=
                std::max(
                    70.0f,
                    std::fabs(
                        b.x-a.x
                    )*0.45f
                );

            bool targetCompatible=false;

            if (hoveredInputNode!=-1)
            {
                Node* target=
                    findNode(
                        hoveredInputNode
                    );

                if (target)
                {
                    targetCompatible=
                        compatible(
                            linkDragType,
                            inputType(
                                target->type,
                                hoveredInputPin
                            )
                        );
                }
            }

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
                hoveredInputNode==-1
                    ? IM_COL32(170,176,186,210)
                    : (targetCompatible
                        ? IM_COL32(110,190,135,245)
                        : IM_COL32(215,92,82,245)),
                2.6f
            );
        }
    }

    // ========================================================
    // INTERACTION
    // ========================================================

    const bool leftClicked=
        canvasHovered &&
        ImGui::IsMouseClicked(
            ImGuiMouseButton_Left
        );

    if (leftClicked &&
        hoveredOutputNode!=-1)
    {
        Node* source=
            findNode(
                hoveredOutputNode
            );

        if (source)
        {
            linkDragActive=true;
            linkDragNode=
                hoveredOutputNode;

            linkDragPin=
                hoveredOutputPin;

            linkDragType=
                outputType(
                    source->type,
                    hoveredOutputPin
                );

            interactionHandled=true;
        }
    }

    if (leftClicked &&
        !interactionHandled)
    {
        // Node selection / drag.
        for (
            auto it=nodes.rbegin();
            it!=nodes.rend();
            ++it)
        {
            Node& node=*it;

            const ImVec2 p=
                screenPos(node);

            const ImVec2 size=
                nodeSize(node.type);

            const ImVec2 scaled(
                size.x*zoom,
                size.y*zoom
            );

            const float headerH=
                36.0f*zoom;

            const bool headerHit=
                mouse.x>=p.x &&
                mouse.x<=p.x+scaled.x &&
                mouse.y>=p.y &&
                mouse.y<=p.y+headerH;

            const bool bodyHit=
                mouse.x>=p.x &&
                mouse.x<=p.x+scaled.x &&
                mouse.y>=p.y &&
                mouse.y<=p.y+scaled.y;

            if (!bodyHit)
                continue;

            if (ImGui::GetIO().KeyCtrl)
            {
                if (isNodeSelected(node.id))
                {
                    selectedNodes.erase(
                        std::remove(
                            selectedNodes.begin(),
                            selectedNodes.end(),
                            node.id
                        ),
                        selectedNodes.end()
                    );
                }
                else
                {
                    selectedNodes.push_back(
                        node.id
                    );
                }
            }
            else if (!isNodeSelected(node.id))
            {
                clearSelection();

                selectedNodes.push_back(
                    node.id
                );
            }

            if (headerHit &&
                isNodeSelected(node.id))
            {
                pushUndo();

                nodeDragging=true;

                dragMouseStart=mouse;

                dragStartPositions.clear();

                for (int id : selectedNodes)
                {
                    Node* selected=
                        findNode(id);

                    if (selected)
                    {
                        dragStartPositions.push_back(
                            {
                                id,
                                selected->pos
                            }
                        );
                    }
                }
            }

            interactionHandled=true;
            break;
        }
    }

    if (leftClicked &&
        !interactionHandled &&
        hoveredLink!=-1)
    {
        if (!ImGui::GetIO().KeyCtrl)
        {
            clearSelection();
        }

        if (!isLinkSelected(
            hoveredLink))
        {
            selectedLinks.push_back(
                hoveredLink
            );
        }

        interactionHandled=true;
    }

    if (leftClicked &&
        !interactionHandled)
    {
        if (!ImGui::GetIO().KeyCtrl)
        {
            clearSelection();
        }

        boxSelecting=true;
        boxStart=mouse;
        boxEnd=mouse;
    }

    if (nodeDragging)
    {
        if (ImGui::IsMouseDown(
            ImGuiMouseButton_Left))
        {
            const ImVec2 delta(
                (mouse.x-dragMouseStart.x)/
                    zoom,
                (mouse.y-dragMouseStart.y)/
                    zoom
            );

            for (
                const auto& start :
                dragStartPositions)
            {
                Node* node=
                    findNode(
                        start.first
                    );

                if (!node)
                    continue;

                ImVec2 target(
                    start.second.x+
                        delta.x,
                    start.second.y+
                        delta.y
                );

                if (snapToGrid)
                {
                    constexpr float snap=
                        24.0f;

                    target.x=
                        std::round(
                            target.x/snap
                        )*snap;

                    target.y=
                        std::round(
                            target.y/snap
                        )*snap;
                }

                node->pos=target;
            }
        }

        if (ImGui::IsMouseReleased(
            ImGuiMouseButton_Left))
        {
            nodeDragging=false;
            dragStartPositions.clear();
        }
    }

    if (boxSelecting)
    {
        if (ImGui::IsMouseDown(
            ImGuiMouseButton_Left))
        {
            boxEnd=mouse;
        }

        const ImVec2 min(
            std::min(
                boxStart.x,
                boxEnd.x
            ),
            std::min(
                boxStart.y,
                boxEnd.y
            )
        );

        const ImVec2 max(
            std::max(
                boxStart.x,
                boxEnd.x
            ),
            std::max(
                boxStart.y,
                boxEnd.y
            )
        );

        dl->AddRectFilled(
            min,
            max,
            IM_COL32(90,130,180,42)
        );

        dl->AddRect(
            min,
            max,
            IM_COL32(125,162,208,200),
            0.0f,
            0,
            1.2f
        );

        if (ImGui::IsMouseReleased(
            ImGuiMouseButton_Left))
        {
            for (const Node& node : nodes)
            {
                const ImVec2 p=
                    screenPos(node);

                const ImVec2 base=
                    nodeSize(node.type);

                const ImVec2 nmax(
                    p.x+
                    base.x*zoom,
                    p.y+
                    base.y*zoom
                );

                const bool intersects=
                    nmax.x>=min.x &&
                    p.x<=max.x &&
                    nmax.y>=min.y &&
                    p.y<=max.y;

                if (intersects &&
                    !isNodeSelected(
                        node.id))
                {
                    selectedNodes.push_back(
                        node.id
                    );
                }
            }

            boxSelecting=false;
        }
    }

    if (linkDragActive &&
        ImGui::IsMouseReleased(
            ImGuiMouseButton_Left))
    {
        bool connected=false;

        Node* source=
            findNode(
                linkDragNode
            );

        Node* target=
            findNode(
                hoveredInputNode
            );

        if (source &&
            target &&
            source->id!=target->id &&
            hoveredInputPin>=0)
        {
            const PinType targetType=
                inputType(
                    target->type,
                    hoveredInputPin
                );

            if (compatible(
                linkDragType,
                targetType))
            {
                pushUndo();

                // One incoming link per input pin.
                links.erase(
                    std::remove_if(
                        links.begin(),
                        links.end(),
                        [&](const Link& link)
                        {
                            return
                                link.toNode==
                                    target->id &&
                                link.toPin==
                                    hoveredInputPin;
                        }
                    ),
                    links.end()
                );

                links.push_back({
                    nextLinkId++,
                    source->id,
                    linkDragPin,
                    target->id,
                    hoveredInputPin
                });

                graphMessage=
                    "Link created";

                graphMessageError=false;
                connected=true;
            }
            else
            {
                graphMessage=
                    std::string(
                        "Cannot connect "
                    )+
                    pinTypeLabel(
                        linkDragType
                    )+
                    " to "+
                    pinTypeLabel(
                        targetType
                    );

                graphMessageError=true;
            }
        }

        if (!connected &&
            hoveredInputNode==-1)
        {
            graphMessage=
                "Link cancelled";

            graphMessageError=false;
        }

        linkDragActive=false;
        linkDragNode=-1;
        linkDragPin=-1;
    }

    // --------------------------------------------------------
    // PAN + MOUSE-WHEEL ZOOM
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

    if (canvasHovered &&
        std::fabs(
            ImGui::GetIO().MouseWheel
        )>0.001f)
    {
        const float oldZoom=
            zoom;

        zoom=
            std::max(
                0.70f,
                std::min(
                    1.55f,
                    zoom+
                    ImGui::GetIO().MouseWheel*
                    0.08f
                )
            );

        if (std::fabs(
            zoom-oldZoom)>0.0001f)
        {
            const float worldX=
                (mouse.x-canvasPos.x)/
                    oldZoom-
                pan.x;

            const float worldY=
                (mouse.y-canvasPos.y)/
                    oldZoom-
                pan.y;

            pan.x=
                (mouse.x-canvasPos.x)/
                    zoom-
                worldX;

            pan.y=
                (mouse.y-canvasPos.y)/
                    zoom-
                worldY;
        }
    }

    // --------------------------------------------------------
    // CANVAS CONTEXT MENU
    // --------------------------------------------------------

    if (ImGui::BeginPopupContextItem(
        "##NodeGraphContext"))
    {
        ImGui::TextDisabled(
            "NODE GRAPH"
        );

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Run Graph",
            "Ctrl+Enter"))
        {
            executeGraph();
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Copy",
            "Ctrl+C",
            false,
            !selectedNodes.empty()))
        {
            copySelection();
        }

        if (ImGui::MenuItem(
            "Paste",
            "Ctrl+V"))
        {
            pasteSelection();
        }

        if (ImGui::MenuItem(
            "Duplicate",
            "Ctrl+D",
            false,
            !selectedNodes.empty()))
        {
            duplicateSelection();
        }

        if (ImGui::MenuItem(
            "Delete",
            "Del",
            false,
            !selectedNodes.empty() ||
            !selectedLinks.empty()))
        {
            deleteSelection();
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Center View"))
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

    ImGui::EndChild();

    // ========================================================
    // NODE INSPECTOR
    // ========================================================

    ImGui::SameLine(0.0f,8.0f);

    ImGui::BeginChild(
        "NodeGraphInspector",
        ImVec2(0.0f,0.0f),
        true
    );

    ImGui::Text("NODE PROPERTIES");
    ImGui::Separator();

    if (selectedNodes.size()==1)
    {
        Node* node=
            findNode(
                selectedNodes.front()
            );

        if (node)
        {
            ImGui::TextDisabled(
                "ID %d",
                node->id
            );

            char titleBuffer[128]{};

            std::snprintf(
                titleBuffer,
                sizeof(titleBuffer),
                "%s",
                node->title.c_str()
            );

            if (ImGui::IsItemActivated())
            {
                propertyEditBefore=
                    serializeGraph();

                propertyEditChanged=false;
            }

            const bool titleChanged=
                ImGui::InputText(
                    "Name",
                    titleBuffer,
                    sizeof(titleBuffer)
                );

            if (ImGui::IsItemActivated())
            {
                propertyEditBefore=
                    serializeGraph();

                propertyEditChanged=false;
            }

            if (titleChanged)
            {
                node->title=titleBuffer;
                propertyEditChanged=true;
            }

            if (ImGui::IsItemDeactivatedAfterEdit() &&
                propertyEditChanged)
            {
                pushSnapshot(
                    propertyEditBefore
                );

                propertyEditChanged=false;
            }

            const bool enabledBefore=
                node->enabled;

            const std::string beforeEnabled=
                serializeGraph();

            if (ImGui::Checkbox(
                "Enabled",
                &node->enabled))
            {
                pushSnapshot(
                    beforeEnabled
                );
            }

            ImGui::Spacing();
            ImGui::Separator();

            auto propertyFloat =
                [&](const char* label,
                    float* value,
                    float speed,
                    float minValue,
                    float maxValue,
                    const char* format)
            {
                const std::string before=
                    serializeGraph();

                ImGui::SetNextItemWidth(
                    -1.0f
                );

                const bool changed=
                    ImGui::DragFloat(
                        label,
                        value,
                        speed,
                        minValue,
                        maxValue,
                        format
                    );

                if (ImGui::IsItemActivated())
                {
                    propertyEditBefore=
                        before;

                    propertyEditChanged=false;
                }

                if (changed)
                {
                    propertyEditChanged=true;
                }

                if (ImGui::IsItemDeactivatedAfterEdit() &&
                    propertyEditChanged)
                {
                    pushSnapshot(
                        propertyEditBefore
                    );

                    propertyEditChanged=false;
                }
            };

            switch (node->type)
            {
                case NodeType::EvidenceInput:
                    ImGui::TextDisabled(
                        "EVIDENCE INPUT"
                    );

                    propertyFloat(
                        "Distance (m)",
                        &node->p0,
                        0.10f,
                        0.0f,
                        10000.0f,
                        "%.2f"
                    );
                    break;

                case NodeType::VehicleInput:
                    ImGui::TextDisabled(
                        "VEHICLE STATE"
                    );

                    propertyFloat(
                        "Mass (kg)",
                        &node->p0,
                        5.0f,
                        1.0f,
                        100000.0f,
                        "%.1f"
                    );

                    propertyFloat(
                        "Velocity (m/s)",
                        &node->p1,
                        0.10f,
                        -200.0f,
                        200.0f,
                        "%.2f"
                    );
                    break;

                case NodeType::SkidAnalysis:
                    ImGui::TextDisabled(
                        "SKID ANALYSIS"
                    );

                    propertyFloat(
                        "Friction coefficient",
                        &node->p0,
                        0.01f,
                        0.01f,
                        2.0f,
                        "%.2f"
                    );
                    break;

                case NodeType::SpeedAnalysis:
                    ImGui::TextDisabled(
                        "SPEED ANALYSIS"
                    );

                    propertyFloat(
                        "Speed multiplier",
                        &node->p0,
                        0.01f,
                        0.0f,
                        10.0f,
                        "%.2f"
                    );
                    break;

                case NodeType::MomentumAnalysis:
                    ImGui::TextDisabled(
                        "MOMENTUM ANALYSIS"
                    );

                    ImGui::TextWrapped(
                        "Consumes two Vehicle State inputs and computes combined linear momentum."
                    );
                    break;

                case NodeType::ResultOutput:
                    ImGui::TextDisabled(
                        "RESULT OUTPUT"
                    );

                    ImGui::TextWrapped(
                        "Collects scalar results from upstream analysis nodes."
                    );
                    break;
            }

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Text("PINS");

            for (
                int pin=0;
                pin<inputCount(node->type);
                ++pin)
            {
                ImGui::TextDisabled(
                    "IN  %s",
                    inputLabel(
                        node->type,
                        pin
                    )
                );

                ImGui::SameLine();

                ImGui::Text(
                    "%s",
                    pinTypeLabel(
                        inputType(
                            node->type,
                            pin
                        )
                    )
                );
            }

            for (
                int pin=0;
                pin<outputCount(node->type);
                ++pin)
            {
                ImGui::TextDisabled(
                    "OUT %s",
                    outputLabel(
                        node->type,
                        pin
                    )
                );

                ImGui::SameLine();

                ImGui::Text(
                    "%s",
                    pinTypeLabel(
                        outputType(
                            node->type,
                            pin
                        )
                    )
                );
            }

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Text("EXECUTION");

            if (node->resultValid)
            {
                if (node->type==
                    NodeType::ResultOutput)
                {
                    ImGui::Text(
                        "Result A: %.3f",
                        node->result
                    );

                    ImGui::Text(
                        "Result B: %.3f",
                        node->result2
                    );
                }
                else
                {
                    ImGui::Text(
                        "Result: %.3f",
                        node->result
                    );
                }
            }
            else if (!node->error.empty())
            {
                ImGui::TextColored(
                    ImVec4(
                        0.90f,
                        0.58f,
                        0.45f,
                        1.0f
                    ),
                    "%s",
                    node->error.c_str()
                );
            }
            else
            {
                ImGui::TextDisabled(
                    "Not executed"
                );
            }
        }
    }
    else if (selectedNodes.size()>1)
    {
        ImGui::Text(
            "%d nodes selected",
            static_cast<int>(
                selectedNodes.size()
            )
        );

        ImGui::TextDisabled(
            "Ctrl-click or box-select supports multi-selection."
        );

        ImGui::Spacing();

        editorButton(
            "RUN GRAPH",
            ImGui::GetContentRegionAvail().x,
            true,
            true
        );
    }
    else if (!selectedLinks.empty())
    {
        ImGui::Text(
            "%d link(s) selected",
            static_cast<int>(
                selectedLinks.size()
            )
        );

        ImGui::Spacing();

        if (editorButton(
            "DELETE LINK",
            ImGui::GetContentRegionAvail().x,
            false,
            true))
        {
            deleteSelection();
        }
    }
    else
    {
        ImGui::TextDisabled(
            "Nothing selected"
        );

        ImGui::Spacing();

        ImGui::TextWrapped(
            "Select a node to edit its properties, or select a link and press Delete."
        );

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Text("GRAPH EXECUTION");

        ImGui::TextDisabled(
            "Executed nodes"
        );

        ImGui::SameLine();

        ImGui::Text(
            "%d",
            lastExecutedNodes
        );

        ImGui::TextDisabled(
            "Errors"
        );

        ImGui::SameLine();

        ImGui::Text(
            "%d",
            lastExecutionErrors
        );
    }

    ImGui::EndChild();

    ImGui::End();
}
'@

# ============================================================
# WRITE BACK
# ============================================================

$text = $text.Replace(
    $oldNodeEditor,
    $newNodeEditor
)

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

# ============================================================
# VERIFY
# ============================================================

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'SAR_NODE_GRAPH_V2',
    'SAR_NODE_CLIP_V1',
    'linkDragActive',
    'Cycle detected',
    'Friction coefficient',
    'Momentum Analysis',
    'Save Graph',
    'Load Graph',
    'doUndo',
    'doRedo',
    'duplicateSelection',
    'boxSelecting',
    'selectedLinks',
    'Cannot connect',
    'RUN GRAPH',
    'Ctrl-click or box-select'
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] Full Sovereign node graph engine installed." -ForegroundColor Cyan
Write-Host "[OK] 1. Pin-to-pin link creation." -ForegroundColor Green
Write-Host "[OK] 2. Link selection/deletion." -ForegroundColor Green
Write-Host "[OK] 3. Typed pin compatibility." -ForegroundColor Green
Write-Host "[OK] 4. Executable graph with cycle/input validation." -ForegroundColor Green
Write-Host "[OK] 5. Skid / Speed / Momentum calculations." -ForegroundColor Green
Write-Host "[OK] 6. Editable node inspector." -ForegroundColor Green
Write-Host "[OK] 7. Save/load .sargraph format." -ForegroundColor Green
Write-Host "[OK] 8. Undo/redo history." -ForegroundColor Green
Write-Host "[OK] 9. Copy/paste/duplicate." -ForegroundColor Green
Write-Host "[OK] 10. Ctrl multi-select." -ForegroundColor Green
Write-Host "[OK] 11. Box selection." -ForegroundColor Green
Write-Host ""
Write-Host "Default graph file:" -ForegroundColor Yellow
Write-Host "  node_graph.sargraph"
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
