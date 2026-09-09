// ROADSAFE_UI_COMPONENT_V21
// Component: Problems Panel
// Included from src/main.cpp; translation-unit behavior is unchanged.

static void drawProblemsPanel()
{
    ImGui::Begin(
        "Problems",
        nullptr,
        ImGuiWindowFlags_NoMove
    );

    int errors=0;
    int warnings=0;
    int info=0;

    for (const SovereignProblemEntry& entry:
         gSovereignProblemEntries)
    {
        switch (entry.level)
        {
            case SovereignDiagnosticLevel::Error:
                errors++;
                break;

            case SovereignDiagnosticLevel::Warning:
                warnings++;
                break;

            case SovereignDiagnosticLevel::Info:
                info++;
                break;
        }
    }

    ImGui::TextDisabled("PROBLEMS");

    ImGui::SameLine();

    ImGui::TextColored(
        sovereignDiagnosticLevelColor(
            SovereignDiagnosticLevel::Error
        ),
        "%d errors",
        errors
    );

    ImGui::SameLine();

    ImGui::TextDisabled("|");

    ImGui::SameLine();

    ImGui::TextColored(
        sovereignDiagnosticLevelColor(
            SovereignDiagnosticLevel::Warning
        ),
        "%d warnings",
        warnings
    );

    ImGui::SameLine();

    ImGui::TextDisabled("|");

    ImGui::SameLine();

    ImGui::Text(
        "%d info",
        info
    );

    ImGui::Separator();

    if (gSovereignProblemEntries.empty())
    {
        ImGui::Spacing();

        ImGui::TextDisabled(
            "No problems have been reported this session."
        );

        ImGui::TextDisabled(
            "Validation, graph and reconstruction diagnostics will appear here."
        );

        ImGui::End();
        return;
    }

    if (ImGui::BeginTable(
        "##SovereignProblemsTable",
        3,
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable,
        ImVec2(0.0f,0.0f)))
    {
        ImGui::TableSetupScrollFreeze(0,1);

        ImGui::TableSetupColumn(
            "SEVERITY",
            ImGuiTableColumnFlags_WidthFixed,
            92.0f
        );

        ImGui::TableSetupColumn(
            "SOURCE",
            ImGuiTableColumnFlags_WidthFixed,
            135.0f
        );

        ImGui::TableSetupColumn(
            "DESCRIPTION",
            ImGuiTableColumnFlags_WidthStretch
        );

        ImGui::TableHeadersRow();

        for (const SovereignProblemEntry& entry:
             gSovereignProblemEntries)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::TextColored(
                sovereignDiagnosticLevelColor(
                    entry.level
                ),
                "%s",
                sovereignDiagnosticLevelName(
                    entry.level
                )
            );

            ImGui::TableSetColumnIndex(1);

            ImGui::TextUnformatted(
                entry.source.c_str()
            );

            ImGui::TableSetColumnIndex(2);

            ImGui::TextWrapped(
                "%s",
                entry.message.c_str()
            );
        }

        ImGui::EndTable();
    }

    ImGui::End();
}
