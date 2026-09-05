param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - DEEP PROPERTIES INSPECTOR" -ForegroundColor Cyan
Write-Host " Phase 3 - Selection-aware object properties" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'ImGui::Begin("Properties")',
    'gEditorShell.selectedEntity',
    'selectedEntityName()'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected Properties marker not found: $marker"
    }
}

if ($text.Contains('"VEHICLE PHYSICS"') -and
    $text.Contains('"EVIDENCE METADATA"') -and
    $text.Contains('"MEASUREMENT SETTINGS"'))
{
    Write-Host "[OK] Deep Properties inspector already appears installed." -ForegroundColor Green
    exit 0
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-deep-properties-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# FIND THE PROPERTIES WINDOW BLOCK
# ============================================================

$beginMarker = 'ImGui::Begin("Properties")'
$beginIndex = $text.IndexOf($beginMarker)

if ($beginIndex -lt 0) {
    throw "Could not locate Properties window."
}

$bodyStart = $beginIndex + $beginMarker.Length

$endMarker = 'ImGui::End();'
$endIndex = $text.IndexOf($endMarker, $bodyStart)

if ($endIndex -lt 0) {
    throw "Could not locate Properties ImGui::End()."
}

$oldBlock = $text.Substring(
    $beginIndex,
    ($endIndex + $endMarker.Length) - $beginIndex
)

# ============================================================
# NEW DEEP INSPECTOR
# ============================================================

$newBlock = @'
ImGui::Begin("Properties");

    // ========================================================
    // CONTEXT HEADER
    // ========================================================

    beginEditorContextHeader(
        "##PropertiesContextHeader"
    );

    ImGui::TextDisabled("INSPECTOR");

    editorContextSeparator();

    ImGui::TextDisabled("Context");
    ImGui::SameLine(0.0f,5.0f);

    ImGui::Text(
        "%s",
        gEditorShell.selectedEntity!=0
            ? selectedEntityName()
            : "Scene"
    );

    editorContextSeparator();

    ImGui::TextDisabled(
        gEditorShell.selectedEntity!=0
            ? "Object Properties"
            : "No object selected"
    );

    endEditorContextHeader();

    // ========================================================
    // LOCAL EDITOR PROPERTY STATE
    // ========================================================

    static float objectPosition[10][3]{};
    static float objectRotation[10][3]{};
    static float objectScale[10][3]={
        {1.0f,1.0f,1.0f},
        {1.0f,1.0f,1.0f},
        {1.0f,1.0f,1.0f},
        {1.0f,1.0f,1.0f},
        {1.0f,1.0f,1.0f},
        {1.0f,1.0f,1.0f},
        {1.0f,1.0f,1.0f},
        {1.0f,1.0f,1.0f},
        {1.0f,1.0f,1.0f},
        {1.0f,1.0f,1.0f}
    };

    static bool objectVisible[10]={
        true,true,true,true,true,
        true,true,true,true,true
    };

    static bool objectLocked[10]{};

    static char vehicleMake[2][48]={
        "Vehicle A",
        "Vehicle B"
    };

    static char vehicleModel[2][48]={
        "Unknown",
        "Unknown"
    };

    static float vehicleMass[2]={
        1500.0f,
        1250.0f
    };

    static float vehicleWheelbase[2]={
        2.70f,
        2.60f
    };

    static float vehicleCgHeight[2]={
        0.55f,
        0.52f
    };

    static float vehicleVelocity[2]={
        12.0f,
        -8.0f
    };

    static float vehicleHeading[2]={
        0.0f,
        180.0f
    };

    static float vehicleSteering[2]={
        0.0f,
        0.0f
    };

    static float vehicleFriction[2]={
        0.70f,
        0.70f
    };

    static float vehicleRestitution[2]={
        0.20f,
        0.20f
    };

    static float vehicleCrushDepth[2]={
        0.0f,
        0.0f
    };

    static int roadSurfaceType=0;
    static float roadFriction=0.72f;
    static float roadGrade=0.0f;

    static float skidLength=24.0f;
    static float skidWidth=0.18f;
    static float skidFriction=0.70f;
    static int skidEvidenceType=0;
    static float skidConfidence=0.90f;

    static float markerConfidence=0.95f;
    static int markerCategory=0;

    static float debrisRadius=2.5f;
    static int debrisPieceCount=12;
    static float debrisConfidence=0.80f;

    static float measurementDistance=12.50f;
    static float measurementAngle=32.0f;
    static int measurementUnits=0;
    static bool measurementLocked=false;

    static char evidenceSource[96]="Scene survey";
    static char objectNotes[256]="";

    const int entity=
        std::max(
            0,
            std::min(
                9,
                gEditorShell.selectedEntity
            )
        );

    auto sectionTitle =
        [](const char* label)
    {
        ImGui::Spacing();

        ImGui::PushStyleColor(
            ImGuiCol_Header,
            ImVec4(0.12f,0.13f,0.145f,1.0f)
        );

        ImGui::PushStyleColor(
            ImGuiCol_HeaderHovered,
            ImVec4(0.16f,0.17f,0.19f,1.0f)
        );

        const bool open=
            ImGui::CollapsingHeader(
                label,
                ImGuiTreeNodeFlags_DefaultOpen
            );

        ImGui::PopStyleColor(2);

        return open;
    };

    auto propertyLabel =
        [](const char* label)
    {
        ImGui::TextDisabled("%s",label);
    };

    // ========================================================
    // NO SELECTION / SCENE SETTINGS
    // ========================================================

    if (entity==0)
    {
        ImGui::TextDisabled("SCENE");

        ImGui::Spacing();

        ImGui::TextWrapped(
            "Select an object in the Outliner or Viewport to edit its reconstruction properties."
        );

        if (sectionTitle("SCENE SETTINGS"))
        {
            propertyLabel("Units");
            ImGui::SameLine();
            ImGui::Text("Metric");

            propertyLabel("Coordinate system");
            ImGui::SameLine();
            ImGui::Text("World");

            propertyLabel("Snap increment");
            ImGui::SameLine();

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat(
                "##InspectorSnap",
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

        if (sectionTitle("WORKSPACE"))
        {
            ImGui::Checkbox(
                "Outliner",
                &gEditorShell.showOutliner
            );

            ImGui::Checkbox(
                "Timeline",
                &gEditorShell.showTimeline
            );

            ImGui::Checkbox(
                "Node Editor",
                &gEditorShell.showNodeEditor
            );
        }

        ImGui::End();
    }
    else
    {
        // ====================================================
        // OBJECT IDENTITY
        // ====================================================

        ImGui::Text(
            "%s",
            selectedEntityName()
        );

        ImGui::SameLine();

        ImGui::TextDisabled(
            "#%02d",
            entity
        );

        ImGui::Spacing();

        if (sectionTitle("TRANSFORM"))
        {
            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat3(
                "Position",
                objectPosition[entity],
                0.01f,
                -10000.0f,
                10000.0f,
                "%.3f m"
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat3(
                "Rotation",
                objectRotation[entity],
                0.25f,
                -360.0f,
                360.0f,
                "%.1f deg"
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat3(
                "Scale",
                objectScale[entity],
                0.01f,
                0.001f,
                1000.0f,
                "%.3f"
            );
        }

        // ====================================================
        // ENVIRONMENT
        // Ground Plane = 1
        // Road Surface = 2
        // ====================================================

        if (entity==1 || entity==2)
        {
            if (sectionTitle("SURFACE"))
            {
                const char* surfaceTypes[]={
                    "Asphalt",
                    "Concrete",
                    "Gravel",
                    "Grass",
                    "Unknown"
                };

                if (entity==2)
                {
                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::Combo(
                        "Surface type",
                        &roadSurfaceType,
                        surfaceTypes,
                        5
                    );
                }

                ImGui::SetNextItemWidth(-1.0f);

                ImGui::SliderFloat(
                    "Friction coefficient",
                    &roadFriction,
                    0.05f,
                    1.50f,
                    "%.2f"
                );

                ImGui::SetNextItemWidth(-1.0f);

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

        // ====================================================
        // VEHICLES
        // Vehicle A = 3
        // Vehicle B = 4
        // ====================================================

        if (entity==3 || entity==4)
        {
            const int vehicleIndex=
                entity-3;

            if (sectionTitle("VEHICLE PHYSICS"))
            {
                ImGui::SetNextItemWidth(-1.0f);

                ImGui::InputText(
                    "Make / ID",
                    vehicleMake[vehicleIndex],
                    sizeof(vehicleMake[vehicleIndex])
                );

                ImGui::SetNextItemWidth(-1.0f);

                ImGui::InputText(
                    "Model",
                    vehicleModel[vehicleIndex],
                    sizeof(vehicleModel[vehicleIndex])
                );

                ImGui::SetNextItemWidth(-1.0f);

                ImGui::DragFloat(
                    "Mass",
                    &vehicleMass[vehicleIndex],
                    5.0f,
                    1.0f,
                    100000.0f,
                    "%.1f kg"
                );

                ImGui::SetNextItemWidth(-1.0f);

                ImGui::DragFloat(
                    "Wheelbase",
                    &vehicleWheelbase[vehicleIndex],
                    0.01f,
                    0.50f,
                    10.0f,
                    "%.2f m"
                );

                ImGui::SetNextItemWidth(-1.0f);

                ImGui::DragFloat(
                    "CG height",
                    &vehicleCgHeight[vehicleIndex],
                    0.01f,
                    0.05f,
                    5.0f,
                    "%.2f m"
                );
            }

            if (sectionTitle("INITIAL STATE"))
            {
                ImGui::SetNextItemWidth(-1.0f);

                ImGui::DragFloat(
                    "Velocity",
                    &vehicleVelocity[vehicleIndex],
                    0.10f,
                    -200.0f,
                    200.0f,
                    "%.2f m/s"
                );

                ImGui::SetNextItemWidth(-1.0f);

                ImGui::DragFloat(
                    "Heading",
                    &vehicleHeading[vehicleIndex],
                    0.25f,
                    -360.0f,
                    360.0f,
                    "%.1f deg"
                );

                ImGui::SetNextItemWidth(-1.0f);

                ImGui::DragFloat(
                    "Steering",
                    &vehicleSteering[vehicleIndex],
                    0.10f,
                    -60.0f,
                    60.0f,
                    "%.1f deg"
                );
            }

            if (sectionTitle("TIRES / ROAD"))
            {
                ImGui::SetNextItemWidth(-1.0f);

                ImGui::SliderFloat(
                    "Friction coefficient",
                    &vehicleFriction[vehicleIndex],
                    0.05f,
                    1.50f,
                    "%.2f"
                );
            }

            if (sectionTitle("COLLISION"))
            {
                ImGui::SetNextItemWidth(-1.0f);

                ImGui::DragFloat(
                    "Crush depth",
                    &vehicleCrushDepth[vehicleIndex],
                    0.01f,
                    0.0f,
                    5.0f,
                    "%.3f m"
                );

                ImGui::SetNextItemWidth(-1.0f);

                ImGui::SliderFloat(
                    "Restitution",
                    &vehicleRestitution[vehicleIndex],
                    0.0f,
                    1.0f,
                    "%.2f"
                );
            }
        }

        // ====================================================
        // EVIDENCE
        // Skid = 5, Marker = 6, Debris = 7
        // ====================================================

        if (entity>=5 && entity<=7)
        {
            if (sectionTitle("EVIDENCE METADATA"))
            {
                ImGui::SetNextItemWidth(-1.0f);

                ImGui::InputText(
                    "Source",
                    evidenceSource,
                    sizeof(evidenceSource)
                );

                if (entity==5)
                {
                    const char* skidTypes[]={
                        "Braking skid",
                        "Yaw mark",
                        "Scuff mark",
                        "Tire scrub"
                    };

                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::Combo(
                        "Evidence type",
                        &skidEvidenceType,
                        skidTypes,
                        4
                    );

                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::DragFloat(
                        "Length",
                        &skidLength,
                        0.05f,
                        0.0f,
                        1000.0f,
                        "%.2f m"
                    );

                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::DragFloat(
                        "Width",
                        &skidWidth,
                        0.01f,
                        0.01f,
                        5.0f,
                        "%.2f m"
                    );

                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::SliderFloat(
                        "Friction coefficient",
                        &skidFriction,
                        0.05f,
                        1.50f,
                        "%.2f"
                    );

                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::SliderFloat(
                        "Confidence",
                        &skidConfidence,
                        0.0f,
                        1.0f,
                        "%.0f%%"
                    );
                }
                else if (entity==6)
                {
                    const char* markerTypes[]={
                        "Reference",
                        "Impact",
                        "Evidence",
                        "Survey"
                    };

                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::Combo(
                        "Category",
                        &markerCategory,
                        markerTypes,
                        4
                    );

                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::SliderFloat(
                        "Confidence",
                        &markerConfidence,
                        0.0f,
                        1.0f,
                        "%.0f%%"
                    );
                }
                else
                {
                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::DragFloat(
                        "Field radius",
                        &debrisRadius,
                        0.05f,
                        0.0f,
                        100.0f,
                        "%.2f m"
                    );

                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::DragInt(
                        "Estimated pieces",
                        &debrisPieceCount,
                        1.0f,
                        0,
                        10000
                    );

                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::SliderFloat(
                        "Confidence",
                        &debrisConfidence,
                        0.0f,
                        1.0f,
                        "%.0f%%"
                    );
                }
            }
        }

        // ====================================================
        // MEASUREMENTS
        // Distance = 8, Angle = 9
        // ====================================================

        if (entity==8 || entity==9)
        {
            if (sectionTitle("MEASUREMENT SETTINGS"))
            {
                const char* units[]={
                    "Metric",
                    "Imperial"
                };

                ImGui::SetNextItemWidth(-1.0f);

                ImGui::Combo(
                    "Units",
                    &measurementUnits,
                    units,
                    2
                );

                if (entity==8)
                {
                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::DragFloat(
                        "Distance",
                        &measurementDistance,
                        0.01f,
                        0.0f,
                        100000.0f,
                        measurementUnits==0
                            ? "%.3f m"
                            : "%.3f ft"
                    );
                }
                else
                {
                    ImGui::SetNextItemWidth(-1.0f);

                    ImGui::DragFloat(
                        "Angle",
                        &measurementAngle,
                        0.10f,
                        -360.0f,
                        360.0f,
                        "%.2f deg"
                    );
                }

                ImGui::Checkbox(
                    "Lock measurement",
                    &measurementLocked
                );
            }
        }

        // ====================================================
        // COMMON DISPLAY / METADATA
        // ====================================================

        if (sectionTitle("VISIBILITY"))
        {
            ImGui::Checkbox(
                "Visible",
                &objectVisible[entity]
            );

            ImGui::Checkbox(
                "Locked",
                &objectLocked[entity]
            );

            ImGui::TextDisabled(
                "Selectable"
            );

            ImGui::SameLine();

            ImGui::Text(
                "%s",
                objectLocked[entity]
                    ? "No"
                    : "Yes"
            );
        }

        if (sectionTitle("METADATA"))
        {
            ImGui::TextDisabled("Object ID");
            ImGui::SameLine();

            ImGui::Text(
                "%d",
                entity
            );

            ImGui::TextDisabled("Type");
            ImGui::SameLine();

            const char* objectType=
                entity<=2
                    ? "Environment"
                    : (entity<=4
                        ? "Vehicle"
                        : (entity<=7
                            ? "Evidence"
                            : "Measurement"));

            ImGui::Text(
                "%s",
                objectType
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::InputTextMultiline(
                "Notes",
                objectNotes,
                sizeof(objectNotes),
                ImVec2(-1.0f,86.0f)
            );
        }

        ImGui::End();
    }
'@

$text = $text.Replace(
    $oldBlock,
    $newBlock
)

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content `
    -Path $MainCpp `
    -Value $text `
    -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$checks = @(
    '"TRANSFORM"',
    '"VEHICLE PHYSICS"',
    '"INITIAL STATE"',
    '"TIRES / ROAD"',
    '"COLLISION"',
    '"EVIDENCE METADATA"',
    '"MEASUREMENT SETTINGS"',
    '"VISIBILITY"',
    '"METADATA"',
    '"Friction coefficient"',
    '"Crush depth"',
    '"Confidence"',
    '"Notes"'
)

foreach ($check in $checks)
{
    if (-not $verify.Contains($check))
    {
        throw "Verification failed after write: $check"
    }
}

$badPair =
    [string]([char]96) +
    "r" +
    [char]96 +
    "n"

if ($verify.Contains($badPair))
{
    throw "Verification failed: literal PowerShell newline text entered main.cpp."
}

Write-Host ""
Write-Host "[DONE] Deep Properties inspector installed." -ForegroundColor Cyan
Write-Host "[OK] Scene settings when nothing is selected." -ForegroundColor Green
Write-Host "[OK] Common Transform section." -ForegroundColor Green
Write-Host "[OK] Vehicle physics / state / tires / collision." -ForegroundColor Green
Write-Host "[OK] Environment surface settings." -ForegroundColor Green
Write-Host "[OK] Evidence metadata and confidence." -ForegroundColor Green
Write-Host "[OK] Measurement-specific controls." -ForegroundColor Green
Write-Host "[OK] Visibility / lock / notes metadata." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
