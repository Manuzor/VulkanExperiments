#pragma once

#include "CoreAPI.hpp"
#include "Allocator.hpp"
#include "ContainerUtils.hpp"

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
Clear(dynamic_array<T>* Array)
{
  if(Array->Num)
  {
    ShrinkBy(Array, Array->Num);
  }
}

template<typename T>
void
Finalize(dynamic_array<T>* Array)
{
  Clear(Array);
  if(Array->Capacity > 0)
  {
    Assert(Array->Allocator);
    Array->Allocator->Deallocate(Array->Ptr);
    Array->Capacity = 0;
  }
}

template<typename T>
void
Reserve(dynamic_array<T>* Array, size_t MinBytesToReserve)
{
  auto NewAllocatedMemory = ContainerReserve(Array->Allocator,
                                             Array->Ptr, Array->Num,
                                             Array->Capacity,
                                             MinBytesToReserve,
                                             DynamicArrayMinimumCapacity);

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
RemoveAt(dynamic_array<T>* Array, size_t Index, size_t CountToRemove = 1)
{
  auto NewData = ContainerRemoveAt(Slice(Array), Index, CountToRemove);
  Assert(NewData.Num == Array->Num - CountToRemove);
  Array->Num = NewData.Num;
}

template<typename T>
bool
RemoveFirst(dynamic_array<T>* Array, const T& Needle)
{
  auto Index = SliceCountUntil(AsConst(Slice(Array)), Needle);
  if(Index == INVALID_INDEX)
    return false;

  RemoveAt(Array, Index);
  return true;
}

template<typename T>
struct scoped_array : public dynamic_array<T>
{
  scoped_array() = default;
  scoped_array(allocator_interface* NewAllocator) { Init(NewAllocator); }
  ~scoped_array() { Finalize(); }

  void Init(allocator_interface* NewAllocator)
  {
    *this = {};
    ::Init(this, NewAllocator);
  }

  void Finalize()
  {
    ::Finalize(this);
  }
};
