// ROADSAFE_UI_COMPONENT_V20
// Component: Case View
// Included from src/main.cpp so behavior/state linkage stays unchanged.

static void drawCaseView()
{
    ImGui::Begin("Case View");

    drawRoadSafePipelineBar();
    ImGui::Spacing();

    beginSurface("CaseHeader",ImVec2(0.0f,62.0f),false,ImGuiWindowFlags_NoScrollbar);
    const ImVec2 h=ImGui::GetCursorScreenPos();
    drawIconBadge(UiGlyph::Folder,h,40.0f,true);

    ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+1.0f));
    ImGui::Text("CASE OVERVIEW");

    ImGui::SetCursorScreenPos(ImVec2(h.x+52.0f,h.y+24.0f));
    ImGui::TextDisabled("CASE COMMAND CENTER");
    const float draftWidth=108.0f;
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetWindowPos().x+ImGui::GetWindowSize().x-draftWidth-12.0f,h.y+5.0f));
    editorButton("DRAFT",draftWidth,true);
    endSurface();
    ImGui::Spacing();

    const float w=ImGui::GetContentRegionAvail().x;
    const float gap=7.0f;
    const float mw=std::max(150.0f,(w-gap*3.0f)/4.0f);
    drawMetricTile("CaseId",UiGlyph::Hash,"CASE ID",roadsafe::textOr(gRoadSafeCase.identity.caseNumber,"UNASSIGNED"),"",mw,64.0f);
    ImGui::SameLine(0.0f,gap); drawMetricTile("CaseDate",UiGlyph::Calendar,"DATE",roadsafe::textOr(gRoadSafeCase.identity.accidentDateTime,"NOT SET"),"",mw,64.0f);
    ImGui::SameLine(0.0f,gap); drawMetricTile("CaseLocation",UiGlyph::Pin,"LOCATION",roadsafe::textOr(gRoadSafeCase.identity.location,"NOT SET"),"",mw,64.0f);
    ImGui::SameLine(0.0f,gap); drawMetricTile("CaseUpdated",UiGlyph::Clock,"UPDATED","JUST NOW","",mw,64.0f);
    ImGui::Spacing();

    ImGui::Text("CASE PIPELINE");
    ImGui::Separator();

    beginSurface(
        "CasePipelineCompact",
        ImVec2(0.0f,88.0f),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        const ImVec2 p=
            ImGui::GetCursorScreenPos();

        const float innerW=
            ImGui::GetWindowContentRegionMax().x-
            ImGui::GetWindowContentRegionMin().x;

        const float stageGap=7.0f;
        const float stageW=
            std::max(
                130.0f,
                (innerW-stageGap*3.0f)/4.0f
            );

        ImDrawList* dl=
            ImGui::GetWindowDrawList();

        const auto stage=
            [&](int index,
                UiGlyph glyph,
                const char* title,
                const char* state,
                float progress,
                ImU32 stateColor)
            {
                const float x=
                    p.x+
                    index*(stageW+stageGap);

                drawIconBadge(
                    glyph,
                    ImVec2(x,p.y+1.0f),
                    32.0f,
                    index<=1
                );

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        x+42.0f,
                        p.y+1.0f
                    )
                );

                ImGui::Text(
                    "%s",
                    title
                );

                ImGui::SetCursorScreenPos(
                    ImVec2(
                        x+42.0f,
                        p.y+24.0f
                    )
                );

                ImGui::TextColored(
                    ImGui::ColorConvertU32ToFloat4(
                        stateColor
                    ),
                    "%s",
                    state
                );

                const float barY=
                    p.y+58.0f;

                dl->AddRectFilled(
                    ImVec2(x,barY),
                    ImVec2(
                        x+stageW-5.0f,
                        barY+4.0f
                    ),
                    IM_COL32(
                        55,
                        59,
                        66,
                        255
                    ),
                    2.0f
                );

                dl->AddRectFilled(
                    ImVec2(x,barY),
                    ImVec2(
                        x+
                        (stageW-5.0f)*
                        std::max(
                            0.0f,
                            std::min(
                                1.0f,
                                progress
                            )
                        ),
                        barY+4.0f
                    ),
                    stateColor,
                    2.0f
                );

                if (index<3)
                {
                    const float connectorX=
                        x+stageW-1.0f;

                    dl->AddLine(
                        ImVec2(
                            connectorX,
                            p.y+17.0f
                        ),
                        ImVec2(
                            connectorX+stageGap-2.0f,
                            p.y+17.0f
                        ),
                        IM_COL32(
                            75,
                            81,
                            90,
                            255
                        ),
                        1.0f
                    );
                }
            };

        stage(
            0,
            UiGlyph::Folder,
            "SCENE SETUP",
            "READY",
            1.0f,
            IM_COL32(
                92,
                205,
                112,
                255
            )
        );

        stage(
            1,
            UiGlyph::Evidence,
            "EVIDENCE",
            "IN PROGRESS",
            0.32f,
            IM_COL32(
                37,
                132,
                229,
                255
            )
        );

        stage(
            2,
            UiGlyph::Bars,
            "ANALYSIS",
            "WAITING",
            0.0f,
            IM_COL32(
                142,
                149,
                160,
                255
            )
        );

        stage(
            3,
            UiGlyph::Document,
            "REPORT",
            "DRAFT",
            0.0f,
            IM_COL32(
                142,
                149,
                160,
                255
            )
        );
    }
    endSurface();
    ImGui::Spacing();
    beginSurface("IncidentSummary",ImVec2(0.0f,70.0f),false,ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 p=ImGui::GetCursorScreenPos();
        const float right=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x;

        drawIconBadge(UiGlyph::Document,p,40.0f,false);

        ImGui::SetCursorScreenPos(ImVec2(p.x+52.0f,p.y+3.0f));
        ImGui::Text("INCIDENT SUMMARY");

        ImGui::SetCursorScreenPos(ImVec2(p.x+52.0f,p.y+29.0f));
        ImGui::TextDisabled(
            "No summary yet.");

        const float editW=132.0f;
        ImGui::SetCursorScreenPos(ImVec2(right-editW-12.0f,p.y+13.0f));
        editorButton("EDIT SUMMARY",editW);
    }
    endSurface();
    ImGui::Spacing();

    const float dashboardW=
        ImGui::GetContentRegionAvail().x;

    const float dashboardGap=
        8.0f;

    const float sideW=
        std::max(
            330.0f,
            dashboardW*0.31f
        );

    const float inventoryW=
        std::max(
            540.0f,
            dashboardW-
            sideW-
            dashboardGap
        );

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

    beginSurface(
        "CaseInventoryDashboard",
        ImVec2(
            inventoryW,
            202.0f
        ),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        const ImVec2 p=
            ImGui::GetCursorScreenPos();

        ImGui::Text(
            "CASE INVENTORY"
        );

        ImGui::SameLine(
            0.0f,
            10.0f
        );

        ImGui::TextDisabled(
            "LIVE CASE RECORDS"
        );

        ImDrawList* dl=
            ImGui::GetWindowDrawList();

        const float innerW=
            ImGui::GetWindowContentRegionMax().x-
            ImGui::GetWindowContentRegionMin().x;

        const float tileGap=
            7.0f;

        const float topTileW=
            (
                innerW-
                tileGap*2.0f
            )/
            3.0f;

        const float bottomTileW=
            (
                innerW-
                tileGap
            )/
            2.0f;

        const float tileH=
            62.0f;

        const float topY=
            p.y+38.0f;

        const float bottomY=
            topY+
            tileH+
            tileGap;

        const auto inventoryTile=
            [&](float x,
                float y,
                float width,
                UiGlyph glyph,
                const char* label,
                std::size_t value,
                bool active)
            {
                const ImVec2 a(
                    x,
                    y
                );

                const ImVec2 b(
                    x+width,
                    y+tileH
                );

                dl->AddRectFilled(
                    a,
                    b,
                    IM_COL32(
                        30,
                        33,
                        38,
                        255
                    ),
                    4.0f
                );

                dl->AddRect(
                    a,
                    b,
                    active
                        ? IM_COL32(
                            63,
                            79,
                            96,
                            255
                        )
                        : IM_COL32(
                            58,
                            62,
                            69,
                            255
                        ),
                    4.0f
                );

                drawIconBadge(
                    glyph,
                    ImVec2(
                        x+8.0f,
                        y+13.0f
                    ),
                    32.0f,
                    active
                );

                dl->AddText(
                    ImVec2(
                        x+50.0f,
                        y+10.0f
                    ),
                    ImGui::GetColorU32(
                        ImGuiCol_TextDisabled
                    ),
                    label
                );

                char countText[32]{};

                std::snprintf(
                    countText,
                    sizeof(countText),
                    "%zu",
                    value
                );

                dl->AddText(
                    ImVec2(
                        x+50.0f,
                        y+31.0f
                    ),
                    ImGui::GetColorU32(
                        ImGuiCol_Text
                    ),
                    countText
                );

                const char* stateText=
                    value>0
                        ? "ACTIVE"
                        : "EMPTY";

                const ImVec2 stateSize=
                    ImGui::CalcTextSize(
                        stateText
                    );

                dl->AddText(
                    ImVec2(
                        b.x-
                        stateSize.x-
                        10.0f,
                        y+31.0f
                    ),
                    value>0
                        ? IM_COL32(
                            95,
                            205,
                            115,
                            255
                        )
                        : IM_COL32(
                            137,
                            144,
                            154,
                            255
                        ),
                    stateText
                );
            };

        inventoryTile(
            p.x,
            topY,
            topTileW,
            UiGlyph::Vehicle,
            "VEHICLES",
            vehicleCount,
            vehicleCount>0
        );

        inventoryTile(
            p.x+
            topTileW+
            tileGap,
            topY,
            topTileW,
            UiGlyph::Evidence,
            "EVIDENCE",
            evidenceCount,
            evidenceCount>0
        );

        inventoryTile(
            p.x+
            (topTileW+tileGap)*2.0f,
            topY,
            topTileW,
            UiGlyph::Measurement,
            "MEASUREMENTS",
            measurementCount,
            measurementCount>0
        );

        inventoryTile(
            p.x,
            bottomY,
            bottomTileW,
            UiGlyph::People,
            "PEOPLE",
            personCount,
            personCount>0
        );

        inventoryTile(
            p.x+
            bottomTileW+
            tileGap,
            bottomY,
            bottomTileW,
            UiGlyph::Witness,
            "WITNESSES",
            witnessCount,
            witnessCount>0
        );
    }
    endSurface();

    ImGui::SameLine(
        0.0f,
        dashboardGap
    );

    ImGui::BeginGroup();

    beginSurface(
        "CaseNextActionDashboard",
        ImVec2(
            sideW,
            112.0f
        ),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        const ImVec2 p=
            ImGui::GetCursorScreenPos();

        drawIconBadge(
            UiGlyph::Next,
            p,
            34.0f,
            true
        );

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
                ? "Complete incident date and location"
                : evidenceCount==0
                    ? "Add case evidence"
                    : measurementCount==0
                        ? "Add scene measurements"
                        : "Continue reconstruction";

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
                p.y+46.0f
            )
        );

        ImGui::TextWrapped(
            "%s",
            actionText
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x,
                p.y+72.0f
            )
        );

        if (editorButton(
            actionButton,
            std::min(
                156.0f,
                sideW-22.0f
            ),
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
    endSurface();

    ImGui::Spacing();

    beginSurface(
        "CasePartiesDashboard",
        ImVec2(
            sideW,
            82.0f
        ),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        const ImVec2 p=
            ImGui::GetCursorScreenPos();

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

        ImGui::SetCursorScreenPos(
            ImVec2(
                p.x,
                p.y+29.0f
            )
        );

        ImGui::TextDisabled(
            "%s",
            partiesText
        );

        const float partyW=
            104.0f;

        const float right=
            ImGui::GetWindowPos().x+
            ImGui::GetWindowContentRegionMax().x;

        ImGui::SetCursorScreenPos(
            ImVec2(
                right-
                partyW-
                8.0f,
                p.y+16.0f
            )
        );

        editorButton(
            "ADD PARTY",
            partyW
        );
    }
    endSurface();

    ImGui::EndGroup();
    ImGui::End();
}

