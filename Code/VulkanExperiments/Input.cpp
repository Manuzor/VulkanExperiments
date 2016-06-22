#include "Input.hpp"


//
// input_slot
//

auto
::ButtonIsUp(input_slot const* Slot)
  -> bool
{
  return !ButtonIsDown(Slot);
}

auto
::SetButtonIsUp(input_slot* Slot, bool NewValue)
  -> void
{
  SetButtonIsDown(Slot, !NewValue);
}

auto
::ButtonIsDown(input_slot const* Slot)
  -> bool
{
  Assert(Slot->Type == input_type::Button);
  return Slot->Value != 0;
}

auto
::SetButtonIsDown(input_slot* Slot, bool NewValue)
  -> void
{
  Assert(Slot->Type == input_type::Button);
  Slot->Value = NewValue ? 1.0f : 0.0f;
}

auto
::AxisValue(input_slot const* Slot)
  -> float
{
  Assert(Slot->Type == input_type::Axis);
  return Slot->Value;
}

auto
::SetAxisValue(input_slot* Slot, float NewValue)
  -> void
{
  Assert(Slot->Type == input_type::Axis);
  Slot->Value = NewValue;
}

auto
::ActionValue(input_slot const* Slot)
  -> float
{
  Assert(Slot->Type == input_type::Action);
  return Slot->Value;
}

auto
::SetActionValue(input_slot* Slot, float NewValue)
  -> void
{
  Assert(Slot->Type == input_type::Action);
  Slot->Value = NewValue;
}

//
// input_value_properties
//

auto
::SetDeadZones(input_value_properties* Properties, float BothValues)
  -> void
{
  Properties->PositiveDeadZone = BothValues;
  Properties->NegativeDeadZone = BothValues;
}


//
// input_context
//

input_slot*
input_context::operator[](input_id SlotId)
{
  auto Slot = Get(&this->Slots, SlotId);
  if(Slot)
    return Slot;

  if(this->Parent)
    return (*this->Parent)[SlotId];

  return nullptr;
}

auto
::Init(input_context* Context, allocator_interface* Allocator)
  -> void
{
  Init(&Context->Slots,           Allocator);
  Init(&Context->ValueProperties, Allocator);
  Init(&Context->SlotMappings,    Allocator);
  Init(&Context->ChangeEvent,     Allocator);
  Init(&Context->CharacterBuffer, Allocator);
}

auto
::Finalize(input_context* Context)
  -> void
{
  Finalize(&Context->CharacterBuffer);
  Finalize(&Context->ChangeEvent);
  Finalize(&Context->SlotMappings);
  Finalize(&Context->ValueProperties);
  Finalize(&Context->Slots);
}

auto
::RegisterInputSlot(input_context* Context, input_type Type, input_id SlotId)
  -> void
{
  auto Slot = GetOrCreate(&Context->Slots, SlotId);
  Assert(Slot);

  Slot->Type = Type;
}

auto
::AddInputSlotMapping(input_context* Context, input_id SlotId, input_id TriggerId, float Scale)
  -> bool
{
  // TODO(Manu): Eliminate duplicates.
  auto NewSlotMapping = &Expand(&Context->SlotMappings);
  NewSlotMapping->SourceSlotId = SlotId;
  NewSlotMapping->TargetSlotId = SlotId;
  NewSlotMapping->Scale = Scale;

  return true;
}

auto
::RemoveInputTrigger(input_context* Context, input_id SlotId, input_id TriggerId)
  -> bool
{
  // TODO(Manu): Implement Context->
  return false;
}

/// Overload for booleans.
///
/// A boolean value is treated as $(D 0.0f) for $(D false) and $(D 1.0f) for
/// $(D true) values.
auto
::UpdateInputSlotValue(input_context* Context, input_id TriggeringSlotId, bool NewValue)
  -> bool
{
  return UpdateInputSlotValue(Context, TriggeringSlotId, NewValue ? 1.0f : 0.0f);
}

