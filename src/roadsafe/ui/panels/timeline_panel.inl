// ROADSAFE_UI_COMPONENT_V21
// Component: Timeline Panel
// Included from src/main.cpp; translation-unit behavior is unchanged.

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
    // ========================================================
    // TIMELINE KEYBOARD SHORTCUTS
    // ========================================================

    {
        const ImGuiIO& io=ImGui::GetIO();

        const bool timelineFocused=
            ImGui::IsWindowFocused(
                ImGuiFocusedFlags_RootAndChildWindows
            );

        if (timelineFocused &&
            !io.WantTextInput)
        {
            const auto key =
                [](ImGuiKey k)
            {
                return ImGui::IsKeyPressed(
                    k,
                    false
                );
            };

            if (!io.KeyCtrl &&
                !io.KeyAlt &&
                !io.KeyShift)
            {
                if (key(ImGuiKey_Space))
                {
                    playing=!playing;
                    playbackAccumulator=0.0f;
                }

                if (key(ImGuiKey_LeftArrow))
                {
                    currentFrame=
                        std::max(
                            0,
                            currentFrame-1
                        );

                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_RightArrow))
                {
                    currentFrame=
                        std::min(
                            totalFrames,
                            currentFrame+1
                        );

                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_Home))
                {
                    currentFrame=0;
                    selectedFrame=0;
                    playing=false;
                }

                if (key(ImGuiKey_End))
                {
                    currentFrame=totalFrames;
                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_1))
                {
                    currentFrame=0;
                    selectedFrame=0;
                    playing=false;
                }

                if (key(ImGuiKey_2))
                {
                    currentFrame=preImpactEnd;
                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_3))
                {
                    currentFrame=impactEnd;
                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_M))
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

                if (key(ImGuiKey_S))
                    snapToFrames=!snapToFrames;

                if (key(ImGuiKey_F))
                    followPlayhead=!followPlayhead;
            }

            if (io.KeyShift &&
                !io.KeyCtrl &&
                !io.KeyAlt)
            {
                if (key(ImGuiKey_LeftArrow))
                {
                    currentFrame=
                        std::max(
                            0,
                            currentFrame-10
                        );

                    selectedFrame=currentFrame;
                    playing=false;
                }

                if (key(ImGuiKey_RightArrow))
                {
                    currentFrame=
                        std::min(
                            totalFrames,
                            currentFrame+10
                        );

                    selectedFrame=currentFrame;
                    playing=false;
                }
            }

            if (io.KeyCtrl &&
                io.KeyShift &&
                !io.KeyAlt &&
                key(ImGuiKey_M))
            {
                userMarkers.fill(-1);
                userMarkerCount=0;
            }
        }
    }

    // ========================================================
    // TRANSPORT / TIMELINE TOOLBAR
    // ========================================================

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(10.0f,5.0f)
    );

    if (editorButton("FIRST",38.0f))
    {
        currentFrame=0;
        selectedFrame=currentFrame;
        playing=false;
    }

    ImGui::SameLine(0.0f,5.0f);

    if (editorButton("STEP BACK",34.0f))
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

    if (editorButton("STEP NEXT",34.0f))
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

    if (editorButton("LAST",38.0f))
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

        const bool linkedTrackSelected=
            (i==0 &&
                gEditorShell.selectedEntity==3) |
            (i==1 &&
                gEditorShell.selectedEntity==4) |
            (i==2 &&
                sharedSelectionIsEvidence()) |
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

        if (markerFrame<viewStart |
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

        dl->AddTriangleFilled(
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

