#include "Input.hpp"
#include "Win32_Input.hpp"

#include "Array.hpp"


static_assert(sizeof(size_t) == 8, "Internals assume 64 bit pointers.");


namespace
{
  struct impl_input_handle
  {
    uint32 ContextId;
    uint32 InternalOffset; // Either an index to a slot, slot_properties
  };

  struct impl_slot
  {
    /// The type of this slot.
    input_type Type;

    arc_string Name;

    /// The value of the slot.
    ///
    /// Interpretation depends on the $(D Type) of this slot.
    ///
    /// \see InputAction, InputButton, InputAxis
    float ValueStore;

    /// The frame in which this value was updated.
    uint64 Frame;

    float PositiveDeadZone = 0.0f; // [0 .. 1]
    float NegativeDeadZone = 0.0f; // [0 .. 1]
    float Sensitivity = 1.0f;
    float Exponent = 1.0f;
  };

  // TODO(Manu): Make use of `event` for some notifications?
  struct impl_context
  {
    struct slot_mapping
    {
      input_slot Source; // When this slots' value is changed, the TargetSlotId will be changed.
      input_slot Target; // This is the slot which value will be changed if SourceSlotId's value changes.
      float Scale;           // A factor used when mapping slots.
    };

    input_context Parent{};
    int UserIndex = -1; // -1 for no associated user, >= 0 for a specific user.
    slice<char> Name{}; // Mostly for debugging. TODO: Use arc_string

    array<impl_slot> Slots{};
    array<slot_mapping> SlotMappings{};

    uint64 Frame{};

    // TODO: Use arc_string here.
    array<char> CharacterBuffer{};

    fixed_block<XUSER_MAX_COUNT, XINPUT_STATE> XInputPreviousState{};

    impl_context(allocator_interface* Allocator)
      : Slots(*Allocator)
      , SlotMappings(*Allocator)
    {
    }
  };

  static array<impl_context*>&
  GetGlobalContextRegistry()
  {
    static mallocator Allocator;
    static array<impl_context*> Array{ Allocator };
    return Array;
  }

  union context_handle_converter
  {
    struct
    {
      size_t ContextId;
    };

    input_context ContextHandle;
  };
  static_assert(sizeof(context_handle_converter) == sizeof(input_context), "Size mismatch.");

  union slot_handle_converter
  {
    struct
    {
      size_t ContextId : 8 * sizeof(size_t) / 2;
      size_t SlotId    : 8 * sizeof(size_t) / 2;
    };

    input_slot SlotHandle;
  };
  static_assert(sizeof(context_handle_converter) == sizeof(input_slot), "Size mismatch.");
}

static input_context
MakeContextHandle(size_t ContextIndex)
{
  context_handle_converter Converter;
  Converter.ContextId = ContextIndex + 1;
  return Converter.ContextHandle;
}

static void
DisassembleContextHandle(input_context ContextHandle, size_t* OutContextIndex, impl_context** OutContext)
{
  context_handle_converter Converter;
  Converter.ContextHandle = ContextHandle;

  auto const ContextIndex = Converter.ContextId - 1;
  if(OutContextIndex)
    *OutContextIndex = ContextIndex;

  impl_context* Context = GetGlobalContextRegistry()[ContextIndex];
  if(OutContext)
    *OutContext = Context;
}

static input_slot
MakeSlotHandle(size_t ContextIndex, size_t SlotIndex)
{
  slot_handle_converter Converter;
  Converter.ContextId = ContextIndex + 1;
  Converter.SlotId = SlotIndex + 1;
  return Converter.SlotHandle;
}

static void
DisassembleSlotHandle(input_slot SlotHandle,
                      size_t* OutContextIndex,
                      size_t* OutSlotIndex,
                      impl_context** OutContext,
                      impl_slot** OutSlot)
{
  slot_handle_converter Converter;
  Converter.SlotHandle = SlotHandle;


  auto const ContextIndex = Converter.ContextId - 1;
  if(OutContextIndex)
    *OutContextIndex = ContextIndex;

  auto const SlotIndex = Converter.SlotId - 1;
  if(OutSlotIndex)
    *OutSlotIndex = SlotIndex;

  impl_context* Context = Converter.ContextId != 0 ? GetGlobalContextRegistry()[ContextIndex] : nullptr;
  if(OutContext)
    *OutContext = Context;

  impl_slot* Slot = Context != nullptr ? &Context->Slots[SlotIndex] : nullptr;
  if(OutSlot)
    *OutSlot = Slot;
}

