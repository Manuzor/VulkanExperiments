#include "TestHeader.hpp"
#include <VulkanExperiments/Event.hpp>

TEST_CASE("Event", "[Event]")
{
  int CallCount = 0;
  int Value = 0;

  test_allocator _Allocator;
  allocator_interface* Allocator = &_Allocator;

  Event<int> IntEvent;
  Init(&IntEvent, Allocator);
  Defer(&, Finalize(&IntEvent));

  auto Listener = [&](int Val){ Value += Val; ++CallCount; };
  AddListener(&IntEvent, Listener);

  IntEvent(42);
  REQUIRE( CallCount == 1 );
  REQUIRE( Value == 42 );

  IntEvent(42);
  REQUIRE( CallCount == 2 );
  REQUIRE( Value == 84 );

  REQUIRE( RemoveListener(&IntEvent, Listener) );

  IntEvent(42);
  REQUIRE( CallCount == 2 );
  REQUIRE( Value == 84 );

  AddListener(&IntEvent, Listener);
  AddListener(&IntEvent, Listener);

  IntEvent(1);
  REQUIRE( CallCount == 4 );
  REQUIRE( Value == 86 );

  REQUIRE( RemoveListener(&IntEvent, Listener) );
  REQUIRE( RemoveListener(&IntEvent, Listener) );
  REQUIRE( !RemoveListener(&IntEvent, Listener) );
}
