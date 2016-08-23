#include "SharedArray.hpp"

static allocator_interface* GlobalSharedArrayDefaultAllocator{};

auto
::SharedArrayDefaultAllocator()
  -> allocator_interface*
{
  if(GlobalSharedArrayDefaultAllocator == nullptr)
  {
    static mallocator Mallocator{};
    GlobalSharedArrayDefaultAllocator = &Mallocator;
  }

  return GlobalSharedArrayDefaultAllocator;
}

auto
::SharedArraySetDefaultAllocator(allocator_interface* Allocator)
  -> void
{
  GlobalSharedArrayDefaultAllocator = Allocator;
}
