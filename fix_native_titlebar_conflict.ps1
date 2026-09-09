param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host " SOVEREIGN - DARK NATIVE WINDOWS TITLE BAR" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor DarkGray
Write-Host ""

$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$MainCpp = Join-Path $ProjectRoot "src\main.cpp"
$CMake = Join-Path $ProjectRoot "CMakeLists.txt"

if (-not (Test-Path $MainCpp)) {
    throw "Could not find: $MainCpp"
}

if (-not (Test-Path $CMake)) {
    throw "Could not find: $CMake"
}

$cpp = Get-Content $MainCpp -Raw
$cmake = Get-Content $CMake -Raw

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$cppBackup = Join-Path $ProjectRoot "src\main.cpp.before-dark-titlebar-$timestamp.bak"
$cmakeBackup = Join-Path $ProjectRoot "CMakeLists.txt.before-dark-titlebar-$timestamp.bak"

Copy-Item $MainCpp $cppBackup -Force
Copy-Item $CMake $cmakeBackup -Force

Write-Host "[OK] Backups created:" -ForegroundColor Green
Write-Host "     $cppBackup"
Write-Host "     $cmakeBackup"

# ============================================================
# 1. Replace only the top GLFW/GLAD include block.
#    Windows headers are deliberately loaded before GLAD so
#    APIENTRY is not redefined afterwards.
# ============================================================

$includePattern = '(?s)^#define GLFW_INCLUDE_NONE\s*\r?\n#include <GLFW/glfw3\.h>\s*\r?\n#include <glad/glad\.h>'

if ($cpp -notmatch $includePattern) {
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

$cpp = [regex]::Replace(
    $cpp,
    $includePattern,
    $includeBlock,
    1
)

Write-Host "[OK] Added guarded Win32/DWM native-window support." -ForegroundColor Green

# ============================================================
# 2. Add the native caption helper once.
# ============================================================

if ($cpp -notmatch 'static\s+void\s+applyNativeWindowTheme\s*\(') {
    $helperMarker = 'constexpr float RIGHT_PANEL_MAX_WIDTH'

    $markerIndex = $cpp.IndexOf($helperMarker)
    if ($markerIndex -lt 0) {
        throw "Could not find RIGHT_PANEL_MAX_WIDTH insertion marker."
    }

    $lineEnd = $cpp.IndexOf("`n", $markerIndex)
    if ($lineEnd -lt 0) {
        throw "Could not resolve insertion line."
    }

    $helper = @'

#ifdef _WIN32
static void applyNativeWindowTheme(GLFWwindow* window)
{
    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd)
        return;

    // Windows 10 20H1+ / Windows 11 dark non-client area.
    // Numeric attributes keep this compatible with older SDK headers.
    constexpr DWORD kUseImmersiveDarkModeLegacy = 19;
    constexpr DWORD kUseImmersiveDarkMode = 20;
    constexpr DWORD kCaptionColor = 35;
    constexpr DWORD kTextColor = 36;

    BOOL dark = TRUE;

    HRESULT hr = DwmSetWindowAttribute(
        hwnd,
        kUseImmersiveDarkMode,
        &dark,
        sizeof(dark)
    );

    if (FAILED(hr))
    {
        DwmSetWindowAttribute(
            hwnd,
            kUseImmersiveDarkModeLegacy,
            &dark,
            sizeof(dark)
        );
    }

    // Supported on newer Windows 11 builds. If unsupported these calls
    // simply fail and immersive dark mode above remains active.
    const COLORREF caption = RGB(24, 25, 28);
    const COLORREF titleText = RGB(242, 244, 247);

    DwmSetWindowAttribute(
        hwnd,
        kCaptionColor,
        &caption,
        sizeof(caption)
    );

    DwmSetWindowAttribute(
        hwnd,
        kTextColor,
        &titleText,
        sizeof(titleText)
    );

    // Ask Windows to redraw the non-client frame immediately.
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

    $cpp = $cpp.Insert($lineEnd + 1, $helper)
    Write-Host "[OK] Added applyNativeWindowTheme()." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] Native window helper already exists." -ForegroundColor DarkGray
}

# ============================================================
# 3. Create the GLFW window hidden so users never see a white
#    title-bar flash before the DWM theme is applied.
# ============================================================

