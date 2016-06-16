#include <Backbone.hpp>
#include <VulkanExperiments/DynamicArray.hpp>

#include "catch.hpp"

TEST_CASE("Dynamic Array", "[dynamic_array]")
{
  mallocator Mallocator;
  allocator_interface* Allocator = &Mallocator;

  dynamic_array<int> Arr;
  ArrayInit(&Arr, Allocator);

  REQUIRE( Arr.Num == 0 );

  SECTION("Reserve")
  {
    ArrayReserve(&Arr, 10);
    REQUIRE( Arr.Num == 0 );
    REQUIRE( Arr.RawData.Num >= 10 );
    REQUIRE( Arr.RawData.Data != nullptr );
  }

  SECTION("PushBack")
  {
    ArrayPushBack(&Arr, 42);
    REQUIRE( Arr.Num == 1 );
    REQUIRE( ArrayData(&Arr)[0] == 42 );
  }
}
