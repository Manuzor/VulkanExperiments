#include "TestHeader.hpp"
#include <Core/Event.hpp>

TEST_CASE("Event", "[Event]")
{
  int CallCount = 0;
  int Value = 0;

  test_allocator _Allocator;
  allocator_interface* Allocator = &_Allocator;

  event<int> IntEvent = {};
  Init(&IntEvent, Allocator);
  Defer [&](){ Finalize(&IntEvent); };

  auto Listener = [&](int Val){ Value += Val; ++CallCount; };
  auto Id1 = AddListener(&IntEvent, Listener);

  IntEvent(42);
  REQUIRE( CallCount == 1 );
  REQUIRE( Value == 42 );

  IntEvent(42);
  REQUIRE( CallCount == 2 );
  REQUIRE( Value == 84 );

  REQUIRE( RemoveListener(&IntEvent, Id1) );

  IntEvent(42);
  REQUIRE( CallCount == 2 );
  REQUIRE( Value == 84 );

  auto Id2 = AddListener(&IntEvent, Listener);
  auto Id3 = AddListener(&IntEvent, Listener);

  IntEvent(1);
  REQUIRE( CallCount == 4 );
  REQUIRE( Value == 86 );

  REQUIRE( RemoveListener(&IntEvent, Id2) );
  REQUIRE( RemoveListener(&IntEvent, Id3) );
  REQUIRE( !RemoveListener(&IntEvent, Id3) );
}
