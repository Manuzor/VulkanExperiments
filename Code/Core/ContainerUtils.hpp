#pragma once

#include "CoreAPI.hpp"
#include "Allocator.hpp"


/// \return Slice containing the new Ptr and Capacity.
template<typename T>
slice<T>
ContainerReserve(allocator_interface& Allocator,
                 T* Ptr,
                 size_t CurrentNum,
                 size_t CurrentCapacity,
                 size_t MinNumToReserve,
                 size_t MinimumCapacity)
{
  // TODO: Replace with something like NextMultipleOf(Max(MinNumToReserve, MinimumCapacity), MinimumCapacity)
  size_t NumToReserve = MinimumCapacity;
  while(NumToReserve < MinNumToReserve)
    NumToReserve *= 2;

  if(CurrentCapacity >= NumToReserve)
    return Slice(CurrentCapacity, Ptr);

  if(Allocator.Resize(Ptr, NumToReserve * SizeOf<T>()))
  {
    return Slice(NumToReserve, Ptr);
  }

  auto OldAllocatedMemory = Slice(CurrentCapacity, Ptr);
  auto OldUsedMemory      = Slice(CurrentNum, Ptr);
  auto NewAllocatedMemory = SliceAllocate<T>(Allocator, NumToReserve);
  auto NewUsedMemory      = Slice(NewAllocatedMemory, 0, CurrentNum);

  Assert(NewAllocatedMemory.Num == NumToReserve);

  if(OldUsedMemory)
  {
    // Move from the old memory.
    SliceMoveConstruct(NewUsedMemory, OldUsedMemory);

    // Destruct and free the old memory.
    SliceDestruct(OldUsedMemory);
    SliceDeallocate(Allocator, OldAllocatedMemory);
  }

  return NewAllocatedMemory;
}

/// \return The updated slice.
template<typename T>
slice<T>
ContainerRemoveAt(slice<T> Data, size_t Index, size_t CountToRemove = 1)
{
  if(CountToRemove == 0)
    return {};

  BoundsCheck(Index + CountToRemove <= Data.Num);

  // Move the entire remaining chunk in the back forward to remove the requests data.
  // SliceMove will take care of destructing the leftovers.
  auto DataToMove = Slice(Data, Index + CountToRemove, Data.Num);
  auto DataToOverwrite = Slice(Data, Index, Data.Num - CountToRemove);
  SliceMove(DataToOverwrite, DataToMove);

  Data.Num -= CountToRemove;

  return Data;
}
