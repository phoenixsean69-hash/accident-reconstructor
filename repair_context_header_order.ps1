param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - REPAIR CONTEXT HEADER ORDERING" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

$required = @(
    'static void drawViewportView()',
    'struct EditorShellState',
    'static EditorShellState gEditorShell;',
    'static const char* selectedEntityName()',
    '"##ViewportContextHeader"'
)

foreach ($marker in $required) {
    if (-not $text.Contains($marker)) {
        throw "Expected marker not found: $marker"
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-context-order-repair-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. ADD FORWARD DECLARATIONS BEFORE drawViewportView()
# ============================================================

$viewportPos = $text.IndexOf('static void drawViewportView()')

if ($viewportPos -lt 0) {
    throw "Could not locate drawViewportView()."
}

$forwardDecls = @'
static bool shellSnapEnabled();
static float shellSnapValue();
static const char* shellSelectedEntityName();

'@

$beforeViewport = $text.Substring(0, $viewportPos)

if (-not $beforeViewport.Contains('static bool shellSnapEnabled();'))
{
    $text = $text.Insert(
        $viewportPos,
        $forwardDecls
    )

    Write-Host "[OK] Added shell accessor forward declarations." -ForegroundColor Green
}

# ============================================================
# 2. REPLACE EARLY VIEWPORT REFERENCES
# ============================================================

$viewportStart = $text.IndexOf('static void drawViewportView()')
$viewportEnd = $text.IndexOf('struct EditorShellState', $viewportStart)

if ($viewportEnd -lt 0) {
    throw "Could not determine early viewport region."
}

$viewportRegion = $text.Substring(
    $viewportStart,
    $viewportEnd - $viewportStart
)

$originalRegion = $viewportRegion

$viewportRegion = $viewportRegion.Replace(
    'gEditorShell.snapEnabled',
    'shellSnapEnabled()'
)

$viewportRegion = $viewportRegion.Replace(
    'gEditorShell.snapValue',
    'shellSnapValue()'
)

$viewportRegion = $viewportRegion.Replace(
    'gEditorShell.selectedEntity!=0',
    'shellSelectedEntityName()[0]!=0'
)

$viewportRegion = $viewportRegion.Replace(
    'selectedEntityName()',
    'shellSelectedEntityName()'
)

if ($viewportRegion -eq $originalRegion) {
    Write-Host "[INFO] Viewport header references were already repaired." -ForegroundColor DarkGray
}
else {
    $text = $text.Remove(
        $viewportStart,
        $viewportEnd - $viewportStart
    ).Insert(
        $viewportStart,
        $viewportRegion
    )

    Write-Host "[OK] Replaced early gEditorShell references in Viewport." -ForegroundColor Green
}

# ============================================================
# 3. ADD ACCESSOR DEFINITIONS AFTER selectedEntityName()
# ============================================================

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
    $end = -1

    for ($i = $braceStart; $i -lt $Source.Length; $i++) {
        $ch = $Source[$i]

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
    }
}

$selectedInfo = Get-FunctionBlockInfo `
    -Source $text `
    -Signature 'static const char* selectedEntityName()'

$accessorDefs = @'

static bool shellSnapEnabled()
{
    return gEditorShell.snapEnabled;
}

static float shellSnapValue()
{
    return gEditorShell.snapValue;
}

static const char* shellSelectedEntityName()
{
    if (gEditorShell.selectedEntity==0)
        return "";

    return selectedEntityName();
}
'@

$afterSelected = $selectedInfo.End + 1

$tail = $text.Substring($afterSelected)

if (-not $tail.Contains('static bool shellSnapEnabled()'))
{
    $text = $text.Insert(
        $afterSelected,
        $accessorDefs
    )

    Write-Host "[OK] Added shell accessor definitions after editor state." -ForegroundColor Green
}

# ============================================================
# SAVE + VERIFY
# ============================================================

Set-Content `
    -Path $MainCpp `
    -Value $text `
    -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$viewportVerifyStart = $verify.IndexOf('static void drawViewportView()')
$viewportVerifyEnd = $verify.IndexOf('struct EditorShellState', $viewportVerifyStart)

if ($viewportVerifyEnd -lt 0) {
    throw "Verification failed: could not locate viewport region."
}

$verifyRegion = $verify.Substring(
    $viewportVerifyStart,
    $viewportVerifyEnd - $viewportVerifyStart
)

if ($verifyRegion.Contains('gEditorShell.')) {
    throw "Verification failed: early Viewport still directly references gEditorShell."
}

if ($verifyRegion.Contains('selectedEntityName()')) {
    throw "Verification failed: early Viewport still directly calls selectedEntityName()."
}

$checks = @(
    'static bool shellSnapEnabled();',
    'static float shellSnapValue();',
    'static const char* shellSelectedEntityName();',
    'static bool shellSnapEnabled()',
    'return gEditorShell.snapEnabled;',
    'return gEditorShell.snapValue;',
    'return selectedEntityName();',
    'shellSelectedEntityName()[0]!=0'
)

foreach ($check in $checks)
{
    if (-not $verify.Contains($check)) {
        throw "Verification failed after write: $check"
    }
}

$badPair =
    [string]([char]96) +
    "r" +
    [char]96 +
    "n"

if ($verify.Contains($badPair)) {
    throw "Verification failed: literal PowerShell newline text entered main.cpp."
}

Write-Host ""
Write-Host "[DONE] Context-header declaration ordering repaired." -ForegroundColor Cyan
Write-Host "[OK] Viewport no longer accesses gEditorShell before declaration." -ForegroundColor Green
Write-Host "[OK] selectedEntityName access routed safely through accessor." -ForegroundColor Green
Write-Host "[OK] Existing editor-state layout was not moved." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
