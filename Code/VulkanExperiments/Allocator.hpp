#pragma once

#include <Backbone.hpp>

#include <new>

class allocator_interface
{
public:
  virtual ~allocator_interface() {}

  virtual void* Allocate(size_t NumBytes, size_t Alignment) = 0;
  virtual bool Deallocate(void* Memory) = 0;
};

class mallocator : public allocator_interface
{
  void* Allocate(size_t NumBytes, size_t Alignment) override;
  bool Deallocate(void* Memory) override;
};

template<typename T>
slice<T>
SliceAllocate(allocator_interface* Allocator, size_t Num)
{
  // TODO Alignment
  auto Ptr = Allocator->Allocate(Num * sizeof(T), 1);
  return Slice(Num, Reinterpret<T*>(Ptr));
}

template<typename T>
void
SliceDeallocate(allocator_interface* Allocator, slice<T> Slice)
{
  Allocator->Deallocate(Slice.Ptr);
}
