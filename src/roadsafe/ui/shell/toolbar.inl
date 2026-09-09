// ROADSAFE_UI_COMPONENT_V20
// Component: Editor Toolbar
// Included from src/main.cpp so behavior/state linkage stays unchanged.
// ROADSAFE_GLOBAL_TOOLBAR_CLEANUP_V23_8
// ROADSAFE_CONTEXT_TOOLBAR_V23_9

enum class RoadSafeToolbarContext
{
    Case,
    Evidence,
    Analysis,
    Viewport
};

static bool roadSafeToolbarTabVisible(
    const char* windowName)
{
    ImGuiWindow* window=
        ImGui::FindWindowByName(
            windowName
        );

    if (!window)
        return false;

    if (window->DockNode)
    {
        return
            window->DockNode->VisibleWindow==
            window;
    }

    return
        window->WasActive &&
        !window->Hidden;
}

static RoadSafeToolbarContext roadSafeToolbarContext()
{
    if (roadSafeToolbarTabVisible(
            "Viewport"))
    {
        return
            RoadSafeToolbarContext::Viewport;
    }

    if (roadSafeToolbarTabVisible(
            "Evidence"))
    {
        return
            RoadSafeToolbarContext::Evidence;
    }

    if (roadSafeToolbarTabVisible(
            "Analysis"))
    {
        return
            RoadSafeToolbarContext::Analysis;
    }

    // Safe first-frame/default behavior:
    // never expose scene-add controls unless Viewport is definitely active.
    return
        RoadSafeToolbarContext::Case;
}

static void roadSafeToolbarCreateEvidence()
{
    roadsafe::EvidenceRecord record;

    record.id=
        gRoadSafeCase.allocateId(
            "EV"
        );

    record.type=
        "Evidence";

    record.description=
        "New Evidence";

    record.collectionStatus=
        "Unreviewed";

    record.lineage.provenance=
        roadsafe::Provenance::Observed;

    record.lineage.confidence=
        roadsafe::Confidence::Unverified;

    gRoadSafeCase.evidence.push_back(
        record
    );

    gRoadSafeCase.touch();
}

static void drawEditorToolbar()
{
    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(9.0f,5.0f)
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(6.0f,4.0f)
    );

    beginSurface(
        "GlobalEditorToolbar",
        ImVec2(0.0f,48.0f),
        false,
        ImGuiWindowFlags_NoScrollbar
    );

    if (editorButton(
            "UNDO",
            74.0f))
    {
        // Hook for command history.
    }

    ImGui::SameLine();

    editorButton(
        "REDO",
        74.0f,
        false,
        false
    );

    ImGui::SameLine();

    toolbarSeparator();

    ImGui::SameLine();

    const RoadSafeToolbarContext context=
        roadSafeToolbarContext();

    switch (context)
    {
        case RoadSafeToolbarContext::Case:
        {
            if (editorButton(
                    "NEW CASE",
                    126.0f))
            {
                ImGui::OpenPopup(
                    "##RoadSafeNewCaseConfirm"
                );
            }

            break;
        }

        case RoadSafeToolbarContext::Evidence:
        {
            if (editorButton(
                    "ADD EVIDENCE",
                    150.0f))
            {
                roadSafeToolbarCreateEvidence();
            }

            break;
        }

        case RoadSafeToolbarContext::Analysis:
        {
            // Analysis is action/result driven rather than "add to scene".
            // Keep this slot empty instead of exposing an irrelevant Add.
            break;
        }

        case RoadSafeToolbarContext::Viewport:
        {
            if (editorButton(
                    "ADD",
                    86.0f))
            {
                ImGui::OpenPopup(
                    "##AddToolbarPopup"
                );
            }

            break;
        }
    }

    if (ImGui::BeginPopupModal(
            "##RoadSafeNewCaseConfirm",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text(
            "Create a new case?"
        );

        ImGui::Spacing();

        ImGui::TextDisabled(
            "The current in-memory case will be cleared."
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (editorButton(
                "CREATE CASE",
                132.0f,
                true))
        {
            gRoadSafeCase=
                roadsafe::RoadSafeCase{};

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (editorButton(
                "CANCEL",
                96.0f))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup(
            "##AddToolbarPopup"))
    {
        ImGui::TextDisabled(
            "ADD TO SCENE"
        );

        ImGui::Separator();

        if (roadSafeMenuItem(
                "Vehicle",
                UiGlyph::Vehicle))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddVehicle
            );
        }

        if (roadSafeMenuItem(
                "Evidence",
                UiGlyph::Evidence))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddEvidence
            );
        }

        if (roadSafeMenuItem(
                "Measurement",
                UiGlyph::Measurement))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddMeasurement
            );
        }

        if (roadSafeMenuItem(
                "Scene Marker",
                UiGlyph::ForensicMarker))
        {
            queueRoadSafeSceneAction(
                RoadSafePendingSceneAction::AddMarker
            );
        }

        ImGui::EndPopup();
    }

    if (context!=
        RoadSafeToolbarContext::Analysis)
    {
        ImGui::SameLine();
    }

    if (editorButton(
            "ASSETS",
            106.0f,
            gRoadSafeAssetLibrary.open,
            true))
    {
        gRoadSafeAssetLibrary.open=true;
        gRoadSafeAssetLibrary.requestFocus=true;
        gRoadSafeAssetLibrary.initialized=false;
    }

    // Right-side command search remains global.
    const float searchW=
        220.0f;

    const float right=
        ImGui::GetWindowWidth()-
        searchW-
        14.0f;

    if (right>
        ImGui::GetCursorPosX()+
        40.0f)
    {
        ImGui::SameLine();

        ImGui::SetCursorPosX(
            right
        );

        ImGui::SetNextItemWidth(
            searchW
        );

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
