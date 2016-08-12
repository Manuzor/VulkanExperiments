#include "DynamicArray.hpp"

static allocator_interface* GlobalDynamicArrayDefaultAllocator{};

auto
::DynamicArrayDefaultAllocator()
  -> allocator_interface*
{
  if(GlobalDynamicArrayDefaultAllocator == nullptr)
  {
    static mallocator Mallocator{};
    GlobalDynamicArrayDefaultAllocator = &Mallocator;
  }

  return GlobalDynamicArrayDefaultAllocator;
}

auto
::DynamicArraySetDefaultAllocator(allocator_interface* Allocator)
  -> void
{
  GlobalDynamicArrayDefaultAllocator = Allocator;
}
