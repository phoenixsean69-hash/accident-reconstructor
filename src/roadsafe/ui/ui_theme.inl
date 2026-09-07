// ROADSAFE_UI_THEME_V22
#pragma once

namespace roadsafe::ui
{
struct Palette
{
    ImVec4 canvas;
    ImVec4 surface;
    ImVec4 surfaceRaised;
    ImVec4 surfaceHover;
    ImVec4 text;
    ImVec4 textSecondary;
    ImVec4 textMuted;
    ImVec4 accent;
    ImVec4 success;
    ImVec4 warning;
    ImVec4 danger;
};

inline Palette currentPalette()
{
    return Palette{
        colorWindow(),
        colorPanel(),
        colorPanelRaised(),
        ImVec4(0.22f,0.23f,0.26f,1.0f),
        colorText(),
        ImVec4(0.78f,0.80f,0.84f,1.0f),
        colorMuted(),
        colorAccent(),
        colorSuccess(),
        ImVec4(0.95f,0.68f,0.20f,1.0f),
        ImVec4(0.92f,0.30f,0.30f,1.0f)
    };
}

inline ImU32 rgba(const ImVec4& value)
{
    return ImGui::ColorConvertFloat4ToU32(value);
}
}