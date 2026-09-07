// ROADSAFE_UI_LAYOUT_V22
#pragma once

namespace roadsafe::ui
{
enum class WidthClass
{
    Compact,
    Standard,
    Wide,
    ExtraWide
};

inline float availableWidth()
{
    return
        std::max(
            1.0f,
            ImGui::GetContentRegionAvail().x
        );
}

inline WidthClass widthClass(
    float width=-1.0f)
{
    if (width<0.0f)
        width=availableWidth();

    if (width<token::BreakpointCompact)
        return WidthClass::Compact;

    if (width<token::BreakpointWide)
        return WidthClass::Standard;

    if (width<token::BreakpointXL)
        return WidthClass::Wide;

    return WidthClass::ExtraWide;
}

inline int responsiveColumns(
    float minimumColumnWidth,
    int maximumColumns=4)
{
    const float usable=
        availableWidth();

    const float safeMinimum=
        std::max(
            1.0f,
            minimumColumnWidth
        );

    const int columns=
        static_cast<int>(
            std::floor(
                usable/safeMinimum
            )
        );

    return
        std::max(
            1,
            std::min(
                maximumColumns,
                columns
            )
        );
}

inline float fillWidth()
{
    return availableWidth();
}
}