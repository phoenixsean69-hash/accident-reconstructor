// ROADSAFE_UI_COMPONENT_V21
// Component: Command Palette
// Included from src/main.cpp; translation-unit behavior is unchanged.

static void drawCommandPalette()
{
    if (!gEditorShell.showCommandPalette)
        return;

    static const SovereignCommand commands[]={
        {1,"Focus Case View","Open the case workspace","Ctrl+1"},
        {2,"Focus Evidence","Open the evidence workspace","Ctrl+2"},
        {3,"Focus Analysis","Open the analysis workspace","Ctrl+3"},
        {4,"Focus Viewport","Open the scene viewport","Ctrl+4"},
        {5,"Focus Timeline","Open the reconstruction timeline","Ctrl+5"},
        {6,"Focus Node Editor","Open the analysis graph","Ctrl+6"},

        {10,"Toggle Outliner","Show or hide Scene Outliner","Ctrl+Shift+O"},
        {11,"Toggle Properties","Show or hide Properties","Ctrl+Shift+I"},
        {12,"Toggle Timeline","Show or hide Timeline","Ctrl+Shift+T"},
        {13,"Toggle Node Editor","Show or hide Node Editor","Ctrl+Shift+N"},

        {20,"Reset Editor Layout","Restore the default dock layout","Ctrl+Shift+R"},
        {21,"Toggle Snap","Enable or disable snapping","Shift+Tab"},
        {22,"Show Shortcut Reference","Open app-wide keyboard shortcuts","F1"},

        {30,"Tool: Select","Activate selection tool","Q"},
        {31,"Tool: Move","Activate move tool","W"},
        {32,"Tool: Rotate","Activate rotate tool","E"},
        {33,"Tool: Scale","Activate scale tool","R"},

        {40,"Mode: Scene","Scene editing workstation mode",""},
        {41,"Mode: Evidence","Evidence placement workstation mode",""},
        {42,"Mode: Measure","Measurement workstation mode",""},
        {43,"Mode: Reconstruct","Reconstruction workstation mode",""},
        {44,"Mode: Review","Review workstation mode",""}
    };

    const ImGuiViewport* viewport=
        ImGui::GetMainViewport();

    const ImVec2 center=
        viewport
            ? viewport->GetCenter()
            : ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

    ImGui::SetNextWindowPos(
        center,
        ImGuiCond_Appearing,
        ImVec2(0.5f,0.34f)
    );

    ImGui::SetNextWindowSize(
        ImVec2(650.0f,430.0f),
        ImGuiCond_Appearing
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(12.0f,12.0f)
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(8.0f,6.0f)
    );

    const ImGuiWindowFlags flags=
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    bool open=
        gEditorShell.showCommandPalette;

    if (ImGui::Begin(
        "Command Palette",
        &open,
        flags))
    {
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();

        const bool submitted=
            ImGui::InputTextWithHint(
                "##CommandPaletteSearch",
                "Type a command...",
                gEditorShell.commandSearch,
                sizeof(
                    gEditorShell.commandSearch
                ),
                ImGuiInputTextFlags_EnterReturnsTrue
            );

        if (ImGui::IsKeyPressed(
            ImGuiKey_Escape))
        {
            open=false;
        }

        ImGui::Separator();

        int visibleCount=0;
        int selectedVisibleIndex=
            gEditorShell.commandPaletteSelection;

        // Clamp selection against this frame's result count later.
        for (const SovereignCommand& command:
             commands)
        {
            const bool matches=
                sovereignCommandMatches(
                    command.name,
                    gEditorShell.commandSearch
                ) ||
                sovereignCommandMatches(
                    command.detail,
                    gEditorShell.commandSearch
                );

            if (matches)
                visibleCount++;
        }

        if (visibleCount<=0)
        {
            gEditorShell.commandPaletteSelection=0;

            ImGui::Spacing();
            ImGui::TextDisabled(
                "No matching commands."
            );
        }
        else
        {
            if (selectedVisibleIndex<0)
                selectedVisibleIndex=0;

            if (selectedVisibleIndex>=visibleCount)
                selectedVisibleIndex=
                    visibleCount-1;

            if (ImGui::IsKeyPressed(
                ImGuiKey_DownArrow))
            {
                selectedVisibleIndex=
                    std::min(
                        visibleCount-1,
                        selectedVisibleIndex+1
                    );
            }

            if (ImGui::IsKeyPressed(
                ImGuiKey_UpArrow))
            {
                selectedVisibleIndex=
                    std::max(
                        0,
                        selectedVisibleIndex-1
                    );
            }

            gEditorShell.commandPaletteSelection=
                selectedVisibleIndex;

            int visibleIndex=0;
            int submittedId=0;

            for (const SovereignCommand& command:
                 commands)
            {
                const bool matches=
                    sovereignCommandMatches(
                        command.name,
                        gEditorShell.commandSearch
                    ) ||
                    sovereignCommandMatches(
                        command.detail,
                        gEditorShell.commandSearch
                    );

                if (!matches)
                    continue;

                const bool selected=
                    visibleIndex==
                    selectedVisibleIndex;

                ImGui::PushID(command.id);

                if (ImGui::Selectable(
                    "##CommandRow",
                    selected,
                    ImGuiSelectableFlags_AllowDoubleClick,
                    ImVec2(0.0f,46.0f)))
                {
                    submittedId=
                        command.id;
                }

                const ImVec2 rowMin=
                    ImGui::GetItemRectMin();

                const ImVec2 rowMax=
                    ImGui::GetItemRectMax();

                ImDrawList* drawList=
                    ImGui::GetWindowDrawList();

                drawList->AddText(
                    ImVec2(
                        rowMin.x+10.0f,
                        rowMin.y+6.0f
                    ),
                    toU32(colorText()),
                    command.name
                );

                drawList->AddText(
                    ImVec2(
                        rowMin.x+10.0f,
                        rowMin.y+25.0f
                    ),
                    toU32(colorMuted()),
                    command.detail
                );

                if (command.shortcut &&
                    command.shortcut[0])
                {
                    const ImVec2 shortcutSize=
                        ImGui::CalcTextSize(
                            command.shortcut
                        );

                    drawList->AddText(
                        ImVec2(
                            rowMax.x-
                                shortcutSize.x-
                                12.0f,
                            rowMin.y+15.0f
                        ),
                        toU32(colorMuted()),
                        command.shortcut
                    );
                }

                ImGui::PopID();

                if (submittedId!=0)
                {
                    executeSovereignCommand(
                        submittedId
                    );
                    break;
                }

                visibleIndex++;
            }

            if (submitted &&
                visibleCount>0)
            {
                int index=0;

                for (const SovereignCommand& command:
                     commands)
                {
                    const bool matches=
                        sovereignCommandMatches(
                            command.name,
                            gEditorShell.commandSearch
                        ) ||
                        sovereignCommandMatches(
                            command.detail,
                            gEditorShell.commandSearch
                        );

                    if (!matches)
                        continue;

                    if (index==
                        gEditorShell.commandPaletteSelection)
                    {
                        executeSovereignCommand(
                            command.id
                        );
                        break;
                    }

                    index++;
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::TextDisabled(
            "Up/Down navigate   Enter run   Esc close"
        );
    }

    ImGui::End();

    gEditorShell.showCommandPalette=open;

    ImGui::PopStyleVar(2);
}
