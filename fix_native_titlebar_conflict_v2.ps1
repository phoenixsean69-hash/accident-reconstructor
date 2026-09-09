param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - NATIVE TITLE BAR + SAFE SHUTDOWN v2" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCppPath = Join-Path $ProjectRoot "src\main.cpp"
$CMakePath   = Join-Path $ProjectRoot "CMakeLists.txt"

if (-not (Test-Path $MainCppPath)) {
    throw "Could not find: $MainCppPath"
}

if (-not (Test-Path $CMakePath)) {
    throw "Could not find: $CMakePath"
}

$MainCppText = Get-Content $MainCppPath -Raw
$CMakeText   = Get-Content $CMakePath -Raw

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"

$MainBackup = Join-Path $ProjectRoot "src\main.cpp.before-titlebar-v2-$timestamp.bak"
$CMakeBackup = Join-Path $ProjectRoot "CMakeLists.txt.before-titlebar-v2-$timestamp.bak"

Copy-Item $MainCppPath $MainBackup -Force
Copy-Item $CMakePath $CMakeBackup -Force

Write-Host "[OK] Backups created:" -ForegroundColor Green
Write-Host "     $MainBackup"
Write-Host "     $CMakeBackup"

# ============================================================
# 1. Add Win32/DWM headers and GLFW native Win32 access
# ============================================================

if ($MainCppText -notmatch 'GLFW_EXPOSE_NATIVE_WIN32') {

    $includePattern = '(?s)^#define GLFW_INCLUDE_NONE\s*\r?\n#include <GLFW/glfw3\.h>\s*\r?\n#include <glad/glad\.h>'

    if ($MainCppText -notmatch $includePattern) {
        throw "Could not locate the expected GLFW/GLAD include block."
    }

    $includeBlock = @'
#define GLFW_INCLUDE_NONE

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <dwmapi.h>
#endif

#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <glad/glad.h>
'@

    $MainCppText = [regex]::Replace(
        $MainCppText,
        $includePattern,
        $includeBlock,
        1
    )

    Write-Host "[OK] Added Win32/DWM + GLFW native access." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] Win32 native access already present." -ForegroundColor DarkGray
}

# ============================================================
# 2. Add the native dark-caption helper
# ============================================================

if ($MainCppText -notmatch 'static\s+void\s+applyNativeWindowTheme\s*\(') {

    $insertAfterPattern = 'constexpr float RIGHT_PANEL_MAX_WIDTH\s*=\s*430\.0f\s*;'

    if ($MainCppText -notmatch $insertAfterPattern) {
        throw "Could not find RIGHT_PANEL_MAX_WIDTH."
    }

    $helper = @'

#ifdef _WIN32
static void applyNativeWindowTheme(GLFWwindow* window)
{
    HWND hwnd = glfwGetWin32Window(window);

    if (!hwnd)
        return;

    // Use numeric IDs so this also compiles with older Windows SDK headers.
    constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;
    constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_NEW = 20;
    constexpr DWORD DWMWA_CAPTION_COLOR_COMPAT = 35;
    constexpr DWORD DWMWA_TEXT_COLOR_COMPAT = 36;

    BOOL useDarkMode = TRUE;

    HRESULT darkResult = DwmSetWindowAttribute(
        hwnd,
        DWMWA_USE_IMMERSIVE_DARK_MODE_NEW,
        &useDarkMode,
        sizeof(useDarkMode)
    );

    if (FAILED(darkResult))
    {
        DwmSetWindowAttribute(
            hwnd,
            DWMWA_USE_IMMERSIVE_DARK_MODE_OLD,
            &useDarkMode,
            sizeof(useDarkMode)
        );
    }

    // On supported Windows 11 builds these make the native caption
    // visually match Sovereign's dark editor chrome.
    const COLORREF captionColor = RGB(24, 25, 28);
    const COLORREF captionText  = RGB(238, 240, 244);

    DwmSetWindowAttribute(
        hwnd,
        DWMWA_CAPTION_COLOR_COMPAT,
        &captionColor,
        sizeof(captionColor)
    );

    DwmSetWindowAttribute(
        hwnd,
        DWMWA_TEXT_COLOR_COMPAT,
        &captionText,
        sizeof(captionText)
    );

    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE |
        SWP_NOSIZE |
        SWP_NOZORDER |
        SWP_NOACTIVATE |
        SWP_FRAMECHANGED
    );
}
#endif
'@

    $MainCppText = [regex]::Replace(
        $MainCppText,
        $insertAfterPattern,
        '$0' + $helper,
        1
    )

    Write-Host "[OK] Added native dark-title-bar helper." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] Native theme helper already exists." -ForegroundColor DarkGray
}

# ============================================================
# 3. Hide the GLFW window until the caption is themed
# ============================================================

if ($MainCppText -notmatch 'GLFW_VISIBLE\s*,\s*GLFW_FALSE') {

    $resizePattern = 'glfwWindowHint\s*\(\s*GLFW_RESIZABLE\s*,\s*GLFW_TRUE\s*\)\s*;'

    if ($MainCppText -notmatch $resizePattern) {
        throw "Could not find GLFW_RESIZABLE window hint."
    }

    $MainCppText = [regex]::Replace(
        $MainCppText,
        $resizePattern,
        '$0' + "`r`n    glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);",
        1
    )

    Write-Host "[OK] Window starts hidden to avoid white-title flash." -ForegroundColor Green
}

