#pragma once

#include "CoreAPI.hpp"
#include <Backbone.hpp>

const size_t GlobalDefaultAlignment = 16;

#if defined(DEBUG)
  size_t CORE_API
  CheckedAlignment(size_t Alignment);
#else
  constexpr size_t
  CheckedAlignment(size_t Alignment)
  {
    return Alignment == 0 ? GlobalDefaultAlignment : Alignment;
  }
#endif


/// \note Alignment arguments must always be a power of two or 0 (if you don't care).
/// \see CheckedAlignment()
class CORE_API allocator_interface
{
public:
  virtual ~allocator_interface() {}

  virtual void* Allocate(memory_size Size, size_t Alignment) = 0;
  virtual void Deallocate(void* Memory) = 0;

  virtual bool Resize(void* Ptr, memory_size NewSize) { return false; }

  virtual memory_size AllocationSize(void* Ptr) { return Bytes(0); }
};

class CORE_API mallocator : public allocator_interface
{
public:
  virtual void* Allocate(memory_size Size, size_t Alignment) override;
  virtual void Deallocate(void* Memory) override;
  virtual bool Resize(void* Ptr, memory_size NewSize) override;
  virtual memory_size AllocationSize(void* Ptr) override;
};

template<typename T>
T*
Allocate(allocator_interface& Allocator)
{
  auto Memory = Allocator.Allocate(SizeOf<T>(), alignof(T));
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
  auto Memory = Allocator.Allocate(Num * SizeOf<T>(), alignof(T));
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
  virtual void Deallocate(void* Memory) override;
  virtual bool Resize(void* Ptr, memory_size NewSize) override;
};
