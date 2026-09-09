param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - DEEP PROPERTIES INSPECTOR V3 SAFE" -ForegroundColor Cyan
Write-Host " Additive injection only - no window replacement" -ForegroundColor DarkGray
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$original = Get-Content $MainCpp -Raw
$text = $original

$required = @(
    'struct EditorShellState',
    'static EditorShellState gEditorShell;',
    'static const char* selectedEntityName()',
    '"##PropertiesContextHeader"',
    'endEditorContextHeader();'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected safe-patch marker not found: $marker"
    }
}

if ($text.Contains('static void drawDeepPropertiesInspectorBody()')) {
    Write-Host "[OK] Deep Properties V3 already installed." -ForegroundColor Green
    exit 0
}

function Get-FunctionBlockInfo {
    param(
        [string]$Source,
        [string]$Signature
    )

    $start = $Source.IndexOf($Signature)
    if ($start -lt 0) {
        throw "Could not find function: $Signature"
    }

    $braceStart = $Source.IndexOf("{", $start)
    if ($braceStart -lt 0) {
        throw "Could not find opening brace for: $Signature"
    }

    $depth = 0
    $inString = $false
    $inChar = $false
    $escape = $false
    $lineComment = $false
    $blockComment = $false
    $end = -1

    for ($i = $braceStart; $i -lt $Source.Length; $i++) {
        $ch = $Source[$i]
        $next = if ($i + 1 -lt $Source.Length) { $Source[$i + 1] } else { [char]0 }

        if ($lineComment) {
            if ($ch -eq "`n") {
                $lineComment = $false
            }
            continue
        }

        if ($blockComment) {
            if ($ch -eq "*" -and $next -eq "/") {
                $blockComment = $false
                $i++
            }
            continue
        }

        if ($escape) {
            $escape = $false
            continue
        }

        if ($inString) {
            if ($ch -eq '\') {
                $escape = $true
                continue
            }
            if ($ch -eq '"') {
                $inString = $false
            }
            continue
        }

        if ($inChar) {
            if ($ch -eq '\') {
                $escape = $true
                continue
            }
            if ($ch -eq "'") {
                $inChar = $false
            }
            continue
        }

        if ($ch -eq "/" -and $next -eq "/") {
            $lineComment = $true
            $i++
            continue
        }

        if ($ch -eq "/" -and $next -eq "*") {
            $blockComment = $true
            $i++
            continue
        }

        if ($ch -eq '"') {
            $inString = $true
            continue
        }

        if ($ch -eq "'") {
            $inChar = $true
            continue
        }

        if ($ch -eq "{") {
            $depth++
            continue
        }

        if ($ch -eq "}") {
            $depth--
            if ($depth -eq 0) {
                $end = $i
                break
            }
        }
    }

    if ($end -lt 0) {
        throw "Could not find closing brace for: $Signature"
    }

    return @{
        Start = $start
        End = $end
        Text = $Source.Substring($start, $end - $start + 1)
    }
}

# ============================================================
# PRE-FLIGHT: confirm Properties context header occurrence
# ============================================================

$headerId = '"##PropertiesContextHeader"'
$headerPos = $text.IndexOf($headerId)

if ($headerPos -lt 0) {
    throw "Properties contextual header not found."
}

$headerEnd = $text.IndexOf(
    'endEditorContextHeader();',
    $headerPos
)

if ($headerEnd -lt 0) {
    throw "Could not find endEditorContextHeader() after Properties header."
}

$headerEnd += 'endEditorContextHeader();'.Length

# Make sure this really belongs near a Properties window, not some unrelated string.
$propertiesBeginPattern = 'ImGui::Begin\s*\(\s*"Properties"'
$propertiesMatches = [regex]::Matches(
    $text,
    $propertiesBeginPattern
)

if ($propertiesMatches.Count -ne 1) {
    throw "Expected exactly one Properties ImGui::Begin(...) call; found $($propertiesMatches.Count)."
}

$propertiesBeginPos = $propertiesMatches[0].Index

if ($headerPos -lt $propertiesBeginPos) {
    throw "Properties header appears before Properties Begin; refusing to patch."
}

if (($headerPos - $propertiesBeginPos) -gt 4000) {
    throw "Properties header is unexpectedly far from Properties Begin; refusing to patch."
}

