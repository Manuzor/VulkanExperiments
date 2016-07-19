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
  this->Impl = &GlobalTempAllocator;
}

temp_allocator::~temp_allocator()
{
  this->Impl = nullptr;
}

allocator_interface*
temp_allocator::operator *()
{
  Assert(this->Impl);
  return Reinterpret<allocator_interface*>(this->Impl);
}
