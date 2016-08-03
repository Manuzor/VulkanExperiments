#include "TestHeader.hpp"
#include <Cfg/Cfg.hpp>
#include <Cfg/CfgParser.hpp>

#include <Core/Log.hpp>


static cfg_source
MakeSourceDataForTesting(char const* String, size_t Offset)
{
  cfg_source Source;
  Source.Value = SliceFromString(String);
  Source.StartLocation = { 1, 1, Offset };
  Source.EndLocation   = { 0, 0, Source.Value.Num };

  return Source;
}

TEST_CASE("Cfg: Skip whitespace and comments", "[Cfg]")
{
  cfg_parsing_context Context{ SliceFromString("Cfg Test 1"), GlobalLog };

  SECTION("Simple")
  {
    auto Source = MakeSourceDataForTesting("// hello\nworld", 0);

    auto Result = CfgSourceSkipWhiteSpaceAndComments(&Source, &Context, cfg_consume_newline::Yes);
    REQUIRE( Result.StartLocation.SourceIndex == 0 );
    REQUIRE( Result.EndLocation.SourceIndex == 9 );
    REQUIRE( Source.StartLocation.SourceIndex == 9 );
    REQUIRE( Source.EndLocation.SourceIndex == Source.Value.Num );
  }

  SECTION("Many Different Comment Styles")
  {
    auto Source = MakeSourceDataForTesting(
      "// C++ style\n\n"
      "/*\n"
      "C style multiline\n"
      "*/\n\n"
      "/*foo=true*/\n\n"
      "# Shell style\n\n"
      "-- Lua style\n\n"
      "text",
      0);

    auto Result = CfgSourceSkipWhiteSpaceAndComments(&Source, &Context, cfg_consume_newline::Yes);
    REQUIRE( Result.StartLocation.SourceIndex == 0 );
    REQUIRE( Result.EndLocation.SourceIndex == 82 );
    REQUIRE( Source.StartLocation.SourceIndex == 82 );
    REQUIRE( Source.EndLocation.SourceIndex == Source.Value.Num );
    REQUIRE( CfgSourceCurrentValue(Source) == SliceFromString("text") );
  }
}

TEST_CASE("Cfg: Parse until", "[Cfg]")
{
  cfg_parsing_context Context{ SliceFromString("Cfg Test 2"), GlobalLog };

}
