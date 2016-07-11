#include "Color.hpp"

#include "Log.hpp"

auto
::SafeInverted(color_linear const& Color)
  -> color_linear
{
  if(IsNormalized(Color))
  {
    return { 1.0f - Color.R, 1.0f - Color.G, 1.0f - Color.B, 1.0f - Color.A };
  }

  return {};
}

auto
::SafeInvert(color_linear* Color)
  -> void
{
  *Color = SafeInverted(*Color);
}

// http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_%28C%29
auto
::ExtractLinearHSV(color_linear const& Source, float* Hue, float* Saturation, float* Value)
  -> void
{
  *Hue = 0.0f;
  *Saturation = 0.0f;
  *Value = Max(Source.R, Max(Source.G, Source.B));

  if(IsNearlyZero(*Value))
    return;

  float const InvValue = 1.0f / *Value;
  float Norm_R = Source.R * InvValue;
  float Norm_G = Source.G * InvValue;
  float Norm_B = Source.B * InvValue;
  float const RGB_Min = Min(Norm_R, Min(Norm_G, Norm_B));
  float const RGB_Max = Max(Norm_R, Min(Norm_G, Norm_B));

  *Saturation = RGB_Max - RGB_Min;

  if(*Saturation == 0)
    return;

  // Normalize saturation
  float const RGB_Delta_Inverse = 1.0f / *Saturation;
  Norm_R = (Norm_R - RGB_Min) * RGB_Delta_Inverse;
  Norm_G = (Norm_G - RGB_Min) * RGB_Delta_Inverse;
  Norm_B = (Norm_B - RGB_Min) * RGB_Delta_Inverse;

  // Hue
  if(RGB_Max == Norm_R)
  {
    *Hue = 60.0f * (Norm_G - Norm_B);

    if(*Hue < 0) *Hue += 360.0f;
  }
  else if(RGB_Max == Norm_G)
  {
    *Hue = 120.0f + 60.0f * (Norm_B - Norm_R);
  }
  else
  {
    *Hue = 240.0f + 60.0f * (Norm_R - Norm_G);
  }
}

// http://www.rapidtables.com/convert/color/hsv-to-rgb.htm
auto
::ColorLinearFromLinearHSV(float Hue, float Saturation, float Value)
  -> color_linear
{
  if(Hue < 0 || Hue > 360)
  {
    LogError("HSV hue value is in invalid range: %f [0, 360]", Hue);
    return {};
  }

  if(Saturation < 0 || Saturation > 1)
  {
    LogError("HSV saturation value is in invalid range: %f [0, 1]", Saturation);
    return {};
  }

  if(Value < 0 || Value > 1)
  {
    LogError("HSV value is in invalid range: %f [0, 1]", Value);
    return {};
  }

  color_linear Result;

  float const C = Saturation * Value;
  float const X = C * (1.0f - Abs(Mod(Hue / 60.0f, 2) - 1.0f));
  float const M = Value - C;

  Result.A = 1.0f;

  if (Hue < 60)
  {
    Result.R = C + M;
    Result.G = X + M;
    Result.B = 0 + M;
  }
  else if (Hue < 120)
  {
    Result.R = X + M;
    Result.G = C + M;
    Result.B = 0 + M;
  }
  else if (Hue < 180)
  {
    Result.R = 0 + M;
    Result.G = C + M;
    Result.B = X + M;
  }
  else if (Hue < 240)
  {
    Result.R = 0 + M;
    Result.G = X + M;
    Result.B = C + M;
  }
  else if (Hue < 300)
  {
    Result.R = X + M;
    Result.G = 0 + M;
    Result.B = C + M;
  }
  else
  {
    Result.R = C + M;
    Result.G = 0 + M;
    Result.B = X + M;
  }

  return Result;
}

auto
::ExtractGammaHSV(color_linear const& Source, float* Hue, float* Saturation, float* Value)
  -> void
{
  auto const GammaR = FromLinearToGamma(Source.R);
  auto const GammaG = FromLinearToGamma(Source.G);
  auto const GammaB = FromLinearToGamma(Source.B);

  auto ColorGamma = ColorLinear(GammaR, GammaG, GammaB, Source.A);
  ExtractLinearHSV(ColorGamma, Hue, Saturation, Value);
}

auto
::ColorLinearFromGammaHSV(float Hue, float Saturation, float Value)
  -> color_linear
{
  auto Color = ColorLinearFromLinearHSV(Hue, Saturation, Value);

  Color.R = FromGammaToLinear(Color.R);
  Color.G = FromGammaToLinear(Color.G);
  Color.B = FromGammaToLinear(Color.B);

  return Color;
}

