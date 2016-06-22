#pragma once

#include "Allocator.hpp"


/// \return Slice containing the new Ptr and Capacity.
template<typename T>
slice<T>
ContainerReserve(allocator_interface* Allocator,
                 T* Ptr,
                 size_t CurrentNum,
                 size_t CurrentCapacity,
                 size_t MinBytesToReserve,
                 size_t MinimumCapacity)
{
  auto BytesToReserve = Max(MinBytesToReserve, MinimumCapacity);
  if(CurrentCapacity >= BytesToReserve)
    return Slice(CurrentCapacity, Ptr);

  auto OldAllocatedMemory = Slice(CurrentCapacity, Ptr);
  auto OldUsedMemory      = Slice(CurrentNum, Ptr);
  auto NewAllocatedMemory = SliceAllocate<T>(Allocator, BytesToReserve);
  auto NewUsedMemory      = Slice(NewAllocatedMemory, 0, CurrentNum);

  Assert(NewAllocatedMemory.Num == BytesToReserve);

  // Move from the old memory.
  SliceMove(NewUsedMemory, AsConst(OldUsedMemory));

  // Destruct and free the old memory.
  SliceDestruct(OldUsedMemory);
  SliceDeallocate(Allocator, OldAllocatedMemory);

  return NewAllocatedMemory;
}

/// \return The updated slice (for convenience).
template<typename T>
slice<T>
ContainerRemoveAt(slice<T> Data, size_t Index, size_t CountToRemove = 1)
{
  BoundsCheck(CountToRemove >= 0);
  BoundsCheck(Index >= 0);
  BoundsCheck(Index + CountToRemove <= Data.Num);

  const size_t EndIndex = Index + CountToRemove;
  auto Hole = Slice(Data, Index, EndIndex);
  SliceDestruct(Hole);

  auto From = Slice(Data, EndIndex, Data.Num);
  auto To   = Slice(Data, Index, Data.Num - CountToRemove);
  SliceMove(To, AsConst(From));
  Data.Num -= CountToRemove;

  return Data;
}
