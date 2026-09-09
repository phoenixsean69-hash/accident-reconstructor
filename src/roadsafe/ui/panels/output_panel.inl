// ROADSAFE_UI_COMPONENT_V21
// Component: Output Panel
// Included from src/main.cpp; translation-unit behavior is unchanged.

static void drawOutputPanel()
{
    ImGui::Begin(
        "Output",
        nullptr,
        ImGuiWindowFlags_NoMove
    );

    ImGui::TextDisabled("OUTPUT");

    ImGui::SameLine();

    ImGui::Text(
        "%d messages",
        static_cast<int>(
            gSovereignOutputEntries.size()
        )
    );

    if (gSovereignOutputEntries.empty())
    {
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextDisabled(
            "No output has been produced this session."
        );

        ImGui::TextDisabled(
            "Graph, analysis, import and AR messages will appear here."
        );

        ImGui::End();
        return;
    }

    ImGui::Separator();

    if (ImGui::BeginTable(
        "##SovereignOutputTable",
        4,
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable,
        ImVec2(0.0f,0.0f)))
    {
        ImGui::TableSetupScrollFreeze(0,1);

        ImGui::TableSetupColumn(
            "TIME",
            ImGuiTableColumnFlags_WidthFixed,
            72.0f
        );

        ImGui::TableSetupColumn(
            "LEVEL",
            ImGuiTableColumnFlags_WidthFixed,
            84.0f
        );

        ImGui::TableSetupColumn(
            "SOURCE",
            ImGuiTableColumnFlags_WidthFixed,
            120.0f
        );

        ImGui::TableSetupColumn(
            "MESSAGE",
            ImGuiTableColumnFlags_WidthStretch
        );

        ImGui::TableHeadersRow();

        for (const SovereignOutputEntry& entry:
             gSovereignOutputEntries)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::Text(
                "%07.2f",
                entry.timeSeconds
            );

            ImGui::TableSetColumnIndex(1);

            ImGui::TextColored(
                sovereignDiagnosticLevelColor(
                    entry.level
                ),
                "%s",
                sovereignDiagnosticLevelName(
                    entry.level
                )
            );

            ImGui::TableSetColumnIndex(2);

            ImGui::TextUnformatted(
                entry.source.c_str()
            );

            ImGui::TableSetColumnIndex(3);

            ImGui::TextWrapped(
                "%s",
                entry.message.c_str()
            );
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

