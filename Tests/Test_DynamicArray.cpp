#include "TestHeader.hpp"
#include <Core/DynamicArray.hpp>


TEST_CASE("Dynamic Array Basics", "[dynamic_array]")
{
  test_allocator _Allocator;
  allocator_interface* Allocator = &_Allocator;

  dynamic_array<int> Arr = {};
  Init(&Arr, Allocator);
  Defer [&](){ Finalize(&Arr); };

  REQUIRE( Arr.Num == 0 );

  SECTION("Reserve")
  {
    Reserve(&Arr, 10);
    REQUIRE( Arr.Num == 0 );
    REQUIRE( Arr.Capacity >= 10 );
    REQUIRE( Arr.Ptr != nullptr );
  }

  SECTION("PushBack")
  {
    Expand(&Arr) = 42;
    REQUIRE( Arr.Num == 1 );
    REQUIRE( Arr[0] == 42 );
  }
}

TEST_CASE("Dynamic Array Reserve", "[dynamic_array]")
{
  test_allocator _Allocator;
  allocator_interface* Allocator = &_Allocator;

  dynamic_array<int> Arr{};
  Defer [&](){ Finalize(&Arr); };

  SECTION("Reserve without custom allocator")
  {
    Reserve(&Arr, 10);
    REQUIRE( Arr.Num == 0 );
    REQUIRE( Arr.Capacity >= 10 );
    REQUIRE( Arr.Ptr != nullptr );
    REQUIRE( Arr.Allocator == DynamicArrayDefaultAllocator() );
  }

  SECTION("Reserve with custom allocator")
  {
    Init(&Arr, Allocator);
    Reserve(&Arr, 10);
    REQUIRE( Arr.Num == 0 );
    REQUIRE( Arr.Capacity >= 10 );
    REQUIRE( Arr.Ptr != nullptr );
  }
}

TEST_CASE("Dynamic Array Expand", "[dynamic_array]")
{
  test_allocator _Allocator;
  allocator_interface* Allocator = &_Allocator;

  dynamic_array<int> Arr = {};
  Init(&Arr, Allocator);
  Defer [&](){ Finalize(&Arr); };

  SECTION("Expand")
  {
    Expand(&Arr) = 42;
    REQUIRE( Arr.Num == 1 );
    REQUIRE( Arr[0] == 42 );
  }

  SECTION("ExpandBy one")
  {
    ExpandBy(&Arr, 1)[0] = 42;
    REQUIRE( Arr.Num == 1 );
    REQUIRE( Arr[0] == 42 );
  }

  SECTION("ExpandBy many")
  {
    auto Result = ExpandBy(&Arr, 3);
    REQUIRE( Result.Num == 3 );
    Result[0] = 42;
    REQUIRE( Arr.Num == 3 );
    REQUIRE( Arr[0] == 42 );
    REQUIRE( Arr[1] == 0 );
    REQUIRE( Arr[2] == 0 );

    int Ints[] = { 0, 1, 2, 3, 4, 5 };
    SliceCopy(ExpandBy(&Arr, 6), AsConst(Slice(Ints)));
    REQUIRE( Arr.Num == 9 );
    REQUIRE( Arr[0] == 42 );
    REQUIRE( Arr[1] == 0 );
    REQUIRE( Arr[2] == 0 );
    REQUIRE( Arr[3] == 0 );
    REQUIRE( Arr[4] == 1 );
    REQUIRE( Arr[5] == 2 );
    REQUIRE( Arr[6] == 3 );
    REQUIRE( Arr[7] == 4 );
    REQUIRE( Arr[8] == 5 );
  }

  SECTION("SetNum")
  {
    SetNum(&Arr, 3);
    REQUIRE( Arr.Num == 3);

    ExpandBy(&Arr, 1);
    REQUIRE( Arr.Num == 4);

    SetNum(&Arr, 3);
    REQUIRE( Arr.Num == 3);
  }
}

TEST_CASE("Dynamic Array Remove", "[dynamic_array]")
{
  test_allocator _Allocator;
  allocator_interface* Allocator = &_Allocator;

  dynamic_array<int> Arr = {};
  Init(&Arr, Allocator);
  Defer [&](){ Finalize(&Arr); };

  int Ints[] = { 0, 1, 2, 3 };
  SliceCopy(ExpandBy(&Arr, 4), AsConst(Slice(Ints)));
  REQUIRE( Arr.Num == 4 );

  SECTION("RemoveAt")
  {
    RemoveAt(&Arr, 1);
    REQUIRE( Arr.Num == 3 );
    REQUIRE( Arr[0] == 0 );
    REQUIRE( Arr[1] == 2 );
    REQUIRE( Arr[2] == 3 );
  }

  SECTION("RemoveFirst")
  {
    REQUIRE(RemoveFirst(&Arr, 1));
    REQUIRE( Arr.Num == 3 );
    REQUIRE( Arr[0] == 0 );
    REQUIRE( Arr[1] == 2 );
    REQUIRE( Arr[2] == 3 );

    REQUIRE(!RemoveFirst(&Arr, 1));
    REQUIRE( Arr.Num == 3 );
    REQUIRE( Arr[0] == 0 );
    REQUIRE( Arr[1] == 2 );
    REQUIRE( Arr[2] == 3 );
  }
}
