// ROADSAFE_UI_COMPONENT_V20
// Component: Editor Toolbar
// Included from src/main.cpp so behavior/state linkage stays unchanged.

static void drawEditorToolbar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(9.0f,5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(5.0f,4.0f));

    beginSurface(
        "GlobalEditorToolbar",
        ImVec2(0.0f,82.0f),
        false,
        ImGuiWindowFlags_NoScrollbar
    );

    // Case/document controls.
    shellIconButton(
        "ToolbarNew",
        UiGlyph::NewCase,
        "New Case  (Ctrl+N)"
    );

    ImGui::SameLine();
    shellIconButton(
        "ToolbarOpen",
        UiGlyph::OpenFile,
        "Open Case  (Ctrl+O)"
    );

    ImGui::SameLine();
    shellIconButton(
        "ToolbarSave",
        UiGlyph::Save,
        "Save Case  (Ctrl+S)"
    );

    ImGui::SameLine();
    toolbarSeparator();

    ImGui::SameLine();
    if (editorButton("UNDO",62.0f))
    {
        // Hook for command history.
    }

    ImGui::SameLine();
    editorButton("REDO",62.0f,false,false);

    ImGui::SameLine();
    toolbarSeparator();

    // Add dropdown.
    ImGui::SameLine();
    if (editorButton("ADD",78.0f))
        ImGui::OpenPopup("##AddToolbarPopup");

    if (ImGui::BeginPopup("##AddToolbarPopup"))
    {
        ImGui::TextDisabled("ADD TO SCENE");
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

    if (editorButton(
            "ASSETS",
            96.0f,
            gRoadSafeAssetLibrary.open,
            true))
    {
        gRoadSafeAssetLibrary.open=true;
        gRoadSafeAssetLibrary.requestFocus=true;

        // Opening Assets always rescans the local library so models
        // installed while RoadSafe was closed/idle appear immediately.
        gRoadSafeAssetLibrary.initialized=false;
    }

    ImGui::SameLine();

    // Transform mode.
    ImGui::SameLine();
    const char* transformModes[]={
        "Select",
        "Move",
        "Rotate",
        "Scale"
    };

    const bool globalSelectionLocked=
        shellSelectedEntityLocked();

    if (globalSelectionLocked &&
        gEditorShell.transformMode!=0)
    {
        gEditorShell.transformMode=0;
    }

    ImGui::BeginDisabled(globalSelectionLocked);

    ImGui::SetNextItemWidth(92.0f);
    ImGui::Combo(
        "##GlobalTransformMode",
        &gEditorShell.transformMode,
        transformModes,
        4
    );

    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Checkbox("Snap",&gEditorShell.snapEnabled);

    ImGui::SameLine();
    ImGui::BeginDisabled(!gEditorShell.snapEnabled);
    ImGui::SetNextItemWidth(82.0f);
    ImGui::DragFloat(
        "##GlobalSnapValue",
        &gEditorShell.snapValue,
        0.01f,
        0.01f,
        10.0f,
        "%.2f m"
    );
    ImGui::EndDisabled();

    // Right-side command search.
    const float searchW=220.0f;
    const float right=ImGui::GetWindowWidth()-searchW-14.0f;

    if (right>ImGui::GetCursorPosX()+20.0f)
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(right);
        ImGui::SetNextItemWidth(searchW);
        if (gEditorShell.focusCommandSearch)
        {
            ImGui::SetKeyboardFocusHere();
            gEditorShell.focusCommandSearch=false;
        }

        ImGui::InputTextWithHint(
            "##GlobalCommandSearch",
            "Search commands...",
            gEditorShell.commandSearch,
            sizeof(gEditorShell.commandSearch)
        );
        if (ImGui::IsItemActivated() ||
            ImGui::IsItemEdited())
        {
            gEditorShell.showCommandPalette=true;
        }
    }

    endSurface();

    ImGui::PopStyleVar(2);
}

