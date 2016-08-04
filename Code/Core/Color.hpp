#pragma once

#include "Math.hpp"

#include <Backbone.hpp>

union color_linear
{
  struct
  {
    float R;
    float G;
    float B;
    float A;
  };

  vec4 Vector;

  float Data[4];
};

static_assert(SizeOf<color_linear>() == 16, "Incorrect size for color_linear.");

union color_linear_ub
{
  struct
  {
    uint8 R;
    uint8 G;
    uint8 B;
    uint8 A;
  };

  uint8 Data[4];
};

static_assert(SizeOf<color_linear_ub>() == 4, "Incorrect size of color_linear_ub.");

union color_gamma_ub
{
  struct
  {
    uint8 R;
    uint8 G;
    uint8 B;
    uint8 A;
  };

  uint8 Data[4];
};

static_assert(SizeOf<color_gamma_ub>() == 4, "Incorrect size of color_gamma_ub.");

//
// Color Construction
//

constexpr color_linear
ColorLinear(float LinearRed, float LinearGreen, float LinearBlue, float LinearAlpha = 1.0f)
{
  return { LinearRed, LinearGreen, LinearBlue, LinearAlpha };
}

void CORE_API
ExtractLinearHSV(color_linear const& Source, float* Hue, float* Saturation, float* Value);

color_linear CORE_API
ColorLinearFromLinearHSV(float Hue, float Saturation, float Value);

void CORE_API
ExtractGammaHSV(color_linear const& Source, float* Hue, float* Saturation, float* Value);

color_linear CORE_API
ColorLinearFromGammaHSV(float Hue, float Saturation, float Value);

color_gamma_ub constexpr
ColorGammaUB(uint8 GammaRed, uint8 GammaGreen, uint8 GammaBlue, uint8 GammaAlpha = 255)
{
  return { GammaRed, GammaGreen, GammaBlue, GammaAlpha };
}

color_gamma_ub constexpr
ColorLinearUB(uint8 LinearRed, uint8 LinearGreen, uint8 LinearBlue, uint8 LinearAlpha = 255)
{
  return { LinearRed, LinearGreen, LinearBlue, LinearAlpha };
}

//
// Color Algorithms and Accessors
//

// http://en.wikipedia.org/wiki/Luminance_%28relative%29
constexpr float
Luminance(color_linear const& Color)
{
  return 0.2126f * Color.R + 0.7152f * Color.G + 0.0722f * Color.B;
}

constexpr bool
IsNormalized(color_linear const& Color)
{
  return Color.R <= 1.0f && Color.G <= 1.0f && Color.B <= 1.0f && Color.A <= 1.0f &&
         Color.R >= 0.0f && Color.G >= 0.0f && Color.B >= 0.0f && Color.A >= 0.0f;
}

constexpr color_linear
Inverted(color_linear const& Color)
{
  return color_linear{ 1.0f - Color.R, 1.0f - Color.G, 1.0f - Color.B, 1.0f - Color.A };
}

inline void
Invert(color_linear* Color)
{
  *Color = Inverted(*Color);
}

/// Yields the color "Black" if the given `Color` is not normalized.
color_linear CORE_API
SafeInverted(color_linear const& Color);

/// \see SafeInverted(color_linear const&)
void CORE_API
SafeInvert(color_linear* Color);

inline float
FromGammaToLinear(float GammaValue)
{
  auto const LinearValue = GammaValue <= 0.04045f ? GammaValue / 12.92f : Pow((GammaValue + 0.055f) / 1.055f, 2.4f);
  return LinearValue;
}

inline float
FromLinearToGamma(float LinearValue)
{
  auto const GammaValue = LinearValue <= 0.0031308f ? 12.92f * LinearValue : 1.055f * Pow(LinearValue, 1.0f / 2.4f) - 0.055f;
  return GammaValue;
}

//
// Color Conversion
//

// Convert: From color_linear to color_linear_ub
template<>
struct impl_convert<color_linear_ub, color_linear>
{
  static constexpr color_linear_ub
  Do(color_linear const& Color)
  {
    return { FloatToUNorm<uint8>(Color.R),
             FloatToUNorm<uint8>(Color.G),
             FloatToUNorm<uint8>(Color.B),
             FloatToUNorm<uint8>(Color.A) };
  }
};

