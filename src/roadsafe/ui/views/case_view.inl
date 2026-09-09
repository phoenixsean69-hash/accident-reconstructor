// ROADSAFE_UI_COMPONENT_V20
// Component: Case View
// Included from src/main.cpp so behavior/state linkage stays unchanged.
// ROADSAFE_CASE_WEB_DASHBOARD_V23

static void drawCaseView()
{
    // ROADSAFE_CASE_SPACING_V23_2
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(
            roadsafe::ui::token::Space16,
            roadsafe::ui::token::Space12
        )
    );

    ImGui::Begin(
        "Case View",
        nullptr,
        ImGuiWindowFlags_AlwaysVerticalScrollbar
    );

    drawRoadSafePipelineBar();
    ImGui::Spacing();

    const auto palette=
        roadsafe::ui::currentPalette();

    // ROADSAFE_CASE_ICON_REFRESH_V23_6_FIXED
    // ROADSAFE_CASE_ICON_CONTAINMENT_V23_7
    // Case dashboard icons use only RoadSafe blue or light gray.
    const ImVec4 caseIconBlue=
        ImVec4(
            0.18f,
            0.52f,
            0.98f,
            1.0f
        );

    const ImVec4 caseIconLight=
        ImVec4(
            0.78f,
            0.82f,
            0.88f,
            1.0f
        );

    const float contentW=
        roadsafe::ui::availableWidth();

    const std::size_t vehicleCount=
        roadSafeActiveVehicleCount();

    const std::size_t evidenceCount=
        roadSafeActiveEvidenceCount();

    const std::size_t measurementCount=
        roadSafeActiveMeasurementCount();

    const std::size_t personCount=
        gRoadSafeCase.persons.size();

    const std::size_t witnessCount=
        gRoadSafeCase.witnesses.size();

    const bool caseDetailsReady=
        !gRoadSafeCase.identity.accidentDateTime.empty() &&
        !gRoadSafeCase.identity.location.empty();

    int readiness=25;

    if (caseDetailsReady)
        readiness+=25;

    if (evidenceCount>0)
        readiness+=25;

    if (measurementCount>0)
        readiness+=25;

    readiness=
        std::max(
            0,
            std::min(
                100,
                readiness
            )
        );

    const bool compact=
        contentW<
        roadsafe::ui::token::BreakpointCompact;

    const bool wide=
        contentW>=
        roadsafe::ui::token::BreakpointWide;

    const float cardGap=
        roadsafe::ui::token::Space16;

    // --------------------------------------------------------
    // CASE HERO
    // --------------------------------------------------------
    const float heroHeight=
        compact
            ? 160.0f
            : 116.0f;

    roadsafe::ui::beginCard(
        "CaseDashboardHero",
        ImVec2(
            0.0f,
            heroHeight
        ),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        const ImVec2 p=
            ImGui::GetCursorScreenPos();

        const float right=
            ImGui::GetWindowPos().x+
            ImGui::GetWindowContentRegionMax().x;

        roadsafe::ui::drawIcon(
            UiGlyph::Folder,
            ImVec2(
                p.x+28.0f,
                p.y+28.0f
            ),
            38.0f,
            caseIconBlue
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x+56.0f,
                p.y+1.0f
            )
        );

        roadsafe::ui::overline(
            "CASE MANAGEMENT"
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x+56.0f,
                p.y+25.0f
            )
        );

        ImGui::Text(
            "%s",
            roadsafe::textOr(
                gRoadSafeCase.identity.caseNumber,
                "UNTITLED CASE"
            )
        );

        char metadata[512]{};

        std::snprintf(
            metadata,
            sizeof(metadata),
            "%s   /   %s",
            roadsafe::textOr(
                gRoadSafeCase.identity.location,
                "Location not set"
            ),
            roadsafe::textOr(
                gRoadSafeCase.identity.accidentDateTime,
                "Date not set"
            )
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x+56.0f,
                p.y+51.0f
            )
        );

        roadsafe::ui::textMuted(
            metadata
        );

        const char* statusText=
            "DRAFT";

        const ImVec2 statusSize=
            ImGui::CalcTextSize(
                statusText
            );

        const float pillW=
            statusSize.x+24.0f;

        const float pillH=
            28.0f;

        // ROADSAFE_CASE_DETAILS_FIT_V23_4
        // editorButton() enforces its full icon+text auto width, so the
        // hero action layout must reserve that full width before anchoring
        // the button group to the right edge.
        const float buttonW=
            190.0f;

        const float actionsW=
            pillW+
            roadsafe::ui::token::Space8+
            buttonW;

        if (!compact)
        {
            const float actionX=
                right-actionsW;

            const float actionY=
                p.y+19.0f;

            ImVec4 pillBg=
                palette.accent;

            pillBg.w=
                0.16f;

            ImDrawList* dl=
                ImGui::GetWindowDrawList();

            dl->AddRectFilled(
                ImVec2(
                    actionX,
                    actionY
                ),
                ImVec2(
                    actionX+pillW,
                    actionY+pillH
                ),
                roadsafe::ui::rgba(
                    pillBg
                ),
                roadsafe::ui::token::RadiusMd
            );

            dl->AddText(
                ImVec2(
                    actionX+12.0f,
                    actionY+
                    (
                        pillH-
                        ImGui::GetFontSize()
                    )*0.5f
                ),
                roadsafe::ui::rgba(
                    palette.accent
                ),
                statusText
            );

            ImGui::SetCursorScreenPos(
                ImVec2(
                    actionX+
                    pillW+
                    roadsafe::ui::token::Space8,
                    actionY-3.0f
                )
            );

            roadsafe::ui::button(
                "CASE DETAILS",
                buttonW
            );
        }
        else
        {
            ImGui::SetCursorScreenPos(
                ImVec2(
                    p.x,
                    p.y+84.0f
                )
            );

            roadsafe::ui::button(
                "CASE DETAILS",
                roadsafe::ui::ButtonWidth::Fill
            );
        }
    }
    roadsafe::ui::endCard();

    ImGui::Spacing();

    // --------------------------------------------------------
    // KPI CARDS
    // --------------------------------------------------------
    int metricColumns=1;

    if (contentW>=980.0f)
        metricColumns=4;
    else if (contentW>=560.0f)
        metricColumns=2;

    const float metricW=
        (
            contentW-
            cardGap*
            static_cast<float>(
                metricColumns-1
            )
        )/
        static_cast<float>(
            metricColumns
        );

    const auto metricCard=
        [&](const char* id,
            UiGlyph glyph,
            const char* label,
            const char* value,
            const char* detail,
            bool accent)
        {
            roadsafe::ui::beginCard(
                id,
                ImVec2(
                    metricW,
                    112.0f
                ),
                true,
                ImGuiWindowFlags_NoScrollbar
            );

            const ImVec2 p=
                ImGui::GetCursorScreenPos();

            // ROADSAFE_CASE_ALIGNMENT_KPI_V23_5
            const ImVec4 metricIconColor=
                accent
                    ? caseIconBlue
                    : caseIconLight;

            roadsafe::ui::drawIcon(
                glyph,
                ImVec2(
                    p.x+30.0f,
                    p.y+28.0f
                ),
                40.0f,
                metricIconColor
            );

            ImDrawList* dl=
                ImGui::GetWindowDrawList();

            dl->AddText(
                ImGui::GetFont(),
                ImGui::GetFontSize()*1.42f,
                ImVec2(
                    p.x+56.0f,
                    p.y-1.0f
                ),
                roadsafe::ui::rgba(
                    palette.text
                ),
                value
            );

            ImGui::SetCursorScreenPos(
                ImVec2(
                    p.x+56.0f,
                    p.y+30.0f
                )
            );

            roadsafe::ui::textSecondary(
                label
            );

            ImGui::SetCursorScreenPos(
                ImVec2(
                    p.x,
                    p.y+59.0f
                )
            );

            roadsafe::ui::textMuted(
                detail
            );

            roadsafe::ui::endCard();
        };

    char vehiclesValue[32]{};
    char evidenceValue[32]{};
    char measurementsValue[32]{};
    char readinessValue[32]{};

    std::snprintf(
        vehiclesValue,
        sizeof(vehiclesValue),
        "%zu",
        vehicleCount
    );

    std::snprintf(
        evidenceValue,
        sizeof(evidenceValue),
        "%zu",
        evidenceCount
    );

    std::snprintf(
        measurementsValue,
        sizeof(measurementsValue),
        "%zu",
        measurementCount
    );

    std::snprintf(
        readinessValue,
        sizeof(readinessValue),
        "%d%%",
        readiness
    );

    metricCard(
        "CaseMetricVehicles",
        UiGlyph::Vehicle,
        "Vehicles",
        vehiclesValue,
        vehicleCount>0
            ? "Active scene vehicles"
            : "No vehicles added",
        vehicleCount>0
    );

    if (metricColumns>1)
        ImGui::SameLine(
            0.0f,
            cardGap
        );

    metricCard(
        "CaseMetricEvidence",
        UiGlyph::Evidence,
        "Evidence",
        evidenceValue,
        evidenceCount>0
            ? "Documented evidence"
            : "No evidence added",
        evidenceCount>0
    );

    if (metricColumns==4)
        ImGui::SameLine(
            0.0f,
            cardGap
        );
    else
        ImGui::Spacing();

    metricCard(
        "CaseMetricMeasurements",
        UiGlyph::Measurement,
        "Measurements",
        measurementsValue,
        measurementCount>0
            ? "Scene measurements"
            : "No measurements added",
        measurementCount>0
    );

    if (metricColumns>1)
        ImGui::SameLine(
            0.0f,
            cardGap
        );

    metricCard(
        "CaseMetricReadiness",
        UiGlyph::Bars,
        "Case readiness",
        readinessValue,
        readiness<100
            ? "Complete missing case inputs"
            : "Ready for reconstruction",
        readiness>=75
    );

    ImGui::Spacing();

    // --------------------------------------------------------
    // MAIN DASHBOARD ROW
    // --------------------------------------------------------
    const float dashboardW=
        roadsafe::ui::availableWidth();

    const bool twoColumn=
        dashboardW>=
        860.0f;

    const float sideW=
        twoColumn
            ? std::max(
                300.0f,
                dashboardW*0.32f
            )
            : dashboardW;

    const float mainW=
        twoColumn
            ? dashboardW-
                sideW-
                cardGap
            : dashboardW;

    roadsafe::ui::beginCard(
        "CaseInvestigationProgress",
        ImVec2(
            mainW,
            236.0f
        ),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        ImGui::Text(
            "INVESTIGATION PROGRESS"
        );

        ImGui::SameLine(
            0.0f,
            roadsafe::ui::token::Space8
        );

        roadsafe::ui::textMuted(
            "CASE WORKFLOW"
        );

        ImGui::Spacing();

        const float innerW=
            ImGui::GetContentRegionAvail().x;

        const float stageGap=
            roadsafe::ui::token::Space8;

        const float stageW=
            (
                innerW-
                stageGap*3.0f
            )/
            4.0f;

        const auto stage=
            [&](const char* id,
                UiGlyph glyph,
                const char* title,
                const char* state,
                float progress,
                const ImVec4& color,
                bool active)
            {
                ImGui::PushID(
                    id
                );

                const ImVec2 p=
                    ImGui::GetCursorScreenPos();

                ImDrawList* dl=
                    ImGui::GetWindowDrawList();

                ImVec4 bg=
                    palette.surface;

                bg.w=
                    1.0f;

                dl->AddRectFilled(
                    p,
                    ImVec2(
                        p.x+stageW,
                        p.y+142.0f
                    ),
                    roadsafe::ui::rgba(
                        bg
                    ),
                    roadsafe::ui::token::RadiusLg
                );

                roadsafe::ui::drawIcon(
                    glyph,
                    ImVec2(
                        p.x+26.0f,
                        p.y+26.0f
                    ),
                    34.0f,
                    active
                        ? caseIconBlue
                        : caseIconLight
                );

                dl->AddText(
                    ImVec2(
                        p.x+12.0f,
                        p.y+56.0f
                    ),
                    roadsafe::ui::rgba(
                        palette.text
                    ),
                    title
                );

                dl->AddText(
                    ImVec2(
                        p.x+12.0f,
                        p.y+80.0f
                    ),
                    roadsafe::ui::rgba(
                        color
                    ),
                    state
                );

                const float barX=
                    p.x+12.0f;

                const float barY=
                    p.y+112.0f;

                const float barW=
                    std::max(
                        12.0f,
                        stageW-24.0f
                    );

                ImVec4 track=
                    palette.surfaceHover;

                track.w=
                    1.0f;

                dl->AddRectFilled(
                    ImVec2(
                        barX,
                        barY
                    ),
                    ImVec2(
                        barX+barW,
                        barY+5.0f
                    ),
                    roadsafe::ui::rgba(
                        track
                    ),
                    2.5f
                );

                dl->AddRectFilled(
                    ImVec2(
                        barX,
                        barY
                    ),
                    ImVec2(
                        barX+
                        barW*
                        std::max(
                            0.0f,
                            std::min(
                                1.0f,
                                progress
                            )
                        ),
                        barY+5.0f
                    ),
                    roadsafe::ui::rgba(
                        color
                    ),
                    2.5f
                );

                ImGui::Dummy(
                    ImVec2(
                        stageW,
                        142.0f
                    )
                );

                ImGui::PopID();
            };

        const ImVec4 success=
            palette.success;

        const ImVec4 accent=
            palette.accent;

        const ImVec4 muted=
            palette.textMuted;

        stage(
            "Scene",
            UiGlyph::Folder,
            "Scene setup",
            vehicleCount>0
                ? "READY"
                : "WAITING",
            vehicleCount>0
                ? 1.0f
                : 0.0f,
            vehicleCount>0
                ? success
                : muted,
            vehicleCount>0
        );

        ImGui::SameLine(
            0.0f,
            stageGap
        );

        stage(
            "Evidence",
            UiGlyph::Evidence,
            "Evidence",
            evidenceCount>0
                ? "IN PROGRESS"
                : "WAITING",
            evidenceCount>0
                ? 0.62f
                : 0.0f,
            evidenceCount>0
                ? accent
                : muted,
            evidenceCount>0
        );

        ImGui::SameLine(
            0.0f,
            stageGap
        );

        const bool analysisReady=
            caseDetailsReady &&
            evidenceCount>0 &&
            measurementCount>0;

        stage(
            "Analysis",
            UiGlyph::Bars,
            "Analysis",
            analysisReady
                ? "READY"
                : "WAITING",
            analysisReady
                ? 0.20f
                : 0.0f,
            analysisReady
                ? accent
                : muted,
            analysisReady
        );

        ImGui::SameLine(
            0.0f,
            stageGap
        );

        stage(
            "Report",
            UiGlyph::Document,
            "Report",
            "DRAFT",
            0.0f,
            muted,
            false
        );
    }
    roadsafe::ui::endCard();

    if (twoColumn)
    {
        ImGui::SameLine(
            0.0f,
            cardGap
        );
    }
    else
    {
        ImGui::Spacing();
    }

    ImGui::BeginGroup();

    roadsafe::ui::beginCard(
        "CaseNextAction",
        ImVec2(
            sideW,
            158.0f
        ),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        const ImVec2 nextIconPos=
            ImGui::GetCursorScreenPos();

        roadsafe::ui::drawIcon(
            UiGlyph::Next,
            ImVec2(
                nextIconPos.x+24.0f,
                nextIconPos.y+24.0f
            ),
            32.0f,
            caseIconBlue
        );

        const ImVec2 p=
            ImGui::GetCursorScreenPos();

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x+46.0f,
                p.y+1.0f
            )
        );

        ImGui::Text(
            "NEXT ACTION"
        );

        const char* actionText=
            !caseDetailsReady
                ? "Complete incident date and location."
                : evidenceCount==0
                    ? "Add evidence to the case."
                    : measurementCount==0
                        ? "Capture scene measurements."
                        : "Continue reconstruction in the viewport.";

        const char* actionButton=
            !caseDetailsReady
                ? "CASE DETAILS"
                : evidenceCount==0
                    ? "OPEN EVIDENCE"
                    : measurementCount==0
                        ? "MEASUREMENTS"
                        : "OPEN VIEWPORT";

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x,
                p.y+48.0f
            )
        );

        ImGui::TextWrapped(
            "%s",
            actionText
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x,
                p.y+82.0f
            )
        );

        if (roadsafe::ui::button(
                actionButton,
                roadsafe::ui::ButtonWidth::Fill,
                true))
        {
            if (!caseDetailsReady)
            {
                ImGui::SetWindowFocus(
                    "Case View"
                );
            }
            else if (evidenceCount==0 ||
                     measurementCount==0)
            {
                ImGui::SetWindowFocus(
                    "Evidence"
                );
            }
            else
            {
                ImGui::SetWindowFocus(
                    "Viewport"
                );
            }
        }
    }
    roadsafe::ui::endCard();

    ImGui::Spacing();

    roadsafe::ui::beginCard(
        "CasePeople",
        ImVec2(
            sideW,
            96.0f
        ),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        ImGui::Text(
            "INVOLVED PARTIES"
        );

        char partiesText[96]{};

        std::snprintf(
            partiesText,
            sizeof(partiesText),
            "%zu people   /   %zu witnesses",
            personCount,
            witnessCount
        );

        roadsafe::ui::textMuted(
            partiesText
        );
    }
    roadsafe::ui::endCard();

    ImGui::EndGroup();

    ImGui::Spacing();

    // --------------------------------------------------------
    // INVENTORY + INCIDENT SUMMARY
    // --------------------------------------------------------
    const float lowerW=
        roadsafe::ui::availableWidth();

    const bool lowerTwoColumn=
        lowerW>=
        860.0f;

    // Keep Next Action, Involved Parties, and Incident Summary
    // on the exact same right-side dashboard rail.
    const float summaryW=
        lowerTwoColumn
            ? sideW
            : lowerW;

    const float inventoryW=
        lowerTwoColumn
            ? lowerW-
                sideW-
                cardGap
            : lowerW;

    // ROADSAFE_CASE_CLIP_FIX_V23_1
    const float lowerCardHeight=
        226.0f;

    roadsafe::ui::beginCard(
        "CaseInventory",
        ImVec2(
            inventoryW,
            lowerCardHeight
        ),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        ImGui::Text(
            "CASE INVENTORY"
        );

        ImGui::SameLine(
            0.0f,
            roadsafe::ui::token::Space8
        );

        roadsafe::ui::textMuted(
            "LIVE RECORDS"
        );

        ImGui::Spacing();

        const auto inventoryRow=
            [&](UiGlyph glyph,
                const char* label,
                std::size_t count)
            {
                const ImVec2 p=
                    ImGui::GetCursorScreenPos();

                roadsafe::ui::drawIcon(
                    glyph,
                    ImVec2(
                        p.x+20.0f,
                        p.y+18.0f
                    ),
                    28.0f,
                    count>0
                        ? caseIconBlue
                        : caseIconLight
                );

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        p.x+40.0f,
                        p.y+1.0f
                    )
                );

                ImGui::Text(
                    "%s",
                    label
                );

                char countText[32]{};

                std::snprintf(
                    countText,
                    sizeof(countText),
                    "%zu",
                    count
                );

                const ImVec2 countSize=
                    ImGui::CalcTextSize(
                        countText
                    );

                const float right=
                    ImGui::GetWindowPos().x+
                    ImGui::GetWindowContentRegionMax().x;

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        right-
                        countSize.x,
                        p.y+2.0f
                    )
                );

                ImGui::Text(
                    "%s",
                    countText
                );

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        p.x,
                        p.y+34.0f
                    )
                );

                ImGui::Dummy(
                    ImVec2(
                        1.0f,
                        1.0f
                    )
                );
            };

        inventoryRow(
            UiGlyph::Vehicle,
            "Vehicles",
            vehicleCount
        );

        inventoryRow(
            UiGlyph::Evidence,
            "Evidence",
            evidenceCount
        );

        inventoryRow(
            UiGlyph::Measurement,
            "Measurements",
            measurementCount
        );

        inventoryRow(
            UiGlyph::People,
            "People and witnesses",
            personCount+witnessCount
        );
    }
    roadsafe::ui::endCard();

    if (lowerTwoColumn)
    {
        ImGui::SameLine(
            0.0f,
            cardGap
        );
    }
    else
    {
        ImGui::Spacing();
    }

    roadsafe::ui::beginCard(
        "CaseIncidentSummary",
        ImVec2(
            summaryW,
            lowerCardHeight
        ),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        const ImVec2 p=
            ImGui::GetCursorScreenPos();

        roadsafe::ui::drawIcon(
            UiGlyph::Document,
            ImVec2(
                p.x+24.0f,
                p.y+22.0f
            ),
            32.0f,
            caseIconLight
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x+46.0f,
                p.y+1.0f
            )
        );

        ImGui::Text(
            "INCIDENT SUMMARY"
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x,
                p.y+48.0f
            )
        );

        ImGui::TextWrapped(
            "%s",
            "No summary has been added to this case yet."
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x,
                p.y+112.0f
            )
        );

        roadsafe::ui::button(
            "EDIT SUMMARY",
            roadsafe::ui::ButtonWidth::Fill
        );
    }
    roadsafe::ui::endCard();

    ImGui::Dummy(
        ImVec2(
            1.0f,
            roadsafe::ui::token::Space16
        )
    );

    ImGui::End();
    ImGui::PopStyleVar();
}
