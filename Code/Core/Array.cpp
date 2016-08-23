#include "Array.hpp"

static allocator_interface* GlobalArrayDefaultAllocator{};

auto
::ArrayDefaultAllocator()
  -> allocator_interface*
{
  if(GlobalArrayDefaultAllocator == nullptr)
  {
    static mallocator Mallocator{};
    GlobalArrayDefaultAllocator = &Mallocator;
  }

  return GlobalArrayDefaultAllocator;
}

auto
::ArraySetDefaultAllocator(allocator_interface* Allocator)
  -> void
{
  GlobalArrayDefaultAllocator = Allocator;
}
