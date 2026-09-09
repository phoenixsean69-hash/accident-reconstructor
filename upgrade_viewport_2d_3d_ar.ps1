param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - VIEWPORT 2D / 3D / AR PASS" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if (-not $text.Contains('static void drawViewportView()')) {
    throw "Expected marker not found: static void drawViewportView()"
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-viewport-2d-3d-ar-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

function Replace-FunctionBlock {
    param(
        [string]$Source,
        [string]$Signature,
        [string]$Replacement
    )

    $start = $Source.IndexOf($Signature)
    if ($start -lt 0) {
        throw "Could not find function signature: $Signature"
    }

    $braceStart = $Source.IndexOf('{', $start)
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
            if ($ch -eq '"') { $inString = $false; continue }
            continue
        }

        if ($inChar) {
            if ($ch -eq '\') { $escape = $true; continue }
            if ($ch -eq "'") { $inChar = $false; continue }
            continue
        }

        if ($ch -eq '"') { $inString = $true; continue }
        if ($ch -eq "'") { $inChar = $true; continue }

        if ($ch -eq '{') {
            $depth++
            continue
        }

        if ($ch -eq '}') {
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

    $oldBlock = $Source.Substring($start, ($end - $start + 1))
    return $Source.Replace($oldBlock, $Replacement)
}

$replacement = @'
static void drawViewportView()
{
    static int viewportMode=0;       // 0 = 2D, 1 = 3D, 2 = AR
    static int activeTool=0;         // 0 = Select, 1 = Move, 2 = Rotate, 3 = Scale, 4 = Vehicle, 5 = Evidence, 6 = Measure
    static int orthoView=0;          // 0 = Top, 1 = Front, 2 = Right
    static int renderStyle=0;        // 0 = Lit, 1 = Wireframe, 2 = Analysis
    static bool showGrid=true;
    static bool showAxes=true;
    static bool showBounds=false;
    static bool showMeasurements=true;
    static bool showPath=false;
    static bool arAnchors=true;
    static bool arCollisionHints=true;
    static float zoom2D=1.00f;
    static float orbitYaw=32.0f;
    static float orbitPitch=18.0f;
    static float arOpacity=0.78f;

    auto modeButton = [&](const char* label,int mode,float width)
    {
        const bool selected=(viewportMode==mode);

        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button,colorAccentMuted());
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,colorAccentMuted());
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,colorAccentMuted());
            ImGui::PushStyleColor(ImGuiCol_Text,colorAccent());
        }

        const bool pressed=ImGui::Button(label,ImVec2(width,30.0f));

        if (selected)
            ImGui::PopStyleColor(4);

        return pressed;
    };

    auto toolButton = [&](const char* id,const char* label,int toolId)
    {
        const bool selected=(activeTool==toolId);

        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button,colorAccentMuted());
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,colorAccentMuted());
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,colorAccentMuted());
            ImGui::PushStyleColor(ImGuiCol_Text,colorAccent());
        }

        const bool pressed=ImGui::Button(label,ImVec2(-1.0f,46.0f));

        if (selected)
            ImGui::PopStyleColor(4);

        if (pressed)
            activeTool=toolId;
    };

    auto footerCard = [&](const char* id,const char* title,const char* value,const char* note,float width)
    {
        ImGui::PushID(id);
        ImGui::BeginChild("##FooterCard",ImVec2(width,56.0f),true,ImGuiWindowFlags_NoScrollbar);

        ImGui::TextDisabled("%s",title);
        ImGui::Text("%s",value);
        ImGui::TextDisabled("%s",note);

        ImGui::EndChild();
        ImGui::PopID();
    };

    ImGui::Begin("Viewport");

    // ======================================================
    // HEADER
    // ======================================================

    ImGui::Text("SCENE VIEWPORT");
    ImGui::SameLine(0.0f,10.0f);
    ImGui::TextDisabled("Switch between 2D planning, 3D scene review and AR preview.");

    const float modeButtonW=92.0f;
    const float rightWidth=(modeButtonW*3.0f)+16.0f;
    const float targetX=ImGui::GetWindowContentRegionMax().x-rightWidth;

    if (targetX > ImGui::GetCursorPosX()+18.0f)
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(targetX);

        if (modeButton("2D PLAN",0,modeButtonW)) viewportMode=0;
        ImGui::SameLine(0.0f,6.0f);
        if (modeButton("3D SCENE",1,modeButtonW)) viewportMode=1;
        ImGui::SameLine(0.0f,6.0f);
        if (modeButton("AR PREVIEW",2,modeButtonW)) viewportMode=2;
    }

    ImGui::Separator();

    // ======================================================
    // MAIN SPLIT: RAIL + VIEWPORT AREA
    // ======================================================

    const float railW=118.0f;

    ImGui::BeginChild(
        "ViewportToolRail",
        ImVec2(railW,0.0f),
        true,
        ImGuiWindowFlags_NoScrollbar
    );

    ImGui::TextDisabled("TOOLS");
    toolButton("ToolSelect","SELECT",0);
    toolButton("ToolMove","MOVE",1);
    toolButton("ToolRotate","ROTATE",2);
    toolButton("ToolScale","SCALE",3);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("CREATE");

    toolButton("ToolVehicle","VEHICLE",4);
    toolButton("ToolEvidence","EVIDENCE",5);
    toolButton("ToolMeasure","MEASURE",6);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("OVERLAYS");

    ImGui::Checkbox("Grid",&showGrid);
    ImGui::Checkbox("Axes",&showAxes);
    ImGui::Checkbox("Bounds",&showBounds);
    ImGui::Checkbox("Measure",&showMeasurements);

    if (viewportMode==2)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("AR");
        ImGui::Checkbox("Anchors",&arAnchors);
        ImGui::Checkbox("Hints",&arCollisionHints);
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild(
        "ViewportCanvasArea",
        ImVec2(0.0f,0.0f),
        false,
        ImGuiWindowFlags_NoScrollbar
    );

    // ======================================================
    // MODE-SPECIFIC TOOLBAR
    // ======================================================

    ImGui::BeginChild(
        "ViewportTopToolbar",
        ImVec2(0.0f,56.0f),
        true,
        ImGuiWindowFlags_NoScrollbar
    );

    if (viewportMode==0)
    {
        const char* orthoModes[]={"Top","Front","Right"};

        ImGui::TextDisabled("2D VIEW");
        ImGui::SameLine(0.0f,10.0f);

        ImGui::SetNextItemWidth(120.0f);
        ImGui::Combo("##OrthoMode",&orthoView,orthoModes,3);

        ImGui::SameLine(0.0f,12.0f);
        ImGui::SetNextItemWidth(140.0f);
        ImGui::SliderFloat("##Zoom2D",&zoom2D,0.50f,2.50f,"Zoom %.2fx");

        ImGui::SameLine(0.0f,12.0f);
        ImGui::Checkbox("Path",&showPath);

        ImGui::SameLine(0.0f,12.0f);
        editorButton("FRAME SCENE",110.0f,false,true);
    }
    else if (viewportMode==1)
    {
        const char* renderModes[]={"Lit","Wireframe","Analysis"};

        ImGui::TextDisabled("3D VIEW");
        ImGui::SameLine(0.0f,10.0f);

        ImGui::SetNextItemWidth(140.0f);
        ImGui::Combo("##RenderStyle",&renderStyle,renderModes,3);

        ImGui::SameLine(0.0f,12.0f);
        ImGui::SetNextItemWidth(130.0f);
        ImGui::SliderFloat("##OrbitYaw",&orbitYaw,-180.0f,180.0f,"Yaw %.0f");

        ImGui::SameLine(0.0f,12.0f);
        ImGui::SetNextItemWidth(130.0f);
        ImGui::SliderFloat("##OrbitPitch",&orbitPitch,-80.0f,80.0f,"Pitch %.0f");

        ImGui::SameLine(0.0f,12.0f);
        editorButton("RESET CAMERA",116.0f,false,true);
    }
    else
    {
        ImGui::TextDisabled("AR PREVIEW");
        ImGui::SameLine(0.0f,10.0f);
        drawStatus("SIMULATED PREVIEW",StatusTone::Accent);

        ImGui::SameLine(0.0f,18.0f);
        ImGui::SetNextItemWidth(150.0f);
        ImGui::SliderFloat("##AROpacity",&arOpacity,0.20f,1.00f,"Overlay %.2f");

        ImGui::SameLine(0.0f,12.0f);
        editorButton("PAIR DEVICE",102.0f,false,false);

        ImGui::SameLine(0.0f,8.0f);
        editorButton("START SESSION",114.0f,true,false);
    }

    ImGui::EndChild();

    ImGui::Spacing();

    // ======================================================
    // RENDER SURFACE
    // ======================================================

    ImGui::BeginChild(
        "ViewportRenderSurface",
        ImVec2(0.0f,-72.0f),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
    );

    const ImVec2 p=ImGui::GetCursorScreenPos();
    const ImVec2 s=ImGui::GetContentRegionAvail();
    ImDrawList* dl=ImGui::GetWindowDrawList();

    dl->AddRectFilled(
        p,
        ImVec2(p.x+s.x,p.y+s.y),
        toU32(colorPanel()),
        0.0f
    );

    // subtle inner frame
    dl->AddRect(
        ImVec2(p.x+1.0f,p.y+1.0f),
        ImVec2(p.x+s.x-1.0f,p.y+s.y-1.0f),
        toU32(colorBorder()),
        0.0f,
        0,
        1.0f
    );

    const ImVec2 center(p.x+s.x*0.5f,p.y+s.y*0.5f);

    if (viewportMode==0)
    {
        // -----------------------------------------------
        // 2D GRID
        // -----------------------------------------------
        const float step=32.0f*zoom2D;

        if (showGrid)
        {
            for (float x=center.x; x<p.x+s.x; x+=step)
                dl->AddLine(ImVec2(x,p.y),ImVec2(x,p.y+s.y),toU32(ImVec4(1,1,1,0.07f)),1.0f);

            for (float x=center.x; x>p.x; x-=step)
                dl->AddLine(ImVec2(x,p.y),ImVec2(x,p.y+s.y),toU32(ImVec4(1,1,1,0.07f)),1.0f);

            for (float y=center.y; y<p.y+s.y; y+=step)
                dl->AddLine(ImVec2(p.x,y),ImVec2(p.x+s.x,y),toU32(ImVec4(1,1,1,0.07f)),1.0f);

            for (float y=center.y; y>p.y; y-=step)
                dl->AddLine(ImVec2(p.x,y),ImVec2(p.x+s.x,y),toU32(ImVec4(1,1,1,0.07f)),1.0f);
        }

        if (showAxes)
        {
            dl->AddLine(ImVec2(p.x+28.0f,center.y),ImVec2(p.x+s.x-28.0f,center.y),IM_COL32(190,72,56,255),2.0f);
            dl->AddLine(ImVec2(center.x,p.y+28.0f),ImVec2(center.x,p.y+s.y-28.0f),IM_COL32(70,170,95,255),2.0f);
            dl->AddText(ImVec2(p.x+s.x-42.0f,center.y+6.0f),IM_COL32(220,110,90,255),"X");
            dl->AddText(ImVec2(center.x+8.0f,p.y+8.0f),IM_COL32(112,220,128,255),"Y");
        }

        // road corridor
        const float roadW=120.0f*zoom2D;
        dl->AddRectFilled(
            ImVec2(center.x-roadW*0.5f,p.y+34.0f),
            ImVec2(center.x+roadW*0.5f,p.y+s.y-34.0f),
            IM_COL32(72,78,86,48),
            0.0f
        );

        // vehicle A/B
        ImVec2 aMin(center.x-95.0f,center.y+62.0f);
        ImVec2 aMax(center.x-18.0f,center.y+138.0f);
        ImVec2 bMin(center.x+18.0f,center.y-138.0f);
        ImVec2 bMax(center.x+95.0f,center.y-62.0f);

        dl->AddRectFilled(aMin,aMax,IM_COL32(135,160,210,120),5.0f);
        dl->AddRect(aMin,aMax,IM_COL32(180,210,255,240),5.0f,0,2.0f);

        dl->AddRectFilled(bMin,bMax,IM_COL32(220,170,100,96),5.0f);
        dl->AddRect(bMin,bMax,IM_COL32(255,205,128,230),5.0f,0,2.0f);

        if (showMeasurements)
        {
            dl->AddLine(
                ImVec2(aMax.x+10.0f,aMin.y+8.0f),
                ImVec2(bMin.x-10.0f,bMax.y-8.0f),
                IM_COL32(210,210,210,190),
                1.6f
            );

            dl->AddText(
                ImVec2(center.x-42.0f,center.y-26.0f),
                IM_COL32(230,230,230,220),
                "4.8 m"
            );
        }

        if (showPath)
        {
            dl->PathLineTo(ImVec2(center.x-58.0f,center.y+160.0f));
            dl->PathBezierCubicCurveTo(
                ImVec2(center.x-40.0f,center.y+80.0f),
                ImVec2(center.x-12.0f,center.y+28.0f),
                ImVec2(center.x+18.0f,center.y-14.0f)
            );
            dl->PathStroke(IM_COL32(255,196,86,235),0,2.0f);
        }

        dl->AddText(ImVec2(p.x+16.0f,p.y+14.0f),toU32(colorText()),"2D PLAN VIEW");
        dl->AddText(ImVec2(p.x+16.0f,p.y+34.0f),toU32(colorMuted()),orthoView==0 ? "Top orthographic layout" : (orthoView==1 ? "Front orthographic layout" : "Right orthographic layout"));
    }
    else if (viewportMode==1)
    {
        // -----------------------------------------------
        // 3D SCENE PREVIEW
        // -----------------------------------------------
        const float horizonY=p.y+s.y*0.34f;

        // sky / ground split
        dl->AddRectFilled(p,ImVec2(p.x+s.x,horizonY),IM_COL32(33,38,45,160),0.0f);
        dl->AddRectFilled(ImVec2(p.x,horizonY),ImVec2(p.x+s.x,p.y+s.y),IM_COL32(24,26,30,210),0.0f);

        if (showGrid)
        {
            const ImVec2 vanish(center.x,horizonY+18.0f);

            for (int i=0;i<15;i++)
            {
                const float t=(float)i/14.0f;
                const float x=p.x + (s.x*t);
                dl->AddLine(
                    ImVec2(x,p.y+s.y-18.0f),
                    vanish,
                    toU32(ImVec4(1,1,1,0.07f)),
                    1.0f
                );
            }

            for (int j=0;j<10;j++)
            {
                const float t=(float)j/9.0f;
                const float y=horizonY + 26.0f + (t*t)*(s.y*0.56f);
                dl->AddLine(
                    ImVec2(p.x+30.0f,y),
                    ImVec2(p.x+s.x-30.0f,y),
                    toU32(ImVec4(1,1,1,0.07f)),
                    1.0f
                );
            }
        }

        // pseudo vehicle block
        const ImVec2 bodyA(center.x-72.0f,center.y+46.0f);
        const ImVec2 bodyB(center.x+10.0f,center.y+80.0f);
        dl->AddRectFilled(bodyA,bodyB,IM_COL32(120,150,210,100),4.0f);
        dl->AddRect(bodyA,bodyB,IM_COL32(180,210,255,230),4.0f,0,2.0f);

        // roof
        dl->AddLine(ImVec2(bodyA.x+18.0f,bodyA.y-24.0f),ImVec2(bodyB.x-18.0f,bodyA.y-24.0f),IM_COL32(180,210,255,210),2.0f);
        dl->AddLine(ImVec2(bodyA.x,bodyA.y),ImVec2(bodyA.x+18.0f,bodyA.y-24.0f),IM_COL32(180,210,255,210),2.0f);
        dl->AddLine(ImVec2(bodyB.x,bodyA.y),ImVec2(bodyB.x-18.0f,bodyA.y-24.0f),IM_COL32(180,210,255,210),2.0f);
        dl->AddLine(ImVec2(bodyA.x+18.0f,bodyA.y-24.0f),ImVec2(bodyB.x-18.0f,bodyA.y-24.0f),IM_COL32(180,210,255,210),2.0f);

        // selection bounds
        if (showBounds)
        {
            dl->AddRect(
                ImVec2(bodyA.x-10.0f,bodyA.y-32.0f),
                ImVec2(bodyB.x+10.0f,bodyB.y+10.0f),
                IM_COL32(240,190,92,220),
                4.0f,
                0,
                1.5f
            );
        }

        // axes gizmo
        if (showAxes)
        {
            const ImVec2 g(ImVec2(p.x+86.0f,p.y+s.y-76.0f));
            dl->AddCircleFilled(g,5.0f,IM_COL32(255,200,60,255));
            dl->AddLine(g,ImVec2(g.x+52.0f,g.y),IM_COL32(190,72,56,255),2.0f);
            dl->AddLine(g,ImVec2(g.x,g.y-52.0f),IM_COL32(70,170,95,255),2.0f);
            dl->AddLine(g,ImVec2(g.x-32.0f,g.y+24.0f),IM_COL32(82,132,220,255),2.0f);

            dl->AddText(ImVec2(g.x+56.0f,g.y-7.0f),IM_COL32(220,110,90,255),"X");
            dl->AddText(ImVec2(g.x-6.0f,g.y-66.0f),IM_COL32(112,220,128,255),"Y");
            dl->AddText(ImVec2(g.x-44.0f,g.y+19.0f),IM_COL32(126,170,255,255),"Z");
        }

        if (renderStyle==1)
        {
            dl->AddRect(
                ImVec2(bodyA.x+8.0f,bodyA.y+10.0f),
                ImVec2(bodyB.x-8.0f,bodyB.y-10.0f),
                IM_COL32(220,220,220,145),
                2.0f,
                0,
                1.2f
            );
        }

        if (renderStyle==2 && showMeasurements)
        {
            dl->AddLine(
                ImVec2(center.x-118.0f,center.y+118.0f),
                ImVec2(center.x+126.0f,center.y+92.0f),
                IM_COL32(255,196,86,220),
                2.0f
            );

            dl->AddText(
                ImVec2(center.x+16.0f,center.y+100.0f),
                IM_COL32(255,210,112,235),
                "impact vector"
            );
        }

        dl->AddText(ImVec2(p.x+16.0f,p.y+14.0f),toU32(colorText()),"3D SCENE VIEW");
        dl->AddText(ImVec2(p.x+16.0f,p.y+34.0f),toU32(colorMuted()),renderStyle==0 ? "Perspective lit preview" : (renderStyle==1 ? "Wireframe inspection" : "Analysis overlay mode"));
    }
    else
    {
        // -----------------------------------------------
        // AR PREVIEW UI SHELL
        // -----------------------------------------------
        const float inset=34.0f;
        const ImVec2 frameMin(p.x+inset,p.y+20.0f);
        const ImVec2 frameMax(p.x+s.x-inset,p.y+s.y-20.0f);

        // simulated camera plate
        dl->AddRectFilled(
            frameMin,
            frameMax,
            IM_COL32(38,42,48,255),
            16.0f
        );

        dl->AddRect(
            frameMin,
            frameMax,
            IM_COL32(130,138,148,255),
            16.0f,
            0,
            1.2f
        );

        // faint feed texture lines
        for (float y=frameMin.y+16.0f; y<frameMax.y; y+=26.0f)
        {
            dl->AddLine(
                ImVec2(frameMin.x+12.0f,y),
                ImVec2(frameMax.x-12.0f,y),
                IM_COL32(255,255,255,12),
                1.0f
            );
        }

        // target overlay
        const ImVec2 boxMin(center.x-124.0f,center.y-58.0f);
        const ImVec2 boxMax(center.x+124.0f,center.y+72.0f);

        dl->AddRect(
            boxMin,
            boxMax,
            IM_COL32(90,210,255,(int)(220.0f*arOpacity)),
            6.0f,
            0,
            2.0f
        );

        // corner brackets
        const float c=22.0f;
        auto addCorner = [&](float x,float y,float dx,float dy)
        {
            dl->AddLine(ImVec2(x,y),ImVec2(x+dx*c,y),IM_COL32(90,210,255,(int)(240.0f*arOpacity)),2.0f);
            dl->AddLine(ImVec2(x,y),ImVec2(x,y+dy*c),IM_COL32(90,210,255,(int)(240.0f*arOpacity)),2.0f);
        };

        addCorner(boxMin.x,boxMin.y,1,1);
        addCorner(boxMax.x,boxMin.y,-1,1);
        addCorner(boxMin.x,boxMax.y,1,-1);
        addCorner(boxMax.x,boxMax.y,-1,-1);

        // anchor marker
        if (arAnchors)
        {
            dl->AddCircle(
                ImVec2(center.x,center.y+90.0f),
                18.0f,
                IM_COL32(90,210,255,210),
                28,
                2.0f
            );

            dl->AddText(
                ImVec2(center.x+26.0f,center.y+80.0f),
                IM_COL32(90,210,255,230),
                "Anchor: Origin"
            );
        }

        if (arCollisionHints)
        {
            dl->AddRect(
                ImVec2(center.x-152.0f,center.y+102.0f),
                ImVec2(center.x+154.0f,center.y+148.0f),
                IM_COL32(255,196,86,200),
                4.0f,
                0,
                1.5f
            );

            dl->AddText(
                ImVec2(center.x-118.0f,center.y+114.0f),
                IM_COL32(255,206,120,235),
                "Estimated collision corridor"
            );
        }

        // top badges
        dl->AddText(ImVec2(frameMin.x+16.0f,frameMin.y+16.0f),toU32(colorText()),"AR PREVIEW");
        dl->AddText(ImVec2(frameMin.x+16.0f,frameMin.y+38.0f),toU32(colorMuted()),"Editor-side mock preview until device session is connected.");

        // status pills
        dl->AddRectFilled(
            ImVec2(frameMax.x-198.0f,frameMin.y+14.0f),
            ImVec2(frameMax.x-102.0f,frameMin.y+40.0f),
            IM_COL32(74,84,96,190),
            13.0f
        );

        dl->AddText(
            ImVec2(frameMax.x-179.0f,frameMin.y+20.0f),
            IM_COL32(215,220,228,245),
            "Tracking"
        );

        dl->AddRectFilled(
            ImVec2(frameMax.x-92.0f,frameMin.y+14.0f),
            ImVec2(frameMax.x-18.0f,frameMin.y+40.0f),
            IM_COL32(74,84,96,190),
            13.0f
        );

        dl->AddText(
            ImVec2(frameMax.x-76.0f,frameMin.y+20.0f),
            IM_COL32(215,220,228,245),
            "Idle"
        );
    }

    ImGui::EndChild();

    ImGui::Spacing();

    // ======================================================
    // FOOTER STATUS CARDS
    // ======================================================

    ImGui::BeginChild(
        "ViewportFooterBand",
        ImVec2(0.0f,60.0f),
        false,
        ImGuiWindowFlags_NoScrollbar
    );

    const float gap=8.0f;
    const float cardW=(ImGui::GetContentRegionAvail().x-(gap*2.0f))/3.0f;

    if (viewportMode==0)
    {
        footerCard("VP2DCardA","Projection","Orthographic","2D planning layout",cardW);
        ImGui::SameLine(0.0f,gap);
        footerCard("VP2DCardB","Scale","1:"+std::to_string((int)(100.0f/zoom2D)).c_str(),"Zoom-sensitive grid",cardW);
        ImGui::SameLine(0.0f,gap);
        footerCard("VP2DCardC","Overlay","Measurements","Scene drafting enabled",cardW);
    }
    else if (viewportMode==1)
    {
        footerCard("VP3DCardA","Camera","Perspective","Free-orbit inspection",cardW);
        ImGui::SameLine(0.0f,gap);
        footerCard("VP3DCardB","Render",renderStyle==0 ? "Lit" : (renderStyle==1 ? "Wireframe" : "Analysis"),"3D reconstruction review",cardW);
        ImGui::SameLine(0.0f,gap);
        footerCard("VP3DCardC","Selection","None","No scene object selected",cardW);
    }
    else
    {
        footerCard("VPARCardA","Tracking","Simulated","Device session not connected",cardW);
        ImGui::SameLine(0.0f,gap);
        footerCard("VPARCardB","Anchors",arAnchors ? "Visible" : "Hidden","Origin and reference guides",cardW);
        ImGui::SameLine(0.0f,gap);
        footerCard("VPARCardC","Session","Preview Only","AR runtime hookup comes next",cardW);
    }

    ImGui::EndChild();

    ImGui::EndChild();
    ImGui::End();
}
'@

$text = Replace-FunctionBlock `
    -Source $text `
    -Signature 'static void drawViewportView()' `
    -Replacement $replacement

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$verifyMarkers = @(
    '2D PLAN',
    '3D SCENE',
    'AR PREVIEW',
    'ViewportToolRail',
    'ViewportRenderSurface',
    'PAIR DEVICE',
    'START SESSION'
)

foreach ($marker in $verifyMarkers) {
    if (-not $verify.Contains($marker)) {
        throw "Verification failed after write: $marker"
    }
}

Write-Host ""
Write-Host "[DONE] Viewport upgraded to 2D / 3D / AR shell." -ForegroundColor Cyan
Write-Host "[OK] Added viewport mode switching." -ForegroundColor Green
Write-Host "[OK] Added per-mode toolbar controls." -ForegroundColor Green
Write-Host "[OK] Added polished render surface placeholders for 2D / 3D / AR." -ForegroundColor Green
Write-Host "[OK] Added footer status cards." -ForegroundColor Green
Write-Host "[OK] Existing global shell / palette / timeline untouched." -ForegroundColor Green
Write-Host ""
Write-Host "Note: AR is a UI preview shell for now, not live device integration yet." -ForegroundColor Yellow
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
