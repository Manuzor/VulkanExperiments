#include "TestHeader.hpp"
#include <Core/SharedArray.hpp>


TEST_CASE("Shared Array Basics", "[shared_array]")
{
  test_allocator _Allocator;
  allocator_interface* Allocator = &_Allocator;

  shared_array<int> Arr{};
  Arr.Allocator = Allocator;

  REQUIRE( Arr.Num() == 0 );

  SECTION("Reserve")
  {
    Reserve(Arr, 10);
    REQUIRE( Arr.Num() == 0 );
    REQUIRE( Arr.Capacity() >= 10 );
    REQUIRE( Arr.Ptr() != nullptr );
  }

  SECTION("PushBack")
  {
    Expand(Arr) = 42;
    REQUIRE( Arr.Num() == 1 );
    REQUIRE( Arr[0] == 42 );
  }
}

TEST_CASE("Shared Array Reserve", "[shared_array]")
{
  test_allocator _Allocator;
  allocator_interface* Allocator = &_Allocator;

  shared_array<int> Arr{};

  SECTION("Reserve without custom allocator")
  {
    Reserve(Arr, 10);
    REQUIRE( Arr.Num() == 0 );
    REQUIRE( Arr.Capacity() >= 10 );
    REQUIRE( Arr.Ptr() != nullptr );
    REQUIRE( Arr.Allocator == SharedArrayDefaultAllocator() );
  }

  SECTION("Reserve with custom allocator")
  {
    Arr.Allocator = Allocator;
    Reserve(Arr, 10);
    REQUIRE( Arr.Num() == 0 );
    REQUIRE( Arr.Capacity() >= 10 );
    REQUIRE( Arr.Ptr() != nullptr );
  }
}

TEST_CASE("Shared Array Expand", "[shared_array]")
{
  test_allocator _Allocator;
  allocator_interface* Allocator = &_Allocator;

  shared_array<int> Arr{};
  Arr.Allocator = Allocator;

  SECTION("Expand")
  {
    Expand(Arr) = 42;
    REQUIRE( Arr.Num() == 1 );
    REQUIRE( Arr[0] == 42 );
  }

  SECTION("ExpandBy one")
  {
    ExpandBy(Arr, 1)[0] = 42;
    REQUIRE( Arr.Num() == 1 );
    REQUIRE( Arr[0] == 42 );
  }

  SECTION("ExpandBy many")
  {
    auto Result = ExpandBy(Arr, 3);
    REQUIRE( Result.Num == 3 );
    Result[0] = 42;
    REQUIRE( Arr.Num() == 3 );
    REQUIRE( Arr[0] == 42 );
    REQUIRE( Arr[1] == 0 );
    REQUIRE( Arr[2] == 0 );

    int Ints[] = { 0, 1, 2, 3, 4, 5 };
    SliceCopy(ExpandBy(Arr, 6), AsConst(Slice(Ints)));
    REQUIRE( Arr.Num() == 9 );
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
    SetNum(Arr, 3);
    REQUIRE( Arr.Num() == 3);

    ExpandBy(Arr, 1);
    REQUIRE( Arr.Num() == 4);

    SetNum(Arr, 3);
    REQUIRE( Arr.Num() == 3);
  }
}

TEST_CASE("Shared Array Remove", "[shared_array]")
{
  test_allocator _Allocator;
  allocator_interface* Allocator = &_Allocator;

  shared_array<int> Arr{};
  Arr.Allocator = Allocator;

  int Ints[] = { 0, 1, 2, 3 };
  SliceCopy(ExpandBy(Arr, 4), AsConst(Slice(Ints)));
  REQUIRE( Arr.Num() == 4 );

  SECTION("RemoveAt")
  {
    RemoveAt(Arr, 1);
    REQUIRE( Arr.Num() == 3 );
    REQUIRE( Arr[0] == 0 );
    REQUIRE( Arr[1] == 2 );
    REQUIRE( Arr[2] == 3 );
  }

  SECTION("RemoveFirst")
  {
    REQUIRE(RemoveFirst(Arr, 1));
    REQUIRE( Arr.Num() == 3 );
    REQUIRE( Arr[0] == 0 );
    REQUIRE( Arr[1] == 2 );
    REQUIRE( Arr[2] == 3 );

    REQUIRE(!RemoveFirst(Arr, 1));
    REQUIRE( Arr.Num() == 3 );
    REQUIRE( Arr[0] == 0 );
    REQUIRE( Arr[1] == 2 );
    REQUIRE( Arr[2] == 3 );
  }
}

TEST_CASE("Shared Array Copy-on-Write", "[shared_array]")
{
  shared_array<int> Foo{};

  Expand(Foo) = 42;
  Expand(Foo) = 1337;

  shared_array<int> Bar{ Foo };
  REQUIRE( Bar.Ptr() == Foo.Ptr() );
  REQUIRE( Bar[0] == 42 );
  REQUIRE( Bar[1] == 1337 );
  REQUIRE( Foo[0] == 42 );
  REQUIRE( Foo[1] == 1337 );

  Bar[0] = 123;
  REQUIRE( Bar.Ptr() != Foo.Ptr() );
  REQUIRE( Bar[0] == 123 );
  REQUIRE( Bar[1] == 1337 );
  REQUIRE( Foo[0] == 42 );
  REQUIRE( Foo[1] == 1337 );

  Reset(Foo);
  REQUIRE( Bar[0] == 123 );
  REQUIRE( Bar[1] == 1337 );
}
