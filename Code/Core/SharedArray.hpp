#pragma once

// This file contains an implementation of a copy-on-write array. It is just
// an experiment and not well tested or used. Consider it alpha-stage quality.

#include "CoreAPI.hpp"
#include "Allocator.hpp"
#include "ContainerUtils.hpp"

#include <Backbone.hpp>


// Measured in number of elements.
#if !defined(SHARED_ARRAY_MINIMUM_CAPACITY)
  #define SHARED_ARRAY_MINIMUM_CAPACITY 32
#endif

CORE_API
allocator_interface*
SharedArrayDefaultAllocator();

CORE_API
void
SharedArraySetDefaultAllocator(allocator_interface* Allocator);


/// Dynamically growing copy-on-write array.
template<typename T>
struct shared_array
{
  struct shared
  {
    int RefCount;
    size_t Capacity;
    size_t Num;
    T* Ptr;
  };

  allocator_interface* Allocator;
  shared* Shared;


  shared_array() = default;
  shared_array(allocator_interface& Allocator) { *this = {}; this->Allocator = &Allocator; }
  shared_array(shared_array const& ToCopy);
  shared_array(shared_array&& ToMove);
  ~shared_array();

  void operator=(shared_array const& ToCopy);
  void operator=(shared_array&& ToMove);

  inline operator bool() const { return this->Num && this->Ptr; }

  /// Index operator to access elements of this array.
  template<typename IndexType>
  T&
  operator[](IndexType Index);

  /// Index operator to access elements of this array.
  template<typename IndexType>
  T const&
  operator[](IndexType Index) const;


  //
  // Accessors
  //
  inline size_t&       Capacity()       { EnsureInitialized(*this); return this->Shared->Capacity; }
  inline size_t const& Capacity() const { EnsureInitialized(*this); return this->Shared->Capacity; }
  inline size_t&       Num()            { EnsureInitialized(*this); return this->Shared->Num; }
  inline size_t const& Num()      const { EnsureInitialized(*this); return this->Shared->Num; }
  inline T*&           Ptr()            { EnsureInitialized(*this); return this->Shared->Ptr; }
  inline T* const&     Ptr()      const { EnsureInitialized(*this); return this->Shared->Ptr; }
};


//
// Implementation Details
//

template<typename T>
typename shared_array<T>::shared*
ImplSharedArrayCreateShared(allocator_interface* Allocator)
{
  Assert(Allocator);
  auto Shared = Allocate<typename shared_array<T>::shared>(*Allocator);
  Assert(Shared);
  // Note: Do not initialize Shared->InitialMemory.
  Shared->RefCount = 1;
  Shared->Capacity = 0;
  Shared->Num = 0;
  Shared->Ptr = nullptr;
  return Shared;
}

template<typename T>
void
ImplSharedArrayDestroyShared(allocator_interface* Allocator, typename shared_array<T>::shared* Shared)
{
  Assert(Allocator);
  Deallocate(*Allocator, Shared);
}

template<typename T>
void
ImplSharedArrayAddRef(typename shared_array<T>::shared* Shared)
{
  if(Shared == nullptr)
    return;

  // TODO: Make thread-safe?
  ++Shared->RefCount;
}

template<typename T>
void
ImplSharedArrayReleaseRef(typename shared_array<T>::shared* Shared, allocator_interface* Allocator)
{
  if(Shared == nullptr)
    return;

  // If Shared is a valid pointer, there _must_ be an Allocator;
  Assert(Allocator);

  // TODO: Make thread-safe?
  --Shared->RefCount;

  if(Shared->RefCount <= 0)
  {
    SliceDestruct(Slice(Shared->Num, Shared->Ptr));
    SliceDeallocate(*Allocator, Slice(Shared->Capacity, Shared->Ptr));
    ImplSharedArrayDestroyShared<T>(Allocator, Shared);
  }
}


//
// Array operations
//

template<typename T>
slice<T>
Slice(shared_array<T>& Array)
{
  return Slice(Array.Num(), Array.Ptr());
}

template<typename T>
slice<T const>
Slice(shared_array<T> const& Array)
{
  return Slice(Array.Num(), AsPtrToConst(Array.Ptr()));
}

template<typename T>
constexpr slice<T>
AllocatedMemory(shared_array<T>& Array)
{
  return Slice(Array.Capacity(), Array.Ptr());
}

template<typename T>
constexpr slice<T>
AllocatedMemory(shared_array<T> const& Array)
{
  return Slice(Array.Capacity(), AsPtrToConst(Array.Ptr()));
}

template<typename T>
void
EnsureInitialized(shared_array<T>& Array)
{
  if(Array.Allocator == nullptr)
    Array.Allocator = SharedArrayDefaultAllocator();

  if(Array.Shared == nullptr)
    Array.Shared = ImplSharedArrayCreateShared<T>(Array.Allocator);
}