inline impl_slot*
GetSlot(input_slot SlotHandle)
{
  impl_slot* Result{};
  DisassembleSlotHandle(SlotHandle, nullptr, nullptr, nullptr, &Result);
  return Result;
}

static float
AttuneValue(impl_slot* Slot, float OldValue)
{
  float NewValue = OldValue;

  if(NewValue > 0 && Slot->PositiveDeadZone > 0.0f)
  {
    NewValue = Max(0.0f, NewValue - Slot->PositiveDeadZone) / (1.0f - Slot->PositiveDeadZone);
  }
  else if(NewValue < 0 && Slot->NegativeDeadZone > 0.0f)
  {
    NewValue = -Max(0.0f, -NewValue - Slot->NegativeDeadZone) / (1.0f - Slot->NegativeDeadZone);
  }

  // Apply the exponent setting, if any.
  if(Slot->Exponent != 1.0f)
  {
    NewValue = Sign(NewValue) * (Pow(Abs(NewValue), Slot->Exponent));
  }

  // Scale the value.
  NewValue *= Slot->Sensitivity;

  return NewValue;
}

static void
UpdateSlotValue(impl_context* Context, input_slot SlotHandle, impl_slot* Slot, float NewValue)
{
  float const AttunedValue = AttuneValue(Slot, NewValue);

  Slot->Frame = Context->Frame;
  Slot->ValueStore = AttunedValue;

  //
  // Apply input mapping
  //
  for(auto& Mapping : Slice(Context->SlotMappings))
  {
    if(Mapping.Source == SlotHandle)
    {
      impl_slot* OtherSlot{};
      DisassembleSlotHandle(Mapping.Target, nullptr, nullptr, nullptr, &OtherSlot);

      Assert(OtherSlot);

      float NewMappedValue = AttunedValue * Mapping.Scale;
      UpdateSlotValue(Context, Mapping.Target, OtherSlot, NewMappedValue);
    }
  }
}

auto
::InputGetSlotValueUnchecked(input_slot SlotHandle)
  -> float
{
  impl_context* Context{};
  impl_slot* Slot{};
  DisassembleSlotHandle(SlotHandle, nullptr, nullptr, &Context, &Slot);

  if(Context == nullptr || Slot == nullptr)
  {
    return 0.0f;
  }

  return Slot->ValueStore;
}

auto
::InputSetSlotValueUnchecked(input_slot SlotHandle, float NewValue)
  -> bool
{
  impl_context* Context{};
  impl_slot* Slot{};
  DisassembleSlotHandle(SlotHandle, nullptr, nullptr, &Context, &Slot);

  if(Context == nullptr || Slot == nullptr)
    return false;

  UpdateSlotValue(Context, SlotHandle, Slot, NewValue);
  return true;
}

auto
::InputActionValue(input_slot SlotHandle)
  -> float
{
  impl_context* Context{};
  impl_slot* Slot{};
  DisassembleSlotHandle(SlotHandle, nullptr, nullptr, &Context, &Slot);

  if(Context == nullptr || Slot == nullptr)
  {
    return 0.0f;
  }

  Assert(Slot->Type == input_type::Action);
  if(Slot->Frame != Context->Frame)
  {
    return 0.0f;
  }

  return Slot->ValueStore;
}

auto
::InputSetActionValue(input_slot SlotHandle, float NewValue)
  -> void
{
  impl_context* Context{};
  impl_slot* Slot{};
  DisassembleSlotHandle(SlotHandle, nullptr, nullptr, &Context, &Slot);

  if(Context && Slot)
  {
    Assert(Slot->Type == input_type::Action);
    UpdateSlotValue(Context, SlotHandle, Slot, NewValue);
  }
}

auto
::InputAxisValue(input_slot SlotHandle)
  -> float
{
  impl_context* Context{};
  impl_slot* Slot{};
  DisassembleSlotHandle(SlotHandle, nullptr, nullptr, &Context, &Slot);

  if(Context == nullptr || Slot == nullptr)
  {
    return 0.0f;
  }

  Assert(Slot->Type == input_type::Axis);
  return Slot->ValueStore;
}

auto
::InputSetAxisValue(input_slot SlotHandle, float NewValue)
  -> void
{
  impl_context* Context{};
  impl_slot* Slot{};
  DisassembleSlotHandle(SlotHandle, nullptr, nullptr, &Context, &Slot);

  if(Context && Slot)
  {
    Assert(Slot->Type == input_type::Axis);
    UpdateSlotValue(Context, SlotHandle, Slot, NewValue);
  }
}

