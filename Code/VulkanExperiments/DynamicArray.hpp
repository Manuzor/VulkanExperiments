#pragma once

#include "Allocator.hpp"

#ifndef DYNAMIC_ARRAY_MIN_ALLOCATION_COUNT
  #define DYNAMIC_ARRAY_MIN_ALLOCATION_COUNT 16
#endif

constexpr size_t DynamicArrayInvalidIndex = (size_t)-1;

template<typename T>
struct dynamic_array
{
  allocator_interface* Allocator;
  slice<T> RawData;
  size_t Num;

  operator bool() const { return Num > 0; }
};

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
  DeleteArray(Array->Allocator, Array->RawData);
}

template<typename T>
slice<T>
Data(dynamic_array<T>* Array)
{
  return Slice(Array->RawData, 0, Array->Num);
}

template<typename T>
size_t
Capacity(dynamic_array<T>* Array)
{
  return Array->RawData.Num;
}

template<typename T>
void
Reserve(dynamic_array<T>* Array, size_t MinBytesToReserve)
{
  auto BytesToReserve = Max(MinBytesToReserve, DYNAMIC_ARRAY_MIN_ALLOCATION_COUNT);
  if(Capacity(Array) >= BytesToReserve)
    return;

  auto NewRawData = NewArray<T>(Array->Allocator, BytesToReserve);
  Assert(bool(NewRawData));
  SliceCopy(Slice(NewRawData, 0, Array->Num), SliceAsConst(Data(Array)));
  DeleteArray(Array->Allocator, Array->RawData);
  Array->RawData = NewRawData;
}

template<typename T>
size_t
PushBack(dynamic_array<T>* Array, T NewElement)
{
  Reserve(Array, Array->Num + 1);
  const auto Index = Array->Num;
  ++Array->Num;
  Array->RawData[Index] = NewElement;
  return Index;
}

template<typename T>
void
PopBack(dynamic_array<T>* Array)
{
  Assert(Array->Num > 0);
  --Array->Num;
}

template<typename T>
void
Clear(dynamic_array<T>* Array)
{
  // TODO Destruct.
  Array->Num = 0;
}
