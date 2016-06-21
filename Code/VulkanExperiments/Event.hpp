#pragma once

#include "DynamicArray.hpp"

#include <functional>

// TODO Replace with a custom implementation?
template<typename TypeSpec>
using Delegate = std::function<TypeSpec>;

template<typename TypeSpec>
constexpr bool operator==(const Delegate<TypeSpec>& A, const Delegate<TypeSpec>& B)
{
  return MemEqualBytes(&A, &B, sizeof(Delegate<TypeSpec>));
}

template<typename TypeSpec>
constexpr bool operator!=(const Delegate<TypeSpec>& A, const Delegate<TypeSpec>& B)
{
  return !(A == B);
}

template<typename... ArgTypes>
struct Event
{
  using ListenerType = Delegate<void(ArgTypes...)>;

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
void Init(Event<ArgTypes...>* Self, allocator_interface* Allocator)
{
  Init(&Self->Listeners, Allocator);
}

template<typename... ArgTypes>
void Finalize(Event<ArgTypes...>* Self)
{
  Finalize(&Self->Listeners);
}

template<typename... ArgTypes>
void AddListener(Event<ArgTypes...>* This, typename Event<ArgTypes...>::ListenerType Listener)
{
  Expand(&This->Listeners) = Move(Listener);
}

template<typename... ArgTypes>
bool RemoveListener(Event<ArgTypes...>* This, typename Event<ArgTypes...>::ListenerType Listener)
{
  size_t ListenerIndex = SliceCountUntil(Slice(&This->Listeners), Listener);
  if(ListenerIndex < 0)
    return false;
  RemoveAt(&This->Listeners, ListenerIndex);
  return true;
}