color_linear const color::AliceBlue            = Convert<color_linear>(ColorGammaUB(0xF0, 0xF8, 0xFF));
color_linear const color::AntiqueWhite         = Convert<color_linear>(ColorGammaUB(0xFA, 0xEB, 0xD7));
color_linear const color::Aqua                 = Convert<color_linear>(ColorGammaUB(0x00, 0xFF, 0xFF));
color_linear const color::Aquamarine           = Convert<color_linear>(ColorGammaUB(0x7F, 0xFF, 0xD4));
color_linear const color::Azure                = Convert<color_linear>(ColorGammaUB(0xF0, 0xFF, 0xFF));
color_linear const color::Beige                = Convert<color_linear>(ColorGammaUB(0xF5, 0xF5, 0xDC));
color_linear const color::Bisque               = Convert<color_linear>(ColorGammaUB(0xFF, 0xE4, 0xC4));
color_linear const color::Black                = Convert<color_linear>(ColorGammaUB(0x00, 0x00, 0x00));
color_linear const color::BlanchedAlmond       = Convert<color_linear>(ColorGammaUB(0xFF, 0xEB, 0xCD));
color_linear const color::Blue                 = Convert<color_linear>(ColorGammaUB(0x00, 0x00, 0xFF));
color_linear const color::BlueViolet           = Convert<color_linear>(ColorGammaUB(0x8A, 0x2B, 0xE2));
color_linear const color::Brown                = Convert<color_linear>(ColorGammaUB(0xA5, 0x2A, 0x2A));
color_linear const color::BurlyWood            = Convert<color_linear>(ColorGammaUB(0xDE, 0xB8, 0x87));
color_linear const color::CadetBlue            = Convert<color_linear>(ColorGammaUB(0x5F, 0x9E, 0xA0));
color_linear const color::Chartreuse           = Convert<color_linear>(ColorGammaUB(0x7F, 0xFF, 0x00));
color_linear const color::Chocolate            = Convert<color_linear>(ColorGammaUB(0xD2, 0x69, 0x1E));
color_linear const color::Coral                = Convert<color_linear>(ColorGammaUB(0xFF, 0x7F, 0x50));
color_linear const color::CornflowerBlue       = Convert<color_linear>(ColorGammaUB(0x64, 0x95, 0xED));
color_linear const color::Cornsilk             = Convert<color_linear>(ColorGammaUB(0xFF, 0xF8, 0xDC));
color_linear const color::Crimson              = Convert<color_linear>(ColorGammaUB(0xDC, 0x14, 0x3C));
color_linear const color::Cyan                 = Convert<color_linear>(ColorGammaUB(0x00, 0xFF, 0xFF));
color_linear const color::DarkBlue             = Convert<color_linear>(ColorGammaUB(0x00, 0x00, 0x8B));
color_linear const color::DarkCyan             = Convert<color_linear>(ColorGammaUB(0x00, 0x8B, 0x8B));
color_linear const color::DarkGoldenRod        = Convert<color_linear>(ColorGammaUB(0xB8, 0x86, 0x0B));
color_linear const color::DarkGray             = Convert<color_linear>(ColorGammaUB(0xA9, 0xA9, 0xA9));
color_linear const color::DarkGreen            = Convert<color_linear>(ColorGammaUB(0x00, 0x64, 0x00));
color_linear const color::DarkKhaki            = Convert<color_linear>(ColorGammaUB(0xBD, 0xB7, 0x6B));
color_linear const color::DarkMagenta          = Convert<color_linear>(ColorGammaUB(0x8B, 0x00, 0x8B));
color_linear const color::DarkOliveGreen       = Convert<color_linear>(ColorGammaUB(0x55, 0x6B, 0x2F));
color_linear const color::DarkOrange           = Convert<color_linear>(ColorGammaUB(0xFF, 0x8C, 0x00));
color_linear const color::DarkOrchid           = Convert<color_linear>(ColorGammaUB(0x99, 0x32, 0xCC));
color_linear const color::DarkRed              = Convert<color_linear>(ColorGammaUB(0x8B, 0x00, 0x00));
color_linear const color::DarkSalmon           = Convert<color_linear>(ColorGammaUB(0xE9, 0x96, 0x7A));
color_linear const color::DarkSeaGreen         = Convert<color_linear>(ColorGammaUB(0x8F, 0xBC, 0x8F));
color_linear const color::DarkSlateBlue        = Convert<color_linear>(ColorGammaUB(0x48, 0x3D, 0x8B));
color_linear const color::DarkSlateGray        = Convert<color_linear>(ColorGammaUB(0x2F, 0x4F, 0x4F));
color_linear const color::DarkTurquoise        = Convert<color_linear>(ColorGammaUB(0x00, 0xCE, 0xD1));
color_linear const color::DarkViolet           = Convert<color_linear>(ColorGammaUB(0x94, 0x00, 0xD3));
color_linear const color::DeepPink             = Convert<color_linear>(ColorGammaUB(0xFF, 0x14, 0x93));
color_linear const color::DeepSkyBlue          = Convert<color_linear>(ColorGammaUB(0x00, 0xBF, 0xFF));
color_linear const color::DimGray              = Convert<color_linear>(ColorGammaUB(0x69, 0x69, 0x69));
color_linear const color::DodgerBlue           = Convert<color_linear>(ColorGammaUB(0x1E, 0x90, 0xFF));
color_linear const color::FireBrick            = Convert<color_linear>(ColorGammaUB(0xB2, 0x22, 0x22));
color_linear const color::FloralWhite          = Convert<color_linear>(ColorGammaUB(0xFF, 0xFA, 0xF0));
color_linear const color::ForestGreen          = Convert<color_linear>(ColorGammaUB(0x22, 0x8B, 0x22));
color_linear const color::Fuchsia              = Convert<color_linear>(ColorGammaUB(0xFF, 0x00, 0xFF));
color_linear const color::Gainsboro            = Convert<color_linear>(ColorGammaUB(0xDC, 0xDC, 0xDC));
color_linear const color::GhostWhite           = Convert<color_linear>(ColorGammaUB(0xF8, 0xF8, 0xFF));
color_linear const color::Gold                 = Convert<color_linear>(ColorGammaUB(0xFF, 0xD7, 0x00));
color_linear const color::GoldenRod            = Convert<color_linear>(ColorGammaUB(0xDA, 0xA5, 0x20));
color_linear const color::Gray                 = Convert<color_linear>(ColorGammaUB(0x80, 0x80, 0x80));
color_linear const color::Green                = Convert<color_linear>(ColorGammaUB(0x00, 0x80, 0x00));
color_linear const color::GreenYellow          = Convert<color_linear>(ColorGammaUB(0xAD, 0xFF, 0x2F));
color_linear const color::HoneyDew             = Convert<color_linear>(ColorGammaUB(0xF0, 0xFF, 0xF0));
color_linear const color::HotPink              = Convert<color_linear>(ColorGammaUB(0xFF, 0x69, 0xB4));
color_linear const color::IndianRed            = Convert<color_linear>(ColorGammaUB(0xCD, 0x5C, 0x5C));
color_linear const color::Indigo               = Convert<color_linear>(ColorGammaUB(0x4B, 0x00, 0x82));
color_linear const color::Ivory                = Convert<color_linear>(ColorGammaUB(0xFF, 0xFF, 0xF0));
color_linear const color::Khaki                = Convert<color_linear>(ColorGammaUB(0xF0, 0xE6, 0x8C));
color_linear const color::Lavender             = Convert<color_linear>(ColorGammaUB(0xE6, 0xE6, 0xFA));
color_linear const color::LavenderBlush        = Convert<color_linear>(ColorGammaUB(0xFF, 0xF0, 0xF5));
color_linear const color::LawnGreen            = Convert<color_linear>(ColorGammaUB(0x7C, 0xFC, 0x00));
color_linear const color::LemonChiffon         = Convert<color_linear>(ColorGammaUB(0xFF, 0xFA, 0xCD));
color_linear const color::LightBlue            = Convert<color_linear>(ColorGammaUB(0xAD, 0xD8, 0xE6));
color_linear const color::LightCoral           = Convert<color_linear>(ColorGammaUB(0xF0, 0x80, 0x80));
color_linear const color::LightCyan            = Convert<color_linear>(ColorGammaUB(0xE0, 0xFF, 0xFF));
color_linear const color::LightGoldenRodYellow = Convert<color_linear>(ColorGammaUB(0xFA, 0xFA, 0xD2));
color_linear const color::LightGray            = Convert<color_linear>(ColorGammaUB(0xD3, 0xD3, 0xD3));
color_linear const color::LightGreen           = Convert<color_linear>(ColorGammaUB(0x90, 0xEE, 0x90));
color_linear const color::LightPink            = Convert<color_linear>(ColorGammaUB(0xFF, 0xB6, 0xC1));
color_linear const color::LightSalmon          = Convert<color_linear>(ColorGammaUB(0xFF, 0xA0, 0x7A));
color_linear const color::LightSeaGreen        = Convert<color_linear>(ColorGammaUB(0x20, 0xB2, 0xAA));
color_linear const color::LightSkyBlue         = Convert<color_linear>(ColorGammaUB(0x87, 0xCE, 0xFA));
color_linear const color::LightSlateGray       = Convert<color_linear>(ColorGammaUB(0x77, 0x88, 0x99));
color_linear const color::LightSteelBlue       = Convert<color_linear>(ColorGammaUB(0xB0, 0xC4, 0xDE));
color_linear const color::LightYellow          = Convert<color_linear>(ColorGammaUB(0xFF, 0xFF, 0xE0));
color_linear const color::Lime                 = Convert<color_linear>(ColorGammaUB(0x00, 0xFF, 0x00));
color_linear const color::LimeGreen            = Convert<color_linear>(ColorGammaUB(0x32, 0xCD, 0x32));
color_linear const color::Linen                = Convert<color_linear>(ColorGammaUB(0xFA, 0xF0, 0xE6));
color_linear const color::Magenta              = Convert<color_linear>(ColorGammaUB(0xFF, 0x00, 0xFF));
color_linear const color::Maroon               = Convert<color_linear>(ColorGammaUB(0x80, 0x00, 0x00));
color_linear const color::MediumAquaMarine     = Convert<color_linear>(ColorGammaUB(0x66, 0xCD, 0xAA));
color_linear const color::MediumBlue           = Convert<color_linear>(ColorGammaUB(0x00, 0x00, 0xCD));
color_linear const color::MediumOrchid         = Convert<color_linear>(ColorGammaUB(0xBA, 0x55, 0xD3));
color_linear const color::MediumPurple         = Convert<color_linear>(ColorGammaUB(0x93, 0x70, 0xDB));
color_linear const color::MediumSeaGreen       = Convert<color_linear>(ColorGammaUB(0x3C, 0xB3, 0x71));
color_linear const color::MediumSlateBlue      = Convert<color_linear>(ColorGammaUB(0x7B, 0x68, 0xEE));
color_linear const color::MediumSpringGreen    = Convert<color_linear>(ColorGammaUB(0x00, 0xFA, 0x9A));
color_linear const color::MediumTurquoise      = Convert<color_linear>(ColorGammaUB(0x48, 0xD1, 0xCC));
color_linear const color::MediumVioletRed      = Convert<color_linear>(ColorGammaUB(0xC7, 0x15, 0x85));
color_linear const color::MidnightBlue         = Convert<color_linear>(ColorGammaUB(0x19, 0x19, 0x70));
color_linear const color::MintCream            = Convert<color_linear>(ColorGammaUB(0xF5, 0xFF, 0xFA));
color_linear const color::MistyRose            = Convert<color_linear>(ColorGammaUB(0xFF, 0xE4, 0xE1));
color_linear const color::Moccasin             = Convert<color_linear>(ColorGammaUB(0xFF, 0xE4, 0xB5));
color_linear const color::NavajoWhite          = Convert<color_linear>(ColorGammaUB(0xFF, 0xDE, 0xAD));
color_linear const color::Navy                 = Convert<color_linear>(ColorGammaUB(0x00, 0x00, 0x80));
color_linear const color::OldLace              = Convert<color_linear>(ColorGammaUB(0xFD, 0xF5, 0xE6));
color_linear const color::Olive                = Convert<color_linear>(ColorGammaUB(0x80, 0x80, 0x00));
color_linear const color::OliveDrab            = Convert<color_linear>(ColorGammaUB(0x6B, 0x8E, 0x23));
color_linear const color::Orange               = Convert<color_linear>(ColorGammaUB(0xFF, 0xA5, 0x00));
color_linear const color::OrangeRed            = Convert<color_linear>(ColorGammaUB(0xFF, 0x45, 0x00));
color_linear const color::Orchid               = Convert<color_linear>(ColorGammaUB(0xDA, 0x70, 0xD6));
color_linear const color::PaleGoldenRod        = Convert<color_linear>(ColorGammaUB(0xEE, 0xE8, 0xAA));
color_linear const color::PaleGreen            = Convert<color_linear>(ColorGammaUB(0x98, 0xFB, 0x98));
color_linear const color::PaleTurquoise        = Convert<color_linear>(ColorGammaUB(0xAF, 0xEE, 0xEE));
color_linear const color::PaleVioletRed        = Convert<color_linear>(ColorGammaUB(0xDB, 0x70, 0x93));
color_linear const color::PapayaWhip           = Convert<color_linear>(ColorGammaUB(0xFF, 0xEF, 0xD5));
color_linear const color::PeachPuff            = Convert<color_linear>(ColorGammaUB(0xFF, 0xDA, 0xB9));
color_linear const color::Peru                 = Convert<color_linear>(ColorGammaUB(0xCD, 0x85, 0x3F));
color_linear const color::Pink                 = Convert<color_linear>(ColorGammaUB(0xFF, 0xC0, 0xCB));
color_linear const color::Plum                 = Convert<color_linear>(ColorGammaUB(0xDD, 0xA0, 0xDD));
color_linear const color::PowderBlue           = Convert<color_linear>(ColorGammaUB(0xB0, 0xE0, 0xE6));
color_linear const color::Purple               = Convert<color_linear>(ColorGammaUB(0x80, 0x00, 0x80));
color_linear const color::RebeccaPurple        = Convert<color_linear>(ColorGammaUB(0x66, 0x33, 0x99));
color_linear const color::Red                  = Convert<color_linear>(ColorGammaUB(0xFF, 0x00, 0x00));
color_linear const color::RosyBrown            = Convert<color_linear>(ColorGammaUB(0xBC, 0x8F, 0x8F));
color_linear const color::RoyalBlue            = Convert<color_linear>(ColorGammaUB(0x41, 0x69, 0xE1));
color_linear const color::SaddleBrown          = Convert<color_linear>(ColorGammaUB(0x8B, 0x45, 0x13));
color_linear const color::Salmon               = Convert<color_linear>(ColorGammaUB(0xFA, 0x80, 0x72));
color_linear const color::SandyBrown           = Convert<color_linear>(ColorGammaUB(0xF4, 0xA4, 0x60));
color_linear const color::SeaGreen             = Convert<color_linear>(ColorGammaUB(0x2E, 0x8B, 0x57));
color_linear const color::SeaShell             = Convert<color_linear>(ColorGammaUB(0xFF, 0xF5, 0xEE));
color_linear const color::Sienna               = Convert<color_linear>(ColorGammaUB(0xA0, 0x52, 0x2D));
color_linear const color::Silver               = Convert<color_linear>(ColorGammaUB(0xC0, 0xC0, 0xC0));
color_linear const color::SkyBlue              = Convert<color_linear>(ColorGammaUB(0x87, 0xCE, 0xEB));
color_linear const color::SlateBlue            = Convert<color_linear>(ColorGammaUB(0x6A, 0x5A, 0xCD));
color_linear const color::SlateGray            = Convert<color_linear>(ColorGammaUB(0x70, 0x80, 0x90));
color_linear const color::Snow                 = Convert<color_linear>(ColorGammaUB(0xFF, 0xFA, 0xFA));
color_linear const color::SpringGreen          = Convert<color_linear>(ColorGammaUB(0x00, 0xFF, 0x7F));
color_linear const color::SteelBlue            = Convert<color_linear>(ColorGammaUB(0x46, 0x82, 0xB4));
color_linear const color::Tan                  = Convert<color_linear>(ColorGammaUB(0xD2, 0xB4, 0x8C));
color_linear const color::Teal                 = Convert<color_linear>(ColorGammaUB(0x00, 0x80, 0x80));
color_linear const color::Thistle              = Convert<color_linear>(ColorGammaUB(0xD8, 0xBF, 0xD8));
color_linear const color::Tomato               = Convert<color_linear>(ColorGammaUB(0xFF, 0x63, 0x47));
color_linear const color::Turquoise            = Convert<color_linear>(ColorGammaUB(0x40, 0xE0, 0xD0));
color_linear const color::Violet               = Convert<color_linear>(ColorGammaUB(0xEE, 0x82, 0xEE));
color_linear const color::Wheat                = Convert<color_linear>(ColorGammaUB(0xF5, 0xDE, 0xB3));
color_linear const color::White                = Convert<color_linear>(ColorGammaUB(0xFF, 0xFF, 0xFF));
color_linear const color::WhiteSmoke           = Convert<color_linear>(ColorGammaUB(0xF5, 0xF5, 0xF5));
color_linear const color::Yellow               = Convert<color_linear>(ColorGammaUB(0xFF, 0xFF, 0x00));
color_linear const color::YellowGreen          = Convert<color_linear>(ColorGammaUB(0x9A, 0xCD, 0x32));
