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


static mallocator GlobalTempAllocator = {};

temp_allocator::temp_allocator()
{
  // Nothing to do here yet.
}

temp_allocator::~temp_allocator()
{
  // Nothing to do here yet.
}

void*
temp_allocator::Allocate(size_t NumBytes, size_t Alignment)
{
  return GlobalTempAllocator.Allocate(NumBytes, Alignment);
}

bool
temp_allocator::Deallocate(void* Memory)
{
  return GlobalTempAllocator.Deallocate(Memory);
}
