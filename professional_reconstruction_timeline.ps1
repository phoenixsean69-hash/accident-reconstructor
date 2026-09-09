param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - PROFESSIONAL RECONSTRUCTION TIMELINE" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

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

    return @{
        Text = $Source.Substring($start, $end - $start + 1)
    }
}

$timelineInfo = Get-FunctionBlock `
    -Source $text `
    -Signature "static void drawTimeline()"

$oldTimeline = $timelineInfo.Text

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-professional-timeline-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

$newTimeline = @'
static void drawTimeline()
{
    static int currentFrame=0;
    static int selectedFrame=0;
    static bool playing=false;
    static bool snapToFrames=true;
    static bool followPlayhead=true;
    static float playbackSpeed=1.0f;
    static float zoom=1.0f;
    static float playbackAccumulator=0.0f;

    static std::array<int,8> userMarkers{
        -1,-1,-1,-1,-1,-1,-1,-1
    };

    static int userMarkerCount=0;

    constexpr int totalFrames=240;
    constexpr int fps=30;

    // The phase boundaries are editor defaults until case-derived
    // reconstruction timing is wired in.
    constexpr int preImpactEnd=90;
    constexpr int impactEnd=120;

    if (playing)
    {
        playbackAccumulator +=
            ImGui::GetIO().DeltaTime *
            playbackSpeed *
            static_cast<float>(fps);

        while (playbackAccumulator>=1.0f)
        {
            currentFrame++;

            if (currentFrame>totalFrames)
            {
                currentFrame=0;
            }

            selectedFrame=currentFrame;
            playbackAccumulator-=1.0f;
        }
    }

    ImGui::Begin(
        "Timeline",
        &gEditorShell.showTimeline,
        ImGuiWindowFlags_NoMove
    );

    // ========================================================
    // TRANSPORT / TIMELINE TOOLBAR
    // ========================================================

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(10.0f,5.0f)
    );

    if (ImGui::Button("|<",ImVec2(38.0f,30.0f)))
    {
        currentFrame=0;
        selectedFrame=currentFrame;
        playing=false;
    }

    ImGui::SameLine(0.0f,5.0f);

    if (ImGui::Button("<",ImVec2(34.0f,30.0f)))
    {
        currentFrame=
            std::max(
                0,
                currentFrame-1
            );

        selectedFrame=currentFrame;
        playing=false;
    }

    ImGui::SameLine(0.0f,7.0f);

    if (!playing)
    {
        if (ImGui::Button(
            "PLAY",
            ImVec2(64.0f,30.0f)))
        {
            playing=true;
            playbackAccumulator=0.0f;
        }
    }
    else
    {
        if (ImGui::Button(
            "PAUSE",
            ImVec2(64.0f,30.0f)))
        {
            playing=false;
        }
    }

    ImGui::SameLine(0.0f,5.0f);

    if (ImGui::Button(
        "STOP",
        ImVec2(62.0f,30.0f)))
    {
        playing=false;
        currentFrame=0;
        selectedFrame=0;
        playbackAccumulator=0.0f;
    }

    ImGui::SameLine(0.0f,7.0f);

    if (ImGui::Button(">",ImVec2(34.0f,30.0f)))
    {
        currentFrame=
            std::min(
                totalFrames,
                currentFrame+1
            );

        selectedFrame=currentFrame;
        playing=false;
    }

    ImGui::SameLine(0.0f,5.0f);

    if (ImGui::Button(">|",ImVec2(38.0f,30.0f)))
    {
        currentFrame=totalFrames;
        selectedFrame=currentFrame;
        playing=false;
    }

    ImGui::SameLine(0.0f,14.0f);

    ImGui::TextDisabled("FRAME");
    ImGui::SameLine(0.0f,6.0f);

    ImGui::SetNextItemWidth(76.0f);

    if (ImGui::InputInt(
        "##TimelineFrame",
        &currentFrame,
        1,
        10))
    {
        currentFrame=
            std::max(
                0,
                std::min(
                    totalFrames,
                    currentFrame
                )
            );

        selectedFrame=currentFrame;
        playing=false;
    }

    ImGui::SameLine(0.0f,14.0f);

    ImGui::TextDisabled("SPEED");
    ImGui::SameLine(0.0f,6.0f);

    const char* speedLabels[]={
        "0.25x",
        "0.5x",
        "1x",
        "2x"
    };

    int speedIndex=
        playbackSpeed<=0.25f
            ? 0
            : (playbackSpeed<=0.5f
                ? 1
                : (playbackSpeed<2.0f
                    ? 2
                    : 3));

    ImGui::SetNextItemWidth(76.0f);

    if (ImGui::Combo(
        "##TimelineSpeed",
        &speedIndex,
        speedLabels,
        4))
    {
        playbackSpeed=
            speedIndex==0
                ? 0.25f
                : (speedIndex==1
                    ? 0.5f
                    : (speedIndex==2
                        ? 1.0f
                        : 2.0f));
    }

    ImGui::SameLine(0.0f,14.0f);

    ImGui::Checkbox(
        "Snap",
        &snapToFrames
    );

    ImGui::SameLine(0.0f,10.0f);

    ImGui::Checkbox(
        "Follow",
        &followPlayhead
    );

    ImGui::SameLine(0.0f,14.0f);

    ImGui::TextDisabled("ZOOM");
    ImGui::SameLine(0.0f,6.0f);

    ImGui::SetNextItemWidth(118.0f);

    ImGui::SliderFloat(
        "##TimelineZoom",
        &zoom,
        0.75f,
        3.0f,
        "%.2fx"
    );

    ImGui::PopStyleVar();

    ImGui::Separator();

    // ========================================================
    // TIMELINE GEOMETRY
    // ========================================================

    const float headerW=154.0f;
    const float rulerH=34.0f;
    const float phaseH=28.0f;
    const float trackH=34.0f;
    const int trackCount=4;

    const ImVec2 canvasPos=
        ImGui::GetCursorScreenPos();

    const ImVec2 avail=
        ImGui::GetContentRegionAvail();

    const float timelineW=
        std::max(
            240.0f,
            avail.x-headerW
        );

    const float totalH=
        rulerH+
        phaseH+
        static_cast<float>(trackCount)*trackH+
        8.0f;

    ImGui::InvisibleButton(
        "##TimelineCanvas",
        ImVec2(avail.x,totalH),
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight
    );

    const bool canvasHovered=
        ImGui::IsItemHovered();

    ImDrawList* dl=
        ImGui::GetWindowDrawList();

    const ImVec2 timelineMin(
        canvasPos.x+headerW,
        canvasPos.y
    );

    const ImVec2 timelineMax(
        canvasPos.x+headerW+timelineW,
        canvasPos.y+totalH
    );

    // Visible frame range.
    const float visibleFrameCount=
        static_cast<float>(totalFrames)/
        zoom;

    const int visibleFrames=
        std::max(
            30,
            static_cast<int>(visibleFrameCount)
        );

    int viewStart=
        followPlayhead
            ? currentFrame-visibleFrames/2
            : 0;

    viewStart=
        std::max(
            0,
            std::min(
                totalFrames-visibleFrames,
                viewStart
            )
        );

    const int viewEnd=
        std::min(
            totalFrames,
            viewStart+visibleFrames
        );

    auto frameToX = [&](int frame)
    {
        const float t=
            static_cast<float>(
                frame-viewStart
            )/
            static_cast<float>(
                std::max(
                    1,
                    viewEnd-viewStart
                )
            );

        return timelineMin.x+
            t*timelineW;
    };

    auto xToFrame = [&](float x)
    {
        const float t=
            std::max(
                0.0f,
                std::min(
                    1.0f,
                    (x-timelineMin.x)/
                    timelineW
                )
            );

        int frame=
            viewStart+
            static_cast<int>(
                t*
                static_cast<float>(
                    viewEnd-viewStart
                )
            );

        if (snapToFrames)
        {
            frame=
                std::max(
                    0,
                    std::min(
                        totalFrames,
                        frame
                    )
                );
        }

        return frame;
    };

    // ========================================================
    // LEFT TRACK HEADER
    // ========================================================

    dl->AddRectFilled(
        canvasPos,
        ImVec2(
            canvasPos.x+headerW-4.0f,
            canvasPos.y+totalH
        ),
        IM_COL32(25,28,32,255)
    );

    dl->AddLine(
        ImVec2(
            canvasPos.x+headerW-4.0f,
            canvasPos.y
        ),
        ImVec2(
            canvasPos.x+headerW-4.0f,
            canvasPos.y+totalH
        ),
        toU32(colorBorder()),
        1.0f
    );

    dl->AddText(
        ImVec2(
            canvasPos.x+10.0f,
            canvasPos.y+9.0f
        ),
        toU32(colorText()),
        "RECONSTRUCTION"
    );

    dl->AddText(
        ImVec2(
            canvasPos.x+10.0f,
            canvasPos.y+rulerH+7.0f
        ),
        toU32(colorMuted()),
        "PHASE"
    );

    const char* trackNames[]={
        "Vehicle A",
        "Vehicle B",
        "Evidence",
        "Measurements"
    };

    for (int i=0;i<trackCount;++i)
    {
        const float y=
            canvasPos.y+
            rulerH+
            phaseH+
            static_cast<float>(i)*trackH;

        if ((i%2)==1)
        {
            dl->AddRectFilled(
                ImVec2(
                    canvasPos.x,
                    y
                ),
                ImVec2(
                    canvasPos.x+headerW-4.0f,
                    y+trackH
                ),
                IM_COL32(30,33,38,255)
            );
        }

        dl->AddText(
            ImVec2(
                canvasPos.x+18.0f,
                y+9.0f
            ),
            toU32(colorText()),
            trackNames[i]
        );
    }

    // ========================================================
    // RULER
    // ========================================================

    dl->AddRectFilled(
        timelineMin,
        ImVec2(
            timelineMax.x,
            timelineMin.y+rulerH
        ),
        IM_COL32(30,33,38,255)
    );

    int majorStep=30;

    if (zoom>=1.5f)
        majorStep=15;

    if (zoom>=2.5f)
        majorStep=10;

    for (int frame=viewStart;frame<=viewEnd;++frame)
    {
        const float x=
            frameToX(frame);

        const bool major=
            (frame%majorStep)==0;

        const bool medium=
            (frame%(majorStep/2))==0;

        const float tickH=
            major
                ? 14.0f
                : (medium
                    ? 9.0f
                    : 5.0f);

        dl->AddLine(
            ImVec2(
                x,
                timelineMin.y+rulerH-tickH
            ),
            ImVec2(
                x,
                timelineMin.y+rulerH
            ),
            major
                ? IM_COL32(158,164,174,210)
                : IM_COL32(88,93,101,180),
            1.0f
        );

        if (major)
        {
            char label[32]{};

            std::snprintf(
                label,
                sizeof(label),
                "%d",
                frame
            );

            dl->AddText(
                ImVec2(
                    x+4.0f,
                    timelineMin.y+5.0f
                ),
                IM_COL32(160,166,176,235),
                label
            );
        }
    }

    // ========================================================
    // PHASE BAND
    // ========================================================

    const float phaseY=
        timelineMin.y+rulerH;

    const float phaseBottom=
        phaseY+phaseH;

    auto phaseRect = [&](int start,int end,ImU32 fill,const char* label)
    {
        const int clampedStart=
            std::max(
                start,
                viewStart
            );

        const int clampedEnd=
            std::min(
                end,
                viewEnd
            );

        if (clampedEnd<=clampedStart)
            return;

        const float x0=
            frameToX(clampedStart);

        const float x1=
            frameToX(clampedEnd);

        dl->AddRectFilled(
            ImVec2(x0,phaseY),
            ImVec2(x1,phaseBottom),
            fill
        );

        const ImVec2 textSize=
            ImGui::CalcTextSize(label);

        if ((x1-x0)>textSize.x+18.0f)
        {
            dl->AddText(
                ImVec2(
                    x0+8.0f,
                    phaseY+6.0f
                ),
                IM_COL32(218,221,226,245),
                label
            );
        }
    };

    phaseRect(
        0,
        preImpactEnd,
        IM_COL32(60,68,78,235),
        "PRE-IMPACT"
    );

    phaseRect(
        preImpactEnd,
        impactEnd,
        IM_COL32(88,74,52,245),
        "IMPACT"
    );

    phaseRect(
        impactEnd,
        totalFrames,
        IM_COL32(54,62,70,235),
        "POST-IMPACT"
    );

    // Phase boundaries.
    const int phaseBoundaries[]={
        preImpactEnd,
        impactEnd
    };

    for (int boundary : phaseBoundaries)
    {
        if (boundary>=viewStart &&
            boundary<=viewEnd)
        {
            const float x=
                frameToX(boundary);

            dl->AddLine(
                ImVec2(
                    x,
                    timelineMin.y
                ),
                ImVec2(
                    x,
                    timelineMax.y
                ),
                IM_COL32(192,151,76,185),
                1.4f
            );
        }
    }

    // ========================================================
    // TRACK LANES
    // ========================================================

    const float tracksTop=
        phaseBottom;

    for (int i=0;i<trackCount;++i)
    {
        const float y0=
            tracksTop+
            static_cast<float>(i)*trackH;

        const float y1=
            y0+trackH;

        dl->AddRectFilled(
            ImVec2(
                timelineMin.x,
                y0
            ),
            ImVec2(
                timelineMax.x,
                y1
            ),
            (i%2)==0
                ? IM_COL32(22,25,29,255)
                : IM_COL32(27,30,35,255)
        );

        dl->AddLine(
            ImVec2(
                timelineMin.x,
                y1
            ),
            ImVec2(
                timelineMax.x,
                y1
            ),
            IM_COL32(54,59,66,210),
            1.0f
        );

        const char* emptyLabel=
            i<2
                ? "No vehicle events"
                : (i==2
                    ? "No evidence events"
                    : "No measurement events");

        dl->AddText(
            ImVec2(
                timelineMin.x+10.0f,
                y0+9.0f
            ),
            IM_COL32(104,110,120,190),
            emptyLabel
        );
    }

    // User markers.
    for (int i=0;i<userMarkerCount;++i)
    {
        const int markerFrame=
            userMarkers[
                static_cast<size_t>(i)
            ];

        if (markerFrame<viewStart ||
            markerFrame>viewEnd)
        {
            continue;
        }

        const float x=
            frameToX(markerFrame);

        dl->AddLine(
            ImVec2(
                x,
                phaseBottom
            ),
            ImVec2(
                x,
                timelineMax.y
            ),
            IM_COL32(205,174,108,210),
            1.5f
        );

        d->AddTriangleFilled(
            ImVec2(x,phaseBottom),
            ImVec2(x-5.0f,phaseBottom+8.0f),
            ImVec2(x+5.0f,phaseBottom+8.0f),
            IM_COL32(220,184,106,235)
        );
    }

    // ========================================================
    // PLAYHEAD
    // ========================================================

    const float playheadX=
        frameToX(currentFrame);

    if (currentFrame>=viewStart &&
        currentFrame<=viewEnd)
    {
        dl->AddLine(
            ImVec2(
                playheadX,
                timelineMin.y
            ),
            ImVec2(
                playheadX,
                timelineMax.y
            ),
            toU32(colorAccent()),
            2.0f
        );

        dl->AddTriangleFilled(
            ImVec2(
                playheadX,
                timelineMin.y+rulerH
            ),
            ImVec2(
                playheadX-7.0f,
                timelineMin.y+rulerH-10.0f
            ),
            ImVec2(
                playheadX+7.0f,
                timelineMin.y+rulerH-10.0f
            ),
            toU32(colorAccent())
        );
    }

    // Selected frame indicator.
    if (selectedFrame>=viewStart &&
        selectedFrame<=viewEnd &&
        selectedFrame!=currentFrame)
    {
        const float selectedX=
            frameToX(selectedFrame);

        dl->AddLine(
            ImVec2(
                selectedX,
                phaseBottom
            ),
            ImVec2(
                selectedX,
                timelineMax.y
            ),
            IM_COL32(150,158,170,125),
            1.0f
        );
    }

    // ========================================================
    // MOUSE INTERACTION
    // ========================================================

    if (canvasHovered)
    {
        const ImVec2 mouse=
            ImGui::GetIO().MousePos;

        if (mouse.x>=timelineMin.x &&
            mouse.x<=timelineMax.x)
        {
            if (ImGui::IsMouseClicked(
                ImGuiMouseButton_Left))
            {
                selectedFrame=
                    xToFrame(mouse.x);

                currentFrame=
                    selectedFrame;

                playing=false;
            }

            if (ImGui::IsMouseDragging(
                ImGuiMouseButton_Left,
                0.0f))
            {
                selectedFrame=
                    xToFrame(mouse.x);

                currentFrame=
                    selectedFrame;

                playing=false;
            }
        }
    }

    // ========================================================
    // TIMELINE CONTEXT MENU
    // ========================================================

    if (ImGui::BeginPopupContextItem(
        "##TimelineContextMenu"))
    {
        ImGui::TextDisabled(
            "TIMELINE"
        );

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Add Marker at Playhead"))
        {
            if (userMarkerCount<
                static_cast<int>(
                    userMarkers.size()
                ))
            {
                userMarkers[
                    static_cast<size_t>(
                        userMarkerCount
                    )
                ]=currentFrame;

                userMarkerCount++;
            }
        }

        if (ImGui::MenuItem(
            "Clear Markers",
            nullptr,
            false,
            userMarkerCount>0))
        {
            userMarkers.fill(-1);
            userMarkerCount=0;
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
            "Jump to Pre-impact"))
        {
            currentFrame=0;
            selectedFrame=0;
            playing=false;
        }

        if (ImGui::MenuItem(
            "Jump to Impact"))
        {
            currentFrame=preImpactEnd;
            selectedFrame=currentFrame;
            playing=false;
        }

        if (ImGui::MenuItem(
            "Jump to Post-impact"))
        {
            currentFrame=impactEnd;
            selectedFrame=currentFrame;
            playing=false;
        }

        ImGui::Separator();

        ImGui::MenuItem(
            "Snap to Frames",
            nullptr,
            &snapToFrames
        );

        ImGui::MenuItem(
            "Follow Playhead",
            nullptr,
            &followPlayhead
        );

        ImGui::EndPopup();
    }

    // ========================================================
    // FOOTER READOUT
    // ========================================================

    ImGui::Spacing();
    ImGui::Separator();

    const float seconds=
        static_cast<float>(currentFrame)/
        static_cast<float>(fps);

    const char* phaseName=
        currentFrame<preImpactEnd
            ? "Pre-impact"
            : (currentFrame<impactEnd
                ? "Impact"
                : "Post-impact");

    ImGui::TextDisabled("CURRENT");
    ImGui::SameLine(0.0f,7.0f);
    ImGui::Text("Frame %d",currentFrame);

    ImGui::SameLine(0.0f,18.0f);
    ImGui::TextDisabled("TIME");
    ImGui::SameLine(0.0f,7.0f);
    ImGui::Text("%.2f s",seconds);

    ImGui::SameLine(0.0f,18.0f);
    ImGui::TextDisabled("PHASE");
    ImGui::SameLine(0.0f,7.0f);
    ImGui::Text("%s",phaseName);

    ImGui::SameLine(0.0f,18.0f);
    ImGui::TextDisabled("FPS");
    ImGui::SameLine(0.0f,7.0f);
    ImGui::Text("%d",fps);

    ImGui::SameLine(0.0f,18.0f);
    ImGui::TextDisabled("MARKERS");
    ImGui::SameLine(0.0f,7.0f);
    ImGui::Text("%d",userMarkerCount);

    ImGui::End();
}
'@