// Convert: From color_linear to color_gamma_ub
template<>
struct impl_convert<color_gamma_ub, color_linear>
{
  // Note: Can't make this constexpr due to FromLinearToGamma() not being constexpr
  static color_gamma_ub
  Do(color_linear const& Color)
  {
    return { FloatToUNorm<uint8>(FromLinearToGamma(Color.R)),
             FloatToUNorm<uint8>(FromLinearToGamma(Color.G)),
             FloatToUNorm<uint8>(FromLinearToGamma(Color.B)),
             FloatToUNorm<uint8>(                  Color.A ) };
  }
};

// Convert: From color_linear_ub to color_linear
template<>
struct impl_convert<color_linear, color_linear_ub>
{
  static constexpr color_linear
  Do(color_linear_ub const& Color)
  {
    return { UNormToFloat<uint8>(Color.R),
             UNormToFloat<uint8>(Color.G),
             UNormToFloat<uint8>(Color.B),
             UNormToFloat<uint8>(Color.A) };
  }
};

// Convert: From color_gamma_ub to color_linear
template<>
struct impl_convert<color_linear, color_gamma_ub>
{
  static constexpr color_linear
  Do(color_gamma_ub const& Color)
  {
    return { FromGammaToLinear(UNormToFloat<uint8>(Color.R)),
             FromGammaToLinear(UNormToFloat<uint8>(Color.G)),
             FromGammaToLinear(UNormToFloat<uint8>(Color.B)),
             UNormToFloat<uint8>(                  Color.A ) };
  }
};

//
// Predefined Colors
//

