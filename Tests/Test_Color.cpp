#include "TestHeader.hpp"
#include <Core/Color.hpp>

TEST_CASE("Color Construction", "[Color]")
{
  SECTION("Gamma Unsigned Byte")
  {
    auto Color = ColorGammaUB(128, 128, 64);

    REQUIRE(Color.R == 128);
    REQUIRE(Color.G == 128);
    REQUIRE(Color.B ==  64);
    REQUIRE(Color.A == 255);
  }

  SECTION("Linear Unsigned Byte")
  {
    auto Color = ColorLinearUB(128, 128, 64);

    REQUIRE(Color.R == 128);
    REQUIRE(Color.G == 128);
    REQUIRE(Color.B ==  64);
    REQUIRE(Color.A == 255);
  }

  SECTION("Linear Float")
  {
    auto Color = ColorLinear(0.5f, 0.5f, 0.2f);

    REQUIRE(Color.R == 0.5f);
    REQUIRE(Color.G == 0.5f);
    REQUIRE(Color.B == 0.2f);
    REQUIRE(Color.A == 1.0f);
  }
}

TEST_CASE("Color Conversion", "[Color]")
{
  SECTION("Gamma UB => Linear Float")
  {
    auto const C1 = ColorGammaUB(0x64, 0x95, 0xED); // Cornflower Blue
    auto const C2 = Convert<color_linear>(C1);

    REQUIRE( AreNearlyEqual(C2.R, 0.12744f) );
    REQUIRE( AreNearlyEqual(C2.G, 0.30054f) );
    REQUIRE( AreNearlyEqual(C2.B, 0.84687f) );
    REQUIRE( C2.A == 1.0f );
  }
}
