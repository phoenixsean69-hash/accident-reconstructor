// ROADSAFE_UI_COMPONENT_V21
// Component: Outliner Panel
// Included from src/main.cpp; translation-unit behavior is unchanged.

static void drawOutliner()
{
    static bool groundVisible=true;
    static bool roadVisible=true;
    static bool vehicleAVisible=true;
    static bool vehicleBVisible=true;
    static bool skidVisible=true;
    static bool markerVisible=true;
    static bool debrisVisible=true;
    static bool distanceVisible=true;
    static bool angleVisible=true;

    static bool groundLocked=false;
    static bool roadLocked=true;
    static bool vehicleALocked=false;
    static bool vehicleBLocked=false;
    static bool skidLocked=false;
    static bool markerLocked=false;
    static bool debrisLocked=false;
    static bool distanceLocked=false;
    static bool angleLocked=false;

    ImGui::Begin(
        "Outliner",
        &gEditorShell.showOutliner,
        ImGuiWindowFlags_NoMove
    );
    // SOVEREIGN_PANEL_SCROLL_LIMITS_V1
    // Inspector-style panels never keep stale horizontal scroll.
    ImGui::SetScrollX(0.0f);
    beginEditorContextHeader(
        "##OutlinerContextHeader"
    );

    ImGui::TextDisabled("SCENE");

    editorContextSeparator();

    ImGui::Text(
        "%zu objects",
        roadSafeActiveSceneEntityCount()
    );

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

    // Header / filter.
    ImGui::Text("SCENE OUTLINER");
    ImGui::SameLine();

    const float addW=34.0f;
    ImGui::SetCursorPosX(
        std::max(
            ImGui::GetCursorPosX(),
            ImGui::GetWindowWidth()-addW-10.0f
        )
    );

    if (ImGui::SmallButton("+"))
        ImGui::OpenPopup("##OutlinerAddPopup");

    if (ImGui::BeginPopup("##OutlinerAddPopup"))
    {
        ImGui::TextDisabled("ADD OBJECT");
        ImGui::Separator();

        if (roadSafeMenuItem("Vehicle", UiGlyph::Vehicle))
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddVehicle
            );

        if (roadSafeMenuItem("Evidence", UiGlyph::Evidence))
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddEvidence
            );

        if (roadSafeMenuItem("Measurement", UiGlyph::Measurement))
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddMeasurement
            );

        if (roadSafeMenuItem("Scene Marker", UiGlyph::ForensicMarker))
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddMarker
            );

        ImGui::EndPopup();
    }

    ImGui::Separator();

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint(
        "##OutlinerSearch",
        "Filter scene...",
        gEditorShell.outlinerSearch,
        sizeof(gEditorShell.outlinerSearch)
    );

    ImGui::Spacing();

    if (ImGui::BeginTable(
        "SceneOutlinerTable",
            5,
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerH))
    {
        ImGui::TableSetupColumn(
            "OBJECT",
            ImGuiTableColumnFlags_WidthStretch,
            1.0f
        );

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
                    hp.x+16.0f,
                    hp.y+14.0f
                ),
                20.0f,
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


        const auto groupHasMatches=
            [](roadsafe::SceneEntityKind kind)
        {
            for (const auto& entity :
                 gRoadSafeCase.sceneEntities)
            {
                if (entity.active &&
                    entity.kind==kind &&
                    shellLabelMatches(
                        entity.name.c_str()
                    ))
                {
                    return true;
                }
            }

            return false;
        };

        const auto drawSceneGroup=
            [&](const char* label,
                roadsafe::SceneEntityKind kind)
        {
            if (!groupHasMatches(kind))
                return;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            const bool open=
                ImGui::TreeNodeEx(
                    label,
                    ImGuiTreeNodeFlags_DefaultOpen |
                    ImGuiTreeNodeFlags_SpanAvailWidth
                );

            if (!open)
                return;

            for (auto& entity :
                 gRoadSafeCase.sceneEntities)
            {
                if (!entity.active ||
                    entity.kind!=kind ||
                    !shellLabelMatches(
                        entity.name.c_str()
                    ))
                {
                    continue;
                }

                outlinerLeafRow(
                    entity.name.c_str(),
                    roadSafeSceneEntityGlyph(entity),
                    entity.legacyId,
                    &entity.visible,
                    &entity.locked
                );
            }

            ImGui::TreePop();
        };

        drawSceneGroup(
            "Environment",
            roadsafe::SceneEntityKind::Environment
        );

        drawSceneGroup(
            "Vehicles",
            roadsafe::SceneEntityKind::Vehicle
        );

        drawSceneGroup(
            "Evidence",
            roadsafe::SceneEntityKind::Evidence
        );

        drawSceneGroup(
            "Measurements",
            roadsafe::SceneEntityKind::Measurement
        );        ImGui::EndTable();
    }

    // Bottom selection summary.
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("SELECTION");

    if (gEditorShell.selectedEntity==0)
        ImGui::TextDisabled("No scene object selected.");
    else
        ImGui::Text("%s",selectedEntityName());

    
    // Real entity rename.
    if (gEditorShell.selectedEntity!=0 &&
        ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows) &&
        ImGui::IsKeyPressed(ImGuiKey_F2))
    {
        beginSovereignEntityRename(
            gEditorShell.selectedEntity
        );
    }

    drawSovereignRenamePopup();

    const bool outlinerFocused=
        ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows
        );

    const bool renameOpen=
        ImGui::IsPopupOpen(
            "Rename Entity"
        );

    if (outlinerFocused &&
        !renameOpen &&
        gEditorShell.selectedEntity!=0)
    {
        if (ImGui::GetIO().KeyCtrl &&
            ImGui::IsKeyPressed(ImGuiKey_D))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::Duplicate,
                gEditorShell.selectedEntity
            );
        }

        if (ImGui::IsKeyPressed(
                ImGuiKey_Delete) &&
            !sovereignEntityLocked(
                gEditorShell.selectedEntity
            ))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::Delete,
                gEditorShell.selectedEntity
            );
        }
    }

    // Row/menu actions are deferred until all scene-vector pointers
    // used by this frame have finished rendering.
    processRoadSafePendingSceneAction();

ImGui::End();
}

