#pragma once

#include "CoreAPI.hpp"
#include "DynamicArray.hpp"

#include <functional>

// TODO Replace with a custom implementation?
template<typename TypeSpec>
using delegate = std::function<TypeSpec>;

template<typename... ArgTypes>
struct event
{
  using listener = delegate<void(ArgTypes...)>;
  struct id { size_t Value; };

  struct registered_listener
  {
    id Id;
    listener Callback;
  };

  dynamic_array<registered_listener> Listeners;
  id NewId;

  void
  operator()(ArgTypes... Args)
  {
    for(auto& Listener : Slice(&Listeners))
    {
      if(Listener.Callback)
        Listener.Callback(Forward<ArgTypes>(Args)...);
    }
  }
};

template<typename... ArgTypes>
void
Init(event<ArgTypes...>* Event, allocator_interface* Allocator)
{
  Init(&Event->Listeners, Allocator);
}

template<typename... ArgTypes>
void
Finalize(event<ArgTypes...>* Event)
{
  Finalize(&Event->Listeners);
}

template<typename... ArgTypes>
typename event<ArgTypes...>::id
AddListener(event<ArgTypes...>* Event, typename event<ArgTypes...>::listener Callback)
{
  auto& Entry = Expand(&Event->Listeners);
  Entry.Id = Event->NewId;
  Entry.Callback = Move(Callback);
  ++Event->NewId.Value;
  return Entry.Id;
}

template<typename... ArgTypes>
bool
RemoveListener(event<ArgTypes...>* Event, typename event<ArgTypes...>::id ListenerId)
{
  using registered_listener = typename event<ArgTypes...>::registered_listener;
  using id = typename event<ArgTypes...>::id;

  size_t ListenerIndex = SliceCountUntil(AsConst(Slice(&Event->Listeners)),
                                         ListenerId,
                                         [](registered_listener const& Listener, id Id)
                                         {
                                           return Listener.Id.Value == Id.Value;
                                         });
  if(ListenerIndex == INVALID_INDEX)
    return false;
  RemoveAt(&Event->Listeners, ListenerIndex);
  return true;
}
