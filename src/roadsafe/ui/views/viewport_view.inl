// ROADSAFE_UI_COMPONENT_V20
// Component: Viewport View
// Included from src/main.cpp so behavior/state linkage stays unchanged.

static void drawViewportView()
{
    static int viewportMode=0;    // 0 = 2D, 1 = 3D, 2 = AR
    static int orthoView=0;       // 0 = Top, 1 = Front, 2 = Right
    static int renderMode=0;      // 0 = Lit, 1 = Wireframe, 2 = Analysis
    static float arOpacity=0.78f;
    static int cameraSpeed=2;
    static bool showBounds=false;
    static bool showMeasurements=true;
    static bool showSafeFrame=false;
    static bool showNames=true;
    static bool showStats=true;
    static bool arShowAnchors=true;
    static bool arShowCollisionGuide=true;
    static bool arRecord=false;
    static float viewportZoom=1.0f;
    // 3D professional controls.
    static int viewPreset3D=0;       // Perspective / Top / Front / Right
    static int gizmoSpace3D=0;       // World / Local
    static float cameraFov=60.0f;
    static bool showGroundShadow=true;
    static bool showNavigationHints=true;

    // AR professional controls.
    static int arPlacementMode=0;    // Origin / Surface / Vehicle
    static int arTrackingQuality=2;  // 0 poor, 1 limited, 2 good
    static bool arOcclusion=true;
    static bool arPlaneMesh=true;
    static bool arReticle=true;
    static int arAnchorCount=0;
    // AR runtime/session state.
    static bool arEditorPreview=false;
    static bool arDeviceConnected=false;
    static bool arSessionRunning=false;
    static bool arRecording=false;
    static int arDeviceProfile=0;
    static int selectedTool=0;

    const bool viewportSelectionLocked=
        shellSelectedEntityLocked();

    if (viewportSelectionLocked &&
        selectedTool>=1 &&
        selectedTool<=3)
    {
        selectedTool=0;
    }
    static bool showGrid=true, showAxes=true;
        // SOVEREIGN_VIEWPORT_FULLSCREEN_REPAIR_V2
    // Keep the original docked Viewport window registered.
    // Full screen is a separate temporary window, so the dock tab
    // is never removed from Case View / Evidence / Analysis.
    if (gViewportFullscreen)
    {
        ImGui::Begin(
            "Viewport",
            nullptr,
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoBringToFrontOnFocus
        );

        ImGui::End();
    }
if (gViewportFullscreen)
    {
        const ImGuiViewport* mainViewport=
            ImGui::GetMainViewport();

        if (mainViewport)
        {
            ImGui::SetNextWindowPos(
                mainViewport->Pos,
                ImGuiCond_Always
            );

            ImGui::SetNextWindowSize(
                mainViewport->Size,
                ImGuiCond_Always
            );

            ImGui::SetNextWindowViewport(
                mainViewport->ID
            );
        }
    }

    const ImGuiWindowFlags viewportWindowFlags=
        gViewportFullscreen
            ? (
                ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoSavedSettings
              )
            : ImGuiWindowFlags_None;

    const char* viewportWindowName=
        gViewportFullscreen
            ? "Viewport Full Screen###SovereignViewportFullscreen"
            : "Viewport";

    ImGui::Begin(
        viewportWindowName,
        nullptr,
        viewportWindowFlags
    );

    auto setViewportFullscreen=
        [&](bool enable)
        {
            if (enable==gViewportFullscreen)
                return;

            // Never undock the real Viewport.
            // We only switch between the docked window and a
            // separate fullscreen presentation window.
            if (enable)
                gNodeEditorFullscreen=false;

            gViewportFullscreen=enable;
        };

    if (ImGui::IsKeyPressed(ImGuiKey_F11))
    {
        setViewportFullscreen(
            !gViewportFullscreen
        );
    }

    if (gViewportFullscreen &&
        ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        setViewportFullscreen(false);
    }

    if (!gViewportFullscreen)
    {beginEditorContextHeader(
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

    if (shellSnapEnabled())
    {
        ImGui::Text(
            "%.2f m",
            shellSnapValue()
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
        shellSelectedEntityName()[0]!=0
            ? shellSelectedEntityName()
            : "None"
    );

    endEditorContextHeader();
    drawEditorModeStrip();
    }
    // ========================================================
    // VIEWPORT KEYBOARD SHORTCUTS
    // ========================================================

    {
        const ImGuiIO& io=ImGui::GetIO();

        const bool viewportFocused=
            ImGui::IsWindowFocused(
                ImGuiFocusedFlags_RootAndChildWindows
            );

        if (viewportFocused &&
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

            // Mode switching.
            if (io.KeyAlt &&
                !io.KeyCtrl &&
                !io.KeyShift)
            {
                if (key(ImGuiKey_1))
                    viewportMode=0;

                if (key(ImGuiKey_2))
                    viewportMode=1;

                if (key(ImGuiKey_3))
                    viewportMode=2;
            }

            if (!io.KeyCtrl &&
                !io.KeyAlt &&
                !io.KeyShift)
            {
                // Unreal/Unity-style transform hotkeys.
                if (key(ImGuiKey_Q))
                    selectedTool=0;

                if (!viewportSelectionLocked && key(ImGuiKey_W))
                    selectedTool=1;

                if (!viewportSelectionLocked && key(ImGuiKey_E))
                    selectedTool=2;

                if (!viewportSelectionLocked && key(ImGuiKey_R))
                    selectedTool=3;

                // Common overlays.
                if (key(ImGuiKey_G))
                    showGrid=!showGrid;

                if (key(ImGuiKey_X))
                    showAxes=!showAxes;

                if (key(ImGuiKey_B))
                    showBounds=!showBounds;

                if (key(ImGuiKey_M))
                    showMeasurements=
                        !showMeasurements;

                if (key(ImGuiKey_N))
                    showNames=!showNames;

                if (key(ImGuiKey_Home))
                {
                    viewportZoom=1.0f;

                    if (viewportMode==1)
                    {
                        viewPreset3D=0;
                        cameraFov=60.0f;
                    }
                }

                if (viewportMode==0)
                {
                    if (key(ImGuiKey_1))
                        orthoView=0;

                    if (key(ImGuiKey_2))
                        orthoView=1;

                    if (key(ImGuiKey_3))
                        orthoView=2;
                }
                else if (viewportMode==1)
                {
                    if (key(ImGuiKey_1))
                        viewPreset3D=0;

                    if (key(ImGuiKey_2))
                        viewPreset3D=1;

                    if (key(ImGuiKey_3))
                        viewPreset3D=2;

                    if (key(ImGuiKey_4))
                        viewPreset3D=3;

                    if (key(ImGuiKey_Z))
                    {
                        renderMode=
                            (renderMode+1)%3;
                    }
                }
                else if (viewportMode==2)
                {
                    if (key(ImGuiKey_P))
                    {
                        arEditorPreview=
                            !arEditorPreview;

                        if (arEditorPreview)
                        {
                            arSessionRunning=false;
                            arRecording=false;
                        }
                    }

                    if (key(ImGuiKey_A))
                    {
                        arAnchorCount=
                            std::min(
                                8,
                                arAnchorCount+1
                            );
                    }

                    if (key(ImGuiKey_C))
                    {
                        arAnchorCount=0;
                    }

                    if (key(ImGuiKey_Escape))
                    {
                        arEditorPreview=false;
                        arRecording=false;
                    }
                }
            }

            if (io.KeyShift &&
                !io.KeyCtrl &&
                !io.KeyAlt &&
                key(ImGuiKey_F))
            {
                showSafeFrame=
                    !showSafeFrame;
            }
        }
    }
    ImGui::BeginChild(
        "ViewportModeStrip",
        ImVec2(0.0f,52.0f),
        true,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    if (editorButton(
        "2D PLAN",
        96.0f,
        viewportMode==0,
        true))
    {
        viewportMode=0;
    }

    ImGui::SameLine(0.0f,6.0f);

    if (editorButton(
        "3D SCENE",
        104.0f,
        viewportMode==1,
        true))
    {
        viewportMode=1;
    }

    ImGui::SameLine(0.0f,6.0f);

    if (editorButton(
        "AR PREVIEW",
        112.0f,
        viewportMode==2,
        true))
    {
        viewportMode=2;
    }

    if (viewportMode==2)
    {
        ImGui::SameLine(0.0f,18.0f);

        if (arEditorPreview)
        {
            ImGui::TextColored(
                ImVec4(0.92f,0.69f,0.18f,1.0f),
                "EDITOR PREVIEW"
            );
        }
        else if (arDeviceConnected && arSessionRunning)
        {
            ImGui::TextColored(
                ImVec4(0.48f,0.84f,0.46f,1.0f),
                "LIVE"
            );
        }
        else
        {
            ImGui::TextDisabled("OFFLINE");
        }

        ImGui::SameLine(0.0f,14.0f);

        if (editorButton(
            arEditorPreview
                ? "EXIT PREVIEW"
                : "EDITOR PREVIEW",
            arEditorPreview
                ? 110.0f
                : 126.0f,
            arEditorPreview,
            true))
        {
            arEditorPreview=!arEditorPreview;

            if (arEditorPreview)
            {
                arSessionRunning=false;
                arRecording=false;
            }
            else
            {
                arRecording=false;
            }
        }

        ImGui::SameLine(0.0f,6.0f);

        if (editorButton(
            "CONNECT DEVICE",
            126.0f,
            false,
            true))
        {
            ImGui::OpenPopup(
                "##CompactARDevicePopup"
            );
        }

        if (ImGui::BeginPopup(
            "##CompactARDevicePopup"))
        {
            ImGui::Text("AR DEVICE");
            ImGui::Separator();

            ImGui::TextDisabled(
                "Live device transport is not wired yet."
            );

            const char* profiles[]={
                "Android Companion",
                "iPhone / iPad",
                "External AR Camera"
            };

            ImGui::SetNextItemWidth(220.0f);

            ImGui::Combo(
                "Device profile",
                &arDeviceProfile,
                profiles,
                3
            );

            ImGui::Spacing();
            ImGui::Separator();

            if (ImGui::MenuItem(
                "Use Editor Preview"))
            {
                arEditorPreview=true;
                arDeviceConnected=false;
                arSessionRunning=false;
                arRecording=false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine(0.0f,14.0f);
        ImGui::TextDisabled("OPACITY");
        ImGui::SameLine(0.0f,6.0f);

        ImGui::SetNextItemWidth(108.0f);

        ImGui::SliderFloat(
            "##AROpacityCompact",
            &arOpacity,
            0.10f,
            1.00f,
            "%.2f"
        );
    }
    else if (viewportMode==1)
    {
        ImGui::SameLine(0.0f,18.0f);
        ImGui::TextDisabled("DISPLAY");
        ImGui::SameLine(0.0f,6.0f);

        const char* renderModes[]={
            "Lit",
            "Wireframe",
            "Analysis"
        };

        ImGui::SetNextItemWidth(118.0f);

        ImGui::Combo(
            "##CompactRenderMode",
            &renderMode,
            renderModes,
            3
        );
    }
    else
    {
        ImGui::SameLine(0.0f,18.0f);
        ImGui::TextDisabled("VIEW");
        ImGui::SameLine(0.0f,6.0f);

        const char* orthoModes[]={
            "Top",
            "Front",
            "Right"
        };

        ImGui::SetNextItemWidth(112.0f);

        ImGui::Combo(
            "##CompactOrthoMode",
            &orthoView,
            orthoModes,
            3
        );
    }

    
    // SOVEREIGN_VISIBLE_FULLSCREEN_EXIT_V1
    // This lives INSIDE ViewportModeStrip, so it cannot disappear
    // behind the 2D / 3D / AR canvas child.
    if (gViewportFullscreen)
    {
        const ImVec2 modeStripWindowPos=
            ImGui::GetWindowPos();

        const float exitFullscreenW=
            172.0f;

        ImGui::SetCursorScreenPos(
            ImVec2(
                modeStripWindowPos.x+
                    ImGui::GetWindowContentRegionMax().x-
                    exitFullscreenW-
                    8.0f,
                modeStripWindowPos.y+
                    8.0f
            )
        );

        if (editorButton(
            "EXIT FULL SCREEN",
            exitFullscreenW,
            true,
            true))
        {
            gViewportFullscreen=false;
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Exit %s full screen | F11 or Esc",
                viewportMode==0
                    ? "2D Plan"
                    : (
                        viewportMode==1
                            ? "3D Scene"
                            : "AR Preview"
                      )
            );
        }
    }
ImGui::EndChild();
    if (viewportMode==2)
    {
        ImGui::Spacing();

        ImGui::BeginChild(
            "ViewportARCompactTools",
            ImVec2(0.0f,52.0f),
            true,
            ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
        );

        ImGui::TextDisabled("PLACEMENT");
        ImGui::SameLine(0.0f,7.0f);

        const char* placementModes[]={
            "Origin",
            "Surface",
            "Vehicle"
        };

        ImGui::SetNextItemWidth(112.0f);

        ImGui::Combo(
            "##CompactARPlacement",
            &arPlacementMode,
            placementModes,
            3
        );

        ImGui::SameLine(0.0f,14.0f);
        ImGui::TextDisabled("TRACKING");
        ImGui::SameLine(0.0f,7.0f);

        const char* trackingModes[]={
            "Poor",
            "Limited",
            "Good"
        };

        ImGui::SetNextItemWidth(104.0f);

        ImGui::Combo(
            "##CompactARTracking",
            &arTrackingQuality,
            trackingModes,
            3
        );

        ImGui::SameLine(0.0f,12.0f);

        ImGui::Checkbox(
            "Occlusion",
            &arOcclusion
        );

        ImGui::SameLine(0.0f,9.0f);

        ImGui::Checkbox(
            "Planes",
            &arPlaneMesh
        );

        ImGui::SameLine(0.0f,9.0f);

        ImGui::Checkbox(
            "Reticle",
            &arReticle
        );

        ImGui::SameLine(0.0f,12.0f);

        const bool canPlaceAnchor=
            arEditorPreview |
            (arDeviceConnected &&
             arSessionRunning);

        if (editorButton(
            "PLACE ANCHOR",
            118.0f,
            true,
            canPlaceAnchor))
        {
            arAnchorCount=
                std::min(
                    8,
                    arAnchorCount+1
                );
        }

        ImGui::SameLine(0.0f,7.0f);

        if (editorButton(
            "MORE  v",
            86.0f,
            false,
            true))
        {
            ImGui::OpenPopup(
                "##CompactARMorePopup"
            );
        }

        if (ImGui::BeginPopup(
            "##CompactARMorePopup"))
        {
            ImGui::TextDisabled(
                "AR SESSION"
            );

            ImGui::Separator();

            if (ImGui::MenuItem(
                "Reset Origin"))
            {
                arPlacementMode=0;
                arAnchorCount=0;
            }

            if (ImGui::MenuItem(
                "Clear Anchors",
                nullptr,
                false,
                arAnchorCount>0))
            {
                arAnchorCount=0;
            }

            ImGui::Separator();

            if (arDeviceConnected)
            {
                if (!arSessionRunning)
                {
                    if (ImGui::MenuItem(
                        "Start Session"))
                    {
                        arSessionRunning=true;
                        arEditorPreview=false;
                    }
                }
                else
                {
                    if (ImGui::MenuItem(
                        "Stop Session"))
                    {
                        arSessionRunning=false;
                        arRecording=false;
                    }
                }
            }
            else
            {
                ImGui::MenuItem(
                    "Start Session",
                    nullptr,
                    false,
                    false
                );
            }

            const bool canRecord=
                arEditorPreview |
                (arDeviceConnected &&
                 arSessionRunning);

            if (ImGui::MenuItem(
                arRecording
                    ? "Stop Recording"
                    : "Record",
                nullptr,
                false,
                canRecord))
            {
                arRecording=!arRecording;
            }

            ImGui::Separator();

            if (ImGui::BeginMenu(
                "Overlays"))
            {
                roadSafeMenuItemToggle("Grid",UiGlyph::Grid,nullptr,&showGrid);

                roadSafeMenuItemToggle("Axes",UiGlyph::Axes,nullptr,&showAxes);

                roadSafeMenuItemToggle("Bounds",UiGlyph::Bounds,nullptr,&showBounds);

                roadSafeMenuItemToggle("Measurements",UiGlyph::Measurement,nullptr,&showMeasurements);

                roadSafeMenuItemToggle("Object Names",UiGlyph::ObjectNames,nullptr,&showNames);

                roadSafeMenuItemToggle("Statistics",UiGlyph::Statistics,nullptr,&showStats);

                roadSafeMenuItemToggle("Safe Frame",UiGlyph::SafeFrame,nullptr,&showSafeFrame);

                ImGui::Separator();

                roadSafeMenuItemToggle("AR Anchors",UiGlyph::Anchor,nullptr,&arShowAnchors);

                roadSafeMenuItemToggle("Collision Guide",UiGlyph::CollisionGuide,nullptr,&arShowCollisionGuide);

                ImGui::EndMenu();
            }

            ImGui::Separator();

            ImGui::TextDisabled(
                "Shortcuts: P Preview   A Anchor   C Clear"
            );

            ImGui::EndPopup();
        }

        
        ImGui::SameLine(0.0f,7.0f);

        if (!gViewportFullscreen &&
            editorButton(
                "FULL SCREEN",
                126.0f,
                false,
                true))
        {
            setViewportFullscreen(true);
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "AR Preview full screen | F11"
            );
        }
ImGui::EndChild();
    }
    ImGui::Spacing();

    if (viewportMode!=2)
    {ImGui::BeginChild(
        "ViewportSecondaryToolbar",
        ImVec2(0.0f,48.0f),
        true,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    // --------------------------------------------------------
    // MODE-SPECIFIC VIEW CONTROLS
    // --------------------------------------------------------

    if (viewportMode==0)
    {
        ImGui::TextDisabled("2D");

        ImGui::SameLine(0.0f,10.0f);

        if (editorButton("FIT",52.0f))
            viewportZoom=1.0f;

        ImGui::SameLine(0.0f,6.0f);

        if (editorButton("ZOOM OUT",30.0f))
            viewportZoom=std::max(0.25f,viewportZoom-0.10f);

        ImGui::SameLine(0.0f,4.0f);

        ImGui::SetNextItemWidth(90.0f);
        ImGui::SliderFloat(
            "##ViewportZoom2D",
            &viewportZoom,
            0.25f,
            3.0f,
            "%.2fx"
        );

        ImGui::SameLine(0.0f,4.0f);

        if (editorButton("ZOOM IN",30.0f))
            viewportZoom=std::min(3.0f,viewportZoom+0.10f);

        ImGui::SameLine(0.0f,12.0f);
        editorButton("FRAME ALL",92.0f);
    }
    else if (viewportMode==1)
    {
        ImGui::TextDisabled("CAMERA");

        ImGui::SameLine(0.0f,10.0f);

        const char* speedLabels[]={
            "Very Slow",
            "Slow",
            "Normal",
            "Fast",
            "Very Fast"
        };

        ImGui::SetNextItemWidth(112.0f);
        ImGui::Combo(
            "##ViewportCameraSpeed",
            &cameraSpeed,
            speedLabels,
            5
        );

        ImGui::SameLine(0.0f,12.0f);
        editorButton("FRAME SELECT",116.0f);

        ImGui::SameLine(0.0f,6.0f);
        editorButton("FRAME ALL",92.0f);
    }
    else
    {
        ImGui::TextDisabled("AR SESSION");

        ImGui::SameLine(0.0f,10.0f);

        editorButton(
            "PAIR DEVICE",
            100.0f,
            false,
            false
        );

        ImGui::SameLine(0.0f,6.0f);

        if (arRecord)
            editorButton("STOP RECORD",108.0f,true);
        else
            editorButton("RECORD",82.0f,false,false);

        ImGui::SameLine(0.0f,12.0f);

        if (editorButton("RESET ORIGIN",112.0f))
        {
            // AR origin reset hook.
        }
    }

    // --------------------------------------------------------
    // OVERLAYS MENU
    // --------------------------------------------------------

    ImGui::SameLine(0.0f,14.0f);

    const float overlaysButtonW =
        ImGui::CalcTextSize("OVERLAYS  v").x +
        (ImGui::GetStyle().FramePadding.x * 2.0f) +
        20.0f;

    if (ImGui::Button(
        "OVERLAYS  v",
        ImVec2(overlaysButtonW,30.0f)))
    {
        ImGui::OpenPopup("##ViewportOverlaysPopup");
    }
    if (!gViewportFullscreen)
    {
    ImGui::SameLine(0.0f,10.0f);

    const char* viewportFullscreenButtonLabel=
        gViewportFullscreen
            ? "EXIT FULL SCREEN"
            : "FULL SCREEN";

    if (editorButton(
        viewportFullscreenButtonLabel,
        gViewportFullscreen
            ? 156.0f
            : 126.0f,
        gViewportFullscreen,
        true))
    {
        setViewportFullscreen(
            !gViewportFullscreen
        );
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "%s | F11 toggle | Esc exit",
            viewportMode==0
                ? "2D Plan full screen"
                : (
                    viewportMode==1
                        ? "3D Scene full screen"
                        : "AR Preview full screen"
                  )
        );
    }
    }

    if (ImGui::BeginPopup("##ViewportOverlaysPopup"))
    {
        ImGui::TextDisabled("VIEWPORT OVERLAYS");
        ImGui::Separator();

        roadSafeMenuItemToggle("Grid",UiGlyph::Grid,nullptr,&showGrid);

        roadSafeMenuItemToggle("Axes",UiGlyph::Axes,nullptr,&showAxes);

        ImGui::MenuItem(
            "Object Bounds",
            nullptr,
            &showBounds
        );

        roadSafeMenuItemToggle("Measurements",UiGlyph::Measurement,nullptr,&showMeasurements);

        roadSafeMenuItemToggle("Object Names",UiGlyph::ObjectNames,nullptr,&showNames);

        roadSafeMenuItemToggle("Statistics",UiGlyph::Statistics,nullptr,&showStats);

        roadSafeMenuItemToggle("Safe Frame",UiGlyph::SafeFrame,nullptr,&showSafeFrame);

        if (viewportMode==2)
        {
            ImGui::Separator();

            roadSafeMenuItemToggle("AR Anchors",UiGlyph::Anchor,nullptr,&arShowAnchors);

            roadSafeMenuItemToggle("Collision Guide",UiGlyph::CollisionGuide,nullptr,&arShowCollisionGuide);
        }

        ImGui::EndPopup();
    }

    ImGui::EndChild();
    }
    if (viewportMode==1 | viewportMode==2)
    {
        ImGui::Spacing();

        if (viewportMode!=2)
    {ImGui::BeginChild(
            "Viewport3DARProStrip",
            ImVec2(0.0f,52.0f),
            true,
            ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
        );

        if (viewportMode==1)
        {
            const char* viewPresets[]={
                "Perspective",
                "Top",
                "Front",
                "Right"
            };

            const char* gizmoSpaces[]={
                "World",
                "Local"
            };

            ImGui::TextDisabled("VIEW");
            ImGui::SameLine(0.0f,8.0f);

            ImGui::SetNextItemWidth(122.0f);
            ImGui::Combo(
                "##Viewport3DViewPreset",
                &viewPreset3D,
                viewPresets,
                4
            );

            ImGui::SameLine(0.0f,12.0f);
            ImGui::TextDisabled("FOV");
            ImGui::SameLine(0.0f,7.0f);

            ImGui::SetNextItemWidth(108.0f);
            ImGui::SliderFloat(
                "##Viewport3DFov",
                &cameraFov,
                25.0f,
                110.0f,
                "%.0f deg"
            );

            ImGui::SameLine(0.0f,12.0f);
            ImGui::TextDisabled("GIZMO");
            ImGui::SameLine(0.0f,7.0f);

            ImGui::SetNextItemWidth(92.0f);
            ImGui::Combo(
                "##Viewport3DGizmoSpace",
                &gizmoSpace3D,
                gizmoSpaces,
                2
            );

            ImGui::SameLine(0.0f,14.0f);
            ImGui::Checkbox(
                "Shadow",
                &showGroundShadow
            );

            ImGui::SameLine(0.0f,10.0f);
            ImGui::Checkbox(
                "Nav Hints",
                &showNavigationHints
            );
        }
        else
        {
            const char* placementModes[]={
                "Origin",
                "Surface",
                "Vehicle"
            };

            const char* trackingStates[]={
                "Poor",
                "Limited",
                "Good"
            };

            ImGui::TextDisabled("PLACEMENT");
            ImGui::SameLine(0.0f,8.0f);

            ImGui::SetNextItemWidth(110.0f);
            ImGui::Combo(
                "##ARPlacementMode",
                &arPlacementMode,
                placementModes,
                3
            );

            ImGui::SameLine(0.0f,12.0f);
            ImGui::TextDisabled("TRACKING");
            ImGui::SameLine(0.0f,7.0f);

            ImGui::SetNextItemWidth(96.0f);
            ImGui::Combo(
                "##ARTrackingQuality",
                &arTrackingQuality,
                trackingStates,
                3
            );

            ImGui::SameLine(0.0f,12.0f);
            ImGui::Checkbox(
                "Occlusion",
                &arOcclusion
            );

            ImGui::SameLine(0.0f,10.0f);
            ImGui::Checkbox(
                "Planes",
                &arPlaneMesh
            );

            ImGui::SameLine(0.0f,10.0f);
            ImGui::Checkbox(
                "Reticle",
                &arReticle
            );

            ImGui::SameLine(0.0f,12.0f);

            if (editorButton(
                "PLACE ANCHOR",
                118.0f,
                true,
                true))
            {
                arAnchorCount=
                    std::min(
                        8,
                        arAnchorCount+1
                    );
            }

            ImGui::SameLine(0.0f,6.0f);

            if (editorButton(
                "CLEAR",
                64.0f,
                false,
                arAnchorCount>0))
            {
                arAnchorCount=0;
            }
        }

        ImGui::EndChild();
    }
    if (viewportMode==2)
    {
        ImGui::Spacing();

        // Compact AR toolbar owns session/device/record controls.
    }
    }
    ImGui::Spacing();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(7.0f,7.0f));
    ImGui::BeginChild("ViewportToolRail",ImVec2(112.0f,0.0f),true,ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);
    bool clicked=false;
    drawRailButton("select","SELECT",0,selectedTool==0,&clicked); if (clicked) selectedTool=0;
    ImGui::BeginDisabled(viewportSelectionLocked);
    drawRailButton("move","MOVE",1,selectedTool==1,&clicked);
    if (clicked)
        selectedTool=1;
    drawRailButton("rotate","ROTATE",2,selectedTool==2,&clicked);
    if (clicked)
        selectedTool=2;
    drawRailButton("scale","SCALE",3,selectedTool==3,&clicked);
    if (clicked)
        selectedTool=3;
    ImGui::EndDisabled();
    ImGui::Separator();
    drawRailButton("vehicle","VEHICLE",4,false,&clicked);
    drawRailButton("evidence","EVIDENCE",5,false,&clicked);
    drawRailButton("measure","MEASURE",6,false,&clicked);
    ImGui::Separator();
    drawRailButton("grid","GRID",7,showGrid,&clicked); if (clicked) showGrid=!showGrid;
    drawRailButton("axes","AXES",8,showAxes,&clicked); if (clicked) showAxes=!showAxes;
    ImGui::EndChild();

    ImGui::SameLine(0.0f,6.0f);
    ImGui::BeginChild("SceneCanvas",ImGui::GetContentRegionAvail(),true,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    const ImVec2 cp=ImGui::GetWindowPos(), cs=ImGui::GetWindowSize();
    ImDrawList* d=ImGui::GetWindowDrawList();

    bool roadSafe3DFrameReady=false;

    if (viewportMode==1 &&
        cs.x>8.0f &&
        cs.y>8.0f)
    {
        roadsafe::RenderSettings roadSafeRenderSettings;
        roadSafeRenderSettings.width=
            static_cast<int>(
                cs.x
            );
        roadSafeRenderSettings.height=
            static_cast<int>(
                cs.y
            );
        roadSafeRenderSettings.fovDegrees=
            cameraFov;
        roadSafeRenderSettings.viewPreset=
            viewPreset3D;
        roadSafeRenderSettings.renderMode=
            renderMode;
        roadSafeRenderSettings.selectedEntityId=
            shellSelectedEntityId();

        if (gRoadSafeRenderer.render(
                gRoadSafeCase,
                roadSafeRenderSettings,
                std::filesystem::path(
                    SFE_ASSET_DIR
                )))
        {
            const GLuint roadSafeTexture=
                gRoadSafeRenderer.colorTexture();

            if (roadSafeTexture!=0)
            {
                d->AddImage(
                    (ImTextureID)(intptr_t)roadSafeTexture,
                    cp,
                    ImVec2(
                        cp.x+cs.x,
                        cp.y+cs.y
                    ),
                    ImVec2(0.0f,1.0f),
                    ImVec2(1.0f,0.0f)
                );

                roadSafe3DFrameReady=true;
            }
        }
    }

    if (!roadSafe3DFrameReady)
    {
        d->AddRectFilled(
            cp,
            ImVec2(cp.x+cs.x,cp.y+cs.y),
            viewportMode==0
                ? IM_COL32(18,20,22,255)
                : (viewportMode==1
                    ? IM_COL32(22,25,30,255)
                    : IM_COL32(27,31,37,255))
        );
    }
    if (viewportMode==0 && showGrid)
    {
        const float gs=32.0f;
        for (float x=cp.x;x<cp.x+cs.x;x+=gs) d->AddLine(ImVec2(x,cp.y),ImVec2(x,cp.y+cs.y),IM_COL32(49,52,55,255));
        for (float y=cp.y;y<cp.y+cs.y;y+=gs) d->AddLine(ImVec2(cp.x,y),ImVec2(cp.x+cs.x,y),IM_COL32(49,52,55,255));
    }
    const ImVec2 center(cp.x+cs.x*.5f,cp.y+cs.y*.5f);
    d->AddCircleFilled(center,7.0f,IM_COL32(238,174,38,255));
    if (viewportMode==0 && showAxes)
    {
        d->AddLine(center,ImVec2(center.x+100.0f,center.y),IM_COL32(190,65,55,255),2.0f);
        d->AddLine(center,ImVec2(center.x,center.y-100.0f),IM_COL32(70,145,80,255),2.0f);
        d->AddText(ImVec2(center.x+105.0f,center.y-10.0f),IM_COL32(220,90,75,255),"X");
        d->AddText(ImVec2(center.x+7.0f,center.y-120.0f),IM_COL32(100,190,110,255),"Y");
    }
    d->AddText(ImVec2(cp.x+16.0f,cp.y+16.0f),IM_COL32(225,226,228,255),viewportMode==0 ? "2D PLAN VIEW" : (viewportMode==1 ? "3D SCENE VIEW" : "AR PREVIEW"));
    d->AddText(ImVec2(cp.x+16.0f,cp.y+40.0f),IM_COL32(135,139,145,255),viewportMode==0 ? (orthoView==0 ? "Top orthographic reconstruction" : (orthoView==1 ? "Front orthographic reconstruction" : "Right orthographic reconstruction")) : (viewportMode==1 ? (renderMode==0 ? "Perspective lit reconstruction" : (renderMode==1 ? "Wireframe inspection" : "Analysis overlay")) : "Editor AR preview - live device integration pending"));
    
    if (viewportMode==1 &&
        !roadSafe3DFrameReady)
    {
        const float horizon=
            cp.y+cs.y*0.34f;

        d->AddLine(
            ImVec2(cp.x+22.0f,horizon),
            ImVec2(cp.x+cs.x-22.0f,horizon),
            IM_COL32(77,81,87,190),
            1.0f
        );

        const ImVec2 vanish(
            center.x,
            horizon+20.0f
        );

        for (int i=0;i<=14;++i)
        {
            const float t=
                static_cast<float>(i)/14.0f;

            const float x=
                cp.x+26.0f+
                (cs.x-52.0f)*t;

            d->AddLine(
                ImVec2(
                    x,
                    cp.y+cs.y-22.0f
                ),
                vanish,
                IM_COL32(62,66,72,155),
                1.0f
            );
        }

        for (int i=0;i<9;++i)
        {
            const float t=
                static_cast<float>(i)/8.0f;

            const float y=
                horizon+
                30.0f+
                (t*t)*(cs.y*.54f);

            d->AddLine(
                ImVec2(cp.x+30.0f,y),
                ImVec2(cp.x+cs.x-30.0f,y),
                IM_COL32(62,66,72,145),
                1.0f
            );
        }


        if (showAxes)
        {
            const ImVec2 gizmo(
                cp.x+82.0f,
                cp.y+cs.y-76.0f
            );

            d->AddLine(
                gizmo,
                ImVec2(gizmo.x+44.0f,gizmo.y),
                IM_COL32(190,65,55,255),
                2.0f
            );

            d->AddLine(
                gizmo,
                ImVec2(gizmo.x,gizmo.y-44.0f),
                IM_COL32(70,145,80,255),
                2.0f
            );

            d->AddLine(
                gizmo,
                ImVec2(gizmo.x-28.0f,gizmo.y+22.0f),
                IM_COL32(74,116,195,255),
                2.0f
            );

            d->AddText(
                ImVec2(gizmo.x+49.0f,gizmo.y-8.0f),
                IM_COL32(220,90,75,255),
                "X"
            );

            d->AddText(
                ImVec2(gizmo.x+5.0f,gizmo.y-56.0f),
                IM_COL32(100,190,110,255),
                "Y"
            );

            d->AddText(
                ImVec2(gizmo.x-40.0f,gizmo.y+18.0f),
                IM_COL32(110,155,230,255),
                "Z"
            );
        }
    }
    else if (viewportMode==2)
    {
        const ImVec2 frameMin(
            cp.x+36.0f,
            cp.y+30.0f
        );

        const ImVec2 frameMax(
            cp.x+cs.x-36.0f,
            cp.y+cs.y-30.0f
        );

        d->AddRect(
            frameMin,
            frameMax,
            IM_COL32(93,100,110,225),
            12.0f,
            0,
            1.5f
        );

        const ImVec2 targetMin(
            center.x-132.0f,
            center.y-72.0f
        );

        const ImVec2 targetMax(
            center.x+132.0f,
            center.y+72.0f
        );

        const int arAlpha=
            static_cast<int>(
                220.0f*arOpacity
            );

        d->AddRect(
            targetMin,
            targetMax,
            IM_COL32(95,195,225,arAlpha),
            6.0f,
            0,
            2.0f
        );

        d->AddCircle(
            ImVec2(
                center.x,
                center.y+101.0f
            ),
            17.0f,
            IM_COL32(95,195,225,215),
            24,
            2.0f
        );

        d->AddText(
            ImVec2(
                center.x+25.0f,
                center.y+92.0f
            ),
            IM_COL32(120,205,232,235),
            "Origin anchor"
        );

        d->AddRectFilled(
            ImVec2(
                frameMax.x-177.0f,
                frameMin.y+14.0f
            ),
            ImVec2(
                frameMax.x-18.0f,
                frameMin.y+42.0f
            ),
            IM_COL32(55,61,69,225),
            14.0f
        );

        d->AddText(
            ImVec2(
                frameMax.x-156.0f,
                frameMin.y+20.0f
            ),
            IM_COL32(218,222,228,255),
            "DEVICE OFFLINE"
        );
    }

    // ========================================================
    // VIEWPORT HUD / NAVIGATION GIZMO
    // ========================================================

    if (showStats && viewportMode!=2)
    {
        const ImVec2 hudMin(
            cp.x+cs.x-168.0f,
            cp.y+14.0f
        );

        const ImVec2 hudMax(
            cp.x+cs.x-14.0f,
            cp.y+98.0f
        );

        d->AddRectFilled(
            hudMin,
            hudMax,
            IM_COL32(22,25,29,220),
            5.0f
        );

        d->AddRect(
            hudMin,
            hudMax,
            IM_COL32(72,77,84,230),
            5.0f,
            0,
            1.0f
        );

        d->AddText(
            ImVec2(hudMin.x+10.0f,hudMin.y+10.0f),
            IM_COL32(215,219,225,255),
            viewportMode==0
                ? "2D PLAN"
                : (viewportMode==1
                    ? "3D SCENE"
                    : "AR PREVIEW")
        );

        d->AddText(
            ImVec2(hudMin.x+10.0f,hudMin.y+37.0f),
            IM_COL32(145,151,160,255),
            "Selection: None"
        );

        d->AddText(
            ImVec2(hudMin.x+10.0f,hudMin.y+64.0f),
            IM_COL32(145,151,160,255),
            "Objects: 0"
        );
    }

    if (showSafeFrame)
    {
        d->AddRect(
            ImVec2(cp.x+cs.x*.08f,cp.y+cs.y*.08f),
            ImVec2(cp.x+cs.x*.92f,cp.y+cs.y*.92f),
            IM_COL32(180,185,192,95),
            0.0f,
            0,
            1.0f
        );
    }

    if (showBounds && viewportMode!=2)
    {
        d->AddRect(
            ImVec2(center.x-104.0f,center.y-82.0f),
            ImVec2(center.x+104.0f,center.y+82.0f),
            IM_COL32(238,174,38,170),
            3.0f,
            0,
            1.5f
        );
    }

    if (showNames && viewportMode!=2)
    {
        d->AddText(
            ImVec2(center.x-36.0f,center.y+92.0f),
            IM_COL32(196,200,206,210),
            "Scene Origin"
        );
    }

    if (showMeasurements && viewportMode==0)
    {
        d->AddLine(
            ImVec2(center.x-118.0f,center.y+78.0f),
            ImVec2(center.x+118.0f,center.y+78.0f),
            IM_COL32(198,202,208,180),
            1.4f
        );

        d->AddText(
            ImVec2(center.x-19.0f,center.y+60.0f),
            IM_COL32(220,223,228,225),
            "4.8 m"
        );
    }

    if (viewportMode==1)
    {
        // Small orientation cube in upper-right, below HUD.
        const ImVec2 cubeCenter(
            cp.x+cs.x-72.0f,
            cp.y+126.0f
        );

        const float q=22.0f;

        d->AddRectFilled(
            ImVec2(cubeCenter.x-q,cubeCenter.y-q),
            ImVec2(cubeCenter.x+q,cubeCenter.y+q),
            IM_COL32(46,51,58,235),
            3.0f
        );

        d->AddRect(
            ImVec2(cubeCenter.x-q,cubeCenter.y-q),
            ImVec2(cubeCenter.x+q,cubeCenter.y+q),
            IM_COL32(125,132,142,235),
            3.0f,
            0,
            1.2f
        );

        d->AddText(
            ImVec2(cubeCenter.x-5.0f,cubeCenter.y-9.0f),
            IM_COL32(224,226,230,255),
            "F"
        );

        d->AddText(
            ImVec2(cubeCenter.x-4.0f,cubeCenter.y-q-18.0f),
            IM_COL32(112,220,128,255),
            "Y"
        );

        d->AddText(
            ImVec2(cubeCenter.x+q+7.0f,cubeCenter.y-8.0f),
            IM_COL32(220,110,90,255),
            "X"
        );
    }

    if (viewportMode==2 && arShowAnchors)
    {
        d->AddText(
            ImVec2(cp.x+18.0f,cp.y+cs.y-52.0f),
            IM_COL32(112,205,232,220),
            "AR Anchors: Visible"
        );
    }

    if (viewportMode==2 && arShowCollisionGuide)
    {
        d->AddRect(
            ImVec2(center.x-160.0f,center.y+116.0f),
            ImVec2(center.x+160.0f,center.y+154.0f),
            IM_COL32(238,174,38,185),
            4.0f,
            0,
            1.4f
        );

        d->AddText(
            ImVec2(center.x-107.0f,center.y+126.0f),
            IM_COL32(240,194,100,225),
            "Estimated collision corridor"
        );
    }

    // ========================================================
    
    // ========================================================
    // VIEWPORT PHASE 3 - PROFESSIONAL 3D / AR VISUAL LAYER
    // ========================================================

    if (viewportMode==1)
    {


        // ----------------------------------------------------
                // Clean empty-state. Real scene objects should be rendered by the
        // actual scene/renderer path, not fabricated here in ImGui.
        {
            const char* emptyTitle="No 3D scene objects loaded";
            const char* emptyNote="Add or import scene objects to begin reconstruction.";

            const ImVec2 titleSize=
                ImGui::CalcTextSize(emptyTitle);

            const ImVec2 noteSize=
                ImGui::CalcTextSize(emptyNote);

            d->AddText(
                ImVec2(
                    center.x-titleSize.x*0.5f,
                    center.y-18.0f
                ),
                IM_COL32(205,209,215,215),
                emptyTitle
            );

            d->AddText(
                ImVec2(
                    center.x-noteSize.x*0.5f,
                    center.y+10.0f
                ),
                IM_COL32(132,138,147,210),
                emptyNote
            );
        }
        // 3D: camera information card
        // ----------------------------------------------------

        const ImVec2 camCardMin(
            cp.x+16.0f,
            cp.y+cs.y-118.0f
        );

        const ImVec2 camCardMax(
            cp.x+276.0f,
            cp.y+cs.y-16.0f
        );

        d->AddRectFilled(
            camCardMin,
            camCardMax,
            IM_COL32(22,25,29,225),
            5.0f
        );

        d->AddRect(
            camCardMin,
            camCardMax,
            IM_COL32(68,74,82,225),
            5.0f,
            0,
            1.0f
        );

        d->AddText(
            ImVec2(
                camCardMin.x+12.0f,
                camCardMin.y+10.0f
            ),
            IM_COL32(220,223,228,255),
            "CAMERA"
        );

        const char* presetLabel =
            viewPreset3D==0
                ? "Perspective"
                : (viewPreset3D==1
                    ? "Top"
                    : (viewPreset3D==2
                        ? "Front"
                        : "Right"));

        d->AddText(
            ImVec2(
                camCardMin.x+12.0f,
                camCardMin.y+36.0f
            ),
            IM_COL32(150,156,166,255),
            presetLabel
        );

        char fovText[64]{};
        std::snprintf(
            fovText,
            sizeof(fovText),
            "FOV %.0f deg  |  %s space",
            cameraFov,
            gizmoSpace3D==0
                ? "World"
                : "Local"
        );

        d->AddText(
            ImVec2(
                camCardMin.x+12.0f,
                camCardMin.y+60.0f
            ),
            IM_COL32(150,156,166,255),
            fovText
        );

        if (showNavigationHints)
        {
            d->AddText(
                ImVec2(
                    camCardMin.x+12.0f,
                    camCardMin.y+82.0f
                ),
                IM_COL32(115,121,130,255),
                "RMB Look   MMB Pan   Wheel Zoom   F Frame"
            );
        }
    }
    else if (viewportMode==2)
    {
        // ----------------------------------------------------
        // AR: simulated camera vignette
        // ----------------------------------------------------

        d->AddRectFilledMultiColor(
            cp,
            ImVec2(
                cp.x+cs.x,
                cp.y+cs.y
            ),
            IM_COL32(8,10,13,76),
            IM_COL32(8,10,13,76),
            IM_COL32(8,10,13,120),
            IM_COL32(8,10,13,120)
        );

        // ----------------------------------------------------
        // AR: center placement reticle
        // ----------------------------------------------------

        if (arReticle)
        {
            const float r=24.0f;

            d->AddCircle(
                center,
                r,
                IM_COL32(104,205,231,225),
                32,
                1.8f
            );

            d->AddLine(
                ImVec2(center.x-r-9.0f,center.y),
                ImVec2(center.x-r+6.0f,center.y),
                IM_COL32(104,205,231,225),
                1.8f
            );

            d->AddLine(
                ImVec2(center.x+r-6.0f,center.y),
                ImVec2(center.x+r+9.0f,center.y),
                IM_COL32(104,205,231,225),
                1.8f
            );

            d->AddCircleFilled(
                center,
                3.5f,
                IM_COL32(104,205,231,235)
            );
        }

        // ----------------------------------------------------
        // AR: detected plane mesh
        // ----------------------------------------------------

        if (arPlaneMesh)
        {
            const float planeY=
                center.y+112.0f;

            for (int i=-5;i<=5;++i)
            {
                const float x=
                    center.x+
                    static_cast<float>(i)*42.0f;

                d->AddLine(
                    ImVec2(
                        x-100.0f,
                        planeY+80.0f
                    ),
                    ImVec2(
                        center.x+
                        static_cast<float>(i)*18.0f,
                        planeY-24.0f
                    ),
                    IM_COL32(92,170,196,76),
                    1.0f
                );
            }

            for (int j=0;j<5;++j)
            {
                const float y=
                    planeY+
                    static_cast<float>(j)*18.0f;

                d->AddLine(
                    ImVec2(
                        center.x-220.0f+
                        static_cast<float>(j)*18.0f,
                        y
                    ),
                    ImVec2(
                        center.x+220.0f-
                        static_cast<float>(j)*18.0f,
                        y
                    ),
                    IM_COL32(92,170,196,70),
                    1.0f
                );
            }
        }

        // ----------------------------------------------------
        // AR: anchor points
        // ----------------------------------------------------

        for (int i=0;i<arAnchorCount;++i)
        {
            const float offsetX=
                static_cast<float>(
                    (i%4)-1
                )*82.0f;

            const float offsetY=
                static_cast<float>(
                    (i/4)
                )*58.0f;

            const ImVec2 a(
                center.x+offsetX,
                center.y+92.0f+offsetY
            );

            d->AddCircleFilled(
                a,
                6.0f,
                IM_COL32(104,205,231,235)
            );

            d->AddCircle(
                a,
                14.0f,
                IM_COL32(104,205,231,190),
                24,
                1.5f
            );

            char anchorLabel[32]{};
            std::snprintf(
                anchorLabel,
                sizeof(anchorLabel),
                "A%d",
                i+1
            );

            d->AddText(
                ImVec2(
                    a.x+18.0f,
                    a.y-8.0f
                ),
                IM_COL32(150,219,239,235),
                anchorLabel
            );
        }

        // ----------------------------------------------------
        d->AddText(
            ImVec2(
                cp.x+cs.x-266.0f,
                cp.y+126.0f
            ),
            IM_COL32(116,122,131,210),
            "AR PLACEMENT / SESSION OVERVIEW"
        );
        // AR: right-side overview panel
        // ----------------------------------------------------

        const ImVec2 arCardMin(
            cp.x+cs.x-324.0f,
            cp.y+150.0f
        );

        const ImVec2 arCardMax(
            cp.x+cs.x-18.0f,
            cp.y+430.0f
        );

        d->AddRectFilled(
            arCardMin,
            arCardMax,
            IM_COL32(22,25,29,228),
            6.0f
        );

        d->AddRect(
            arCardMin,
            arCardMax,
            IM_COL32(68,74,82,225),
            6.0f,
            0,
            1.0f
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+12.0f
            ),
            IM_COL32(220,223,228,255),
            "AR OVERVIEW"
        );

        d->AddLine(
            ImVec2(arCardMin.x+14.0f,arCardMin.y+36.0f),
            ImVec2(arCardMax.x-14.0f,arCardMin.y+36.0f),
            IM_COL32(56,61,69,220),
            1.0f
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+52.0f
            ),
            IM_COL32(145,151,160,255),
            "Mode"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+52.0f
            ),
            IM_COL32(214,217,222,255),
            arEditorPreview
                ? "Editor Preview"
                : (arDeviceConnected && arSessionRunning
                    ? "Live Session"
                    : "Offline")
        );

        const char* trackingLabel =
            arTrackingQuality==2
                ? "Good"
                : (arTrackingQuality==1
                    ? "Limited"
                    : "Poor");

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+82.0f
            ),
            IM_COL32(145,151,160,255),
            "Tracking"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+82.0f
            ),
            arTrackingQuality==2
                ? IM_COL32(120,205,140,255)
                : IM_COL32(230,185,92,255),
            trackingLabel
        );

        const char* placementLabel =
            arPlacementMode==0
                ? "Origin"
                : (arPlacementMode==1
                    ? "Surface"
                    : "Vehicle");

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+112.0f
            ),
            IM_COL32(145,151,160,255),
            "Placement"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+112.0f
            ),
            IM_COL32(214,217,222,255),
            placementLabel
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+142.0f
            ),
            IM_COL32(145,151,160,255),
            "Anchors"
        );

        char anchorsText[32]{};
        std::snprintf(
            anchorsText,
            sizeof(anchorsText),
            "%d",
            arAnchorCount
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+142.0f
            ),
            IM_COL32(214,217,222,255),
            anchorsText
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+172.0f
            ),
            IM_COL32(145,151,160,255),
            "Occlusion"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+172.0f
            ),
            IM_COL32(214,217,222,255),
            arOcclusion ? "On" : "Off"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+202.0f
            ),
            IM_COL32(145,151,160,255),
            "Planes"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+202.0f
            ),
            IM_COL32(214,217,222,255),
            arPlaneMesh ? "Visible" : "Hidden"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+232.0f
            ),
            IM_COL32(145,151,160,255),
            "Reticle"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+150.0f,
                arCardMin.y+232.0f
            ),
            IM_COL32(214,217,222,255),
            arReticle ? "Enabled" : "Disabled"
        );

        d->AddLine(
            ImVec2(arCardMin.x+14.0f,arCardMin.y+260.0f),
            ImVec2(arCardMax.x-14.0f,arCardMin.y+260.0f),
            IM_COL32(56,61,69,220),
            1.0f
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+276.0f
            ),
            IM_COL32(145,151,160,255),
            "READY STATE"
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+304.0f
            ),
            IM_COL32(115,121,130,255),
            "Use Editor Preview for layout and placement validation."
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+330.0f
            ),
            IM_COL32(115,121,130,255),
            "Connect a device later for live camera and tracking."
        );

        d->AddText(
            ImVec2(
                arCardMin.x+14.0f,
                arCardMin.y+360.0f
            ),
            IM_COL32(98,179,205,235),
            "Right side now carries live AR overview information."
        );
    }

    if (viewportMode==2)
    {
        const bool arFeedActive=
            arEditorPreview |
            (arDeviceConnected && arSessionRunning);

        if (!arFeedActive)
        {
            const float panelW=460.0f;
            const float panelH=146.0f;

            const ImVec2 panelMin(
                cp.x + cs.x*0.34f - panelW*0.5f,
                center.y - panelH*0.5f
            );

            const ImVec2 panelMax(
                cp.x + cs.x*0.34f + panelW*0.5f,
                center.y + panelH*0.5f
            );

            d->AddRectFilled(
                panelMin,
                panelMax,
                IM_COL32(22,25,29,238),
                7.0f
            );

            d->AddRect(
                panelMin,
                panelMax,
                IM_COL32(72,78,86,240),
                7.0f,
                0,
                1.0f
            );

            d->AddText(
                ImVec2(
                    panelMin.x+22.0f,
                    panelMin.y+20.0f
                ),
                IM_COL32(224,227,231,255),
                "NO AR SESSION"
            );

            d->AddText(
                ImVec2(
                    panelMin.x+22.0f,
                    panelMin.y+52.0f
                ),
                IM_COL32(148,154,164,255),
                "Connect a compatible device or use Editor Preview."
            );

            d->AddText(
                ImVec2(
                    panelMin.x+22.0f,
                    panelMin.y+78.0f
                ),
                IM_COL32(118,124,133,255),
                "Live camera tracking, anchors and occlusion will appear here."
            );

            d->AddText(
                ImVec2(
                    panelMin.x+22.0f,
                    panelMin.y+108.0f
                ),
                IM_COL32(100,182,209,235),
                "AR mode is ready for runtime integration."
            );
        }
        else
        {
            const ImVec2 statusMin(
                cp.x+18.0f,
                cp.y+18.0f
            );

            const ImVec2 statusMax(
                cp.x+276.0f,
                cp.y+108.0f
            );

            d->AddRectFilled(
                statusMin,
                statusMax,
                IM_COL32(22,25,29,225),
                5.0f
            );

            d->AddRect(
                statusMin,
                statusMax,
                IM_COL32(69,75,83,225),
                5.0f,
                0,
                1.0f
            );

            d->AddText(
                ImVec2(
                    statusMin.x+12.0f,
                    statusMin.y+10.0f
                ),
                IM_COL32(222,225,230,255),
                arEditorPreview
                    ? "EDITOR AR PREVIEW"
                    : "LIVE AR SESSION"
            );

            d->AddText(
                ImVec2(
                    statusMin.x+12.0f,
                    statusMin.y+36.0f
                ),
                IM_COL32(145,151,160,255),
                arEditorPreview
                    ? "Tracking: Simulated"
                    : (arTrackingQuality==2
                        ? "Tracking: Good"
                        : (arTrackingQuality==1
                            ? "Tracking: Limited"
                            : "Tracking: Poor"))
            );

            char runtimeText[96]{};
            std::snprintf(
                runtimeText,
                sizeof(runtimeText),
                "Anchors %d   Occlusion %s   Recording %s",
                arAnchorCount,
                arOcclusion ? "On" : "Off",
                arRecording ? "On" : "Off"
            );

            d->AddText(
                ImVec2(
                    statusMin.x+12.0f,
                    statusMin.y+61.0f
                ),
                IM_COL32(145,151,160,255),
                runtimeText
            );
        }
    }