template<typename T>
void
Reserve(shared_array<T>& Array, size_t MinBytesToReserve)
{
  EnsureInitialized(Array);
  bool const IsUnique = Array.Shared->RefCount <= 1;
  if(IsUnique)
  {
    auto NewAllocatedMemory = ContainerReserve(*Array.Allocator,
                                               Array.Ptr(), Array.Num(),
                                               Array.Capacity(),
                                               MinBytesToReserve,
                                               SHARED_ARRAY_MINIMUM_CAPACITY);

    // Update array members.
    Array.Ptr() = NewAllocatedMemory.Ptr;
    Array.Capacity() = NewAllocatedMemory.Num;
  }
  else
  {
    // Allocate memory and copy over the old data, regardless of whether the
    // currently shared data may have enough capacity.

    auto& Allocator = *Array.Allocator;

    // Gather current array data.
    auto OldAllocatedMemory = AllocatedMemory(Array);
    auto OldUsedMemory = Slice(Array);

    // Prepare new array data.
    auto NewAllocatedMemory = SliceAllocate<T>(Allocator, Max(MinBytesToReserve, OldAllocatedMemory.Num));
    auto NewUsedMemory = Slice(NewAllocatedMemory, 0, OldUsedMemory.Num);

    // Copy the old data over (if any).
    SliceCopy(NewUsedMemory, AsConst(OldUsedMemory));

    // Let go of the old Shared reference.
    ImplSharedArrayReleaseRef<T>(Array.Shared, &Allocator);

    // Create a new Shared reference and initialize it.
    Array.Shared = ImplSharedArrayCreateShared<T>(&Allocator);
    Array.Shared->Capacity = NewAllocatedMemory.Num;
    Array.Shared->Num = NewUsedMemory.Num;
    Array.Shared->Ptr = NewAllocatedMemory.Ptr;
  }
}

template<typename T>
void
EnsureUnique(shared_array<T>& Array)
{
  Reserve(Array, 0);
}

template<typename T>
slice<T>
ExpandBy(shared_array<T>& Array, size_t Amount)
{
  Reserve(Array, Array.Num() + Amount);
  const auto BeginIndex = Array.Num();
  Array.Num() += Amount;
  const auto EndIndex = Array.Num();
  auto Result = Slice(AllocatedMemory(Array), BeginIndex, EndIndex);
  SliceConstruct(Result);
  return Result;
}

template<typename T>
T&
Expand(shared_array<T>& Array)
{
  return ExpandBy(Array, 1)[0];
}

template<typename T, typename U>
void
Append(shared_array<T>& Array, slice<U> More)
{
  if(More.Num)
  {
    auto NewSpace = ExpandBy(Array, More.Num);
    SliceCopy(NewSpace, More);
  }
}

template<typename T>
void
SetNum(shared_array<T>& Array, size_t NewNum)
{
  if(Array.Num() < NewNum)
  {
    ExpandBy(Array, NewNum - Array.Num());
  }
  else if(Array.Num() > NewNum)
  {
    ShrinkBy(Array, Array.Num() - NewNum);
  }
}

template<typename T>
void
EnsureNum(shared_array<T>& Array, size_t MinNum)
{
  if(Array.Num() < MinNum)
  {
    SetNum(Array, MinNum);
  }
}

template<typename T>
void
ShrinkBy(shared_array<T>& Array, size_t Amount)
{
  BoundsCheck(Array.Num() >= Amount);
  EnsureUnique(Array);
  auto ToDestruct = Slice(Slice(Array), Array.Num() - Amount, Array.Num());
  SliceDestruct(ToDestruct);
  Array.Num() -= Amount;
}

template<typename T>
void
Clear(shared_array<T>& Array)
{
  if(Array.Num())
  {
    ShrinkBy(Array, Array.Num());
  }
}

template<typename T>
void
Reset(shared_array<T>& Array)
{
  if(Array.Shared)
  {
    ImplSharedArrayReleaseRef<T>(Array.Shared, Array.Allocator);
    Array.Shared = nullptr;
  }
}

// TODO: Make this use BeginIndex and EndIndex instead?
template<typename T>
void
RemoveAt(shared_array<T>& Array, size_t Index, size_t CountToRemove = 1)
{
  EnsureUnique(Array);
  auto NewData = ContainerRemoveAt(Slice(Array), Index, CountToRemove);
  Assert(NewData.Num == Array.Num() - CountToRemove);
  Array.Num() = NewData.Num;
}

template<typename T>
bool
RemoveFirst(shared_array<T>& Array, const T& Needle)
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
shared_array<T>::shared_array(shared_array<T> const& ToCopy)
{
  this->Allocator = ToCopy.Allocator;
  this->Shared = ToCopy.Shared;
  ImplSharedArrayAddRef<T>(this->Shared);
}

template<typename T>
shared_array<T>::shared_array(shared_array<T>&& ToMove)
{
  this->Allocator = ToMove.Allocator;
  this->Shared = ToMove.Shared;

  // Steal ToMove's Shared
  ToMove->Shared = nullptr;
}

template<typename T>
shared_array<T>::~shared_array()
{
  Reset(*this);
}

template<typename T>
void shared_array<T>::operator=(shared_array const& ToCopy)
{
  if(this->Shared != ToCopy.Shared)
  {
    ImplSharedArrayReleaseRef<T>(this->Shared, this->Allocator);
    this->Shared = ToCopy.Shared;
    ImplSharedArrayAddRef<T>(this->Shared);
  }
}

template<typename T>
void shared_array<T>::operator=(shared_array&& ToMove)
{
  if(this->Shared != ToCopy.Shared)
  {
    ImplSharedArrayReleaseRef<T>(this->Shared, this->Allocator);
    this->Shared = ToCopy.Shared;
    ToCopy.Shared = nullptr;
  }
}

template<typename T>
template<typename IndexType>
T&
shared_array<T>::operator[](IndexType Index)
{
  EnsureUnique(*this);
  BoundsCheck(Index >= 0 && Index < this->Shared->Num);
  return this->Shared->Ptr[Index];
}

template<typename T>
template<typename IndexType>
T const&
shared_array<T>::operator[](IndexType Index) const
{
  Assert(this->Shared != nullptr);
  BoundsCheck(Index >= 0 && Index < this->Shared->Num);
  return this->Shared->Ptr[Index];
}