/// Return: Will return $(D false) if the slot does not exist.
auto
::UpdateInputSlotValue(input_context* Context, input_id TriggeringSlotId, float NewValue)
  -> bool
{
  auto TriggeringSlot = Get(&Context->Slots, TriggeringSlotId);
  if(TriggeringSlot == nullptr)
    return false;

  NewValue = AttuneInputValue(Context, TriggeringSlotId, NewValue);

  TriggeringSlot->Frame = Context->CurrentFrame;
  TriggeringSlot->Value = NewValue;

  //
  // Apply input mapping
  //

  for(auto& Mapping : Slice(&Context->SlotMappings))
  {
    if(Mapping.SourceSlotId == TriggeringSlotId)
    {
      float NewMappedValue = NewValue * Mapping.Scale;
      UpdateInputSlotValue(Context, Mapping.TargetSlotId, NewMappedValue);
    }
  }

  return true;
}

#include "Input_SystemInputSlots.hpp"

/// Applies special settings for the given input slot, if there are any, and
/// returns an adjusted value.
auto
::AttuneInputValue(input_context const* Context, input_id SlotId, float RawValue)
  -> float
{
  float NewValue = RawValue;

  auto Str = XInput::Unknown;

  auto Properties = Get(&Context->ValueProperties, SlotId);
  if(Properties == nullptr)
    return RawValue;

  if(NewValue > 0 && Properties->PositiveDeadZone > 0.0f)
  {
    NewValue = Max(0, NewValue - Properties->PositiveDeadZone) / (1.0f - Properties->PositiveDeadZone);
  }
  else if(NewValue < 0 && Properties->NegativeDeadZone > 0.0f)
  {
    NewValue = -Max(0, -NewValue - Properties->NegativeDeadZone) / (1.0f - Properties->NegativeDeadZone);
  }

  // Apply the exponent setting, if any.
  if(Properties->Exponent != 1.0f)
  {
    NewValue = Sign(NewValue) * (Pow(Abs(NewValue), Properties->Exponent));
  }

  // Scale the value.
  NewValue *= Properties->Sensitivity;

  return NewValue;
}

auto
::BeginInputFrame(input_context* Context)
  -> void
{
  // Context will never happen...
  Assert(Context->CurrentFrame < IntMaxValue<decltype(Context->CurrentFrame)>());
  Context->CurrentFrame++;

  Clear(&Context->CharacterBuffer);
}

auto
::EndInputFrame(input_context* Context)
  -> void
{
  auto Ids = Keys(&Context->Slots);
  auto Slots = Values(&Context->Slots);
  for(size_t Index = 0; Index < Context->Slots.Num; ++Index)
  {
    auto Slot = &Slots[Index];
    if(Slot->Frame < Context->CurrentFrame)
      continue;

    auto Id = Ids[Index];
    Context->ChangeEvent(Id, Slot);
  }
}


//
// System Input Slots
//

namespace XInput
{
  input_id Unknown = SliceFromString("XInput_Unknown");

  //
  // Buttons
  //
  input_id DPadUp      = SliceFromString("XInput_DPadUp");
  input_id DPadDown    = SliceFromString("XInput_DPadDown");
  input_id DPadLeft    = SliceFromString("XInput_DPadLeft");
  input_id DPadRight   = SliceFromString("XInput_DPadRight");
  input_id Start       = SliceFromString("XInput_Start");
  input_id Back        = SliceFromString("XInput_Back");
  input_id LeftThumb   = SliceFromString("XInput_LeftThumb");
  input_id RightThumb  = SliceFromString("XInput_RightThumb");
  input_id LeftBumper  = SliceFromString("XInput_LeftBumper");
  input_id RightBumper = SliceFromString("XInput_RightBumper");
  input_id A           = SliceFromString("XInput_A");
  input_id B           = SliceFromString("XInput_B");
  input_id X           = SliceFromString("XInput_X");
  input_id Y           = SliceFromString("XInput_Y");

  //
  // Axes
  //
  input_id LeftTrigger  = SliceFromString("XInput_LeftTrigger");
  input_id RightTrigger = SliceFromString("XInput_RightTrigger");
  input_id XLeftStick   = SliceFromString("XInput_XLeftStick");
  input_id YLeftStick   = SliceFromString("XInput_YLeftStick");
  input_id XRightStick  = SliceFromString("XInput_XRightStick");
  input_id YRightStick  = SliceFromString("XInput_YRightStick");
}

namespace Mouse
{
  input_id Unknown = SliceFromString("Mouse_Unknown");

