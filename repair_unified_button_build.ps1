param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - REPAIR UNIFIED BUTTON BUILD" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

$text = Get-Content $MainCpp -Raw

if (-not $text.Contains('static bool editorButton(')) {
    throw "Could not find editorButton()."
}

if (-not $text.Contains('drawUnifiedButtonLabel(')) {
    throw "Could not find unified button-label code."
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $ProjectRoot "src\main.cpp.before-button-build-repair-$timestamp.bak"
Copy-Item $MainCpp $backup -Force

Write-Host "[OK] Backup created:" -ForegroundColor Green
Write-Host "     $backup"

# ============================================================
# 1. RESTORE editorButton DEFAULT ARGUMENTS
# ============================================================

$editorStart = $text.IndexOf('static bool editorButton(')

if ($editorStart -lt 0) {
    throw "Could not locate editorButton()."
}

$editorParenEnd = $text.IndexOf(')', $editorStart)

if ($editorParenEnd -lt 0) {
    throw "Could not locate editorButton parameter list."
}

$editorHeader = $text.Substring(
    $editorStart,
    $editorParenEnd - $editorStart + 1
)

$newHeader = $editorHeader

$newHeader = [regex]::Replace(
    $newHeader,
    'float\s+width(?:\s*=\s*[^,\r\n\)]+)?',
    'float width=0.0f',
    1
)

$newHeader = [regex]::Replace(
    $newHeader,
    'bool\s+active(?:\s*=\s*[^,\r\n\)]+)?',
    'bool active=false',
    1
)

$newHeader = [regex]::Replace(
    $newHeader,
    'bool\s+enabled(?:\s*=\s*[^,\r\n\)]+)?',
    'bool enabled=true',
    1
)

if ($newHeader -eq $editorHeader) {
    Write-Host "[INFO] editorButton defaults already looked correct." -ForegroundColor DarkGray
}
else {
    $text = $text.Remove(
        $editorStart,
        $editorHeader.Length
    ).Insert(
        $editorStart,
        $newHeader
    )

    Write-Host "[OK] Restored editorButton(label, width, active=false, enabled=true)." -ForegroundColor Green
}

# ============================================================
# 2. ADD FORWARD DECLARATION FOR drawUnifiedButtonLabel
# ============================================================

$prototype = @'
static void drawUnifiedButtonLabel(
    ImDrawList* drawList,
    const ImVec2& minPos,
    const ImVec2& maxPos,
    const char* label,
    const ImVec4& textColor,
    bool enabled);

'@

$prototypeNeedle = 'static void drawUnifiedButtonLabel('
$helperPos = $text.IndexOf($prototypeNeedle)
$editorPos = $text.IndexOf('static bool editorButton(')

if ($helperPos -lt 0) {
    throw "Could not locate drawUnifiedButtonLabel implementation."
}

if ($helperPos -gt $editorPos) {
    # Only add a declaration if one does not already exist before editorButton.
    $beforeEditor = $text.Substring(0, $editorPos)

    if (-not $beforeEditor.Contains($prototypeNeedle)) {
        $text = $text.Insert(
            $editorPos,
            $prototype
        )

        Write-Host "[OK] Added forward declaration for drawUnifiedButtonLabel()." -ForegroundColor Green
    }
    else {
        Write-Host "[INFO] Forward declaration already exists." -ForegroundColor DarkGray
    }
}
else {
    Write-Host "[INFO] drawUnifiedButtonLabel already appears before editorButton." -ForegroundColor DarkGray
}

# ============================================================
# 3. REMOVE CODEPAGE-WARNING BULLET CHARACTERS
# ============================================================

$bullet = [char]0x25CF

if ($text.Contains([string]$bullet)) {
    $text = $text.Replace(
        ([string]$bullet + " EDITOR PREVIEW"),
        "EDITOR PREVIEW"
    )

    $text = $text.Replace(
        ([string]$bullet + " LIVE"),
        "LIVE"
    )

    $text = $text.Replace(
        ([string]$bullet + " OFFLINE"),
        "OFFLINE"
    )

    $text = $text.Replace(
        [string]$bullet,
        ""
    )

    Write-Host "[OK] Removed U+25CF status bullets that triggered C4566." -ForegroundColor Green
}

# ============================================================
# 4. DEFENSIVE CHECK FOR BAD POWERSHELL ESCAPES
# ============================================================

$badPair = [string]([char]96) + "r" + [char]96 + "n"

if ($text.Contains($badPair)) {
    throw "Found literal PowerShell `r`n text in main.cpp. Run the earlier backtick repair first."
}

# ============================================================
# WRITE + VERIFY
# ============================================================

Set-Content -Path $MainCpp -Value $text -Encoding UTF8

$verify = Get-Content $MainCpp -Raw

$verifyEditor = $verify.IndexOf('static bool editorButton(')

if ($verifyEditor -lt 0) {
    throw "Verification failed: editorButton missing."
}

$verifyEnd = $verify.IndexOf(')', $verifyEditor)

if ($verifyEnd -lt 0) {
    throw "Verification failed: editorButton header malformed."
}

$verifyHeader = $verify.Substring(
    $verifyEditor,
    $verifyEnd - $verifyEditor + 1
)

if (-not $verifyHeader.Contains('float width=0.0f')) {
    throw "Verification failed: width default missing."
}

if (-not $verifyHeader.Contains('bool active=false')) {
    throw "Verification failed: active default missing."
}

if (-not $verifyHeader.Contains('bool enabled=true')) {
    throw "Verification failed: enabled default missing."
}

$beforeEditorVerify = $verify.Substring(0, $verifyEditor)

if (-not $beforeEditorVerify.Contains('static void drawUnifiedButtonLabel(')) {
    throw "Verification failed: drawUnifiedButtonLabel declaration is not visible before editorButton."
}

if ($verify.Contains([string][char]0x25CF)) {
    throw "Verification failed: U+25CF bullet still remains."
}

Write-Host ""
Write-Host "[DONE] Unified-button build repair applied." -ForegroundColor Cyan
Write-Host "[OK] editorButton 1/2/3/4-argument call compatibility restored." -ForegroundColor Green
Write-Host "[OK] drawUnifiedButtonLabel declaration order fixed." -ForegroundColor Green
Write-Host "[OK] AR bullet code-page warnings removed." -ForegroundColor Green
Write-Host ""
Write-Host "Rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
