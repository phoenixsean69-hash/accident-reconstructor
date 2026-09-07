// ROADSAFE_UI_COMPONENT_V21
// Component: Shortcut Toast
// Included from src/main.cpp; translation-unit behavior is unchanged.

static void drawShortcutToast()
{
    if (gEditorShell.shortcutToast[0]==0)
        return;

    if (ImGui::GetTime()>
        gEditorShell.shortcutToastUntil)
    {
        gEditorShell.shortcutToast[0]=0;
        return;
    }

    const ImGuiViewport* viewport=
        ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x+
                viewport->WorkSize.x-18.0f,
            viewport->WorkPos.y+
                viewport->WorkSize.y-18.0f
        ),
        ImGuiCond_Always,
        ImVec2(1.0f,1.0f)
    );

    ImGui::SetNextWindowBgAlpha(0.94f);

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(14.0f,9.0f)
    );

    if (ImGui::Begin(
        "##ShortcutToast",
        nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav))
    {
        ImGui::TextUnformatted(
            gEditorShell.shortcutToast
        );
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

