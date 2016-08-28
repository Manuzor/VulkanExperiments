#pragma once

#include "CoreAPI.hpp"
#include "Allocator.hpp"
#include "ContainerUtils.hpp"

#include <Backbone.hpp>


// Measured in number of elements.
#if !defined(ARRAY_MINIMUM_CAPACITY)
  #define ARRAY_MINIMUM_CAPACITY 32
#endif

CORE_API
allocator_interface*
ArrayDefaultAllocator();

CORE_API
void
ArraySetDefaultAllocator(allocator_interface* Allocator);


/// Dynamically growing array.
template<typename T>
struct array
{
  allocator_interface* Allocator{};
  size_t Capacity{};
  size_t Num{};
  T* Ptr{};


  array() = default;
  inline array(allocator_interface& Allocator) : array() { this->Allocator = &Allocator; }

  array(array const& ToCopy) = default;

  array(array&& ToMove);

  ~array();

  void
  operator=(array&& ToMove);

  constexpr operator bool() const { return this->Num && this->Ptr; }

  /// Index operator to access elements of this array.
  template<typename IndexType>
  T&
  operator[](IndexType Index);

  /// Index operator to access elements of this array.
  template<typename IndexType>
  T const&
  operator[](IndexType Index) const;
};


//
// Array operations
//

template<typename T>
slice<T>
Slice(array<T>& Array)
{
  return Slice(Array.Num, Array.Ptr);
}

template<typename T>
slice<T const>
Slice(array<T> const& Array)
{
  return Slice(Array.Num, AsPtrToConst(Array.Ptr));
}

template<typename T>
constexpr slice<T>
AllocatedMemory(array<T>& Array)
{
  return Slice(Array.Capacity, Array.Ptr);
}

template<typename T>
constexpr slice<T>
AllocatedMemory(array<T> const& Array)
{
  return Slice(Array.Capacity, AsPtrToConst(Array.Ptr));
}

template<typename T>
void
EnsureInitialized(array<T>& Array)
{
  if(Array.Allocator == nullptr)
    Array.Allocator = ArrayDefaultAllocator();
  Assert(Array.Allocator);
}

template<typename T>
void
Reserve(array<T>& Array, size_t MinBytesToReserve)
{
  EnsureInitialized(Array);
  auto NewAllocatedMemory = ContainerReserve(*Array.Allocator,
                                             Array.Ptr, Array.Num,
                                             Array.Capacity,
                                             MinBytesToReserve,
                                             ARRAY_MINIMUM_CAPACITY);

  // Update array members.
  Array.Ptr = NewAllocatedMemory.Ptr;
  Array.Capacity = NewAllocatedMemory.Num;
}

template<typename T>
slice<T>
ExpandBy(array<T>& Array, size_t Amount)
{
  if(Amount == 0)
    return {};

  Reserve(Array, Array.Num + Amount);
  auto const BeginIndex = Array.Num;
  Array.Num += Amount;
  auto const EndIndex = Array.Num;
  auto Result = Slice(AllocatedMemory(Array), BeginIndex, EndIndex);
  SliceConstruct(Result);
  return Result;
}

template<typename T>
T&
Expand(array<T>& Array)
{
  return ExpandBy(Array, 1)[0];
}

template<typename T, typename U>
void
Append(array<T>& Array, U const& Item)
{
  Reserve(Array, Array.Num + 1);
  auto const Index = Array.Num;
  ++Array.Num;
  auto NewPtr = &Array.Ptr[Index];
  MemConstruct(1, NewPtr, Item);
}

template<typename T, typename U>
inline void
operator +=(array<T>& Array, U const& Value)
{
  Append(Array, Value);
}

template<typename T, typename U>
void
Append(array<T>& Array, slice<U const> More)
{
  if(More.Num == 0)
    return;

  Reserve(Array, Array.Num + More.Num);
  auto const BeginIndex = Array.Num;
  Array.Num += More.Num;
  auto const EndIndex = Array.Num;
  auto NewElements = Slice(AllocatedMemory(Array), BeginIndex, EndIndex);
  SliceCopyConstruct(NewElements, AsConst(More));
}

template<typename T, typename U>
inline void
operator +=(array<T>& Array, slice<U> More)
{
  Append(Array, AsConst(More));
}