if ($cpp -notmatch 'glfwWindowHint\s*\(\s*GLFW_VISIBLE\s*,\s*GLFW_FALSE\s*\)') {
    $resizePattern = 'glfwWindowHint\s*\(\s*GLFW_RESIZABLE\s*,\s*GLFW_TRUE\s*\)\s*;'

    if ($cpp -notmatch $resizePattern) {
        throw "Could not find GLFW_RESIZABLE hint."
    }

    $cpp = [regex]::Replace(
        $cpp,
        $resizePattern,
        '$0' + "`r`n    glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);",
        1
    )

    Write-Host "[OK] Window now starts hidden while native chrome is themed." -ForegroundColor Green
}

# ============================================================
# 4. Theme + reveal immediately after context setup.
# ============================================================

if ($cpp -notmatch 'applyNativeWindowTheme\s*\(\s*window\s*\)') {
    $swapPattern = 'glfwSwapInterval\s*\(\s*1\s*\)\s*;'

    if ($cpp -notmatch $swapPattern) {
        throw "Could not find glfwSwapInterval(1)."
    }

    $afterSwap = @'
glfwSwapInterval(1);

#ifdef _WIN32
    applyNativeWindowTheme(window);
#endif

    glfwShowWindow(window);
'@

    $cpp = [regex]::Replace(
        $cpp,
        $swapPattern,
        $afterSwap,
        1
    )

    Write-Host "[OK] Dark caption is applied before the window is shown." -ForegroundColor Green
}

# ============================================================
# 5. Link dwmapi in CMake.
# ============================================================

if ($cmake -notmatch '\bdwmapi\b') {
    $linkPattern = 'target_link_libraries\s*\(\s*sfe\s+PRIVATE\s+opengl32\s+gdi32\s*\)'

    if ($cmake -notmatch $linkPattern) {
        throw "Could not find the Windows opengl32/gdi32 link line in CMakeLists.txt."
    }

    $cmake = [regex]::Replace(
        $cmake,
        $linkPattern,
        'target_link_libraries(sfe PRIVATE opengl32 gdi32 dwmapi)',
        1
    )

    Write-Host "[OK] Added dwmapi to Windows linker libraries." -ForegroundColor Green
}
else {
    Write-Host "[SKIP] dwmapi is already linked." -ForegroundColor DarkGray
}

Set-Content -Path $MainCpp -Value $cpp -Encoding UTF8
Set-Content -Path $CMake -Value $cmake -Encoding UTF8

# ============================================================
# VERIFY
# ============================================================

$cppVerify = Get-Content $MainCpp -Raw
$cmakeVerify = Get-Content $CMake -Raw

$checks = @(
    @{ Ok = ($cppVerify -match 'GLFW_EXPOSE_NATIVE_WIN32'); Name = 'GLFW native Win32 access' },
    @{ Ok = ($cppVerify -match 'applyNativeWindowTheme\s*\(\s*window\s*\)'); Name = 'Native theme call' },
    @{ Ok = ($cppVerify -match 'GLFW_VISIBLE\s*,\s*GLFW_FALSE'); Name = 'Hidden-until-themed startup' },
    @{ Ok = ($cppVerify -match 'glfwShowWindow\s*\(\s*window\s*\)'); Name = 'Window reveal' },
    @{ Ok = ($cmakeVerify -match '\bdwmapi\b'); Name = 'DWM linker dependency' }
)

foreach ($check in $checks) {
    if (-not $check.Ok) {
        throw "Verification failed: $($check.Name)"
    }
}

Write-Host ""
Write-Host "[DONE] Native title-bar conflict fixed." -ForegroundColor Cyan
Write-Host "[OK] Windows caption keeps normal drag/minimize/maximize/close behavior." -ForegroundColor Green
Write-Host "[OK] Caption is dark before the window becomes visible." -ForegroundColor Green
Write-Host "[OK] ImGui menu/tabs were not redesigned." -ForegroundColor Green
Write-Host "[OK] Sovereign screen layouts were not touched." -ForegroundColor Green
Write-Host ""
Write-Host "Because CMakeLists.txt changed, reconfigure once, then rebuild:" -ForegroundColor Yellow
Write-Host "  cmake -S . -B out\build\x64-Debug"
Write-Host "  cmake --build out\build\x64-Debug --config Debug"
Write-Host ""
