#include <Backbone.hpp>
#include <VulkanExperiments/DynamicArray.hpp>

#include "catch.hpp"

TEST_CASE("Dynamic Array", "[dynamic_array]")
{
  mallocator Mallocator;
  allocator_interface* Allocator = &Mallocator;

  dynamic_array<int> Arr = {};
  Init(&Arr, Allocator);
  Defer(&, Finalize(&Arr));

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
