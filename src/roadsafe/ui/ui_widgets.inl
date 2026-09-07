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
    // ROADSAFE_DASHBOARD_CARD_SPACING_V23_2
    // ROADSAFE_DASHBOARD_INNER_GUTTER_V23_3
    //
    // Dear ImGui child windows can suppress WindowPadding depending on
    // child-window flags/version. Do not rely on inherited child padding
    // for dashboard cards. Create a real inset content child instead.
    flags |=
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoScrollbar;

    const Palette palette=
        currentPalette();

    ImGui::PushStyleColor(
        ImGuiCol_ChildBg,
        raised
            ? palette.surfaceRaised
            : palette.surface
    );

    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(
            0.0f,
            0.0f,
            0.0f,
            0.0f
        )
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_ChildRounding,
        token::RadiusLg
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(
            0.0f,
            0.0f
        )
    );

    ImGui::BeginChild(
        id,
        size,
        false,
        flags
    );

    const ImVec2 cardPadding(
        token::Space20,
        token::Space16
    );

    const ImVec2 cardSize=
        ImGui::GetWindowSize();

    const ImVec2 innerSize(
        std::max(
            1.0f,
            cardSize.x-
            cardPadding.x*2.0f
        ),
        std::max(
            1.0f,
            cardSize.y-
            cardPadding.y*2.0f
        )
    );

    ImGui::SetCursorPos(
        cardPadding
    );

    ImGui::BeginChild(
        "##RoadSafeDashboardCardContent",
        innerSize,
        false,
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );
}

inline void endCard()
{
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
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