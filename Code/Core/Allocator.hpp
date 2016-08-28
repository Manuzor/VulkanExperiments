#pragma once

#include "CoreAPI.hpp"
#include <Backbone.hpp>


class CORE_API allocator_interface
{
public:
  virtual ~allocator_interface() {}

  virtual void* Allocate(memory_size Size, size_t Alignment) = 0;
  virtual bool Deallocate(void* Memory) = 0;
};

class CORE_API mallocator : public allocator_interface
{
public:
  virtual void* Allocate(memory_size Size, size_t Alignment) override;
  virtual bool Deallocate(void* Memory) override;
};

template<typename T>
T*
Allocate(allocator_interface& Allocator)
{
  // TODO Alignment
  enum { Alignment = __alignof(T) };
  auto Memory = Allocator.Allocate(SizeOf<T>(), Alignment);
  auto Ptr = Reinterpret<T*>(Memory);
  return Ptr;
}

template<typename T>
void
Deallocate(allocator_interface& Allocator, T* Object)
{
  Allocator.Deallocate(Object);
}

template<typename T>
slice<T>
SliceAllocate(allocator_interface& Allocator, size_t Num)
{
  // TODO Alignment
  enum { Alignment = __alignof(T) };
  auto Memory = Allocator.Allocate(Num * SizeOf<T>(), Alignment);
  return Slice(Num, Reinterpret<T*>(Memory));
}

template<typename T>
void
SliceDeallocate(allocator_interface& Allocator, slice<T> Slice)
{
  Allocator.Deallocate(Slice.Ptr);
}

template<typename T, typename... ArgTypes>
inline T*
New(allocator_interface& Allocator, ArgTypes&&... Args)
{
  auto Ptr = Allocate<T>(Allocator);
  MemConstruct(1, Ptr, Forward<ArgTypes>(Args)...);
  return Ptr;
}

template<typename T>
inline void
Delete(allocator_interface& Allocator, T* Ptr)
{
  MemDestruct(1, Ptr);
  Deallocate(Allocator, Ptr);
}

/// An allocator wrapper that is intended to be used only for a short period
/// of time in the scope it was created.
///
/// \note The dereference operator (\c operator*) is overloaded to retrieve a
///       reference to an allocator interface (\c allocator_interface&).
struct CORE_API temp_allocator : public allocator_interface
{
  temp_allocator();
  temp_allocator(temp_allocator const&) = delete; // No copy
  virtual ~temp_allocator();

  virtual void* Allocate(memory_size Size, size_t Alignment) override;
  virtual bool Deallocate(void* Memory) override;
};