template<typename T>
void
ShrinkBy(array<T>& Array, size_t Amount)
{
  if(Amount == 0)
    return;

  BoundsCheck(Array.Num >= Amount);
  auto ToDestruct = Slice(Slice(Array), Array.Num - Amount, Array.Num);
  SliceDestruct(ToDestruct);
  Array.Num -= Amount;
}

template<typename T>
void
Clear(array<T>& Array)
{
  if(Array.Num)
  {
    SliceDestruct(Slice(Array));
    Array.Num = 0;
  }
}

template<typename T>
void
SetNum(array<T>& Array, size_t NewNum)
{
  if(Array.Num < NewNum)
  {
    ExpandBy(Array, NewNum - Array.Num);
  }
  else if(Array.Num > NewNum)
  {
    ShrinkBy(Array, Array.Num - NewNum);
  }
}

template<typename T>
void
EnsureNum(array<T>& Array, size_t MinNum)
{
  if(Array.Num < MinNum)
  {
    SetNum(Array, MinNum);
  }
}

template<typename T>
void
Reset(array<T>& Array)
{
  SliceDestruct(Slice(Array));
  if(Array.Allocator && Array.Capacity > 0)
  {
    SliceDeallocate(*Array.Allocator, AllocatedMemory(Array));
    Array.Capacity = 0;
  }

  // Just to make sure.
  Array.Num = 0;
  Array.Ptr = nullptr;
}

// TODO: Make this use BeginIndex and EndIndex instead?
template<typename T>
void
RemoveAt(array<T>& Array, size_t Index, size_t CountToRemove = 1)
{
  auto NewData = ContainerRemoveAt(Slice(Array), Index, CountToRemove);
  Assert(NewData.Num == Array.Num - CountToRemove);
  Array.Num = NewData.Num;
}

template<typename T>
bool
RemoveFirst(array<T>& Array, const T& Needle)
{
  auto Index = SliceCountUntil(AsConst(Slice(Array)), Needle);
  if(Index == INVALID_INDEX)
    return false;

  RemoveAt(Array, Index);
  return true;
}

//
// ==============================================
//


template<typename T>
array<T>::array(array&& ToMove)
  : Allocator(ToMove.Allocator)
  , Capacity(ToMove.Capacity)
  , Num(ToMove.Num)
  , Ptr(ToMove.Ptr)
{
  ToMove.Capacity = 0;
  ToMove.Num = 0;
  ToMove.Ptr = nullptr;
}

template<typename T>
array<T>::~array()
{
  Reset(*this);
}

template<typename T>
void
array<T>::operator=(array&& ToMove)
{
  // TODO: Is Ptr check enough? Think of Num etc.
  if(this->Ptr != ToMove.Ptr)
  {
    Reset(*this);

    // Move data over.
    this->Allocator = ToMove.Allocator;
    this->Capacity = ToMove.Capacity;
    this->Num = ToMove.Num;
    this->Ptr = ToMove.Ptr;

    // Clear other data.
    ToMove.Capacity = 0;
    ToMove.Num = 0;
    ToMove.Ptr = nullptr;
  }
}

template<typename T>
template<typename IndexType>
T&
array<T>::operator[](IndexType Index)
{
  BoundsCheck(Index >= 0 && Index < this->Num);
  return this->Ptr[Index];
}

template<typename T>
template<typename IndexType>
T const&
array<T>::operator[](IndexType Index) const
{
  BoundsCheck(Index >= 0 && Index < this->Num);
  return this->Ptr[Index];
}

//
// Exported instantiations for common types.
//

CORE_TEMPLATE_EXPORT(struct, array<uint8>);
CORE_TEMPLATE_EXPORT(struct, array<int8>);
CORE_TEMPLATE_EXPORT(struct, array<uint16>);
CORE_TEMPLATE_EXPORT(struct, array<int16>);
CORE_TEMPLATE_EXPORT(struct, array<uint32>);
CORE_TEMPLATE_EXPORT(struct, array<int32>);
CORE_TEMPLATE_EXPORT(struct, array<uint64>);
CORE_TEMPLATE_EXPORT(struct, array<int64>);
CORE_TEMPLATE_EXPORT(struct, array<float>);
CORE_TEMPLATE_EXPORT(struct, array<double>);
