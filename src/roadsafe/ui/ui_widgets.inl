// ROADSAFE_UI_WIDGETS_V22
#pragma once

namespace roadsafe::ui
{
enum class ButtonKind
{
    Primary,
    Secondary,
    Ghost,
    Toolbar
};

enum class ButtonWidth
{
    Content,
    Fill
};

inline bool button(
    const char* label,
    float width=0.0f,
    bool active=false,
    bool enabled=true)
{
    return
        ::editorButton(
            label,
            width,
            active,
            enabled
        );
}

inline bool button(
    const char* label,
    ButtonWidth widthMode,
    bool active=false,
    bool enabled=true)
{
    const float width=
        widthMode==ButtonWidth::Fill
            ? availableWidth()
            : 0.0f;

    return
        ::editorButton(
            label,
            width,
            active,
            enabled
        );
}

inline void beginCard(
    const char* id,
    const ImVec2& size=ImVec2(0.0f,0.0f),
    bool raised=false,
    ImGuiWindowFlags flags=0)
{
    ::beginSurface(
        id,
        size,
        raised,
        flags
    );
}

inline void endCard()
{
    ::endSurface();
}

inline void badge(
    const char* text,
    const ImVec4& color)
{
    if (!text)
        return;

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        color
    );

    ImGui::TextUnformatted(text);

    ImGui::PopStyleColor();
}
}