auto
::InputButtonIsDown(input_slot SlotHandle)
  -> bool
{
  impl_context* Context{};
  impl_slot* Slot{};
  DisassembleSlotHandle(SlotHandle, nullptr, nullptr, &Context, &Slot);

  if(Context == nullptr || Slot == nullptr)
  {
    return false;
  }

  Assert(Slot->Type == input_type::Button);
  return Slot->ValueStore != 0.0f;
}

auto
::InputButtonIsUp(input_slot SlotHandle)
  -> bool
{
  return !InputButtonIsDown(SlotHandle);
}

auto
::InputButtonWasPressed(input_slot SlotHandle)
  -> bool
{
  impl_context* Context{};
  impl_slot* Slot{};
  DisassembleSlotHandle(SlotHandle, nullptr, nullptr, &Context, &Slot);

  if(Context == nullptr || Slot == nullptr)
  {
    return false;
  }

  Assert(Slot->Type == input_type::Button);
  bool const IsDownRightNow = Slot->ValueStore != 0.0f;
  return IsDownRightNow && Slot->Frame == Context->Frame;
}

auto
::InputButtonWasReleased(input_slot SlotHandle)
  -> bool
{
  impl_context* Context{};
  impl_slot* Slot{};
  DisassembleSlotHandle(SlotHandle, nullptr, nullptr, &Context, &Slot);

  if(Context == nullptr || Slot == nullptr)
  {
    return false;
  }

  Assert(Slot->Type == input_type::Button);
  bool const IsUpRightNow = Slot->ValueStore == 0.0f;
  return IsUpRightNow && Slot->Frame == Context->Frame;
}

auto
::InputSetButton(input_slot SlotHandle, bool IsDown)
  -> void
{
  impl_context* Context{};
  impl_slot* Slot{};
  DisassembleSlotHandle(SlotHandle, nullptr, nullptr, &Context, &Slot);

  if(Context && Slot)
  {
    Assert(Slot->Type == input_type::Button);
    UpdateSlotValue(Context, SlotHandle, Slot, IsDown ? 1.0f : 0.0f);
  }
}


auto
::InputPositiveDeadZone(input_slot SlotHandle)
  -> float
{
  auto Slot = GetSlot(SlotHandle);
  return Slot->PositiveDeadZone;
}

auto
::InputSetPositiveDeadZone(input_slot SlotHandle, float NewValue)
  -> void
{
  auto Slot = GetSlot(SlotHandle);
  Slot->PositiveDeadZone = NewValue;
}

auto
::InputNegativeDeadZone(input_slot SlotHandle)
  -> float
{
  auto Slot = GetSlot(SlotHandle);
  return Slot->NegativeDeadZone;
}

auto
::InputSetNegativeDeadZone(input_slot SlotHandle, float NewValue)
  -> void
{
  auto Slot = GetSlot(SlotHandle);
  Slot->NegativeDeadZone = NewValue;
}

auto
::InputSensitivity(input_slot SlotHandle)
  -> float
{
  auto Slot = GetSlot(SlotHandle);
  return Slot->Sensitivity;
}

auto
::InputSetSensitivity(input_slot SlotHandle, float NewValue)
  -> void
{
  auto Slot = GetSlot(SlotHandle);
  Slot->Sensitivity = NewValue;
}

auto
::InputExponent(input_slot SlotHandle)
  -> float
{
  auto Slot = GetSlot(SlotHandle);
  return Slot->Exponent;
}

auto
::InputSetExponent(input_slot SlotHandle, float NewValue)
  -> void
{
  auto Slot = GetSlot(SlotHandle);
  Slot->Exponent = NewValue;
}



auto
::InputCreateContext(allocator_interface* Allocator)
  -> input_context
{
  size_t ContextIndex = IntMaxValue<size_t>();

  // Find a free slot.
  for(size_t Index = 0; Index < GetGlobalContextRegistry().Num; ++Index)
  {
    if(GetGlobalContextRegistry()[Index] == nullptr)
    {
      ContextIndex = Index;
      break;
    }
  }

  if(ContextIndex == IntMaxValue<size_t>())
  {
    ContextIndex = GetGlobalContextRegistry().Num;
    Expand(GetGlobalContextRegistry());
  }

  auto Context = New<impl_context>(*Allocator, Allocator);
  GetGlobalContextRegistry()[ContextIndex] = Context;

  auto ContextHandle = MakeContextHandle(ContextIndex);
  return ContextHandle;
}

