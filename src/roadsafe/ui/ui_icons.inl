// ROADSAFE_UI_ICONS_V22
#pragma once

namespace roadsafe::ui
{
using Icon = ::UiGlyph;

inline void drawIcon(
    ImDrawList* drawList,
    Icon icon,
    const ImVec2& center,
    float size,
    ImU32 color)
{
    ::drawGlyph(
        drawList,
        icon,
        center,
        size,
        color
    );
}

inline void drawIcon(
    Icon icon,
    const ImVec2& center,
    float size,
    const ImVec4& color)
{
    ::drawGlyph(
        ImGui::GetWindowDrawList(),
        icon,
        center,
        size,
        ImGui::ColorConvertFloat4ToU32(color)
    );
}
}