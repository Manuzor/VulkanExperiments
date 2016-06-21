#pragma once

#include "Allocator.hpp"

#include <Backbone.hpp>


constexpr size_t DynamicArrayMinimumCapacity = 16;

template<typename T>
struct dynamic_array
{
  allocator_interface* Allocator;
  size_t Capacity;
  size_t Num;
  T* Ptr;

  operator bool() const { return Num && Ptr; }

  /// Index operator to access elements of the slice.
  template<typename T>
  auto
  operator[](T Index) const
    -> decltype(Ptr[Index])
  {
    BoundsCheck(Index >= 0 && Index < Num);
    return Ptr[Index];
  }
};

template<>
struct dynamic_array<void>
{
  allocator_interface* Allocator;
  size_t Capacity;
  size_t Num;
  void* Ptr;

  operator bool() const { return Num && Ptr; }
};

template<>
struct dynamic_array<void const>
{
  allocator_interface* Allocator;
  size_t Capacity;
  size_t Num;
  void const* Ptr;

  operator bool() const { return Num && Ptr; }
};

template<typename T>
slice<T>
Slice(dynamic_array<T>* Array)
{
  return Slice(Array->Num, Array->Ptr);
}

template<typename T>
slice<T>
AllocatedMemory(dynamic_array<T>* Array)
{
  return Slice(Array->Capacity, Array->Ptr);
}

template<typename T>
void
Init(dynamic_array<T>* Array, allocator_interface* Allocator)
{
  Array->Allocator = Allocator;
}

template<typename T>
void
Finalize(dynamic_array<T>* Array)
{
  SliceDeallocate(Array->Allocator, AllocatedMemory(Array));
}

template<typename T>
void
Reserve(dynamic_array<T>* Array, size_t MinBytesToReserve)
{
  auto BytesToReserve = Max(MinBytesToReserve, DynamicArrayMinimumCapacity);
  if(Array->Capacity >= BytesToReserve)
    return;

  auto OldAllocatedMemory = AllocatedMemory(Array);
  auto OldUsedMemory = Slice(Array);
  auto NewAllocatedMemory = SliceAllocate<T>(Array->Allocator, BytesToReserve);
  auto NewUsedMemory = Slice(NewAllocatedMemory, 0, Array->Num);

  Assert(NewAllocatedMemory.Num == BytesToReserve);

  // Move from the old memory.
  SliceMove(NewUsedMemory, SliceAsConst(OldUsedMemory));

  // Destruct and free the old memory.
  SliceDestruct(OldUsedMemory);
  SliceDeallocate(Array->Allocator, OldAllocatedMemory);

  // Update array members.
  Array->Ptr = NewAllocatedMemory.Ptr;
  Array->Capacity = NewAllocatedMemory.Num;
}

template<typename T>
slice<T>
ExpandBy(dynamic_array<T>* Array, size_t Amount)
{
  Reserve(Array, Array->Num + Amount);
  const auto BeginIndex = Array->Num;
  Array->Num += Amount;
  const auto EndIndex = Array->Num;
  auto Result = Slice(AllocatedMemory(Array), BeginIndex, EndIndex);
  SliceDefaultConstruct(Result);
  return Result;
}

template<typename T>
T&
Expand(dynamic_array<T>* Array)
{
  return ExpandBy(Array, 1)[0];
}

template<typename T>
void
ShrinkBy(dynamic_array<T>* Array, size_t Amount)
{
  Assert(Array->Num >= Amount);
  auto ToDestruct = Slice(Slice(Array), Array->Num - Amount, Array->Num);
  SliceDestruct(ToDestruct);
  Array->Num -= Amount;
}

template<typename T>
void
Clear(dynamic_array<T>* Array)
{
  ShrinkBy(Array, Array->Num);
}

template<typename T>
void
RemoveAt(dynamic_array<T>* Array, size_t Index, size_t CountToRemove = 1)
{
  BoundsCheck(CountToRemove >= 0);
  BoundsCheck(Index >= 0);
  BoundsCheck(Index + CountToRemove <= Array->Num);

  const size_t EndIndex = Index + CountToRemove;
  auto Hole = Slice(Slice(Array), Index, EndIndex);
  SliceDestruct(Hole);

  auto From = Slice(Slice(Array), EndIndex, Array->Num);
  auto To   = Slice(Slice(Array), Index, Array->Num - CountToRemove);
  SliceMove(To, SliceAsConst(From));
  Array->Num -= CountToRemove;
}

template<typename T>
bool
RemoveFirst(dynamic_array<T>* Array, const T& Needle)
{
  auto Index = SliceCountUntil(Slice(Array), Needle);
  if(Index == INVALID_INDEX)
    return false;

  RemoveAt(Array, Index);
  return true;
}