// VIEWPORT CONTEXT MENU
    // ========================================================

    if (ImGui::BeginPopupContextWindow(
        "##ViewportContextMenu",
        ImGuiPopupFlags_MouseButtonRight |
        ImGuiPopupFlags_NoOpenOverItems))
    {
        ImGui::TextDisabled("VIEWPORT");
        ImGui::Separator();
        if (ImGui::MenuItem(
            gViewportFullscreen
                ? "Exit Full Screen"
                : "Enter Full Screen",
            "F11",
            gViewportFullscreen))
        {
            setViewportFullscreen(
                !gViewportFullscreen
            );
        }

        ImGui::Separator();

        ImGui::MenuItem(
            "Frame Selection",
            "F"
        );

        ImGui::MenuItem(
            "Frame All",
            "Home"
        );

        ImGui::Separator();

        roadSafeMenuItemToggle("Grid",UiGlyph::Grid,nullptr,&showGrid);

        roadSafeMenuItemToggle("Axes",UiGlyph::Axes,nullptr,&showAxes);

        roadSafeMenuItemToggle("Bounds",UiGlyph::Bounds,nullptr,&showBounds);

        roadSafeMenuItemToggle("Measurements",UiGlyph::Measurement,nullptr,&showMeasurements);

        ImGui::Separator();

        if (viewportMode==0)
        {
            if (roadSafeMenuItem("Top", UiGlyph::TopView))
                orthoView=0;

            if (roadSafeMenuItem("Front", UiGlyph::FrontView))
                orthoView=1;

            if (roadSafeMenuItem("Right", UiGlyph::RightView))
                orthoView=2;
        }
        else if (viewportMode==1)
        {
            if (roadSafeMenuItem("Lit", UiGlyph::Lit))
                renderMode=0;

            if (roadSafeMenuItem("Wireframe", UiGlyph::Wireframe))
                renderMode=1;

            if (roadSafeMenuItem("Analysis", UiGlyph::Analysis))
                renderMode=2;
        }
        else
        {
            roadSafeMenuItemToggle("AR Anchors",UiGlyph::Anchor,nullptr,&arShowAnchors);

            roadSafeMenuItemToggle("Collision Guide",UiGlyph::CollisionGuide,nullptr,&arShowCollisionGuide);
        }

        ImGui::EndPopup();
    }