  //
  // Buttons
  //
  input_id LeftButton   = SliceFromString("Mouse_LeftButton");
  input_id MiddleButton = SliceFromString("Mouse_MiddleButton");
  input_id RightButton  = SliceFromString("Mouse_RightButton");
  input_id ExtraButton1 = SliceFromString("Mouse_ExtraButton1");
  input_id ExtraButton2 = SliceFromString("Mouse_ExtraButton2");

  //
  // Axes
  //
  input_id XPosition = SliceFromString("Mouse_XPosition");
  input_id YPosition = SliceFromString("Mouse_YPosition");

  //
  // Actions
  //
  input_id XDelta                   = SliceFromString("Mouse_XDelta");
  input_id YDelta                   = SliceFromString("Mouse_YDelta");
  input_id VerticalWheelDelta       = SliceFromString("Mouse_VerticalWheelDelta");
  input_id HorizontalWheelDelta     = SliceFromString("Mouse_HorizontalWheelDelta");
  input_id LeftButton_DoubleClick   = SliceFromString("Mouse_LeftButton_DoubleClick");
  input_id MiddleButton_DoubleClick = SliceFromString("Mouse_MiddleButton_DoubleClick");
  input_id RightButton_DoubleClick  = SliceFromString("Mouse_RightButton_DoubleClick");
  input_id ExtraButton1_DoubleClick = SliceFromString("Mouse_ExtraButton1_DoubleClick");
  input_id ExtraButton2_DoubleClick = SliceFromString("Mouse_ExtraButton2_DoubleClick");
}

namespace Keyboard
{
  input_id Unknown = SliceFromString("Keyboard_Unknown");

  input_id Escape       = SliceFromString("Keyboard_Escape");
  input_id Space        = SliceFromString("Keyboard_Space");
  input_id Tab          = SliceFromString("Keyboard_Tab");
  input_id LeftShift    = SliceFromString("Keyboard_LeftShift");
  input_id LeftControl  = SliceFromString("Keyboard_LeftControl");
  input_id LeftAlt      = SliceFromString("Keyboard_LeftAlt");
  input_id LeftSystem   = SliceFromString("Keyboard_LeftSystem");
  input_id RightShift   = SliceFromString("Keyboard_RightShift");
  input_id RightControl = SliceFromString("Keyboard_RightControl");
  input_id RightAlt     = SliceFromString("Keyboard_RightAlt");
  input_id RightSystem  = SliceFromString("Keyboard_RightSystem");
  input_id Application  = SliceFromString("Keyboard_Application");
  input_id Backspace    = SliceFromString("Keyboard_Backspace");
  input_id Return       = SliceFromString("Keyboard_Return");

  input_id Insert   = SliceFromString("Keyboard_Insert");
  input_id Delete   = SliceFromString("Keyboard_Delete");
  input_id Home     = SliceFromString("Keyboard_Home");
  input_id End      = SliceFromString("Keyboard_End");
  input_id PageUp   = SliceFromString("Keyboard_PageUp");
  input_id PageDown = SliceFromString("Keyboard_PageDown");

  input_id Up    = SliceFromString("Keyboard_Up");
  input_id Down  = SliceFromString("Keyboard_Down");
  input_id Left  = SliceFromString("Keyboard_Left");
  input_id Right = SliceFromString("Keyboard_Right");

  //
  // Digit Keys
  //
  input_id Digit_0  = SliceFromString("Keyboard_Digit_0");
  input_id Digit_1  = SliceFromString("Keyboard_Digit_1");
  input_id Digit_2  = SliceFromString("Keyboard_Digit_2");
  input_id Digit_3  = SliceFromString("Keyboard_Digit_3");
  input_id Digit_4  = SliceFromString("Keyboard_Digit_4");
  input_id Digit_5  = SliceFromString("Keyboard_Digit_5");
  input_id Digit_6  = SliceFromString("Keyboard_Digit_6");
  input_id Digit_7  = SliceFromString("Keyboard_Digit_7");
  input_id Digit_8  = SliceFromString("Keyboard_Digit_8");
  input_id Digit_9  = SliceFromString("Keyboard_Digit_9");

