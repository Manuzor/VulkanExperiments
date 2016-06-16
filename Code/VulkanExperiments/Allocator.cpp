#include "Allocator.hpp"

#include <stdlib.h>

slice<void>
mallocator::Allocate(size_t NumBytes, size_t Alignment)
{
  auto Ptr = ::malloc(NumBytes);
  return CreateSlice(NumBytes, Ptr);
}

bool
mallocator::Deallocate(slice<void> Memory)
{
  ::free(Memory.Data);
  return true;
}
