// ROADSAFE_UI_COMPONENT_V20
// Component: Analysis View
// Included from src/main.cpp so behavior/state linkage stays unchanged.

static void drawAnalysisView()
{
    static bool selectOverviewOnFirstFrame = true;

    ImGui::Begin("Analysis");

    // ========================================================
    // FIXED WORKSPACE HEADER
    // ========================================================

    beginSurface("AnalysisHeader",ImVec2(0.0f,62.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 p=ImGui::GetCursorScreenPos();
        const float right=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x;

        drawIconBadge(UiGlyph::Bars,p,42.0f,true);

        ImGui::SetCursorScreenPos(ImVec2(p.x+54.0f,p.y+2.0f));
        ImGui::Text("ANALYSIS WORKSPACE");

        ImGui::SetCursorScreenPos(ImVec2(p.x+54.0f,p.y+29.0f));
        ImGui::TextDisabled(
            "READINESS  /  METHODS  /  RESULTS"
        );

        const float guideW=138.0f;
        ImGui::SetCursorScreenPos(ImVec2(right-guideW-12.0f,p.y+15.0f));
        editorButton("ANALYSIS GUIDE",guideW,true);
    }
    endSurface();

    ImGui::Spacing();

    // ========================================================
    // INTERNAL ANALYSIS TABS
    // ========================================================

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(14.0f,7.0f));

    if (ImGui::BeginTabBar(
        "AnalysisWorkspaceTabs",
        ImGuiTabBarFlags_FittingPolicyScroll))
    {
        const ImGuiTabItemFlags overviewFlags =
            selectOverviewOnFirstFrame
                ? ImGuiTabItemFlags_SetSelected
                : ImGuiTabItemFlags_None;

        // ====================================================
        // TAB 1: OVERVIEW
        // ====================================================

        if (ImGui::BeginTabItem("Overview",nullptr,overviewFlags))
        {
            ImGui::Spacing();

            const bool caseLocated=
                !gRoadSafeCase.identity.location.empty();

            const bool caseDated=
                !gRoadSafeCase.identity.accidentDateTime.empty();

            const bool vehiclesReady=
                roadSafeActiveVehicleCount()>0;

            const bool evidenceReady=
                roadSafeActiveEvidenceCount()>0;

            const bool measurementsReady=
                roadSafeActiveMeasurementCount()>0;

            const int readinessChecks=
                (caseLocated && caseDated ? 1 : 0)+
                (vehiclesReady ? 1 : 0)+
                (evidenceReady ? 1 : 0)+
                (measurementsReady ? 1 : 0);

            const float readiness=
                static_cast<float>(
                    readinessChecks
                )/
                4.0f;

            const int readinessPercent=
                static_cast<int>(
                    readiness*
                    100.0f+
                    0.5f
                );

            const float contentW=
                ImGui::GetContentRegionAvail().x;

            const float gap=
                8.0f;

            const float leftW=
                std::max(
                    360.0f,
                    contentW*0.58f
                );

            const float rightW=
                std::max(
                    280.0f,
                    contentW-leftW-gap
                );

            beginSurface(
                "OverviewReadinessVisual",
                ImVec2(
                    leftW,
                    190.0f
                ),
                true,
                ImGuiWindowFlags_NoScrollbar
            );
            {
                const ImVec2 p=
                    ImGui::GetCursorScreenPos();

                ImGui::Text(
                    "ANALYSIS READINESS"
                );

                ImGui::TextDisabled(
                    "PREREQUISITES"
                );

                ImDrawList* dl=
                    ImGui::GetWindowDrawList();

                const ImVec2 center(
                    p.x+82.0f,
                    p.y+103.0f
                );

                const float radius=
                    48.0f;

                dl->AddCircle(
                    center,
                    radius,
                    IM_COL32(
                        54,
                        59,
                        67,
                        255
                    ),
                    64,
                    8.0f
                );

                if (readiness>0.0f)
                {
                    dl->PathArcTo(
                        center,
                        radius,
                        -1.57079632679f,
                        -1.57079632679f+
                        6.28318530718f*
                        readiness,
                        48
                    );

                    dl->PathStroke(
                        IM_COL32(
                            43,
                            142,
                            238,
                            255
                        ),
                        0,
                        8.0f
                    );
                }

                char percentText[32]{};

                std::snprintf(
                    percentText,
                    sizeof(percentText),
                    "%d%%",
                    readinessPercent
                );

                const ImVec2 percentSize=
                    ImGui::CalcTextSize(
                        percentText
                    );

                dl->AddText(
                    ImVec2(
                        center.x-
                        percentSize.x*0.5f,
                        center.y-
                        percentSize.y*0.5f
                    ),
                    IM_COL32(
                        242,
                        245,
                        250,
                        255
                    ),
                    percentText
                );

                const float checklistX=
                    p.x+160.0f;

                const auto readinessRow=
                    [&](float y,
                        const char* label,
                        bool ready)
                    {
                        dl->AddCircleFilled(
                            ImVec2(
                                checklistX+5.0f,
                                y+7.0f
                            ),
                            4.0f,
                            ready
                                ? IM_COL32(
                                    91,
                                    203,
                                    111,
                                    255
                                )
                                : IM_COL32(
                                    196,
                                    145,
                                    39,
                                    255
                                )
                        );

                        dl->AddText(
                            ImVec2(
                                checklistX+18.0f,
                                y
                            ),
                            ready
                                ? IM_COL32(
                                    220,
                                    226,
                                    235,
                                    255
                                )
                                : IM_COL32(
                                    158,
                                    165,
                                    176,
                                    255
                                ),
                            label
                        );
                    };

                readinessRow(
                    p.y+50.0f,
                    "Case location + date",
                    caseLocated && caseDated
                );

                readinessRow(
                    p.y+77.0f,
                    "Vehicles assigned",
                    vehiclesReady
                );

                readinessRow(
                    p.y+104.0f,
                    "Evidence available",
                    evidenceReady
                );

                readinessRow(
                    p.y+131.0f,
                    "Measurements available",
                    measurementsReady
                );
            }
            endSurface();

            ImGui::SameLine(
                0.0f,
                gap
            );

            beginSurface(
                "OverviewNextStepVisual",
                ImVec2(
                    rightW,
                    190.0f
                ),
                true,
                ImGuiWindowFlags_NoScrollbar
            );
            {
                const ImVec2 p=
                    ImGui::GetCursorScreenPos();

                drawIconBadge(
                    UiGlyph::Info,
                    p,
                    36.0f,
                    true
                );

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        p.x+48.0f,
                        p.y+1.0f
                    )
                );

                ImGui::Text(
                    "NEXT STEP"
                );

                const char* nextStep=
                    !(caseLocated && caseDated)
                        ? "Set incident date and location"
                        : !evidenceReady
                            ? "Add or link evidence"
                            : !measurementsReady
                                ? "Add scene measurements"
                                : "Review analysis modules";

                const char* blocker=
                    readiness>=1.0f
                        ? "READY TO PROCEED"
                        : "PREREQUISITE OPEN";

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        p.x,
                        p.y+60.0f
                    )
                );

                ImGui::TextWrapped(
                    "%s",
                    nextStep
                );

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        p.x,
                        p.y+103.0f
                    )
                );

                ImGui::TextColored(
                    readiness>=1.0f
                        ? ImVec4(
                            0.36f,
                            0.80f,
                            0.44f,
                            1.0f
                        )
                        : ImVec4(
                            0.98f,
                            0.68f,
                            0.08f,
                            1.0f
                        ),
                    "%s",
                    blocker
                );

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        p.x,
                        p.y+137.0f
                    )
                );

                const char* nextActionLabel=
                    !(caseLocated && caseDated)
                        ? "SET CASE DETAILS"
                        : !evidenceReady
                            ? "OPEN EVIDENCE"
                            : !measurementsReady
                                ? "ADD MEASUREMENTS"
                                : "REVIEW MODULES";

                if (editorButton(
                    nextActionLabel,
                    std::min(
                        166.0f,
                        rightW-24.0f
                    ),
                    true))
                {
                    if (!(caseLocated && caseDated))
                    {
                        ImGui::SetWindowFocus(
                            "Case View"
                        );
                    }
                    else if (!evidenceReady ||
                             !measurementsReady)
                    {
                        ImGui::SetWindowFocus(
                            "Evidence"
                        );
                    }
                    else
                    {
                        ImGui::SetWindowFocus(
                            "Analysis"
                        );
                    }
                }
            }
            endSurface();

            ImGui::Spacing();

            const float moduleGap=
                7.0f;

            const float moduleW=
                std::max(
                    135.0f,
                    (
                        ImGui::GetContentRegionAvail().x-
                        moduleGap*3.0f
                    )/
                    4.0f
                );

            drawMetricTile(
                "AnalysisQuickSpeed",
                UiGlyph::SpeedAnalysis,
                "SPEED",
                measurementsReady
                    ? "AVAILABLE"
                    : "LOCKED",
                "",
                moduleW,
                66.0f
            );

            ImGui::SameLine(
                0.0f,
                moduleGap
            );

            drawMetricTile(
                "AnalysisQuickImpact",
                UiGlyph::Impact,
                "IMPACT",
                evidenceReady
                    ? "AVAILABLE"
                    : "LOCKED",
                "",
                moduleW,
                66.0f
            );

            ImGui::SameLine(
                0.0f,
                moduleGap
            );

            drawMetricTile(
                "AnalysisQuickTrajectory",
                UiGlyph::Trajectory,
                "TRAJECTORY",
                measurementsReady
                    ? "AVAILABLE"
                    : "LOCKED",
                "",
                moduleW,
                66.0f
            );

            ImGui::SameLine(
                0.0f,
                moduleGap
            );

            drawMetricTile(
                "AnalysisQuickSight",
                UiGlyph::LineOfSight,
                "LINE OF SIGHT",
                caseLocated
                    ? "AVAILABLE"
                    : "LOCKED",
                "",
                moduleW,
                66.0f
            );

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Modules"))
        {
            ImGui::Spacing();

            ImGui::Text("ANALYSIS MODULES");
            ImGui::SameLine(0.0f,10.0f);
            ImGui::TextDisabled("Choose a reconstruction method based on the evidence available.");
            ImGui::Separator();

            const float contentW=ImGui::GetContentRegionAvail().x;
            const float gap=8.0f;
            const float moduleW=std::max(280.0f,(contentW-gap)/2.0f);

            drawModuleCard(
                "TabbedSkidModule",
                UiGlyph::Ruler,
                "SKID ANALYSIS",
                "Calculate vehicle speed from skid distance, friction and road-surface data.",
                "NOT STARTED",
                StatusTone::Neutral,
                "CONFIGURE",
                moduleW
            );

            ImGui::SameLine(0.0f,gap);

            drawModuleCard(
                "TabbedMomentumModule",
                UiGlyph::Momentum,
                "MOMENTUM ANALYSIS",
                "Analyze vehicle motion using conservation of momentum and collision dynamics.",
                "AWAITING EVIDENCE",
                StatusTone::Accent,
                "OPEN",
                moduleW
            );

            ImGui::Spacing();

            drawModuleCard(
                "TabbedSpeedModule",
                UiGlyph::Speed,
                "SPEED ANALYSIS",
                "Determine vehicle speed from crush, throw distance and simulation evidence.",
                "AWAITING EVIDENCE",
                StatusTone::Accent,
                "OPEN",
                moduleW
            );

            ImGui::SameLine(0.0f,gap);

            drawModuleCard(
                "TabbedResultsModule",
                UiGlyph::Report,
                "RECONSTRUCTION RESULTS",
                "Compile analysis outputs into the complete incident reconstruction and report.",
                "NOT STARTED",
                StatusTone::Neutral,
                "OPEN",
                moduleW
            );

            ImGui::EndTabItem();
        }

        // ====================================================
        // TAB 3: WORKFLOW
        // ====================================================

        if (ImGui::BeginTabItem("Workflow"))
        {
            ImGui::Spacing();

            ImGui::Text("ANALYSIS WORKFLOW");
            ImGui::SameLine(0.0f,10.0f);
            ImGui::TextDisabled("Move through the reconstruction process one stage at a time.");
            ImGui::Separator();

            struct WorkflowStep
            {
                const char* id;
                int number;
                UiGlyph glyph;
                const char* title;
                const char* note;
                const char* state;
                StatusTone tone;
            };

            const WorkflowStep steps[]={
                {"WorkflowCollect",1,UiGlyph::Document,"COLLECT EVIDENCE",
                 "Add skid marks, debris fields, scene markers and measurements.",
                 "CURRENT STEP",StatusTone::Accent},

                {"WorkflowLink",2,UiGlyph::Link,"LINK MEASUREMENTS",
                 "Associate the collected evidence with scene elements and vehicles.",
                 "WAITING",StatusTone::Neutral},

                {"WorkflowRun",3,UiGlyph::Bars,"RUN ANALYSIS",
                 "Configure the required reconstruction modules and execute calculations.",
                 "WAITING",StatusTone::Neutral},

                {"WorkflowReview",4,UiGlyph::Report,"REVIEW RESULTS",
                 "Validate calculated outputs and prepare reconstruction findings.",
                 "WAITING",StatusTone::Neutral}
            };

            const float contentW=ImGui::GetContentRegionAvail().x;
            const float gap=8.0f;
            const float cardW=std::max(280.0f,(contentW-gap)/2.0f);

            auto drawWorkflowCard = [](const WorkflowStep& step,float width)
            {
                beginSurface(
                    step.id,
                    ImVec2(width,148.0f),
                    true,
                    ImGuiWindowFlags_NoScrollbar
                );

                const ImVec2 p=ImGui::GetCursorScreenPos();
                ImDrawList* dl=ImGui::GetWindowDrawList();
                const ImVec4 tone=toneColor(step.tone);

                dl->AddCircle(
                    ImVec2(p.x+20.0f,p.y+20.0f),
                    20.0f,
                    toU32(tone),
                    24,
                    2.0f
                );

                char number[8]{};
                std::snprintf(number,sizeof(number),"%d",step.number);
                const ImVec2 numberSize=ImGui::CalcTextSize(number);

                dl->AddText(
                    ImVec2(
                        p.x+20.0f-numberSize.x*0.5f,
                        p.y+20.0f-numberSize.y*0.5f
                    ),
                    toU32(tone),
                    number
                );

                drawIconBadge(
                    step.glyph,
                    ImVec2(p.x+52.0f,p.y+1.0f),
                    38.0f,
                    step.tone==StatusTone::Accent
                );

                ImGui::SetCursorScreenPos(ImVec2(p.x+102.0f,p.y+3.0f));
                ImGui::Text("%s",step.title);

                ImGui::SetCursorScreenPos(ImVec2(p.x+102.0f,p.y+29.0f));
                drawStatus(step.state,step.tone);

                ImGui::SetCursorScreenPos(ImVec2(p.x+20.0f,p.y+74.0f));
                ImGui::PushTextWrapPos(p.x+width-20.0f);
                ImGui::TextDisabled("%s",step.note);
                ImGui::PopTextWrapPos();

                endSurface();
            };

            drawWorkflowCard(steps[0],cardW);
            ImGui::SameLine(0.0f,gap);
            drawWorkflowCard(steps[1],cardW);

            ImGui::Spacing();

            drawWorkflowCard(steps[2],cardW);
            ImGui::SameLine(0.0f,gap);
            drawWorkflowCard(steps[3],cardW);

            ImGui::EndTabItem();
        }

        // ====================================================
        // TAB 4: RESULTS
        // ====================================================

        if (ImGui::BeginTabItem("Results"))
        {
            ImGui::Spacing();

            ImGui::Text("RESULTS & REPORTING");
            ImGui::SameLine(0.0f,10.0f);
            ImGui::TextDisabled("Calculated reconstruction outputs will appear here.");
            ImGui::Separator();

            beginSurface("ResultsEmptyState",ImVec2(0.0f,184.0f),false,ImGuiWindowFlags_NoScrollbar);
            {
                const ImVec2 pos=ImGui::GetWindowPos();
                const ImVec2 size=ImGui::GetWindowSize();

                const ImVec2 iconPos(
                    pos.x+size.x*0.5f-21.0f,
                    pos.y+24.0f
                );

                drawIconBadge(UiGlyph::Report,iconPos,42.0f,false);

                const char* title="No reconstruction results yet";
                const char* note=
                    "Run an analysis module to generate calculated speeds, momentum values and reportable findings.";

                const float titleW=ImGui::CalcTextSize(title).x;
                const float noteW=ImGui::CalcTextSize(note).x;

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        pos.x+(size.x-titleW)*0.5f,
                        pos.y+78.0f
                    )
                );
                ImGui::Text("%s",title);

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        pos.x+std::max(18.0f,(size.x-noteW)*0.5f),
                        pos.y+107.0f
                    )
                );
                ImGui::TextDisabled("%s",note);

                const float reportW=138.0f;
                ImGui::SetCursorScreenPos(
                    ImVec2(
                        pos.x+(size.x-reportW)*0.5f,
                        pos.y+139.0f
                    )
                );
                editorButton("VIEW REPORTS",reportW,false,false);
            }
            endSurface();

            ImGui::Spacing();

            ImGui::Text("QUICK ACTIONS");
            ImGui::Separator();

            const float contentW=ImGui::GetContentRegionAvail().x;
            const float gap=8.0f;
            const float actionW=std::max(
                210.0f,
                (contentW-(gap*2.0f))/3.0f
            );

            auto drawActionCard = [](
                const char* id,
                UiGlyph glyph,
                const char* title,
                const char* note,
                const char* action,
                float width,
                bool primary,
                bool enabled)
            {
                beginSurface(
                    id,
                    ImVec2(width,126.0f),
                    true,
                    ImGuiWindowFlags_NoScrollbar
                );

                const ImVec2 p=ImGui::GetCursorScreenPos();

                drawIconBadge(glyph,p,36.0f,primary);

                ImGui::SetCursorScreenPos(ImVec2(p.x+48.0f,p.y+1.0f));
                ImGui::Text("%s",title);

                ImGui::SetCursorScreenPos(ImVec2(p.x,p.y+48.0f));
                ImGui::PushTextWrapPos(p.x+width-18.0f);
                ImGui::TextDisabled("%s",note);
                ImGui::PopTextWrapPos();

                ImGui::SetCursorPosY(84.0f);
                editorButton(
                    action,
                    ImGui::GetContentRegionAvail().x,
                    primary,
                    enabled
                );

                endSurface();
            };

            drawActionCard(
                "ResultActionSkid",
                UiGlyph::Ruler,
                "START SKID ANALYSIS",
                "Configure skid-distance and friction inputs.",
                "START",
                actionW,
                true,
                true
            );

            ImGui::SameLine(0.0f,gap);

            drawActionCard(
                "ResultActionEvidence",
                UiGlyph::Link,
                "LINK EVIDENCE",
                "Associate measurements with the reconstruction scene.",
                "OPEN EVIDENCE",
                actionW,
                false,
                true
            );

            ImGui::SameLine(0.0f,gap);

            drawActionCard(
                "ResultActionAll",
                UiGlyph::Bars,
                "RUN ALL ANALYSES",
                "Execute all configured analysis modules.",
                "RUN ALL",
                actionW,
                false,
                false
            );

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
        selectOverviewOnFirstFrame=false;
    }

    ImGui::PopStyleVar();

    ImGui::End();
}

