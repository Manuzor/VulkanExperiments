#include "Allocator.hpp"

#include <stdlib.h>
#include <stdio.h>

// TODO: %I in printf is MS-specific!
// See here: https://msdn.microsoft.com/en-us/library/tcxf1dw6.aspx

// TODO: Move this somewhere central. Maybe on the commandline?
#define PLATFORM_WINDOWS 1

#if defined(DEBUG)

auto
::CheckedAlignment(size_t Alignment)
  -> size_t
{
  if(Alignment == 0)
    return GlobalDefaultAlignment;

  Assert(IsPowerOfTwo(Alignment));
  return Alignment;
}
#endif

void*
mallocator::Allocate(memory_size Size, size_t Alignment)
{
  Alignment = CheckedAlignment(Alignment);
  #if PLATFORM_WINDOWS
    auto Ptr = _aligned_malloc(ToBytes(Size), Alignment);
  #else
    Assert(!"Not implemented");
    void* Ptr = nullptr;
  #endif

  // printf("mallocator: Allocated 0x%Ix with %.03fKiB (%.03fMiB)\n", Reinterpret<size_t>(Ptr), ToKiB(Size), ToMiB(Size));
  return Ptr;
}

void
mallocator::Deallocate(void* Memory)
{
  // printf("mallocator: Deallocating 0x%Ix\n", Reinterpret<size_t>(Memory));

  #if PLATFORM_WINDOWS
    _aligned_free(Memory);
  #else
    Assert(!"Not implemented");
  #endif
}

bool
mallocator::Resize(void* Ptr, memory_size NewSize)
{
  #if PLATFORM_WINDOWS
    return _expand(Ptr, ToBytes(NewSize)) != nullptr;
  #else
    Assert(0);
    return false;
  #endif
}

memory_size
mallocator::AllocationSize(void* Ptr)
{
  #if PLATFORM_WINDOWS
    // TODO: Test if this is valid for stuff allocated with _aligned_malloc.
    return Bytes(_msize(Ptr));
  #else
    Assert(!"Not implemented");
    return Bytes(0);
  #endif
}


static allocator_interface*
GetGlobalTempAllocator()
{
  static mallocator GlobalTempAllocator{};
  return &GlobalTempAllocator;
}

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
  return GetGlobalTempAllocator()->Allocate(Size, Alignment);
}

void
temp_allocator::Deallocate(void* Ptr)
{
  GetGlobalTempAllocator()->Deallocate(Ptr);
}

bool
temp_allocator::Resize(void* Ptr, memory_size NewSize)
{
  return GetGlobalTempAllocator()->Resize(Ptr, NewSize);
}
