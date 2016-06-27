#include "Allocator.hpp"

#include <stdlib.h>

void*
mallocator::Allocate(size_t NumBytes, size_t Alignment)
{
  auto Ptr = ::malloc(NumBytes);
  return Ptr;
}

bool
mallocator::Deallocate(void* Memory)
{
  ::free(Memory);
  return true;
}