namespace color
{
  extern CORE_API color_linear const AliceBlue;              ///< #F0F8FF
  extern CORE_API color_linear const AntiqueWhite;           ///< #FAEBD7
  extern CORE_API color_linear const Aqua;                   ///< #00FFFF
  extern CORE_API color_linear const Aquamarine;             ///< #7FFFD4
  extern CORE_API color_linear const Azure;                  ///< #F0FFFF
  extern CORE_API color_linear const Beige;                  ///< #F5F5DC
  extern CORE_API color_linear const Bisque;                 ///< #FFE4C4
  extern CORE_API color_linear const Black;                  ///< #000000
  extern CORE_API color_linear const BlanchedAlmond;         ///< #FFEBCD
  extern CORE_API color_linear const Blue;                   ///< #0000FF
  extern CORE_API color_linear const BlueViolet;             ///< #8A2BE2
  extern CORE_API color_linear const Brown;                  ///< #A52A2A
  extern CORE_API color_linear const BurlyWood;              ///< #DEB887
  extern CORE_API color_linear const CadetBlue;              ///< #5F9EA0
  extern CORE_API color_linear const Chartreuse;             ///< #7FFF00
  extern CORE_API color_linear const Chocolate;              ///< #D2691E
  extern CORE_API color_linear const Coral;                  ///< #FF7F50
  extern CORE_API color_linear const CornflowerBlue;         ///< #6495ED  The original!
  extern CORE_API color_linear const Cornsilk;               ///< #FFF8DC
  extern CORE_API color_linear const Crimson;                ///< #DC143C
  extern CORE_API color_linear const Cyan;                   ///< #00FFFF
  extern CORE_API color_linear const DarkBlue;               ///< #00008B
  extern CORE_API color_linear const DarkCyan;               ///< #008B8B
  extern CORE_API color_linear const DarkGoldenRod;          ///< #B8860B
  extern CORE_API color_linear const DarkGray;               ///< #A9A9A9
  extern CORE_API color_linear const DarkGreen;              ///< #006400
  extern CORE_API color_linear const DarkKhaki;              ///< #BDB76B
  extern CORE_API color_linear const DarkMagenta;            ///< #8B008B
  extern CORE_API color_linear const DarkOliveGreen;         ///< #556B2F
  extern CORE_API color_linear const DarkOrange;             ///< #FF8C00
  extern CORE_API color_linear const DarkOrchid;             ///< #9932CC
  extern CORE_API color_linear const DarkRed;                ///< #8B0000
  extern CORE_API color_linear const DarkSalmon;             ///< #E9967A
  extern CORE_API color_linear const DarkSeaGreen;           ///< #8FBC8F
  extern CORE_API color_linear const DarkSlateBlue;          ///< #483D8B
  extern CORE_API color_linear const DarkSlateGray;          ///< #2F4F4F
  extern CORE_API color_linear const DarkTurquoise;          ///< #00CED1
  extern CORE_API color_linear const DarkViolet;             ///< #9400D3
  extern CORE_API color_linear const DeepPink;               ///< #FF1493
  extern CORE_API color_linear const DeepSkyBlue;            ///< #00BFFF
  extern CORE_API color_linear const DimGray;                ///< #696969
  extern CORE_API color_linear const DodgerBlue;             ///< #1E90FF
  extern CORE_API color_linear const FireBrick;              ///< #B22222
  extern CORE_API color_linear const FloralWhite;            ///< #FFFAF0
  extern CORE_API color_linear const ForestGreen;            ///< #228B22
  extern CORE_API color_linear const Fuchsia;                ///< #FF00FF
  extern CORE_API color_linear const Gainsboro;              ///< #DCDCDC
  extern CORE_API color_linear const GhostWhite;             ///< #F8F8FF
  extern CORE_API color_linear const Gold;                   ///< #FFD700
  extern CORE_API color_linear const GoldenRod;              ///< #DAA520
  extern CORE_API color_linear const Gray;                   ///< #808080
  extern CORE_API color_linear const Green;                  ///< #008000
  extern CORE_API color_linear const GreenYellow;            ///< #ADFF2F
  extern CORE_API color_linear const HoneyDew;               ///< #F0FFF0
  extern CORE_API color_linear const HotPink;                ///< #FF69B4
  extern CORE_API color_linear const IndianRed;              ///< #CD5C5C
  extern CORE_API color_linear const Indigo;                 ///< #4B0082
  extern CORE_API color_linear const Ivory;                  ///< #FFFFF0
  extern CORE_API color_linear const Khaki;                  ///< #F0E68C
  extern CORE_API color_linear const Lavender;               ///< #E6E6FA
  extern CORE_API color_linear const LavenderBlush;          ///< #FFF0F5
  extern CORE_API color_linear const LawnGreen;              ///< #7CFC00
  extern CORE_API color_linear const LemonChiffon;           ///< #FFFACD
  extern CORE_API color_linear const LightBlue;              ///< #ADD8E6
  extern CORE_API color_linear const LightCoral;             ///< #F08080
  extern CORE_API color_linear const LightCyan;              ///< #E0FFFF
  extern CORE_API color_linear const LightGoldenRodYellow;   ///< #FAFAD2
  extern CORE_API color_linear const LightGray;              ///< #D3D3D3
  extern CORE_API color_linear const LightGreen;             ///< #90EE90
  extern CORE_API color_linear const LightPink;              ///< #FFB6C1
  extern CORE_API color_linear const LightSalmon;            ///< #FFA07A
  extern CORE_API color_linear const LightSeaGreen;          ///< #20B2AA
  extern CORE_API color_linear const LightSkyBlue;           ///< #87CEFA
  extern CORE_API color_linear const LightSlateGray;         ///< #778899
  extern CORE_API color_linear const LightSteelBlue;         ///< #B0C4DE
  extern CORE_API color_linear const LightYellow;            ///< #FFFFE0
  extern CORE_API color_linear const Lime;                   ///< #00FF00
  extern CORE_API color_linear const LimeGreen;              ///< #32CD32
  extern CORE_API color_linear const Linen;                  ///< #FAF0E6
  extern CORE_API color_linear const Magenta;                ///< #FF00FF
  extern CORE_API color_linear const Maroon;                 ///< #800000
  extern CORE_API color_linear const MediumAquaMarine;       ///< #66CDAA
  extern CORE_API color_linear const MediumBlue;             ///< #0000CD
  extern CORE_API color_linear const MediumOrchid;           ///< #BA55D3
  extern CORE_API color_linear const MediumPurple;           ///< #9370DB
  extern CORE_API color_linear const MediumSeaGreen;         ///< #3CB371
  extern CORE_API color_linear const MediumSlateBlue;        ///< #7B68EE
  extern CORE_API color_linear const MediumSpringGreen;      ///< #00FA9A
  extern CORE_API color_linear const MediumTurquoise;        ///< #48D1CC
  extern CORE_API color_linear const MediumVioletRed;        ///< #C71585
  extern CORE_API color_linear const MidnightBlue;           ///< #191970
  extern CORE_API color_linear const MintCream;              ///< #F5FFFA
  extern CORE_API color_linear const MistyRose;              ///< #FFE4E1
  extern CORE_API color_linear const Moccasin;               ///< #FFE4B5
  extern CORE_API color_linear const NavajoWhite;            ///< #FFDEAD
  extern CORE_API color_linear const Navy;                   ///< #000080
  extern CORE_API color_linear const OldLace;                ///< #FDF5E6
  extern CORE_API color_linear const Olive;                  ///< #808000
  extern CORE_API color_linear const OliveDrab;              ///< #6B8E23
  extern CORE_API color_linear const Orange;                 ///< #FFA500
  extern CORE_API color_linear const OrangeRed;              ///< #FF4500
  extern CORE_API color_linear const Orchid;                 ///< #DA70D6
  extern CORE_API color_linear const PaleGoldenRod;          ///< #EEE8AA
  extern CORE_API color_linear const PaleGreen;              ///< #98FB98
  extern CORE_API color_linear const PaleTurquoise;          ///< #AFEEEE
  extern CORE_API color_linear const PaleVioletRed;          ///< #DB7093
  extern CORE_API color_linear const PapayaWhip;             ///< #FFEFD5
  extern CORE_API color_linear const PeachPuff;              ///< #FFDAB9
  extern CORE_API color_linear const Peru;                   ///< #CD853F
  extern CORE_API color_linear const Pink;                   ///< #FFC0CB
  extern CORE_API color_linear const Plum;                   ///< #DDA0DD
  extern CORE_API color_linear const PowderBlue;             ///< #B0E0E6
  extern CORE_API color_linear const Purple;                 ///< #800080
  extern CORE_API color_linear const RebeccaPurple;          ///< #663399
  extern CORE_API color_linear const Red;                    ///< #FF0000
  extern CORE_API color_linear const RosyBrown;              ///< #BC8F8F
  extern CORE_API color_linear const RoyalBlue;              ///< #4169E1
  extern CORE_API color_linear const SaddleBrown;            ///< #8B4513
  extern CORE_API color_linear const Salmon;                 ///< #FA8072
  extern CORE_API color_linear const SandyBrown;             ///< #F4A460
  extern CORE_API color_linear const SeaGreen;               ///< #2E8B57
  extern CORE_API color_linear const SeaShell;               ///< #FFF5EE
  extern CORE_API color_linear const Sienna;                 ///< #A0522D
  extern CORE_API color_linear const Silver;                 ///< #C0C0C0
  extern CORE_API color_linear const SkyBlue;                ///< #87CEEB
  extern CORE_API color_linear const SlateBlue;              ///< #6A5ACD
  extern CORE_API color_linear const SlateGray;              ///< #708090
  extern CORE_API color_linear const Snow;                   ///< #FFFAFA
  extern CORE_API color_linear const SpringGreen;            ///< #00FF7F
  extern CORE_API color_linear const SteelBlue;              ///< #4682B4
  extern CORE_API color_linear const Tan;                    ///< #D2B48C
  extern CORE_API color_linear const Teal;                   ///< #008080
  extern CORE_API color_linear const Thistle;                ///< #D8BFD8
  extern CORE_API color_linear const Tomato;                 ///< #FF6347
  extern CORE_API color_linear const Turquoise;              ///< #40E0D0
  extern CORE_API color_linear const Violet;                 ///< #EE82EE
  extern CORE_API color_linear const Wheat;                  ///< #F5DEB3
  extern CORE_API color_linear const White;                  ///< #FFFFFF
  extern CORE_API color_linear const WhiteSmoke;             ///< #F5F5F5
  extern CORE_API color_linear const Yellow;                 ///< #FFFF00
  extern CORE_API color_linear const YellowGreen;            ///< #9ACD32
}
