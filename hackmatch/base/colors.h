#pragma once

#include <array>
#include <cmath>
#include "../includes.h"

static float alphaFactor = 1.f;

template <typename DerivedT>
class color4_base
{
    float newAlphaFactor = alphaFactor;

public:
    union
    {
        float x, r, h;
    };

    union
    {
        float y, g, s;
    };

    union
    {
        float z, b, l, v;
    };

    union
    {
        float w, a;
    };

    color4_base() {}

    color4_base(const ImVec4& v4)
    {
        x = v4.x;
        y = v4.y;
        z = v4.z;
        a = v4.w;
    }

    color4_base(float nx, float ny, float nz, float na)
    {
        x = nx;
        y = ny;
        z = nz;
        a = na;
    }

    color4_base(float nx, float ny, float nz)
    {
        x = nx;
        y = ny;
        z = nz;
        a = 1.0f;
    }

    color4_base(unsigned col, bool alpha = false)
    {
        a = alpha ? (col >> 24 & 0xFF) / 255.0f : 1.0f;
        x = (col >> 16 & 0xFF) / 255.0f;
        y = (col >> 8 & 0xFF) / 255.0f;
        z = (col >> 0 & 0xFF) / 255.0f;
    }

    [[nodiscard]] ImVec4 v4() const
    {
        return ImVec4(x, y, z, a);
    }

    unsigned Pack(bool Alpha = true)
    {
        return (Alpha ? (static_cast<unsigned>(a * 255.0f) << 24) : 0) + (static_cast<unsigned>(x * 255.0f) << 16) +
            (static_cast<unsigned>(y * 255.0f) << 8) + static_cast<unsigned>(z * 255.0f);
    }

    ImU32 imGui()
    {
        DerivedT col(static_cast<DerivedT&>(*this));
        return IM_COL32(col.r, col.g, col.b, col.a);
    }

    ImVec4 imGuiText()
    {
        DerivedT col(static_cast<DerivedT&>(*this));
        return ImVec4(col.r / 255.f, col.g / 255.f, col.b / 255.f, 1.f * newAlphaFactor);
    }

    ImVec4 imVec4()
    {
        DerivedT col(static_cast<DerivedT&>(*this));
        auto imCol = ImColor(IM_COL32(col.r, col.g, col.b, col.a));
        return ImVec4(imCol.Value.x, imCol.Value.y, imCol.Value.z, imCol.Value.w);
    }

    DerivedT with255()
    {
        DerivedT col(static_cast<DerivedT&>(*this));
        return DerivedT(col.r * 255.f, col.g * 255.f, col.b * 255.f, col.a * 255.f);
    }

    DerivedT without255()
    {
        DerivedT col(static_cast<DerivedT&>(*this));
        return DerivedT(col.r / 255.f,
            col.g / 255.f,
            col.b / 255.f,
            col.a / 255.f);
    }

    DerivedT WithAlpha(float alpha)
    {
        DerivedT col(static_cast<DerivedT&>(*this));
        col.a = alpha * newAlphaFactor;
        return col;
    }
};

class ColorRGBA : public color4_base<ColorRGBA>
{
public:
    using color4_base::color4_base;
    ColorRGBA() = default;
};

inline unsigned long RgbaToDword(ColorRGBA rgba)
{
    return ((rgba.a * 256 + rgba.r) * 256 + rgba.g) * 256 + rgba.b;
}

inline ColorRGBA DwordToRgba(unsigned long dwColor)
{
    ColorRGBA tmp;

    tmp.b = dwColor % 256;
    dwColor /= 256;
    tmp.g = dwColor % 256;
    dwColor /= 256;
    tmp.r = dwColor % 256;
    dwColor /= 256;
    tmp.a = dwColor % 256;

    return tmp;
}

inline void ConvertHSVtoRGB(float h, float s, float v, float& outR, float& outG, float& outB) noexcept
{
    ImGui::ColorConvertHSVtoRGB(h, s, v, outR, outG, outB);
}


#define LERP(A, B, T) ((A) + (T) * ((B) - (A)))
inline unsigned int HealthColor(float fraction) noexcept
{
    float r, g, b;
    constexpr auto greenHue = 1.f / 3.f;
    constexpr auto redHue = 0.f;
    ConvertHSVtoRGB(LERP(redHue, greenHue, fraction), 1.f, 1.f, r, g, b);
    return ColorRGBA(static_cast<int>(r * 255.f), static_cast<int>(g * 255.f), static_cast<int>(b * 255.f), 255.f).imGui();
}

