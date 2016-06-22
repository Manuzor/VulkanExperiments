#pragma once

#include "DynamicArray.hpp"

#include <functional>

// TODO Replace with a custom implementation?
template<typename TypeSpec>
using delegate = std::function<TypeSpec>;

template<typename TypeSpec>
constexpr bool operator==(const delegate<TypeSpec>& A, const delegate<TypeSpec>& B)
{
  return MemEqualBytes(sizeof(delegate<TypeSpec>), &A, &B);
}

template<typename TypeSpec>
constexpr bool operator!=(const delegate<TypeSpec>& A, const delegate<TypeSpec>& B)
{
  return !(A == B);
}

template<typename... ArgTypes>
struct event
{
  using ListenerType = delegate<void(ArgTypes...)>;

  dynamic_array<ListenerType> Listeners;

  void operator()(ArgTypes&&... Args)
  {
    for(auto& Listener : Slice(&Listeners))
    {
      if(Listener)
        Listener(Forward<ArgTypes>(Args)...);
    }
  }
};

template<typename... ArgTypes>
void Init(event<ArgTypes...>* Event, allocator_interface* Allocator)
{
  Init(&Event->Listeners, Allocator);
}

template<typename... ArgTypes>
void Finalize(event<ArgTypes...>* Event)
{
  Finalize(&Event->Listeners);
}

template<typename... ArgTypes>
void AddListener(event<ArgTypes...>* Event, typename event<ArgTypes...>::ListenerType Listener)
{
  Expand(&Event->Listeners) = Move(Listener);
}

template<typename... ArgTypes>
bool RemoveListener(event<ArgTypes...>* Event, typename event<ArgTypes...>::ListenerType Listener)
{
  size_t ListenerIndex = SliceCountUntil(Slice(&Event->Listeners), Listener);
  if(ListenerIndex < 0)
    return false;
  RemoveAt(&Event->Listeners, ListenerIndex);
  return true;
}
