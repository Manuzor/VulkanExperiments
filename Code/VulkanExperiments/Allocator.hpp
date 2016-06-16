#pragma once

#include <Backbone.hpp>

#include <new>

class allocator_interface
{
public:
  virtual ~allocator_interface() {}

  virtual slice<void> Allocate(size_t NumBytes, size_t Alignment) = 0;
  virtual bool Deallocate(slice<void> Memory) = 0;
};

class mallocator : public allocator_interface
{
  slice<void> Allocate(size_t NumBytes, size_t Alignment) override;
  bool Deallocate(slice<void> Memory) override;
};

template<typename T>
slice<T>
NewArray(allocator_interface* Allocator, size_t Num)
{
  // TODO Alignment
  auto Memory = Allocator->Allocate(Num * sizeof(T), 1);
  auto Ptr = new (Memory.Data) T[Num];
  return CreateSlice(Num, Ptr);
}

template<typename T>
void
DeleteArray(allocator_interface* Allocator, slice<T> Array)
{
  Allocator->Deallocate(SliceReinterpret<void>(Array));
}
