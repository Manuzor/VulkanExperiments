#include "TestHeader.hpp"
#include <Core/String.hpp>

TEST_CASE("String", "[String]")
{
  arc_string Foo;

  REQUIRE( StrIsEmpty(Foo) );

  SECTION("Append")
  {
    StrAppend(Foo, "Foo"_S);
    REQUIRE( Slice(Foo) == "Foo"_S );

    SECTION("Clear")
    {
      StrClear(Foo);
      REQUIRE( StrIsEmpty(Foo) );
    }

    SECTION("Modify after copy")
    {
      auto Bar = Foo;
      Bar += "bar";
      REQUIRE( Slice(Foo) == "Foo"_S );
      REQUIRE( Slice(Bar) == "Foobar"_S );

      Bar = "Bar!";
      REQUIRE( Slice(Bar) == "Bar!"_S );
    }
  }

  SECTION("Simple Set")
  {
    StrSet(Foo, "Bar"_S);
    REQUIRE( Slice(Foo) == "Bar"_S );

    Foo = "Baz";
    REQUIRE( Slice(Foo) == "Baz"_S );
  }

  SECTION("Concat")
  {
    Foo = "Foo";
    auto Foobar = Foo + "bar";
    REQUIRE( Slice(Foo) == "Foo"_S );
    REQUIRE( Slice(Foobar) == "Foobar"_S );

    Foobar = "zzz";
    REQUIRE( Slice(Foo) == "Foo"_S );
    REQUIRE( Slice(Foobar) == "zzz"_S );
  }
}
