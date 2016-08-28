#include "Allocator.hpp"

#include <stdlib.h>
#include <stdio.h>

// TODO: %I in printf is MS-specific!
// See here: https://msdn.microsoft.com/en-us/library/tcxf1dw6.aspx

void*
mallocator::Allocate(memory_size Size, size_t Alignment)
{
  auto Ptr = ::malloc(Size);
  printf("mallocator: Allocated %Ix with %.03fKiB (%.03fMiB)\n",
         Reinterpret<size_t>(Ptr), ToKiB(Size), ToMiB(Size));
  return Ptr;
}

bool
mallocator::Deallocate(void* Memory)
{
  printf("mallocator: Deallocating %Ix\n", Reinterpret<size_t>(Memory));
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
temp_allocator::Allocate(memory_size Size, size_t Alignment)
{
  return GlobalTempAllocator.Allocate(Size, Alignment);
}

bool
temp_allocator::Deallocate(void* Memory)
{
  return GlobalTempAllocator.Deallocate(Memory);
}
