param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - AR VIEWPORT PROFESSIONAL SESSION PASS" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

function Get-FunctionBlock {
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
    $end = -1

    for ($i = $braceStart; $i -lt $Source.Length; $i++) {
        $ch = $Source[$i]

        if ($escape) {
            $escape = $false
            continue
        }

        if ($inString) {
            if ($ch -eq '\') { $escape = $true; continue }
            if ($ch -eq '"') { $inString = $false }
            continue
        }

        if ($inChar) {
            if ($ch -eq '\') { $escape = $true; continue }
            if ($ch -eq "'") { $inChar = $false }
            continue
        }

        if ($ch -eq '"') { $inString = $true; continue }
        if ($ch -eq "'") { $inChar = $true; continue }

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
        Text = $Source.Substring($start, $end - $start + 1)
    }
}

$info = Get-FunctionBlock `
    -Source $text `
    -Signature "static void drawViewportView()"

$func = $info.Text

$required = @(
    'ViewportModeStrip',
    'Viewport3DARProStrip',
    'static int arAnchorCount=0;',
    'SceneCanvas'
)

foreach ($marker in $required) {
    if (-not $func.Contains($marker)) {
        throw "Expected AR viewport marker not found: $marker"
    }
}

if ($func.Contains("ViewportARSessionBar")) {
    Write-Host "[OK] AR professional session pass already installed." -ForegroundColor Green
    exit 0
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-ar-session-pass-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. ADD HONEST AR SESSION STATE
# ============================================================

$stateMarker = 'static int arAnchorCount=0;'

$extraState = @'

    // AR runtime/session state.
    static bool arEditorPreview=false;
    static bool arDeviceConnected=false;
    static bool arSessionRunning=false;
    static bool arRecording=false;
    static int arDeviceProfile=0;
'@

$func = $func.Replace(
    $stateMarker,
    $stateMarker + $extraState
)

Write-Host "[OK] Added AR runtime/session state." -ForegroundColor Green

# ============================================================
# 2. INSERT AR SESSION BAR AFTER 3D/AR PRO STRIP
# ============================================================

$proPos = $func.IndexOf('"Viewport3DARProStrip"')
if ($proPos -lt 0) {
    throw "Could not find Viewport3DARProStrip."
}

$proEnd = $func.IndexOf('ImGui::EndChild();', $proPos)
if ($proEnd -lt 0) {
    throw "Could not find Viewport3DARProStrip EndChild()."
}

$proEnd += 'ImGui::EndChild();'.Length

$sessionBar = @'

    if (viewportMode==2)
    {
        ImGui::Spacing();

        ImGui::BeginChild(
            "ViewportARSessionBar",
            ImVec2(0.0f,54.0f),
            true,
            ImGuiWindowFlags_NoScrollbar
        );

        ImGui::TextDisabled("SESSION");
        ImGui::SameLine(0.0f,10.0f);

        if (arEditorPreview)
            drawStatus("EDITOR PREVIEW",StatusTone::Accent);
        else if (arDeviceConnected && arSessionRunning)
            drawStatus("LIVE SESSION",StatusTone::Success);
        else if (arDeviceConnected)
            drawStatus("DEVICE CONNECTED",StatusTone::Neutral);
        else
            drawStatus("OFFLINE",StatusTone::Neutral);

        ImGui::SameLine(0.0f,16.0f);

        if (!arEditorPreview)
        {
            if (editorButton(
                "EDITOR PREVIEW",
                126.0f,
                false,
                true))
            {
                arEditorPreview=true;
                arSessionRunning=false;
                arRecording=false;
            }
        }
        else
        {
            if (editorButton(
                "EXIT PREVIEW",
                110.0f,
                false,
                true))
            {
                arEditorPreview=false;
                arRecording=false;
            }
        }

        ImGui::SameLine(0.0f,7.0f);

        if (editorButton(
            arDeviceConnected
                ? "DEVICE"
                : "CONNECT DEVICE",
            arDeviceConnected
                ? 82.0f
                : 126.0f,
            false,
            true))
        {
            ImGui::OpenPopup("##ARDeviceSetupPopup");
        }

        ImGui::SameLine(0.0f,7.0f);

        if (arDeviceConnected)
        {
            if (!arSessionRunning)
            {
                if (editorButton(
                    "START SESSION",
                    118.0f,
                    true,
                    true))
                {
                    arSessionRunning=true;
                    arEditorPreview=false;
                }
            }
            else
            {
                if (editorButton(
                    "STOP SESSION",
                    112.0f,
                    false,
                    true))
                {
                    arSessionRunning=false;
                    arRecording=false;
                }
            }
        }
        else
        {
            editorButton(
                "START SESSION",
                118.0f,
                false,
                false
            );
        }

        ImGui::SameLine(0.0f,7.0f);

        const bool canRecord=
            arEditorPreview ||
            (arDeviceConnected && arSessionRunning);

        if (arRecording)
        {
            if (editorButton(
                "STOP RECORD",
                108.0f,
                true,
                true))
            {
                arRecording=false;
            }
        }
        else
        {
            if (editorButton(
                "RECORD",
                82.0f,
                false,
                canRecord))
            {
                arRecording=true;
            }
        }

        if (ImGui::BeginPopup("##ARDeviceSetupPopup"))
        {
            ImGui::Text("AR DEVICE SETUP");
            ImGui::Separator();

            ImGui::TextDisabled(
                "Live mobile transport is not wired into the renderer yet."
            );

            ImGui::Spacing();

            const char* deviceProfiles[]={
                "Android Companion",
                "iPhone / iPad",
                "External AR Camera"
            };

            ImGui::SetNextItemWidth(220.0f);
            ImGui::Combo(
                "Device profile",
                &arDeviceProfile,
                deviceProfiles,
                3
            );

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::TextDisabled(
                "Use Editor Preview for UI/placement testing today."
            );

            ImGui::Spacing();

            if (ImGui::Button(
                "USE EDITOR PREVIEW",
                ImVec2(190.0f,0.0f)))
            {
                arEditorPreview=true;
                arDeviceConnected=false;
                arSessionRunning=false;
                arRecording=false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::EndChild();
    }
'@

$func = $func.Insert(
    $proEnd,
    $sessionBar
)

Write-Host "[OK] Added honest AR session/device controls." -ForegroundColor Green

# ============================================================
# 3. INSERT AR OFFLINE/PREVIEW HUD BEFORE CONTEXT MENU
# ============================================================

$contextPos = $func.IndexOf('// VIEWPORT CONTEXT MENU')

if ($contextPos -lt 0) {
    $contextPos = $func.IndexOf('if (ImGui::BeginPopupContextWindow(')
}

if ($contextPos -lt 0) {
    throw "Could not find viewport context-menu insertion point."
}

$arHud = @'

    if (viewportMode==2)
    {
        const bool arFeedActive=
            arEditorPreview ||
            (arDeviceConnected && arSessionRunning);

        if (!arFeedActive)
        {
            const float panelW=460.0f;
            const float panelH=146.0f;

            const ImVec2 panelMin(
                center.x-panelW*0.5f,
                center.y-panelH*0.5f
            );

            const ImVec2 panelMax(
                center.x+panelW*0.5f,
                center.y+panelH*0.5f
            );

            d->AddRectFilled(
                panelMin,
                panelMax,
                IM_COL32(22,25,29,238),
                7.0f
            );

            d->AddRect(
                panelMin,
                panelMax,
                IM_COL32(72,78,86,240),
                7.0f,
                0,
                1.0f
            );

            d->AddText(
                ImVec2(
                    panelMin.x+22.0f,
                    panelMin.y+20.0f
                ),
                IM_COL32(224,227,231,255),
                "NO AR SESSION"
            );

            d->AddText(
                ImVec2(
                    panelMin.x+22.0f,
                    panelMin.y+52.0f
                ),
                IM_COL32(148,154,164,255),
                "Connect a compatible device or use Editor Preview."
            );

            d->AddText(
                ImVec2(
                    panelMin.x+22.0f,
                    panelMin.y+78.0f
                ),
                IM_COL32(118,124,133,255),
                "Live camera tracking, anchors and occlusion will appear here."
            );

            d->AddText(
                ImVec2(
                    panelMin.x+22.0f,
                    panelMin.y+108.0f
                ),
                IM_COL32(100,182,209,235),
                "AR mode is ready for runtime integration."
            );
        }
        else
        {
            const ImVec2 statusMin(
                cp.x+18.0f,
                cp.y+18.0f
            );

            const ImVec2 statusMax(
                cp.x+276.0f,
                cp.y+108.0f
            );

            d->AddRectFilled(
                statusMin,
                statusMax,
                IM_COL32(22,25,29,225),
                5.0f
            );

            d->AddRect(
                statusMin,
                statusMax,
                IM_COL32(69,75,83,225),
                5.0f,
                0,
                1.0f
            );

            d->AddText(
                ImVec2(
                    statusMin.x+12.0f,
                    statusMin.y+10.0f
                ),
                IM_COL32(222,225,230,255),
                arEditorPreview
                    ? "EDITOR AR PREVIEW"
                    : "LIVE AR SESSION"
            );

            d->AddText(
                ImVec2(
                    statusMin.x+12.0f,
                    statusMin.y+36.0f
                ),
                IM_COL32(145,151,160,255),
                arEditorPreview
                    ? "Tracking: Simulated"
                    : (arTrackingQuality==2
                        ? "Tracking: Good"
                        : (arTrackingQuality==1
                            ? "Tracking: Limited"
                            : "Tracking: Poor"))
            );

            char runtimeText[96]{};
            std::snprintf(
                runtimeText,
                sizeof(runtimeText),
                "Anchors %d   Occlusion %s   Recording %s",
                arAnchorCount,
                arOcclusion ? "On" : "Off",
                arRecording ? "On" : "Off"
            );

            d->AddText(
                ImVec2(
                    statusMin.x+12.0f,
                    statusMin.y+61.0f
                ),
                IM_COL32(145,151,160,255),
                runtimeText
            );
        }
    }

'@

$func = $func.Insert(
    $contextPos,
    $arHud
)

Write-Host "[OK] Added AR offline/preview/live status HUD." -ForegroundColor Green

# ============================================================
# 4. WRITE BACK
# ============================================================

$text = $text.Replace(
    $info.Text,
    $func
)

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

# ============================================================
# VERIFY
# ============================================================

$verify = Get-Content $MainCpp -Raw

$checks = @(
    'ViewportARSessionBar',
    '##ARDeviceSetupPopup',
    'EDITOR PREVIEW',
    'NO AR SESSION',
    'arEditorPreview',
    'arDeviceConnected',
    'arSessionRunning',
    'arRecording'
)

foreach ($check in $checks) {
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

Write-Host ""
Write-Host "[DONE] AR professional session pass installed." -ForegroundColor Cyan
Write-Host "[OK] Offline state is explicit." -ForegroundColor Green
Write-Host "[OK] Editor Preview is separate from Live Device." -ForegroundColor Green
Write-Host "[OK] Session + recording controls added." -ForegroundColor Green
Write-Host "[OK] Device setup popup added." -ForegroundColor Green
Write-Host "[OK] No claim of a real connected device is made." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
