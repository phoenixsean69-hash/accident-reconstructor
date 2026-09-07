// ROADSAFE_UI_COMPONENT_V21
// Component: Status Bar
// Included from src/main.cpp; translation-unit behavior is unchanged.

static void drawSovereignStatusBar()
{
    const ImGuiViewport* viewport=
        ImGui::GetMainViewport();

    if (!viewport)
        return;

    const ImVec2 statusPos(
        viewport->WorkPos.x,
        viewport->WorkPos.y+
        viewport->WorkSize.y-
        SOVEREIGN_STATUS_BAR_HEIGHT
    );

    const ImVec2 statusSize(
        viewport->WorkSize.x,
        SOVEREIGN_STATUS_BAR_HEIGHT
    );

    ImGui::SetNextWindowPos(
        statusPos,
        ImGuiCond_Always
    );

    ImGui::SetNextWindowSize(
        statusSize,
        ImGuiCond_Always
    );

    ImGui::SetNextWindowViewport(
        viewport->ID
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(10.0f,5.0f)
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowBorderSize,
        1.0f
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowRounding,
        0.0f
    );

    ImGui::PushStyleColor(
        ImGuiCol_WindowBg,
        ImVec4(0.075f,0.079f,0.086f,1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.23f,0.245f,0.27f,1.0f)
    );

    const ImGuiWindowFlags flags=
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin(
        "##SovereignStatusBar",
        nullptr,
        flags))
    {
        auto separator = []()
        {
            ImGui::SameLine(0.0f,10.0f);

            ImGui::TextColored(
                ImVec4(0.31f,0.33f,0.37f,1.0f),
                "|"
            );

            ImGui::SameLine(0.0f,10.0f);
        };

        // ----------------------------------------------------
        // LEFT: application state
        // ----------------------------------------------------

        ImGui::TextColored(
            ImVec4(0.53f,0.80f,0.56f,1.0f),
            "READY"
        );

        separator();

        ImGui::TextDisabled(
            "Scene"
        );

        ImGui::SameLine(0.0f,5.0f);

        ImGui::Text(
        "%zu objects",
        roadSafeActiveSceneEntityCount()
    );

        separator();

        ImGui::TextDisabled(
            "Selected"
        );

        ImGui::SameLine(0.0f,5.0f);

        const char* selectedName=
            selectedEntityName();

        ImGui::Text(
            "%s",
            selectedName &&
            selectedName[0]
                ? selectedName
                : "None"
        );

        separator();

        ImGui::TextDisabled(
            "Snap"
        );

        ImGui::SameLine(0.0f,5.0f);

        if (gEditorShell.snapEnabled)
        {
            ImGui::Text(
                "%.2f m",
                gEditorShell.snapValue
            );
        }
        else
        {
            ImGui::TextDisabled(
                "Off"
            );
        }

        separator();

        ImGui::TextDisabled(
            "Units"
        );

        ImGui::SameLine(0.0f,5.0f);

        ImGui::Text(
            "Metric"
        );

        // ----------------------------------------------------
        // RIGHT: performance + discoverability
        // ----------------------------------------------------

        const float fps=
            ImGui::GetIO().Framerate;

        char rightText[128]{};

        std::snprintf(
            rightText,
            sizeof(rightText),
            "%.0f FPS  |  %dE %dW  |  F1 Shortcuts",
            fps,
            sovereignProblemErrorCount(),
            sovereignProblemWarningCount()
        );

        const float rightWidth=
            ImGui::CalcTextSize(
                rightText
            ).x;

        const float targetX=
            ImGui::GetWindowContentRegionMax().x-
            rightWidth;

        if (targetX>
            ImGui::GetCursorPosX()+20.0f)
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(
                targetX
            );

            ImGui::TextDisabled(
                "%s",
                rightText
            );
        }
    }

    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}