inline auto RainbowColor(float time, float speed, float alpha) noexcept
{
    constexpr float pi = 3.14159265359;
    return std::array{ std::sin(speed * time) * 0.5f + 0.5f, std::sin(speed * time + 2 * pi / 3) * 0.5f + 0.5f,
                      std::sin(speed * time + 4 * pi / 3) * 0.5f + 0.5f, alpha };
}

inline void SetAlphaFactor(float a) noexcept { alphaFactor = a; }

// Colors for menu style
namespace Colors
{
    // "Static" colors
    inline ColorRGBA AliceBlue = ColorRGBA(229.f, 236.f, 244.f, 255.f);
    inline ColorRGBA AbsoluteZero = ColorRGBA(0.f, 70.f, 184.f, 255.f);
    inline ColorRGBA Azure = ColorRGBA(51.f, 129.f, 255.f, 255.f);
    inline ColorRGBA BlueCrayola = ColorRGBA(24.f, 113.f, 255.f, 255.f);
    inline ColorRGBA BrightXiketic = ColorRGBA(26.f, 31.f, 52.f, 255.f);
    inline ColorRGBA CobaltBlue = ColorRGBA(8.f, 66.f, 166.f, 255.f);
    inline ColorRGBA CobaltBlueLow = ColorRGBA(8.f, 66.f, 166.f, 200.f);
    inline ColorRGBA Independence = ColorRGBA(61.f, 63.f, 83.f, 255.f);
    inline ColorRGBA OxfordBlue = ColorRGBA(25.f, 30.f, 51.f, 255.f);
    inline ColorRGBA RubyRed = ColorRGBA(164.f, 0.f, 37.f, 255.f);
    inline ColorRGBA SpaceCadet = ColorRGBA(26.f, 31.f, 52.f, 255.f);
    inline ColorRGBA SlateGray = ColorRGBA(107.f, 114.f, 128.f, 255.f);
    inline ColorRGBA UltramarineBlue = ColorRGBA(0.f, 98.f, 255.f, 255.f);
    inline ColorRGBA UltramarineBlueLow = ColorRGBA(0.f, 98.f, 255.f, 200.f);
    inline ColorRGBA White = ColorRGBA(255.f, 255.f, 255.f, 255.f);
    inline ColorRGBA Xiketic = ColorRGBA(19.f, 21.f, 39.f, 255.f);

    // UI colors
    inline ColorRGBA tabsBgColor = OxfordBlue;      // Used for tabs of main menu
    inline ColorRGBA subFontColor = SlateGray;      // Used for subFont
    inline ColorRGBA subTitleFontColor = SlateGray;      // Used for subTitleFont
    inline ColorRGBA mainFontColor = AliceBlue;     // Used for default font
    inline ColorRGBA windowBgColor = Xiketic;       // Used for default window bg
    inline ColorRGBA childBgColor = Xiketic;        // Used for default child bg
    inline ColorRGBA frameColor = BrightXiketic;    // Used for hovered/active frame
    inline ColorRGBA headerColor = UltramarineBlue; // Used for hovered/active frame
    inline ColorRGBA iconColor = UltramarineBlue;   // Used for icons in main menu

    inline ColorRGBA buttonColor = BlueCrayola;            // Used for button bg
    inline ColorRGBA buttonHoveredColor = UltramarineBlue; // Used for hovered button
    inline ColorRGBA buttonActiveColor = CobaltBlue;       // Used for active button

    inline ColorRGBA toggleButtonBgColor = Xiketic;           // Used for toggle button bg
    inline ColorRGBA toggleButtonHoveredColor = Xiketic;      // Used for hovered toggle button
    inline ColorRGBA toggleButtonActiveColor = CobaltBlueLow; // Used for active toggle button

    inline ColorRGBA toggleButtonCircleColor = UltramarineBlue;           // Used for toggle button's circle
    inline ColorRGBA toggleButtonCircleHoveredColor = UltramarineBlueLow; // Used for toggle button's hovered circle

    inline ColorRGBA sliderColor = Xiketic;
    inline ColorRGBA sliderColorAfter = CobaltBlueLow;

    inline ColorRGBA errorColor = RubyRed; // Used for error messages
    inline ColorRGBA inactiveColor = SpaceCadet; // Used for inactive elements
    inline ColorRGBA inactiveTextColor = SlateGray;
}
