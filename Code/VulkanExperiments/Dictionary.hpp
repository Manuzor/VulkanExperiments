#pragma once

#include "Allocator.hpp"
#include "ContainerUtils.hpp"

#include <Backbone.hpp>


constexpr size_t DictionaryMinimumCapacity = 16;

template<typename K, typename V>
struct dictionary
{
  allocator_interface* Allocator;

  /// The number of valid key-value pairs.
  size_t Num;

  size_t Capacity;
  K* KeysPtr;
  V* ValuesPtr;
};

template<typename K, typename V>
void
Init(dictionary<K, V>* Dict, allocator_interface* Allocator)
{
  Dict->Allocator = Allocator;
}

template<typename K, typename V>
void
Clear(dictionary<K, V>* Dict)
{
  if(Dict->Num)
  {
    SliceDestruct(Keys(Dict));
    SliceDestruct(Values(Dict));
    Dict->Num = 0;
  }
}

template<typename K, typename V>
void
Finalize(dictionary<K, V>* Dict)
{
  Clear(Dict);
  if(Dict->Capacity)
  {
    Dict->Allocator->Deallocate(Dict->KeysPtr);
    Dict->Allocator->Deallocate(Dict->ValuesPtr);
    Dict->Capacity = 0;
  }
}

template<typename K, typename V>
void
Reserve(dictionary<K, V>* Dict, size_t MinBytesToReserve)
{
  auto NewKeys = ContainerReserve(Dict->Allocator,
                                  Dict->KeysPtr, Dict->Num,
                                  Dict->Capacity,
                                  MinBytesToReserve,
                                  DictionaryMinimumCapacity);

  auto NewValues = ContainerReserve(Dict->Allocator,
                                    Dict->ValuesPtr, Dict->Num,
                                    Dict->Capacity,
                                    MinBytesToReserve,
                                    DictionaryMinimumCapacity);

  Assert(NewKeys.Num == NewValues.Num);

  // Update members.
  Dict->Capacity  = NewKeys.Num;
  Dict->KeysPtr   = NewKeys.Ptr;
  Dict->ValuesPtr = NewValues.Ptr;
}

template<typename K, typename V>
slice<K>
Keys(dictionary<K, V>* Dict)
{
  return Slice(Dict->Num, Dict->KeysPtr);
}

template<typename K, typename V>
slice<K const>
Keys(dictionary<K, V> const* Dict)
{
  return Slice(Dict->Num, AsPtrToConst(Dict->KeysPtr));
}

template<typename K, typename V>
slice<V>
Values(dictionary<K, V>* Dict)
{
  return Slice(Dict->Num, Dict->ValuesPtr);
}

template<typename K, typename V>
slice<V const>
Values(dictionary<K, V> const* Dict)
{
  return Slice(Dict->Num, AsPtrToConst(Dict->ValuesPtr));
}

template<typename K, typename V, typename IndexType>
V*
Get(dictionary<K, V>* Dict, IndexType KeyIndex)
{
  auto ArrayIndex = SliceCountUntil(AsConst(Keys(Dict)), KeyIndex);
  if(ArrayIndex == INVALID_INDEX)
    return nullptr;
  return &Values(Dict)[ArrayIndex];
}

template<typename K, typename V, typename IndexType>
V const*
Get(dictionary<K, V> const* Dict, IndexType KeyIndex)
{
  auto ArrayIndex = SliceCountUntil(Keys(Dict), KeyIndex);
  if(ArrayIndex == INVALID_INDEX)
    return nullptr;
  return &Values(Dict)[ArrayIndex];
}

template<typename K, typename V, typename IndexType>
V*
GetOrCreate(dictionary<K, V>* Dict, IndexType KeyIndex)
{
  auto ArrayIndex = SliceCountUntil(AsConst(Keys(Dict)), KeyIndex);
  if(ArrayIndex >= 0)
    return &Values(Dict)[ArrayIndex];

  Reserve(Dict, Dict->Capacity + 1);

  ArrayIndex = Dict->Num++;

  auto NewKeyPtr = Dict->KeysPtr + ArrayIndex;
  MemConstruct(1, NewKeyPtr, KeyIndex);

  auto NewValuePtr = Dict->ValuesPtr + ArrayIndex;
  MemDefaultConstruct(1, NewValuePtr);

  return NewValuePtr;
}

template<typename K, typename V, typename IndexType>
bool
Remove(dictionary<K, V>* Dict, IndexType KeyIndex)
{
  auto ArrayIndex = SliceCountUntil(AsConst(Keys(Dict)), KeyIndex);
  if(ArrayIndex == INVALID_INDEX)
    return false;

  auto NewKeys   = ContainerRemoveAt(Keys(Dict),   ArrayIndex);
  auto NewValues = ContainerRemoveAt(Values(Dict), ArrayIndex);
  --Dict->Num;
  return true;
}
