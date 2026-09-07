// ROADSAFE_UI_COMPONENT_V21
// Component: Properties Window
// Included from src/main.cpp; translation-unit behavior is unchanged.

static void drawProperties()
{
    static float position[3]={0.0f,0.0f,0.0f};
    static float rotation[3]={0.0f,0.0f,0.0f};
    static float scale[3]={1.0f,1.0f,1.0f};

    static bool objectVisible=true;
    static bool objectLocked=false;
    static char objectName[128]="Untitled Object";

    ImGui::Begin(
        "Properties",
        &gEditorShell.showProperties,
        ImGuiWindowFlags_NoMove
    );
    // SOVEREIGN_PANEL_SCROLL_LIMITS_V1
    // Inspector-style panels never keep stale horizontal scroll.
    ImGui::SetScrollX(0.0f);
    // Responsive Properties context header.
    // The previous single-line header clipped "Object Properties"
    // in the normal narrow inspector width.
    beginSurface(
        "##PropertiesContextHeader",
        ImVec2(0.0f,64.0f),
        false,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    const ImVec2 propertyHeaderPos=
        ImGui::GetCursorScreenPos();

    UiGlyph propertyHeaderGlyph=
        UiGlyph::Info;

    if (const auto* propertyEntity=
            roadSafeSceneEntityConst(
                gEditorShell.selectedEntity
            ))
    {
        switch (propertyEntity->kind)
        {
            case roadsafe::SceneEntityKind::Vehicle:
                propertyHeaderGlyph=UiGlyph::Vehicle;
                break;

            case roadsafe::SceneEntityKind::Evidence:
                propertyHeaderGlyph=UiGlyph::Evidence;
                break;

            case roadsafe::SceneEntityKind::Measurement:
                propertyHeaderGlyph=UiGlyph::Measurement;
                break;

            case roadsafe::SceneEntityKind::Environment:
                propertyHeaderGlyph=UiGlyph::Environment;
                break;
        }
    }

    drawIconBadge(
        propertyHeaderGlyph,
        propertyHeaderPos,
        30.0f,
        false
    );

    ImGui::SetCursorScreenPos(
        ImVec2(
            propertyHeaderPos.x+42.0f,
            propertyHeaderPos.y
        )
    );

    ImGui::TextDisabled(
        gEditorShell.selectedEntity!=0
            ? "INSPECTOR / OBJECT"
            : "INSPECTOR / SCENE"
    );

    ImGui::SetCursorScreenPos(
        ImVec2(
            propertyHeaderPos.x+42.0f,
            propertyHeaderPos.y+23.0f
        )
    );

    ImGui::PushTextWrapPos(
        ImGui::GetWindowPos().x+
        ImGui::GetWindowContentRegionMax().x
    );

    ImGui::Text(
        "%s",
        gEditorShell.selectedEntity!=0
            ? selectedEntityName()
            : "Scene Settings"
    );

    ImGui::PopTextWrapPos();

    endSurface();
    ImGui::Spacing();
    drawDeepPropertiesInspectorBody();
    // SOVEREIGN_PROPERTIES_LEGACY_CLEANED_V1
    // Deep Properties V3 is now the single inspector body.
    // The older duplicate selection / transform / object / analysis
    // inspector has intentionally been removed.
ImGui::End();
}

