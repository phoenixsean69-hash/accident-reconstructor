// ROADSAFE_UI_COMPONENT_V21
// Component: Shortcut Reference
// Included from src/main.cpp; translation-unit behavior is unchanged.

static void drawShortcutReferenceWindow()
{
    if (!gEditorShell.showShortcutReference)
        return;

    ImGui::SetNextWindowSize(
        ImVec2(780.0f,620.0f),
        ImGuiCond_FirstUseEver
    );

    if (!ImGui::Begin(
        "Keyboard Shortcuts",
        &gEditorShell.showShortcutReference))
    {
        ImGui::End();
        return;
    }

    auto shortcutRow =
        [](const char* action,
           const char* key,
           const char* scope)
    {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(action);

        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("%s",key);

        ImGui::TableSetColumnIndex(2);
        ImGui::TextDisabled("%s",scope);
    };

    if (ImGui::BeginTable(
        "##ShortcutReferenceTable",
        3,
        ImGuiTableFlags_BordersInnerH |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn(
            "ACTION",
            ImGuiTableColumnFlags_WidthStretch,
            0.48f
        );

        ImGui::TableSetupColumn(
            "SHORTCUT",
            ImGuiTableColumnFlags_WidthStretch,
            0.22f
        );

        ImGui::TableSetupColumn(
            "SCOPE",
            ImGuiTableColumnFlags_WidthStretch,
            0.30f
        );

        ImGui::TableHeadersRow();

        shortcutRow(
            "Shortcut reference",
            "F1",
            "Global"
        );

        shortcutRow(
            "Command search",
            "Ctrl+Shift+P / Ctrl+K",
            "Global"
        );

        shortcutRow(
            "Case / Evidence / Analysis / Viewport",
            "Ctrl+1 / 2 / 3 / 4",
            "Workspace"
        );

        shortcutRow(
            "Timeline / Node Editor",
            "Ctrl+5 / Ctrl+6",
            "Workspace"
        );

        shortcutRow(
            "Toggle Outliner",
            "Ctrl+Shift+O",
            "Workspace"
        );

        shortcutRow(
            "Toggle Properties",
            "Ctrl+Shift+I",
            "Workspace"
        );

        shortcutRow(
            "Toggle Timeline",
            "Ctrl+Shift+T",
            "Workspace"
        );

        shortcutRow(
            "Toggle Node Editor",
            "Ctrl+Shift+N",
            "Workspace"
        );

        shortcutRow(
            "Reset workspace layout",
            "Ctrl+Shift+R",
            "Workspace"
        );

        shortcutRow(
            "Select / Move / Rotate / Scale",
            "Q / W / E / R",
            "Editor"
        );

        shortcutRow(
            "Toggle snapping",
            "Shift+Tab",
            "Editor"
        );

        shortcutRow(
            "Clear selection",
            "Shift+A",
            "Editor"
        );

        shortcutRow(
            "2D / 3D / AR viewport",
            "Alt+1 / Alt+2 / Alt+3",
            "Viewport"
        );

        shortcutRow(
            "Viewport tool Select/Move/Rotate/Scale",
            "Q / W / E / R",
            "Viewport"
        );

        shortcutRow(
            "Top / Front / Right",
            "1 / 2 / 3",
            "2D Viewport"
        );

        shortcutRow(
            "Perspective / Top / Front / Right",
            "1 / 2 / 3 / 4",
            "3D Viewport"
        );

        shortcutRow(
            "Cycle Lit/Wireframe/Analysis",
            "Z",
            "3D Viewport"
        );

        shortcutRow(
            "Grid / Axes / Bounds / Measurements / Names",
            "G / X / B / M / N",
            "Viewport"
        );

        shortcutRow(
            "Safe frame",
            "Shift+F",
            "Viewport"
        );

        shortcutRow(
            "Editor Preview / Place Anchor / Clear Anchors",
            "P / A / C",
            "AR Viewport"
        );

        shortcutRow(
            "Play / Pause",
            "Space",
            "Timeline"
        );

        shortcutRow(
            "Step frame",
            "Left / Right",
            "Timeline"
        );

        shortcutRow(
            "Step 10 frames",
            "Shift+Left / Shift+Right",
            "Timeline"
        );

        shortcutRow(
            "Start / End",
            "Home / End",
            "Timeline"
        );

        shortcutRow(
            "Pre-impact / Impact / Post-impact",
            "1 / 2 / 3",
            "Timeline"
        );

        shortcutRow(
            "Add / Clear markers",
            "M / Ctrl+Shift+M",
            "Timeline"
        );

        shortcutRow(
            "Snap / Follow playhead",
            "S / F",
            "Timeline"
        );

        shortcutRow(
            "Run graph",
            "Ctrl+Enter",
            "Node Editor"
        );

        shortcutRow(
            "Save / Load graph",
            "Ctrl+S / Ctrl+O",
            "Node Editor"
        );

        shortcutRow(
            "Undo / Redo",
            "Ctrl+Z / Ctrl+Y",
            "Node Editor"
        );

        shortcutRow(
            "Copy / Paste / Duplicate",
            "Ctrl+C / Ctrl+V / Ctrl+D",
            "Node Editor"
        );

        shortcutRow(
            "Select all / Delete",
            "Ctrl+A / Delete",
            "Node Editor"
        );

        shortcutRow(
            "Center graph",
            "Home",
            "Node Editor"
        );

        shortcutRow(
            "Grid / Snap",
            "G / Shift+Tab",
            "Node Editor"
        );

        shortcutRow(
            "Cancel link / clear selection",
            "Esc",
            "Node Editor"
        );

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::TextDisabled(
        "Shortcuts are suppressed while typing into text fields unless they use a dedicated Ctrl modifier."
    );

    ImGui::End();
}