# ============================================================
# PRE-FLIGHT: helper insertion point after selectedEntityName()
# ============================================================

$selectedInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static const char* selectedEntityName()'

$gEditorPos = $text.IndexOf('static EditorShellState gEditorShell;')

if ($gEditorPos -lt 0 -or $gEditorPos -gt $selectedInfo.Start) {
    throw "gEditorShell is not declared before selectedEntityName(); refusing to inject helper."
}

# ============================================================
# HELPER FUNCTION
# ============================================================

$helper = @'

static void drawDeepPropertiesInspectorBody()
{
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

    auto section =
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

    if (entity==0)
    {
        if (section("SCENE SETTINGS"))
        {
            ImGui::TextDisabled("Units");
            ImGui::SameLine();
            ImGui::TextUnformatted("Metric");

            ImGui::TextDisabled("Coordinate system");
            ImGui::SameLine();
            ImGui::TextUnformatted("World");

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat(
                "Snap increment",
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

    ImGui::Text(
        "%s",
        selectedEntityName()
    );

    ImGui::SameLine();

    ImGui::TextDisabled(
        "#%02d",
        entity
    );

    if (section("TRANSFORM"))
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

    // Ground Plane / Road Surface
    if (entity==1 || entity==2)
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

    // Vehicle A / Vehicle B
    if (entity==3 || entity==4)
    {
        const int i=entity-3;

        if (section("VEHICLE PHYSICS"))
        {
            ImGui::SetNextItemWidth(-1.0f);

            ImGui::InputText(
                "Make / ID",
                vehicleMake[i],
                sizeof(vehicleMake[i])
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::InputText(
                "Model",
                vehicleModel[i],
                sizeof(vehicleModel[i])
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat(
                "Mass",
                &vehicleMass[i],
                5.0f,
                1.0f,
                100000.0f,
                "%.1f kg"
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat(
                "Wheelbase",
                &vehicleWheelbase[i],
                0.01f,
                0.50f,
                10.0f,
                "%.2f m"
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat(
                "CG height",
                &vehicleCgHeight[i],
                0.01f,
                0.05f,
                5.0f,
                "%.2f m"
            );
        }

        if (section("INITIAL STATE"))
        {
            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat(
                "Velocity",
                &vehicleVelocity[i],
                0.10f,
                -200.0f,
                200.0f,
                "%.2f m/s"
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat(
                "Heading",
                &vehicleHeading[i],
                0.25f,
                -360.0f,
                360.0f,
                "%.1f deg"
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat(
                "Steering",
                &vehicleSteering[i],
                0.10f,
                -60.0f,
                60.0f,
                "%.1f deg"
            );
        }

        if (section("TIRES / ROAD"))
        {
            ImGui::SetNextItemWidth(-1.0f);

            ImGui::SliderFloat(
                "Friction coefficient",
                &vehicleFriction[i],
                0.05f,
                1.50f,
                "%.2f"
            );
        }

        if (section("COLLISION"))
        {
            ImGui::SetNextItemWidth(-1.0f);

            ImGui::DragFloat(
                "Crush depth",
                &vehicleCrushDepth[i],
                0.01f,
                0.0f,
                5.0f,
                "%.3f m"
            );

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::SliderFloat(
                "Restitution",
                &vehicleRestitution[i],
                0.0f,
                1.0f,
                "%.2f"
            );
        }
    }

    // Evidence: Skid / Marker / Debris
    if (entity>=5 && entity<=7)
    {
        if (section("EVIDENCE METADATA"))
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
                    "%.2f"
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
                    "%.2f"
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
                    "%.2f"
                );
            }
        }
    }

    // Measurements: Distance / Angle
    if (entity==8 || entity==9)
    {
        if (section("MEASUREMENT SETTINGS"))
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

    if (section("VISIBILITY"))
    {
        ImGui::Checkbox(
            "Visible",
            &objectVisible[entity]
        );

        ImGui::Checkbox(
            "Locked",
            &objectLocked[entity]
        );

        ImGui::TextDisabled("Selectable");
        ImGui::SameLine();

        ImGui::TextUnformatted(
            objectLocked[entity]
                ? "No"
                : "Yes"
        );
    }

    if (section("METADATA"))
    {
        ImGui::TextDisabled("Object ID");
        ImGui::SameLine();
        ImGui::Text("%d",entity);

        ImGui::TextDisabled("Type");
        ImGui::SameLine();

        const char* type=
            entity<=2
                ? "Environment"
                : (entity<=4
                    ? "Vehicle"
                    : (entity<=7
                        ? "Evidence"
                        : "Measurement"));

        ImGui::TextUnformatted(type);

        ImGui::SetNextItemWidth(-1.0f);

        ImGui::InputTextMultiline(
            "Notes",
            objectNotes,
            sizeof(objectNotes),
            ImVec2(-1.0f,82.0f)
        );
    }
}
'@

# Insert helper after selectedEntityName()
$helperInsertAt = $selectedInfo.End + 1
$text = $text.Insert(
    $helperInsertAt,
    $helper
)

# ============================================================
# RECOMPUTE Properties header position after helper insertion
# ============================================================

$headerPos = $text.IndexOf($headerId)
if ($headerPos -lt 0) {
    throw "Internal error: Properties header vanished after helper insertion."
}

$headerEnd = $text.IndexOf(
    'endEditorContextHeader();',
    $headerPos
)

if ($headerEnd -lt 0) {
    throw "Internal error: Properties context header close vanished."
}

$headerEnd += 'endEditorContextHeader();'.Length

$call = @'

    drawDeepPropertiesInspectorBody();

'@

$text = $text.Insert(
    $headerEnd,
    $call
)

# ============================================================
# IN-MEMORY VERIFICATION BEFORE WRITE
# ============================================================

$checks = @(
    'static void drawDeepPropertiesInspectorBody()',
    'drawDeepPropertiesInspectorBody();',
    '"VEHICLE PHYSICS"',
    '"INITIAL STATE"',
    '"TIRES / ROAD"',
    '"COLLISION"',
    '"EVIDENCE METADATA"',
    '"MEASUREMENT SETTINGS"',
    '"VISIBILITY"',
    '"METADATA"'
)

foreach ($check in $checks) {
    if (-not $text.Contains($check)) {
        throw "Pre-write verification failed: $check"
    }
}

if ([regex]::Matches(
    $text,
    'drawDeepPropertiesInspectorBody\(\);'
).Count -ne 1)
{
    throw "Pre-write verification failed: expected exactly one helper call."
}

if ([regex]::Matches(
    $text,
    'static void drawDeepPropertiesInspectorBody\(\)'
).Count -ne 1)
{
    throw "Pre-write verification failed: expected exactly one helper definition."
}

$badPair =
    [string]([char]96) +
    "r" +
    [char]96 +
    "n"

if ($text.Contains($badPair)) {
    throw "Pre-write verification failed: literal PowerShell newline text detected."
}

# Ensure original Properties contextual header is still present.
if (-not $text.Contains('"##PropertiesContextHeader"')) {
    throw "Pre-write verification failed: existing Properties header was damaged."
}

# ============================================================
# ONLY NOW CREATE BACKUP + WRITE
# ============================================================

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-deep-properties-v3-$timestamp.bak"

Copy-Item $MainCpp $backup -Force

Write-Host "[OK] All pre-write checks passed." -ForegroundColor Green
Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

Set-Content `
    -Path $MainCpp `
    -Value $text `
    -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        # Restore automatically if post-write verification somehow fails.
        Copy-Item $backup $MainCpp -Force
        throw "Post-write verification failed and backup was restored: $check"
    }
}

Write-Host ""
Write-Host "[DONE] Deep Properties Inspector V3 installed safely." -ForegroundColor Cyan
Write-Host "[OK] No Properties window replacement." -ForegroundColor Green
Write-Host "[OK] Existing contextual header preserved." -ForegroundColor Green
Write-Host "[OK] Scene settings." -ForegroundColor Green
Write-Host "[OK] Transform." -ForegroundColor Green
Write-Host "[OK] Vehicle physics / state / tires / collision." -ForegroundColor Green
Write-Host "[OK] Environment surface controls." -ForegroundColor Green
Write-Host "[OK] Evidence metadata / confidence." -ForegroundColor Green
Write-Host "[OK] Measurement settings." -ForegroundColor Green
Write-Host "[OK] Visibility / metadata / notes." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
