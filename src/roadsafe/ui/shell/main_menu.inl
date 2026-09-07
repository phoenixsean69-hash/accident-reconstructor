// ROADSAFE_UI_COMPONENT_V20
// Component: Main Menu
// Included from src/main.cpp so behavior/state linkage stays unchanged.

static void drawMainMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
        return;

    // FILE
    if (ImGui::BeginMenu("File"))
    {
        roadSafeMenuItem("New Case",UiGlyph::NewCase,"Ctrl+N");
        roadSafeMenuItem("Open Case...",UiGlyph::OpenFile,"Ctrl+O");

        if (ImGui::BeginMenu("Open Recent"))
        {
            ImGui::MenuItem("No recent cases",nullptr,false,false);
            ImGui::EndMenu();
        }

        ImGui::Separator();
        roadSafeMenuItem("Save",UiGlyph::Save,"Ctrl+S");
        roadSafeMenuItem("Save As...",UiGlyph::SaveAs,"Ctrl+Shift+S");

        ImGui::Separator();

        if (ImGui::BeginMenu("Import"))
        {
            roadSafeMenuItem("Evidence...", UiGlyph::Evidence);
            roadSafeMenuItem("Scene Data...", UiGlyph::Scene);
            roadSafeMenuItem("Vehicle Data...", UiGlyph::Vehicle);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Export"))
        {
            roadSafeMenuItem("Case Package...", UiGlyph::Package);
            roadSafeMenuItem("Report...", UiGlyph::ReportExport);
            roadSafeMenuItem("Scene Snapshot...", UiGlyph::Snapshot);
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (roadSafeMenuItem("Exit",UiGlyph::Exit,"Alt+F4"))
            gEditorShell.requestExit=true;

        ImGui::EndMenu();
    }

    // EDIT
    if (ImGui::BeginMenu("Edit"))
    {
        roadSafeMenuItem("Undo",UiGlyph::Undo,"Ctrl+Z");
        roadSafeMenuItem("Redo",UiGlyph::Redo,"Ctrl+Y",false,false);

        ImGui::Separator();
        roadSafeMenuItem("Cut",UiGlyph::Cut,"Ctrl+X");
        roadSafeMenuItem("Copy",UiGlyph::Copy,"Ctrl+C");
        roadSafeMenuItem("Paste",UiGlyph::Paste,"Ctrl+V");

        ImGui::Separator();
        roadSafeMenuItem("Duplicate",UiGlyph::Duplicate,"Ctrl+D");
        roadSafeMenuItem("Delete",UiGlyph::Delete,"Del");

        ImGui::Separator();
        roadSafeMenuItem("Preferences...", UiGlyph::Preferences);
        ImGui::EndMenu();
    }

    // VIEW
    if (ImGui::BeginMenu("View"))
    {
        roadSafeMenuItemToggle("Scene Outliner",UiGlyph::ViewSidebar,"Ctrl+Shift+O",&gEditorShell.showOutliner);

        roadSafeMenuItemToggle("Properties",UiGlyph::Tune,"Ctrl+Shift+I",&gEditorShell.showProperties);

        roadSafeMenuItemToggle("Timeline",UiGlyph::Timeline,"Ctrl+Shift+T",&gEditorShell.showTimeline);

        roadSafeMenuItemToggle("Node Editor",UiGlyph::NodeEditor,"Ctrl+Shift+N",&gEditorShell.showNodeEditor);

        ImGui::Separator();

        if (roadSafeMenuItem("Reset Workspace Layout",UiGlyph::ResetLayout,"Ctrl+Shift+R"))
            gEditorShell.resetLayoutRequested=true;

        ImGui::Separator();

        if (ImGui::BeginMenu("Viewport"))
        {
            roadSafeMenuItem("Perspective", UiGlyph::Perspective);
            roadSafeMenuItem("Top", UiGlyph::TopView);
            roadSafeMenuItem("Front", UiGlyph::FrontView);
            roadSafeMenuItem("Right", UiGlyph::RightView);
            ImGui::Separator();
            ImGui::MenuItem("Frame Selection","F");
            ImGui::MenuItem("Frame All","Home");
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    // SCENE
    if (ImGui::BeginMenu("Scene"))
    {
        if (ImGui::BeginMenu("Add"))
        {
            roadSafeMenuItem("Vehicle", UiGlyph::Vehicle);
            roadSafeMenuItem("Evidence", UiGlyph::Evidence);
            roadSafeMenuItem("Measurement", UiGlyph::Measurement);
            roadSafeMenuItem("Scene Marker", UiGlyph::ForensicMarker);
            ImGui::EndMenu();
        }

        ImGui::Separator();
        roadSafeMenuItem("Focus Selection",UiGlyph::FrameSelection,"F");
        roadSafeMenuItem("Select All",UiGlyph::SelectAll,"Ctrl+A");
        roadSafeMenuItem("Deselect All",UiGlyph::Deselect,"Alt+A");

        ImGui::Separator();
        roadSafeMenuItemToggle("Snapping",UiGlyph::Snap,"Shift+Tab",&gEditorShell.snapEnabled);

        ImGui::EndMenu();
    }

    // TOOLS
    if (ImGui::BeginMenu("Tools"))
    {
        if (ImGui::BeginMenu("Analysis"))
        {
            roadSafeMenuItem("Skid Analysis", UiGlyph::SkidAnalysis);
            roadSafeMenuItem("Momentum Analysis", UiGlyph::MomentumAnalysis);
            roadSafeMenuItem("Speed Analysis", UiGlyph::SpeedAnalysis);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Measurement"))
        {
            roadSafeMenuItem("Distance Tool", UiGlyph::Distance);
            roadSafeMenuItem("Angle Tool", UiGlyph::Angle);
            roadSafeMenuItem("Reference Marker", UiGlyph::ForensicMarker);
            ImGui::EndMenu();
        }

        ImGui::Separator();
        roadSafeMenuItem("Validate Case", UiGlyph::CaseValidate);
        roadSafeMenuItem("Command Palette...",UiGlyph::Palette,"Ctrl+Shift+P");

        ImGui::EndMenu();
    }

    // HELP
    if (ImGui::BeginMenu("Help"))
    {
        roadSafeMenuItem("Documentation", UiGlyph::Documentation);
        if (roadSafeMenuItem("Keyboard Shortcuts",UiGlyph::Keyboard,"F1"))
            gEditorShell.showShortcutReference=true;
        ImGui::Separator();
        roadSafeMenuItem("About RoadSafe AR", UiGlyph::About);
        ImGui::EndMenu();
    }

    // ROADSAFE AR product / workflow entry.
    if (ImGui::BeginMenu("RoadSafe AR"))
    {
        ImGui::TextDisabled(
            "FORENSIC INVESTIGATION"
        );

        ImGui::Separator();

        drawRoadSafePipelineMenuItems();

        ImGui::Separator();

        ImGui::TextDisabled(
            "ROAD-SAFETY INTELLIGENCE"
        );

        ImGui::BeginDisabled();

        ImGui::MenuItem(
            "Scene Map"
        );

        ImGui::MenuItem(
            "Analytics"
        );

        ImGui::MenuItem(
            "Risk Intelligence"
        );

        ImGui::EndDisabled();

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