# Small typo-proofing: use dl consistently.
$newTimeline = $newTimeline.Replace(
    '        d->AddTriangleFilled(',
    '        dl->AddTriangleFilled('
)

$text = $text.Replace(
    $oldTimeline,
    $newTimeline
)

# ============================================================
# INCREASE TIMELINE DOCK HEIGHT
# ============================================================

$oldRatioPattern =
    'ImGui::DockBuilderSplitNode\s*\(\s*center\s*,\s*ImGuiDir_Down\s*,\s*0\.16f\s*,'

if ([regex]::IsMatch($text,$oldRatioPattern))
{
    $text = [regex]::Replace(
        $text,
        $oldRatioPattern,
        'ImGui::DockBuilderSplitNode(center,ImGuiDir_Down,0.26f,',
        1
    )

    Write-Host "[OK] Timeline dock ratio increased from 16% to 26%." -ForegroundColor Green
}
else
{
    Write-Host "[INFO] Exact 0.16 timeline dock ratio not found; timeline function still upgraded." -ForegroundColor Yellow
}

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    '##TimelineCanvas',
    'RECONSTRUCTION',
    'PRE-IMPACT',
    'POST-IMPACT',
    'Add Marker at Playhead',
    'Follow Playhead',
    'No vehicle events',
    'CURRENT',
    'MARKERS'
)

foreach ($check in $checks)
{
    if (-not $verify.Contains($check))
    {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] Professional reconstruction timeline installed." -ForegroundColor Cyan
Write-Host "[OK] Transport / frame stepping / playback speed." -ForegroundColor Green
Write-Host "[OK] Zoom + snap + follow-playhead controls." -ForegroundColor Green
Write-Host "[OK] Time ruler + draggable playhead." -ForegroundColor Green
Write-Host "[OK] Pre-impact / Impact / Post-impact phase band." -ForegroundColor Green
Write-Host "[OK] Vehicle / Evidence / Measurement lanes." -ForegroundColor Green
Write-Host "[OK] Right-click marker + navigation menu." -ForegroundColor Green
Write-Host "[OK] Existing 2D / 3D / AR viewport untouched." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
