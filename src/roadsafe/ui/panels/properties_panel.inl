// ROADSAFE_UI_COMPONENT_V20
// Component: Properties Panel
// Included from src/main.cpp so behavior/state linkage stays unchanged.

static void drawDeepPropertiesInspectorBody()
{
    static int roadSurfaceType=0;
    static float roadFriction=0.72f;
    static float roadGrade=0.0f;

    const int entity=
        gEditorShell.selectedEntity;

    auto* selectedSceneEntity=
        entity!=0
            ? roadSafeSceneEntity(entity)
            : nullptr;

    // ROADSAFE_PROPERTIES_HEADER_BLENDER_V18
    auto section=
        [](const char* label)
    {
        ImGui::Spacing();

        ImGui::PushStyleColor(
            ImGuiCol_Header,
            ImVec4(0.16f,0.17f,0.19f,1.0f)
        );

        ImGui::PushStyleColor(
            ImGuiCol_HeaderHovered,
            ImVec4(0.21f,0.22f,0.25f,1.0f)
        );

        ImGui::PushStyleColor(
            ImGuiCol_HeaderActive,
            ImVec4(0.23f,0.24f,0.27f,1.0f)
        );

        const std::string sectionName=
            label
                ? label
                : "";

        std::string displayLabel="     ";
        displayLabel+=sectionName;

        const bool open=
            ImGui::CollapsingHeader(
                displayLabel.c_str(),
                ImGuiTreeNodeFlags_DefaultOpen
            );

        const ImVec2 headerMin=
            ImGui::GetItemRectMin();

        const ImVec2 headerMax=
            ImGui::GetItemRectMax();

        UiGlyph glyph=
            UiGlyph::Info;

        if (sectionName=="TRANSFORM")
            glyph=UiGlyph::Transform;
        else if (sectionName=="VEHICLE IDENTITY")
            glyph=UiGlyph::VehicleIdentity;
        else if (sectionName=="VEHICLE PHYSICS")
            glyph=UiGlyph::VehiclePhysics;
        else if (sectionName=="INITIAL STATE")
            glyph=UiGlyph::Speed;
        else if (sectionName=="TIRES / ROAD")
            glyph=UiGlyph::Tire;
        else if (sectionName=="COLLISION")
            glyph=UiGlyph::Crash;
        else if (sectionName=="FORENSIC LINEAGE")
            glyph=UiGlyph::Provenance;
        else if (sectionName=="EVIDENCE METADATA")
            glyph=UiGlyph::Evidence;
        else if (sectionName=="SKID / TIRE MARK")
            glyph=UiGlyph::SkidMark;
        else if (sectionName=="SCENE MARKER")
            glyph=UiGlyph::ForensicMarker;
        else if (sectionName=="DEBRIS FIELD")
            glyph=UiGlyph::Debris;
        else if (sectionName=="MEASUREMENT SETTINGS")
            glyph=UiGlyph::Measurement;
        else if (sectionName=="3D ASSET / PBR")
            glyph=UiGlyph::Asset;
        else if (sectionName=="VISIBILITY")
            glyph=UiGlyph::Eye;
        else if (sectionName=="METADATA")
            glyph=UiGlyph::Fingerprint;
        else if (sectionName=="SCENE SETTINGS")
            glyph=UiGlyph::Settings;

        if (headerMax.x-headerMin.x>70.0f)
        {
            drawGlyph(
                ImGui::GetWindowDrawList(),
                glyph,
                ImVec2(
                    headerMin.x+31.0f,
                    (headerMin.y+headerMax.y)*0.5f
                ),
                17.0f,
                toU32(
                    open
                        ? colorText()
                        : colorMuted()
                )
            );
        }

        ImGui::PopStyleColor(3);
        return open;
    };
    // Keep enough room for the property label on the right.
    // Narrow inspectors no longer sacrifice labels to oversized editors.
    // ROADSAFE_PROPERTIES_WIDTH_V18
    const auto editorWidth=[]()
    {
        const float available=
            std::max(
                1.0f,
                ImGui::GetContentRegionAvail().x
            );

        const float labelReserve=
            std::min(
                150.0f,
                std::max(
                    120.0f,
                    available*0.38f
                )
            );

        return
            std::max(
                178.0f,
                available-labelReserve
            );
    };
    if (entity==0)
    {
        if (section("SCENE SETTINGS"))
        {
            ImGui::TextDisabled("Units");
            ImGui::SameLine();
            ImGui::TextUnformatted("Metric");

            ImGui::TextDisabled("Coordinate system");
            ImGui::SameLine();
            ImGui::TextUnformatted("World / local metric");

            ImGui::TextDisabled("Snap increment");

            ImGui::SetNextItemWidth(
                std::min(
                    ImGui::GetContentRegionAvail().x,
                    180.0f
                )
            );

            ImGui::DragFloat(
                "##SnapIncrement",
                &gEditorShell.snapValue,
                0.01f,
                0.01f,
                10.0f,
                "%.2f m"
            );

            ImGui::Checkbox(
                "Enable snapping",
                &gEditorShell.snapEnabled
            );
        }

        return;
    }

    if (!selectedSceneEntity)
    {
        ImGui::TextDisabled(
            "Selected scene object is no longer available."
        );
        return;
    }

    const bool entityEditLocked=
        selectedSceneEntity->locked;

    roadsafe::VehicleRecord* vehicle=nullptr;
    roadsafe::EvidenceRecord* evidence=nullptr;
    roadsafe::MeasurementRecord* measurement=nullptr;

    if (selectedSceneEntity->recordIndex>=0)
    {
        const std::size_t recordIndex=
            static_cast<std::size_t>(
                selectedSceneEntity->recordIndex
            );

        switch (selectedSceneEntity->kind)
        {
            case roadsafe::SceneEntityKind::Vehicle:
                if (recordIndex<
                    gRoadSafeCase.vehicles.size() &&
                    gRoadSafeCase.vehicles[
                        recordIndex
                    ].active)
                {
                    vehicle=
                        &gRoadSafeCase.vehicles[
                            recordIndex
                        ];
                }
                break;

            case roadsafe::SceneEntityKind::Evidence:
                if (recordIndex<
                    gRoadSafeCase.evidence.size() &&
                    gRoadSafeCase.evidence[
                        recordIndex
                    ].active)
                {
                    evidence=
                        &gRoadSafeCase.evidence[
                            recordIndex
                        ];
                }
                break;

            case roadsafe::SceneEntityKind::Measurement:
                if (recordIndex<
                    gRoadSafeCase.measurements.size() &&
                    gRoadSafeCase.measurements[
                        recordIndex
                    ].active)
                {
                    measurement=
                        &gRoadSafeCase.measurements[
                            recordIndex
                        ];
                }
                break;

            case roadsafe::SceneEntityKind::Environment:
                break;
        }
    }

    if (entityEditLocked)
    {
        ImGui::TextColored(
            colorAccent(),
            "LOCKED"
        );

        ImGui::SameLine(0.0f,8.0f);
        ImGui::TextDisabled(
            "Unlock in Visibility to edit."
        );
        ImGui::Spacing();
    }

    ImGui::Text(
        "%s",
        selectedSceneEntity->name.c_str()
    );

    ImGui::SameLine();

    ImGui::TextDisabled(
        "#%d",
        entity
    );

    if (entityEditLocked)
        ImGui::BeginDisabled();

    if (section("TRANSFORM"))
    {
        ImGui::SetNextItemWidth(
            editorWidth()
        );

        if (ImGui::DragFloat3(
            "Position",
            selectedSceneEntity->position.data(),
            0.01f,
            -10000.0f,
            10000.0f,
            "%.3f m"))
        {
            gRoadSafeCase.touch();
        }

        ImGui::SetNextItemWidth(
            editorWidth()
        );

        if (ImGui::DragFloat3(
            "Rotation",
            selectedSceneEntity->rotationDegrees.data(),
            0.25f,
            -360.0f,
            360.0f,
            "%.1f deg"))
        {
            gRoadSafeCase.touch();
        }

        ImGui::SetNextItemWidth(
            editorWidth()
        );

        if (ImGui::DragFloat3(
            "Scale",
            selectedSceneEntity->scale.data(),
            0.01f,
            0.001f,
            1000.0f,
            "%.3f"))
        {
            gRoadSafeCase.touch();
        }
    }

    if (selectedSceneEntity->kind==
        roadsafe::SceneEntityKind::Environment)
    {
        if (section("SURFACE"))
        {
            const char* surfaceTypes[]={
                "Asphalt",
                "Concrete",
                "Gravel",
                "Grass",
                "Unknown"
            };

            ImGui::SetNextItemWidth(
                editorWidth()
            );

            ImGui::Combo(
                "Surface type",
                &roadSurfaceType,
                surfaceTypes,
                5
            );

            ImGui::SetNextItemWidth(
                editorWidth()
            );

            ImGui::SliderFloat(
                "Friction coefficient",
                &roadFriction,
                0.05f,
                1.50f,
                "%.2f"
            );

            ImGui::SetNextItemWidth(
                editorWidth()
            );

            ImGui::DragFloat(
                "Road grade",
                &roadGrade,
                0.05f,
                -45.0f,
                45.0f,
                "%.2f deg"
            );
        }
    }

    if (selectedSceneEntity->kind==
        roadsafe::SceneEntityKind::Vehicle)
    {
        if (!vehicle)
        {
            ImGui::TextColored(
                ImVec4(0.92f,0.55f,0.20f,1.0f),
                "Vehicle record link is unavailable."
            );
        }
        else
        {
            if (section("VEHICLE IDENTITY"))
            {
                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Registration",
                    vehicle->registration,
                    96
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Make / ID",
                    vehicle->make,
                    96
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Model",
                    vehicle->model,
                    96
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Vehicle type",
                    vehicle->vehicleType,
                    96
                );
            }

            if (section("VEHICLE PHYSICS"))
            {
                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::DragFloat(
                    "Mass",
                    &vehicle->massKg,
                    5.0f,
                    1.0f,
                    100000.0f,
                    "%.1f kg"))
                {
                    gRoadSafeCase.touch();
                }

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::DragFloat(
                    "Wheelbase",
                    &vehicle->wheelbaseMeters,
                    0.01f,
                    0.50f,
                    10.0f,
                    "%.2f m"))
                {
                    gRoadSafeCase.touch();
                }

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::DragFloat(
                    "CG height",
                    &vehicle->cgHeightMeters,
                    0.01f,
                    0.05f,
                    5.0f,
                    "%.2f m"))
                {
                    gRoadSafeCase.touch();
                }
            }

            if (section("INITIAL STATE"))
            {
                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::DragFloat(
                    "Velocity",
                    &vehicle->initialSpeedMetersPerSecond,
                    0.10f,
                    -200.0f,
                    200.0f,
                    "%.2f m/s"))
                {
                    gRoadSafeCase.touch();
                }

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::DragFloat(
                    "Heading",
                    &vehicle->sceneHeadingDegrees,
                    0.25f,
                    -360.0f,
                    360.0f,
                    "%.1f deg"))
                {
                    gRoadSafeCase.touch();
                }

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::DragFloat(
                    "Steering",
                    &vehicle->steeringDegrees,
                    0.10f,
                    -60.0f,
                    60.0f,
                    "%.1f deg"))
                {
                    gRoadSafeCase.touch();
                }
            }

            if (section("TIRES / ROAD"))
            {
                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::SliderFloat(
                    "Friction coefficient",
                    &vehicle->frictionCoefficient,
                    0.05f,
                    1.50f,
                    "%.2f"))
                {
                    gRoadSafeCase.touch();
                }
            }

            if (section("COLLISION"))
            {
                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::DragFloat(
                    "Crush depth",
                    &vehicle->crushDepthMeters,
                    0.01f,
                    0.0f,
                    5.0f,
                    "%.3f m"))
                {
                    gRoadSafeCase.touch();
                }

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::SliderFloat(
                    "Restitution",
                    &vehicle->restitution,
                    0.0f,
                    1.0f,
                    "%.2f"))
                {
                    gRoadSafeCase.touch();
                }
            }

            if (section("FORENSIC LINEAGE"))
            {
                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeProvenanceCombo(
                    "Provenance",
                    vehicle->lineage.provenance
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeConfidenceCombo(
                    "Confidence",
                    vehicle->lineage.confidence
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Source",
                    vehicle->lineage.sourceId,
                    128
                );
            }
        }
    }

    if (selectedSceneEntity->kind==
        roadsafe::SceneEntityKind::Evidence)
    {
        if (!evidence)
        {
            ImGui::TextColored(
                ImVec4(0.92f,0.55f,0.20f,1.0f),
                "Evidence record link is unavailable."
            );
        }
        else
        {
            if (section("EVIDENCE METADATA"))
            {
                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Evidence type",
                    evidence->type,
                    96
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Collection status",
                    evidence->collectionStatus,
                    96
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Source",
                    evidence->lineage.sourceId,
                    128
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeProvenanceCombo(
                    "Provenance",
                    evidence->lineage.provenance
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeConfidenceCombo(
                    "Confidence class",
                    evidence->lineage.confidence
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::SliderFloat(
                    "Confidence score",
                    &evidence->confidenceScore,
                    0.0f,
                    1.0f,
                    "%.2f"))
                {
                    gRoadSafeCase.touch();
                }
            }

            if (evidence->type=="Skid Mark" &&
                section("SKID / TIRE MARK"))
            {
                const char* skidTypes[]={
                    "Braking skid",
                    "Yaw mark",
                    "Scuff mark",
                    "Tire scrub"
                };

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::Combo(
                    "Evidence subtype",
                    &evidence->categoryIndex,
                    skidTypes,
                    4))
                {
                    gRoadSafeCase.touch();
                }

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::DragFloat(
                    "Length",
                    &evidence->lengthMeters,
                    0.05f,
                    0.0f,
                    1000.0f,
                    "%.2f m"))
                {
                    gRoadSafeCase.touch();
                }

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::DragFloat(
                    "Width",
                    &evidence->widthMeters,
                    0.01f,
                    0.01f,
                    5.0f,
                    "%.2f m"))
                {
                    gRoadSafeCase.touch();
                }

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::SliderFloat(
                    "Friction coefficient",
                    &evidence->frictionCoefficient,
                    0.05f,
                    1.50f,
                    "%.2f"))
                {
                    gRoadSafeCase.touch();
                }
            }

            if (evidence->type=="Scene Marker" &&
                section("SCENE MARKER"))
            {
                const char* markerTypes[]={
                    "Reference",
                    "Impact",
                    "Evidence",
                    "Survey"
                };

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::Combo(
                    "Category",
                    &evidence->categoryIndex,
                    markerTypes,
                    4))
                {
                    gRoadSafeCase.touch();
                }
            }

            if (evidence->type=="Debris Field" &&
                section("DEBRIS FIELD"))
            {
                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::DragFloat(
                    "Field radius",
                    &evidence->fieldRadiusMeters,
                    0.05f,
                    0.0f,
                    100.0f,
                    "%.2f m"))
                {
                    gRoadSafeCase.touch();
                }

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::DragInt(
                    "Estimated pieces",
                    &evidence->estimatedPieceCount,
                    1.0f,
                    0,
                    10000))
                {
                    gRoadSafeCase.touch();
                }
            }
        }
    }

    if (selectedSceneEntity->kind==
        roadsafe::SceneEntityKind::Measurement)
    {
        if (!measurement)
        {
            ImGui::TextColored(
                ImVec4(0.92f,0.55f,0.20f,1.0f),
                "Measurement record link is unavailable."
            );
        }
        else
        {
            if (section("MEASUREMENT SETTINGS"))
            {
                const char* units[]={
                    "Metric",
                    "Imperial"
                };

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Measurement type",
                    measurement->type,
                    96
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (ImGui::Combo(
                    "Units",
                    &measurement->unitSystem,
                    units,
                    2))
                {
                    gRoadSafeCase.touch();
                }

                ImGui::SetNextItemWidth(
                    editorWidth()
                );

                if (measurement->type=="Angle")
                {
                    if (ImGui::DragFloat(
                        "Angle",
                        &measurement->value,
                        0.10f,
                        -360.0f,
                        360.0f,
                        "%.2f deg"))
                    {
                        gRoadSafeCase.touch();
                    }
                }
                else
                {
                    if (ImGui::DragFloat(
                        "Value",
                        &measurement->value,
                        0.01f,
                        0.0f,
                        100000.0f,
                        measurement->unitSystem==0
                            ? "%.3f m"
                            : "%.3f ft"))
                    {
                        gRoadSafeCase.touch();
                    }
                }

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Unit label",
                    measurement->unit,
                    32
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Method",
                    measurement->method,
                    128
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeInputText(
                    "Source",
                    measurement->sourceDescription,
                    128
                );

                if (ImGui::Checkbox(
                    "Lock measurement",
                    &measurement->locked))
                {
                    gRoadSafeCase.touch();
                }
            }

            if (section("FORENSIC LINEAGE"))
            {
                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeProvenanceCombo(
                    "Provenance",
                    measurement->lineage.provenance
                );

                ImGui::SetNextItemWidth(
                    editorWidth()
                );
                roadSafeConfidenceCombo(
                    "Confidence",
                    measurement->lineage.confidence
                );
            }
        }
    }

    if (section("3D ASSET / PBR"))
    {
        ImGui::TextDisabled(
            "Native GLB/glTF model assignment."
        );

        if (editorButton(
                "BROWSE ASSET LIBRARY",
                ImGui::GetContentRegionAvail().x,
                true,
                true))
        {
            gRoadSafeAssetLibrary.open=true;
            gRoadSafeAssetLibrary.requestFocus=true;
            gRoadSafeAssetLibrary.initialized=false;
        }

        if (!selectedSceneEntity->
                 asset.sourcePath.empty())
        {
            ImGui::TextDisabled(
                "Assigned model"
            );

            ImGui::PushTextWrapPos(
                ImGui::GetCursorPosX()+
                ImGui::GetContentRegionAvail().x
            );

            ImGui::TextUnformatted(
                selectedSceneEntity->
                    asset.sourcePath.c_str()
            );

            ImGui::PopTextWrapPos();

            if (editorButton(
                    "CLEAR ASSET",
                    ImGui::GetContentRegionAvail().x,
                    false,
                    true))
            {
                selectedSceneEntity->asset=
                    roadsafe::AssetReference{};

                gRoadSafeCase.touch();
                gRoadSafeRenderer.clearAssetCache();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(
            editorWidth()
        );
        roadSafeInputText(
            "Asset ID",
            selectedSceneEntity->asset.assetId,
            128
        );

        ImGui::SetNextItemWidth(
            editorWidth()
        );
        roadSafeInputText(
            "GLB / glTF path",
            selectedSceneEntity->asset.sourcePath,
            260
        );

        ImGui::SetNextItemWidth(
            editorWidth()
        );
        roadSafeInputText(
            "Collision mesh",
            selectedSceneEntity->asset.collisionPath,
            260
        );

        ImGui::SetNextItemWidth(
            editorWidth()
        );
        roadSafeInputText(
            "Material profile",
            selectedSceneEntity->asset.materialProfile,
            128
        );

        ImGui::SetNextItemWidth(
            editorWidth()
        );
        roadSafeInputText(
            "Source URL",
            selectedSceneEntity->asset.sourceUrl,
            260
        );

        ImGui::SetNextItemWidth(
            editorWidth()
        );
        roadSafeInputText(
            "Author",
            selectedSceneEntity->asset.author,
            128
        );

        ImGui::SetNextItemWidth(
            editorWidth()
        );
        roadSafeInputText(
            "License",
            selectedSceneEntity->asset.licenseName,
            128
        );

        ImGui::SetNextItemWidth(
            editorWidth()
        );
        roadSafeInputText(
            "Attribution",
            selectedSceneEntity->asset.attribution,
            260
        );

        ImGui::SetNextItemWidth(
            editorWidth()
        );

        if (ImGui::DragFloat(
            "Meters per unit",
            &selectedSceneEntity->asset.metersPerUnit,
            0.01f,
            0.0001f,
            1000.0f,
            "%.4f"))
        {
            gRoadSafeCase.touch();
        }

        ImGui::SetNextItemWidth(
            editorWidth()
        );

        if (ImGui::DragInt(
            "LOD bias",
            &selectedSceneEntity->asset.lodBias,
            1.0f,
            -2,
            4))
        {
            gRoadSafeCase.touch();
        }

        if (ImGui::Checkbox(
            "PBR materials",
            &selectedSceneEntity->asset.pbrEnabled))
        {
            gRoadSafeCase.touch();
        }

        if (ImGui::Checkbox(
            "Cast shadows",
            &selectedSceneEntity->asset.castShadows))
        {
            gRoadSafeCase.touch();
        }

        if (ImGui::Checkbox(
            "Receive shadows",
            &selectedSceneEntity->asset.receiveShadows))
        {
            gRoadSafeCase.touch();
        }

        if (ImGui::Checkbox(
            "AR ready",
            &selectedSceneEntity->asset.arReady))
        {
            gRoadSafeCase.touch();
        }

        ImGui::TextDisabled(
            "Native renderer active. Browse Assets to assign installed models."
        );
    }

    if (entityEditLocked)
        ImGui::EndDisabled();

    if (section("VISIBILITY"))
    {
        if (ImGui::Checkbox(
            "Visible",
            &selectedSceneEntity->visible))
        {
            gRoadSafeCase.touch();
        }

        if (ImGui::Checkbox(
            "Locked",
            &selectedSceneEntity->locked))
        {
            gRoadSafeCase.touch();
        }

        ImGui::TextDisabled("Selectable");
        ImGui::SameLine();

        ImGui::TextUnformatted(
            selectedSceneEntity->locked
                ? "No"
                : "Yes"
        );
    }

    if (section("METADATA"))
    {
        ImGui::TextDisabled("Object ID");
        ImGui::SameLine();
        ImGui::Text(
            "%s",
            selectedSceneEntity->id.c_str()
        );

        ImGui::TextDisabled("Editor ID");
        ImGui::SameLine();
        ImGui::Text(
            "%d",
            entity
        );

        ImGui::TextDisabled("Type");
        ImGui::SameLine();

        const char* type="Unknown";

        switch (selectedSceneEntity->kind)
        {
            case roadsafe::SceneEntityKind::Environment:
                type="Environment";
                break;

            case roadsafe::SceneEntityKind::Vehicle:
                type="Vehicle";
                break;

            case roadsafe::SceneEntityKind::Evidence:
                type="Evidence";
                break;

            case roadsafe::SceneEntityKind::Measurement:
                type="Measurement";
                break;
        }

        ImGui::TextUnformatted(type);

        ImGui::TextDisabled("Domain record");
        ImGui::SameLine();

        if (selectedSceneEntity->recordIndex>=0)
            ImGui::Text(
                "%d",
                selectedSceneEntity->recordIndex
            );
        else
            ImGui::TextUnformatted("-");

        ImGui::SetNextItemWidth(
            -1.0f
        );

        roadSafeInputTextMultiline(
            "Notes",
            selectedSceneEntity->notes,
            ImVec2(-1.0f,82.0f),
            512
        );
    }
}