auto
::InputDestroyContext(allocator_interface* Allocator, input_context ContextHandle)
  -> void
{
  size_t ContextIndex{};
  impl_context* Context{};
  DisassembleContextHandle(ContextHandle, &ContextIndex, &Context);

  if(Context)
  {
    Delete(*Allocator, Context);
    GetGlobalContextRegistry()[ContextIndex] = nullptr;
  }
}

auto
::InputGetSlot(input_context ContextHandle, arc_string SlotName)
  -> input_slot
{
  size_t ContextIndex{};
  impl_context* Context{};
  DisassembleContextHandle(ContextHandle, &ContextIndex, &Context);

  if(Context == nullptr)
    return nullptr;

  auto const Predicate = [](impl_slot const& Slot, arc_string const& Name) { return Slot.Name == Name; };
  auto const SlotIndex = SliceCountUntil(AsConst(Slice(Context->Slots)), SlotName, Predicate);
  if(SlotIndex == INVALID_INDEX)
    return nullptr;

  return MakeSlotHandle(ContextIndex, SlotIndex);
}

auto
::InputRegisterSlot(input_context ContextHandle, input_type SlotType, arc_string SlotName)
  -> input_slot
{
  size_t ContextIndex{};
  impl_context* Context{};
  DisassembleContextHandle(ContextHandle, &ContextIndex, &Context);

  if(Context == nullptr)
    return nullptr;

  auto const Predicate = [](impl_slot const& Slot, arc_string const& Name) { return Slot.Name == Name; };
  auto const SlotIndex = SliceCountUntil(AsConst(Slice(Context->Slots)), SlotName, Predicate);
  if(SlotIndex != INVALID_INDEX)
  {
    // Slot already exists.
    return nullptr;
  }

  auto const NewSlotIndex = Context->Slots.Num;
  auto Slot = &Expand(Context->Slots);
  Slot->Name = SlotName;
  Slot->Type = SlotType;

  return MakeSlotHandle(ContextIndex, NewSlotIndex);
}

/// Maps changes done to \a SlotHandleFrom to \a SlotHandleTo, applying the given \a Scale.
auto
::InputAddSlotMapping(input_slot SlotHandleFrom,
                      input_slot SlotHandleTo,
                      float Scale)
  -> bool
{
  impl_context* Context{};
  DisassembleSlotHandle(SlotHandleFrom, nullptr, nullptr, &Context, nullptr);

  if(Context == nullptr)
    return false;

  {
    impl_context* OtherContext{};
    DisassembleSlotHandle(SlotHandleTo, nullptr, nullptr, &OtherContext, nullptr);

    if(OtherContext == nullptr || OtherContext != Context)
      return false;
  }

  // TODO(Manu): Eliminate duplicates.
  auto NewSlotMapping = &Expand(Context->SlotMappings);
  NewSlotMapping->Source = SlotHandleFrom;
  NewSlotMapping->Target = SlotHandleTo;
  NewSlotMapping->Scale = Scale;

  return true;
}

auto
::InputRemoveSlotMapping(input_slot SlotHandleFrom,
                         input_slot SlotHandleTo)
  -> bool
{
  // TODO: Implement.
  return false;
}

auto
::InputGetCharacterBuffer(input_context ContextHandle)
-> array<char>*
{
  impl_context* Context{};
  DisassembleContextHandle(ContextHandle, nullptr, &Context);

  if(Context == nullptr)
    return nullptr;

  return &Context->CharacterBuffer;
}

auto
::InputSetUserIndex(input_context ContextHandle, int UserIndex)
  -> void
{
  impl_context* Context{};
  DisassembleContextHandle(ContextHandle, nullptr, &Context);

  if(Context)
  {
    Context->UserIndex = UserIndex;
  }
}

auto
::InputGetUserIndex(input_context ContextHandle)
  -> int
{
  impl_context* Context{};
  DisassembleContextHandle(ContextHandle, nullptr, &Context);

  if(Context == nullptr)
    return -1;

  return Context->UserIndex;
}

auto
::InputBeginFrame(input_context ContextHandle)
  -> void
{
  impl_context* Context{};
  DisassembleContextHandle(ContextHandle, nullptr, &Context);

  if(Context)
  {
    // Context will never happen...
    Assert(Context->Frame < IntMaxValue<decltype(Context->Frame)>());
    Context->Frame++;

    Clear(Context->CharacterBuffer);
  }
}

auto
::InputEndFrame(input_context ContextHandle)
  -> void
{
  // Nothing to do here yet.
}


#include "Win32_Input.cpp.inl"
