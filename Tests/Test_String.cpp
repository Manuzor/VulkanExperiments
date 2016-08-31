// Note: Core/Time.hpp needs to be included before catch.hpp for some reason.
// probably because of the word "time".
#include <Core/Time.hpp>

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

static void
StringBenchmark(arc_string const& String)
{
  for(size_t Iteration = 0; Iteration < 1000000; ++Iteration)
  {
    auto Copy = String;
    Copy += "Appended";
  }

  for(size_t Iteration = 0; Iteration < 100000; ++Iteration)
  {
    auto Concatenated = String + "Concat";
  }
}

TEST_CASE("String Benchmark", "[String][Benchmark]")
{
  SECTION("Default string allocator.")
  {
    arc_string String;

    stopwatch Stopwatch;
    StopwatchStart(&Stopwatch);
    StringBenchmark(String);
    StopwatchStop(&Stopwatch);
    printf("Default string allocator: %f", TimeAsSeconds(StopwatchTime(&Stopwatch)));
  }

  SECTION("mallocator")
  {
    mallocator Allocator;
    arc_string String{ Allocator };

    stopwatch Stopwatch;
    StopwatchStart(&Stopwatch);
    StringBenchmark(String);
    StopwatchStop(&Stopwatch);
    printf("mallocator: %f", TimeAsSeconds(StopwatchTime(&Stopwatch)));
  }
}