# ============================================================
# 4. Apply native theme before the window is shown
# ============================================================

if ($MainCppText -notmatch 'applyNativeWindowTheme\s*\(\s*window\s*\)') {

    $swapPattern = 'glfwSwapInterval\s*\(\s*1\s*\)\s*;'

    if ($MainCppText -notmatch $swapPattern) {
        throw "Could not find glfwSwapInterval(1)."
    }

    $swapReplacement = @'
glfwSwapInterval(1);

#ifdef _WIN32
    applyNativeWindowTheme(window);
#endif

    glfwShowWindow(window);
'@

    $MainCppText = [regex]::Replace(
        $MainCppText,
        $swapPattern,
        $swapReplacement,
        1
    )

    Write-Host "[OK] Native caption themed before window reveal." -ForegroundColor Green
}

# ============================================================
# 5. CRITICAL: repair malformed shutdown from earlier patch
#
# Current repo can contain:
# for (...) if (texture) /* comment */
# ImGui_ImplOpenGL3_Shutdown();
#
# which makes Shutdown() the body of the if/for.
# ============================================================

$badCleanupPattern = 'for\s*\(\s*GLuint&\s+texture\s*:\s*gToolIcons\s*\)\s*if\s*\(\s*texture\s*\)\s*/\*\s*texture released when OpenGL context is destroyed\s*\*/\s*\r?\n\s*ImGui_ImplOpenGL3_Shutdown\s*\(\s*\)\s*;'

if ($MainCppText -match $badCleanupPattern) {

    $safeCleanup = @'
// Tool icon textures are owned by the OpenGL context.
// They are released automatically when the context is destroyed.
// Do not place ImGui shutdown inside the icon loop.
    ImGui_ImplOpenGL3_Shutdown();
'@

    $MainCppText = [regex]::Replace(
        $MainCppText,
        $badCleanupPattern,
        $safeCleanup,
        1
    )

    Write-Host "[OK] Repaired malformed shutdown loop." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] Malformed shutdown loop not present." -ForegroundColor DarkGray
}

# ============================================================
# 6. Link dwmapi
# ============================================================

if ($CMakeText -notmatch '\bdwmapi\b') {

    $windowsLinkPattern = 'target_link_libraries\s*\(\s*sfe\s+PRIVATE\s+opengl32\s+gdi32\s*\)'

    if ($CMakeText -notmatch $windowsLinkPattern) {
        throw "Could not find Windows OpenGL/GDI link line in CMakeLists.txt."
    }

    $CMakeText = [regex]::Replace(
        $CMakeText,
        $windowsLinkPattern,
        'target_link_libraries(sfe PRIVATE opengl32 gdi32 dwmapi)',
        1
    )

    Write-Host "[OK] Linked dwmapi." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] dwmapi already linked." -ForegroundColor DarkGray
}

# ============================================================
# SAVE
# ============================================================

Set-Content -Path $MainCppPath -Value $MainCppText -Encoding UTF8
Set-Content -Path $CMakePath -Value $CMakeText -Encoding UTF8

# ============================================================
# VERIFY
# ============================================================

$MainVerify = Get-Content $MainCppPath -Raw
$CMakeVerify = Get-Content $CMakePath -Raw

$checks = @(
    @{ Name = "GLFW native Win32 include"; Ok = ($MainVerify -match 'GLFW_EXPOSE_NATIVE_WIN32') },
    @{ Name = "Native window theme helper"; Ok = ($MainVerify -match 'static\s+void\s+applyNativeWindowTheme') },
    @{ Name = "Native window theme call"; Ok = ($MainVerify -match 'applyNativeWindowTheme\s*\(\s*window\s*\)') },
    @{ Name = "Hidden startup"; Ok = ($MainVerify -match 'GLFW_VISIBLE\s*,\s*GLFW_FALSE') },
    @{ Name = "Window reveal"; Ok = ($MainVerify -match 'glfwShowWindow\s*\(\s*window\s*\)') },
    @{ Name = "DWM library"; Ok = ($CMakeVerify -match '\bdwmapi\b') }
)

foreach ($check in $checks) {
    if (-not $check.Ok) {
        throw "Verification failed: $($check.Name)"
    }
}

if ($MainVerify -match $badCleanupPattern) {
    throw "Verification failed: malformed shutdown loop still exists."
}

Write-Host ""
Write-Host "[DONE] Native-title + shutdown fix installed." -ForegroundColor Cyan
Write-Host "[OK] Native Windows title bar will be dark-themed." -ForegroundColor Green
Write-Host "[OK] Standard minimize/maximize/close behavior retained." -ForegroundColor Green
Write-Host "[OK] Repeated ImGui OpenGL shutdown bug removed if present." -ForegroundColor Green
Write-Host "[OK] ImGui workspace layout/palette untouched." -ForegroundColor Green
Write-Host ""
Write-Host "Now reconfigure + rebuild Debug:" -ForegroundColor Yellow
Write-Host "  cmake -S . -B out\build\x64-Debug"
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
