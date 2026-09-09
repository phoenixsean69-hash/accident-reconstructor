// ROADSAFE_UI_TOKENS_V22
#pragma once

namespace roadsafe::ui
{
namespace token
{
    // Spacing scale - RoadSafe's equivalent of CSS spacing tokens.
    inline constexpr float Space2  = 2.0f;
    inline constexpr float Space4  = 4.0f;
    inline constexpr float Space6  = 6.0f;
    inline constexpr float Space8  = 8.0f;
    inline constexpr float Space12 = 12.0f;
    inline constexpr float Space16 = 16.0f;
    inline constexpr float Space20 = 20.0f;
    inline constexpr float Space24 = 24.0f;
    inline constexpr float Space32 = 32.0f;

    // Corner radius scale.
    inline constexpr float RadiusSm = 4.0f;
    inline constexpr float RadiusMd = 7.0f;
    inline constexpr float RadiusLg = 11.0f;
    inline constexpr float RadiusXl = 14.0f;

    // Standard control geometry.
    inline constexpr float ControlHeightSm = 28.0f;
    inline constexpr float ControlHeight   = 34.0f;
    inline constexpr float ControlHeightLg = 40.0f;
    inline constexpr float ToolbarHeight   = 42.0f;

    // Dashboard / editor breakpoints.
    inline constexpr float BreakpointCompact = 620.0f;
    inline constexpr float BreakpointWide    = 980.0f;
    inline constexpr float BreakpointXL      = 1320.0f;

    // Content widths.
    inline constexpr float InspectorMinWidth = 300.0f;
    inline constexpr float InspectorIdealWidth = 360.0f;
    inline constexpr float DashboardCardMinWidth = 220.0f;
}
}