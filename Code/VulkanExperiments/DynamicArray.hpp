#pragma once

#include "Allocator.hpp"

#ifndef DYNAMIC_ARRAY_MIN_ALLOCATION_COUNT
  #define DYNAMIC_ARRAY_MIN_ALLOCATION_COUNT 16
#endif

constexpr size_t ArrayInvalidIndex = (size_t)-1;

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
ArrayInit(dynamic_array<T>* Array, allocator_interface* Allocator)
{
  Array->Allocator = Allocator;
}

template<typename T>
slice<T>
ArrayData(dynamic_array<T>* Array)
{
  return Slice(Array->RawData, 0, Array->Num);
}

template<typename T>
size_t
ArrayCapacity(dynamic_array<T>* Array)
{
  return Array->RawData.Num;
}

template<typename T>
void
ArrayReserve(dynamic_array<T>* Array, size_t MinBytesToReserve)
{
  auto BytesToReserve = Max(MinBytesToReserve, DYNAMIC_ARRAY_MIN_ALLOCATION_COUNT);
  if(ArrayCapacity(Array) >= BytesToReserve)
    return;

  auto NewRawData = NewArray<T>(Array->Allocator, BytesToReserve);
  Assert(bool(NewRawData));
  SliceCopy(Slice(NewRawData, 0, Array->Num), SliceAsConst(ArrayData(Array)));
  DeleteArray(Array->Allocator, Array->RawData);
  Array->RawData = NewRawData;
}

template<typename T>
size_t
ArrayPushBack(dynamic_array<T>* Array, T NewElement)
{
  ArrayReserve(Array, Array->Num + 1);
  const auto Index = Array->Num;
  ++Array->Num;
  Array->RawData[Index] = NewElement;
  return Index;
}

template<typename T>
void
ArrayPopBack(dynamic_array<T>* Array)
{
  Assert(Array->Num > 0);
  --Array->Num;
}

template<typename T>
void
ArrayClear(dynamic_array<T>* Array)
{
  // TODO Destruct.
  Array->Num = 0;
}
