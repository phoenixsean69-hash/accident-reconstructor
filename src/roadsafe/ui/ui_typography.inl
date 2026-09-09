// ROADSAFE_UI_TYPOGRAPHY_V22
#pragma once

namespace roadsafe::ui
{
inline void textPrimary(const char* text)
{
    if (!text)
        return;

    ImGui::TextUnformatted(text);
}

inline void textSecondary(const char* text)
{
    if (!text)
        return;

    ImGui::TextColored(
        currentPalette().textSecondary,
        "%s",
        text
    );
}

inline void textMuted(const char* text)
{
    if (!text)
        return;

    ImGui::TextColored(
        currentPalette().textMuted,
        "%s",
        text
    );
}

inline void overline(const char* text)
{
    if (!text)
        return;

    ImGui::TextColored(
        currentPalette().textMuted,
        "%s",
        text
    );
}
}