// ROADSAFE_UI_COMPONENT_V20
// Component: Evidence View
// Included from src/main.cpp so behavior/state linkage stays unchanged.

static void drawEvidenceView()
{
    static int selectedEvidenceIndex=-1;

    ImGui::Begin("Evidence");

    const std::size_t evidenceCount=
        roadSafeActiveEvidenceCount();

    std::size_t documentedCount=0;

    for (const auto& evidence:
         gRoadSafeCase.evidence)
    {
        if (!evidence.active)
            continue;

        if (!evidence.description.empty() &&
            !evidence.collectionStatus.empty())
        {
            ++documentedCount;
        }
    }

    std::vector<bool> evidenceLinked(
        gRoadSafeCase.evidence.size(),
        false
    );

    for (const auto& entity:
         gRoadSafeCase.sceneEntities)
    {
        if (!entity.active ||
            entity.kind!=
                roadsafe::SceneEntityKind::Evidence ||
            entity.recordIndex<0)
        {
            continue;
        }

        const std::size_t recordIndex=
            static_cast<std::size_t>(
                entity.recordIndex
            );

        if (recordIndex>=
            gRoadSafeCase.evidence.size())
        {
            continue;
        }

        if (!gRoadSafeCase.evidence[
                recordIndex
            ].active)
        {
            continue;
        }

        evidenceLinked[
            recordIndex
        ]=true;
    }

    const std::size_t linkedCount=
        static_cast<std::size_t>(
            std::count(
                evidenceLinked.begin(),
                evidenceLinked.end(),
                true
            )
        );

    if (selectedEvidenceIndex<0 ||
        static_cast<std::size_t>(
            selectedEvidenceIndex
        )>=gRoadSafeCase.evidence.size() ||
        !gRoadSafeCase.evidence[
            static_cast<std::size_t>(
                selectedEvidenceIndex
            )
        ].active)
    {
        selectedEvidenceIndex=-1;

        for (std::size_t i=0;
             i<gRoadSafeCase.evidence.size();
             ++i)
        {
            if (gRoadSafeCase.evidence[i].active)
            {
                selectedEvidenceIndex=
                    static_cast<int>(i);
                break;
            }
        }
    }

    const roadsafe::EvidenceRecord* latestEvidence=
        nullptr;

    for (auto it=
            gRoadSafeCase.evidence.rbegin();
         it!=gRoadSafeCase.evidence.rend();
         ++it)
    {
        if (it->active)
        {
            latestEvidence=
                &(*it);
            break;
        }
    }

    char evidenceCountText[32]{};
    char documentedText[32]{};
    char linkedText[32]{};

    std::snprintf(
        evidenceCountText,
        sizeof(evidenceCountText),
        "%zu",
        evidenceCount
    );

    std::snprintf(
        documentedText,
        sizeof(documentedText),
        "%zu",
        documentedCount
    );

    std::snprintf(
        linkedText,
        sizeof(linkedText),
        "%zu",
        linkedCount
    );

    const char* latestText=
        latestEvidence
            ? (
                !latestEvidence->description.empty()
                    ? latestEvidence->description.c_str()
                    : latestEvidence->id.c_str()
              )
            : "NONE";

    beginSurface(
        "EvidenceHeader",
        ImVec2(
            0.0f,
            62.0f
        ),
        false,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        const ImVec2 h=
            ImGui::GetCursorScreenPos();

        const float right=
            ImGui::GetWindowPos().x+
            ImGui::GetWindowContentRegionMax().x;

        drawIconBadge(
            UiGlyph::Image,
            h,
            40.0f,
            true
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                h.x+52.0f,
                h.y+1.0f
            )
        );

        ImGui::Text(
            "EVIDENCE WORKSPACE"
        );

        ImGui::SetCursorScreenPos(
            ImVec2(
                h.x+52.0f,
                h.y+24.0f
            )
        );

        ImGui::TextDisabled(
            "INTAKE  /  REVIEW  /  SCENE LINKAGE"
        );

        const float importW=
            124.0f;

        const float addW=
            132.0f;

        ImGui::SetCursorScreenPos(
            ImVec2(
                right-
                importW-
                addW-
                10.0f,
                h.y+5.0f
            )
        );

        editorButton(
            "ADD EVIDENCE",
            addW,
            true
        );

        ImGui::SameLine();

        editorButton(
            "IMPORT PHOTOS",
            importW
        );
    }
    endSurface();

    ImGui::Spacing();

    beginSurface(
        "EvidenceFilters",
        ImVec2(
            0.0f,
            42.0f
        ),
        false,
        ImGuiWindowFlags_NoScrollbar
    );

    editorButton(
        "ALL",
        70.0f,
        true
    );

    ImGui::SameLine();
    editorButton(
        "PHOTOS",
        82.0f
    );

    ImGui::SameLine();
    editorButton(
        "SKID MARKS",
        106.0f
    );

    ImGui::SameLine();
    editorButton(
        "DEBRIS",
        82.0f
    );

    ImGui::SameLine();
    editorButton(
        "MEASUREMENTS",
        118.0f
    );

    ImGui::SameLine();
    editorButton(
        "MARKERS",
        88.0f
    );

    const float searchW=
        220.0f;

    const float rightEdge=
        ImGui::GetWindowWidth()-
        searchW-
        14.0f;

    if (rightEdge>
        ImGui::GetCursorPosX()+
        12.0f)
    {
        ImGui::SameLine();

        ImGui::SetCursorPosX(
            rightEdge
        );

        static char search[128]="";

        ImGui::SetNextItemWidth(
            searchW
        );

        ImGui::InputTextWithHint(
            "##EvidenceSearch",
            "Search evidence...",
            search,
            sizeof(search)
        );
    }

    endSurface();

    ImGui::Spacing();

    const float width=
        ImGui::GetContentRegionAvail().x;

    const float metricGap=
        7.0f;

    const float metricW=
        std::max(
            150.0f,
            (
                width-
                metricGap*3.0f
            )/
            4.0f
        );

    drawMetricTile(
        "EvidenceTotal",
        UiGlyph::Evidence,
        "EVIDENCE",
        evidenceCountText,
        "",
        metricW,
        62.0f
    );

    ImGui::SameLine(
        0.0f,
        metricGap
    );

    drawMetricTile(
        "EvidenceDocumented",
        UiGlyph::Verified,
        "DOCUMENTED",
        documentedText,
        "",
        metricW,
        62.0f
    );

    ImGui::SameLine(
        0.0f,
        metricGap
    );

    drawMetricTile(
        "EvidenceLinked",
        UiGlyph::EvidenceLink,
        "SCENE LINKS",
        linkedText,
        "",
        metricW,
        62.0f
    );

    ImGui::SameLine(
        0.0f,
        metricGap
    );

    drawMetricTile(
        "EvidenceLatest",
        UiGlyph::Clock,
        "LATEST",
        latestText,
        "",
        metricW,
        62.0f
    );

    ImGui::Spacing();

    const float workspaceW=
        ImGui::GetContentRegionAvail().x;

    const float panelGap=
        8.0f;

    const float inspectorW=
        std::max(
            320.0f,
            workspaceW*0.31f
        );

    const float libraryW=
        std::max(
            520.0f,
            workspaceW-
            inspectorW-
            panelGap
        );

    const float workspaceH=
        std::max(
            330.0f,
            ImGui::GetContentRegionAvail().y-
            8.0f
        );

    beginSurface(
        "EvidenceLibraryLive",
        ImVec2(
            libraryW,
            workspaceH
        ),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        ImGui::Text(
            "EVIDENCE LIBRARY"
        );

        ImGui::SameLine(
            0.0f,
            10.0f
        );

        ImGui::TextDisabled(
            "%zu RECORDS",
            evidenceCount
        );

        ImGui::Separator();

        if (evidenceCount==0)
        {
            const ImVec2 wp=
                ImGui::GetWindowPos();

            const ImVec2 ws=
                ImGui::GetWindowSize();

            const ImVec2 center(
                wp.x+
                ws.x*0.5f,
                wp.y+
                112.0f
            );

            drawGlyph(
                ImGui::GetWindowDrawList(),
                UiGlyph::Image,
                center,
                50.0f,
                toU32(
                    colorMuted()
                )
            );

            ImGui::SetCursorPosY(
                137.0f
            );

            const char* title=
                "NO EVIDENCE YET";

            ImGui::SetCursorPosX(
                std::max(
                    12.0f,
                    (
                        ws.x-
                        ImGui::CalcTextSize(
                            title
                        ).x
                    )*
                    0.5f
                )
            );

            ImGui::Text(
                "%s",
                title
            );

            const char* note=
                "Add or import evidence to begin.";

            ImGui::SetCursorPosX(
                std::max(
                    12.0f,
                    (
                        ws.x-
                        ImGui::CalcTextSize(
                            note
                        ).x
                    )*
                    0.5f
                )
            );

            ImGui::TextDisabled(
                "%s",
                note
            );
        }
        else
        {
            ImGui::BeginChild(
                "##EvidenceRecordList",
                ImVec2(
                    0.0f,
                    -1.0f
                ),
                false
            );

            for (std::size_t i=0;
                 i<gRoadSafeCase.evidence.size();
                 ++i)
            {
                const auto& evidence=
                    gRoadSafeCase.evidence[i];

                if (!evidence.active)
                    continue;

                ImGui::PushID(
                    static_cast<int>(i)
                );

                const bool selected=
                    selectedEvidenceIndex==
                    static_cast<int>(i);

                const ImVec2 rowP=
                    ImGui::GetCursorScreenPos();

                const float rowW=
                    ImGui::GetContentRegionAvail().x;

                const float rowH=
                    58.0f;

                ImGui::InvisibleButton(
                    "##EvidenceRecord",
                    ImVec2(
                        rowW,
                        rowH
                    )
                );

                if (ImGui::IsItemClicked())
                {
                    selectedEvidenceIndex=
                        static_cast<int>(i);
                }

                ImDrawList* dl=
                    ImGui::GetWindowDrawList();

                if (selected)
                {
                    dl->AddRectFilled(
                        rowP,
                        ImVec2(
                            rowP.x+rowW,
                            rowP.y+rowH
                        ),
                        IM_COL32(
                            36,
                            45,
                            57,
                            255
                        ),
                        4.0f
                    );

                    dl->AddRect(
                        rowP,
                        ImVec2(
                            rowP.x+rowW,
                            rowP.y+rowH
                        ),
                        IM_COL32(
                            53,
                            126,
                            206,
                            255
                        ),
                        4.0f
                    );
                }
                else
                {
                    dl->AddLine(
                        ImVec2(
                            rowP.x,
                            rowP.y+
                            rowH-
                            1.0f
                        ),
                        ImVec2(
                            rowP.x+
                            rowW,
                            rowP.y+
                            rowH-
                            1.0f
                        ),
                        IM_COL32(
                            56,
                            60,
                            66,
                            255
                        )
                    );
                }

                drawIconBadge(
                    roadSafeEvidenceGlyph(evidence),
                    ImVec2(
                        rowP.x+8.0f,
                        rowP.y+12.0f
                    ),
                    32.0f,
                    selected
                );

                const char* rowTitle=
                    !evidence.description.empty()
                        ? evidence.description.c_str()
                        : (
                            !evidence.id.empty()
                                ? evidence.id.c_str()
                                : "Evidence"
                          );

                dl->AddText(
                    ImVec2(
                        rowP.x+50.0f,
                        rowP.y+9.0f
                    ),
                    ImGui::GetColorU32(
                        ImGuiCol_Text
                    ),
                    rowTitle
                );

                std::string rowMeta=
                    evidence.type.empty()
                        ? "Evidence"
                        : evidence.type;

                if (!evidence.collectionStatus.empty())
                {
                    rowMeta+=
                        "  /  "+
                        evidence.collectionStatus;
                }

                dl->AddText(
                    ImVec2(
                        rowP.x+50.0f,
                        rowP.y+31.0f
                    ),
                    ImGui::GetColorU32(
                        ImGuiCol_TextDisabled
                    ),
                    rowMeta.c_str()
                );

                if (i<evidenceLinked.size() &&
                    evidenceLinked[i])
                {
                    const char* sceneText=
                        "SCENE";

                    const ImVec2 sceneSize=
                        ImGui::CalcTextSize(
                            sceneText
                        );

                    dl->AddText(
                        ImVec2(
                            rowP.x+
                            rowW-
                            sceneSize.x-
                            12.0f,
                            rowP.y+
                            20.0f
                        ),
                        IM_COL32(
                            91,
                            203,
                            111,
                            255
                        ),
                        sceneText
                    );
                }

                ImGui::PopID();
            }

            ImGui::EndChild();
        }
    }
    endSurface();

    ImGui::SameLine(
        0.0f,
        panelGap
    );

    beginSurface(
        "EvidenceInspectorLive",
        ImVec2(
            inspectorW,
            workspaceH
        ),
        true,
        ImGuiWindowFlags_NoScrollbar
    );
    {
        ImGui::TextDisabled(
            "EVIDENCE INSPECTOR"
        );

        ImGui::Separator();

        if (selectedEvidenceIndex<0)
        {
            ImGui::Spacing();

            ImGui::Text(
                "SELECT AN ITEM"
            );

            ImGui::TextDisabled(
                "Choose an evidence record to inspect."
            );
        }
        else
        {
            const std::size_t selectedIndex=
                static_cast<std::size_t>(
                    selectedEvidenceIndex
                );

            const auto& evidence=
                gRoadSafeCase.evidence[
                    selectedIndex
                ];

            const char* inspectorTitle=
                !evidence.description.empty()
                    ? evidence.description.c_str()
                    : (
                        !evidence.id.empty()
                            ? evidence.id.c_str()
                            : "Evidence"
                      );

            ImGui::Spacing();

            ImGui::TextWrapped(
                "%s",
                inspectorTitle
            );

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            const auto field=
                [](const char* label,
                   const char* value)
                {
                    ImGui::TextDisabled(
                        "%s",
                        label
                    );

                    ImGui::TextWrapped(
                        "%s",
                        (
                            value &&
                            value[0]!='\0'
                        )
                            ? value
                            : "—"
                    );

                    ImGui::Spacing();
                };

            field(
                "TYPE",
                evidence.type.c_str()
            );

            field(
                "ID",
                evidence.id.c_str()
            );

            field(
                "COLLECTION",
                evidence.collectionStatus.c_str()
            );

            field(
                "PROVENANCE",
                roadsafe::provenanceName(
                    evidence.lineage.provenance
                )
            );

            field(
                "CONFIDENCE",
                roadsafe::confidenceName(
                    evidence.lineage.confidence
                )
            );

            field(
                "SOURCE",
                evidence.lineage.sourceId.c_str()
            );

            field(
                "SCENE LINK",
                (
                    selectedIndex<
                        evidenceLinked.size() &&
                    evidenceLinked[
                        selectedIndex
                    ]
                )
                    ? "LINKED"
                    : "NOT LINKED"
            );

            if (evidence.lengthMeters>0.0f)
            {
                ImGui::TextDisabled(
                    "LENGTH"
                );

                ImGui::Text(
                    "%.2f m",
                    evidence.lengthMeters
                );

                ImGui::Spacing();
            }

            if (evidence.widthMeters>0.0f)
            {
                ImGui::TextDisabled(
                    "WIDTH"
                );

                ImGui::Text(
                    "%.2f m",
                    evidence.widthMeters
                );

                ImGui::Spacing();
            }

            if (evidence.fieldRadiusMeters>0.0f)
            {
                ImGui::TextDisabled(
                    "FIELD RADIUS"
                );

                ImGui::Text(
                    "%.2f m",
                    evidence.fieldRadiusMeters
                );

                ImGui::Spacing();
            }

            if (evidence.estimatedPieceCount>0)
            {
                ImGui::TextDisabled(
                    "PIECE COUNT"
                );

                ImGui::Text(
                    "%d",
                    evidence.estimatedPieceCount
                );
            }
        }
    }
    endSurface();

    ImGui::End();
}