  //
  // Numpad
  //
  input_id Numpad_Add      = SliceFromString("Keyboard_Numpad_Add");
  input_id Numpad_Subtract = SliceFromString("Keyboard_Numpad_Subtract");
  input_id Numpad_Multiply = SliceFromString("Keyboard_Numpad_Multiply");
  input_id Numpad_Divide   = SliceFromString("Keyboard_Numpad_Divide");
  input_id Numpad_Decimal  = SliceFromString("Keyboard_Numpad_Decimal");
  input_id Numpad_Enter    = SliceFromString("Keyboard_Numpad_Enter");

  input_id Numpad_0 = SliceFromString("Keyboard_Numpad_0");
  input_id Numpad_1 = SliceFromString("Keyboard_Numpad_1");
  input_id Numpad_2 = SliceFromString("Keyboard_Numpad_2");
  input_id Numpad_3 = SliceFromString("Keyboard_Numpad_3");
  input_id Numpad_4 = SliceFromString("Keyboard_Numpad_4");
  input_id Numpad_5 = SliceFromString("Keyboard_Numpad_5");
  input_id Numpad_6 = SliceFromString("Keyboard_Numpad_6");
  input_id Numpad_7 = SliceFromString("Keyboard_Numpad_7");
  input_id Numpad_8 = SliceFromString("Keyboard_Numpad_8");
  input_id Numpad_9 = SliceFromString("Keyboard_Numpad_9");

  //
  // F-Keys
  //
  input_id F1  = SliceFromString("Keyboard_F1");
  input_id F2  = SliceFromString("Keyboard_F2");
  input_id F3  = SliceFromString("Keyboard_F3");
  input_id F4  = SliceFromString("Keyboard_F4");
  input_id F5  = SliceFromString("Keyboard_F5");
  input_id F6  = SliceFromString("Keyboard_F6");
  input_id F7  = SliceFromString("Keyboard_F7");
  input_id F8  = SliceFromString("Keyboard_F8");
  input_id F9  = SliceFromString("Keyboard_F9");
  input_id F10 = SliceFromString("Keyboard_F10");
  input_id F11 = SliceFromString("Keyboard_F11");
  input_id F12 = SliceFromString("Keyboard_F12");
  input_id F13 = SliceFromString("Keyboard_F13");
  input_id F14 = SliceFromString("Keyboard_F14");
  input_id F15 = SliceFromString("Keyboard_F15");
  input_id F16 = SliceFromString("Keyboard_F16");
  input_id F17 = SliceFromString("Keyboard_F17");
  input_id F18 = SliceFromString("Keyboard_F18");
  input_id F19 = SliceFromString("Keyboard_F19");
  input_id F20 = SliceFromString("Keyboard_F20");
  input_id F21 = SliceFromString("Keyboard_F21");
  input_id F22 = SliceFromString("Keyboard_F22");
  input_id F23 = SliceFromString("Keyboard_F23");
  input_id F24 = SliceFromString("Keyboard_F24");

  //
  // Keys
  //
  input_id A = SliceFromString("Keyboard_A");
  input_id B = SliceFromString("Keyboard_B");
  input_id C = SliceFromString("Keyboard_C");
  input_id D = SliceFromString("Keyboard_D");
  input_id E = SliceFromString("Keyboard_E");
  input_id F = SliceFromString("Keyboard_F");
  input_id G = SliceFromString("Keyboard_G");
  input_id H = SliceFromString("Keyboard_H");
  input_id I = SliceFromString("Keyboard_I");
  input_id J = SliceFromString("Keyboard_J");
  input_id K = SliceFromString("Keyboard_K");
  input_id L = SliceFromString("Keyboard_L");
  input_id M = SliceFromString("Keyboard_M");
  input_id N = SliceFromString("Keyboard_N");
  input_id O = SliceFromString("Keyboard_O");
  input_id P = SliceFromString("Keyboard_P");
  input_id Q = SliceFromString("Keyboard_Q");
  input_id R = SliceFromString("Keyboard_R");
  input_id S = SliceFromString("Keyboard_S");
  input_id T = SliceFromString("Keyboard_T");
  input_id U = SliceFromString("Keyboard_U");
  input_id V = SliceFromString("Keyboard_V");
  input_id W = SliceFromString("Keyboard_W");
  input_id X = SliceFromString("Keyboard_X");
  input_id Y = SliceFromString("Keyboard_Y");
  input_id Z = SliceFromString("Keyboard_Z");
}