ImGui::EndChild();
    ImGui::PopStyleVar();
    
ImGui::End();
    if (gViewportFullscreen)
    {
        const ImGuiViewport* exitViewport=
            ImGui::GetMainViewport();

        if (exitViewport)
        {
            ImGui::SetNextWindowPos(
                ImVec2(
                    exitViewport->Pos.x+
                        exitViewport->Size.x-
                        14.0f,
                    exitViewport->Pos.y+
                        12.0f
                ),
                ImGuiCond_Always,
                ImVec2(1.0f,0.0f)
            );

            ImGui::SetNextWindowViewport(
                exitViewport->ID
            );
        }

        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGui::PushStyleVar(
            ImGuiStyleVar_WindowPadding,
            ImVec2(0.0f,0.0f)
        );

        const ImGuiWindowFlags exitFlags=
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin(
            "##SovereignViewportFullscreenExit",
            nullptr,
            exitFlags
        );

        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(
                0.12f,
                0.42f,
                0.78f,
                1.0f
            )
        );

        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(
                0.15f,
                0.48f,
                0.86f,
                1.0f
            )
        );

        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(
                0.10f,
                0.36f,
                0.69f,
                1.0f
            )
        );

        const bool exitViewportFullscreen=
            ImGui::Button(
                "EXIT FULL SCREEN",
                ImVec2(
                    170.0f,
                    34.0f
                )
            );

        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Exit %s full screen | F11 or Esc",
                viewportMode==0
                    ? "2D Plan"
                    : (
                        viewportMode==1
                            ? "3D Scene"
                            : "AR Preview"
                      )
            );
        }

        ImGui::End();
        ImGui::PopStyleVar();

        if (exitViewportFullscreen)
        {
            setViewportFullscreen(false);
        }
    }

}

