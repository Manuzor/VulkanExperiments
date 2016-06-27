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

input_context::~input_context()
{
}

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

/// Applies special settings for the given input slot, if there are any, and
/// returns an adjusted value.
auto
::AttuneInputValue(input_context const* Context, input_id SlotId, float RawValue)
  -> float
{
  float NewValue = RawValue;

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

namespace x_input
{
  CORE_API input_id Unknown = SliceFromString("XInput_Unknown");

  //
  // Buttons
  //
  CORE_API input_id DPadUp      = SliceFromString("XInput_DPadUp");
  CORE_API input_id DPadDown    = SliceFromString("XInput_DPadDown");
  CORE_API input_id DPadLeft    = SliceFromString("XInput_DPadLeft");
  CORE_API input_id DPadRight   = SliceFromString("XInput_DPadRight");
  CORE_API input_id Start       = SliceFromString("XInput_Start");
  CORE_API input_id Back        = SliceFromString("XInput_Back");
  CORE_API input_id LeftThumb   = SliceFromString("XInput_LeftThumb");
  CORE_API input_id RightThumb  = SliceFromString("XInput_RightThumb");
  CORE_API input_id LeftBumper  = SliceFromString("XInput_LeftBumper");
  CORE_API input_id RightBumper = SliceFromString("XInput_RightBumper");
  CORE_API input_id A           = SliceFromString("XInput_A");
  CORE_API input_id B           = SliceFromString("XInput_B");
  CORE_API input_id X           = SliceFromString("XInput_X");
  CORE_API input_id Y           = SliceFromString("XInput_Y");

  //
  // Axes
  //
  CORE_API input_id LeftTrigger  = SliceFromString("XInput_LeftTrigger");
  CORE_API input_id RightTrigger = SliceFromString("XInput_RightTrigger");
  CORE_API input_id XLeftStick   = SliceFromString("XInput_XLeftStick");
  CORE_API input_id YLeftStick   = SliceFromString("XInput_YLeftStick");
  CORE_API input_id XRightStick  = SliceFromString("XInput_XRightStick");
  CORE_API input_id YRightStick  = SliceFromString("XInput_YRightStick");
}

namespace mouse
{
  CORE_API input_id Unknown = SliceFromString("Mouse_Unknown");

  //
  // Buttons
  //
  CORE_API input_id LeftButton   = SliceFromString("Mouse_LeftButton");
  CORE_API input_id MiddleButton = SliceFromString("Mouse_MiddleButton");
  CORE_API input_id RightButton  = SliceFromString("Mouse_RightButton");
  CORE_API input_id ExtraButton1 = SliceFromString("Mouse_ExtraButton1");
  CORE_API input_id ExtraButton2 = SliceFromString("Mouse_ExtraButton2");

  //
  // Axes
  //
  CORE_API input_id XPosition = SliceFromString("Mouse_XPosition");
  CORE_API input_id YPosition = SliceFromString("Mouse_YPosition");

  //
  // Actions
  //
  CORE_API input_id XDelta                   = SliceFromString("Mouse_XDelta");
  CORE_API input_id YDelta                   = SliceFromString("Mouse_YDelta");
  CORE_API input_id VerticalWheelDelta       = SliceFromString("Mouse_VerticalWheelDelta");
  CORE_API input_id HorizontalWheelDelta     = SliceFromString("Mouse_HorizontalWheelDelta");
  CORE_API input_id LeftButton_DoubleClick   = SliceFromString("Mouse_LeftButton_DoubleClick");
  CORE_API input_id MiddleButton_DoubleClick = SliceFromString("Mouse_MiddleButton_DoubleClick");
  CORE_API input_id RightButton_DoubleClick  = SliceFromString("Mouse_RightButton_DoubleClick");
  CORE_API input_id ExtraButton1_DoubleClick = SliceFromString("Mouse_ExtraButton1_DoubleClick");
  CORE_API input_id ExtraButton2_DoubleClick = SliceFromString("Mouse_ExtraButton2_DoubleClick");
}

namespace keyboard
{
  CORE_API input_id Unknown = SliceFromString("Keyboard_Unknown");

  CORE_API input_id Escape       = SliceFromString("Keyboard_Escape");
  CORE_API input_id Space        = SliceFromString("Keyboard_Space");
  CORE_API input_id Tab          = SliceFromString("Keyboard_Tab");
  CORE_API input_id LeftShift    = SliceFromString("Keyboard_LeftShift");
  CORE_API input_id LeftControl  = SliceFromString("Keyboard_LeftControl");
  CORE_API input_id LeftAlt      = SliceFromString("Keyboard_LeftAlt");
  CORE_API input_id LeftSystem   = SliceFromString("Keyboard_LeftSystem");
  CORE_API input_id RightShift   = SliceFromString("Keyboard_RightShift");
  CORE_API input_id RightControl = SliceFromString("Keyboard_RightControl");
  CORE_API input_id RightAlt     = SliceFromString("Keyboard_RightAlt");
  CORE_API input_id RightSystem  = SliceFromString("Keyboard_RightSystem");
  CORE_API input_id Application  = SliceFromString("Keyboard_Application");
  CORE_API input_id Backspace    = SliceFromString("Keyboard_Backspace");
  CORE_API input_id Return       = SliceFromString("Keyboard_Return");

  CORE_API input_id Insert   = SliceFromString("Keyboard_Insert");
  CORE_API input_id Delete   = SliceFromString("Keyboard_Delete");
  CORE_API input_id Home     = SliceFromString("Keyboard_Home");
  CORE_API input_id End      = SliceFromString("Keyboard_End");
  CORE_API input_id PageUp   = SliceFromString("Keyboard_PageUp");
  CORE_API input_id PageDown = SliceFromString("Keyboard_PageDown");

  CORE_API input_id Up    = SliceFromString("Keyboard_Up");
  CORE_API input_id Down  = SliceFromString("Keyboard_Down");
  CORE_API input_id Left  = SliceFromString("Keyboard_Left");
  CORE_API input_id Right = SliceFromString("Keyboard_Right");

  //
  // Digit Keys
  //
  CORE_API input_id Digit_0  = SliceFromString("Keyboard_Digit_0");
  CORE_API input_id Digit_1  = SliceFromString("Keyboard_Digit_1");
  CORE_API input_id Digit_2  = SliceFromString("Keyboard_Digit_2");
  CORE_API input_id Digit_3  = SliceFromString("Keyboard_Digit_3");
  CORE_API input_id Digit_4  = SliceFromString("Keyboard_Digit_4");
  CORE_API input_id Digit_5  = SliceFromString("Keyboard_Digit_5");
  CORE_API input_id Digit_6  = SliceFromString("Keyboard_Digit_6");
  CORE_API input_id Digit_7  = SliceFromString("Keyboard_Digit_7");
  CORE_API input_id Digit_8  = SliceFromString("Keyboard_Digit_8");
  CORE_API input_id Digit_9  = SliceFromString("Keyboard_Digit_9");

  //
  // Numpad
  //
  CORE_API input_id Numpad_Add      = SliceFromString("Keyboard_Numpad_Add");
  CORE_API input_id Numpad_Subtract = SliceFromString("Keyboard_Numpad_Subtract");
  CORE_API input_id Numpad_Multiply = SliceFromString("Keyboard_Numpad_Multiply");
  CORE_API input_id Numpad_Divide   = SliceFromString("Keyboard_Numpad_Divide");
  CORE_API input_id Numpad_Decimal  = SliceFromString("Keyboard_Numpad_Decimal");
  CORE_API input_id Numpad_Enter    = SliceFromString("Keyboard_Numpad_Enter");

  CORE_API input_id Numpad_0 = SliceFromString("Keyboard_Numpad_0");
  CORE_API input_id Numpad_1 = SliceFromString("Keyboard_Numpad_1");
  CORE_API input_id Numpad_2 = SliceFromString("Keyboard_Numpad_2");
  CORE_API input_id Numpad_3 = SliceFromString("Keyboard_Numpad_3");
  CORE_API input_id Numpad_4 = SliceFromString("Keyboard_Numpad_4");
  CORE_API input_id Numpad_5 = SliceFromString("Keyboard_Numpad_5");
  CORE_API input_id Numpad_6 = SliceFromString("Keyboard_Numpad_6");
  CORE_API input_id Numpad_7 = SliceFromString("Keyboard_Numpad_7");
  CORE_API input_id Numpad_8 = SliceFromString("Keyboard_Numpad_8");
  CORE_API input_id Numpad_9 = SliceFromString("Keyboard_Numpad_9");

  //
  // F-Keys
  //
  CORE_API input_id F1  = SliceFromString("Keyboard_F1");
  CORE_API input_id F2  = SliceFromString("Keyboard_F2");
  CORE_API input_id F3  = SliceFromString("Keyboard_F3");
  CORE_API input_id F4  = SliceFromString("Keyboard_F4");
  CORE_API input_id F5  = SliceFromString("Keyboard_F5");
  CORE_API input_id F6  = SliceFromString("Keyboard_F6");
  CORE_API input_id F7  = SliceFromString("Keyboard_F7");
  CORE_API input_id F8  = SliceFromString("Keyboard_F8");
  CORE_API input_id F9  = SliceFromString("Keyboard_F9");
  CORE_API input_id F10 = SliceFromString("Keyboard_F10");
  CORE_API input_id F11 = SliceFromString("Keyboard_F11");
  CORE_API input_id F12 = SliceFromString("Keyboard_F12");
  CORE_API input_id F13 = SliceFromString("Keyboard_F13");
  CORE_API input_id F14 = SliceFromString("Keyboard_F14");
  CORE_API input_id F15 = SliceFromString("Keyboard_F15");
  CORE_API input_id F16 = SliceFromString("Keyboard_F16");
  CORE_API input_id F17 = SliceFromString("Keyboard_F17");
  CORE_API input_id F18 = SliceFromString("Keyboard_F18");
  CORE_API input_id F19 = SliceFromString("Keyboard_F19");
  CORE_API input_id F20 = SliceFromString("Keyboard_F20");
  CORE_API input_id F21 = SliceFromString("Keyboard_F21");
  CORE_API input_id F22 = SliceFromString("Keyboard_F22");
  CORE_API input_id F23 = SliceFromString("Keyboard_F23");
  CORE_API input_id F24 = SliceFromString("Keyboard_F24");

  //
  // Keys
  //
  CORE_API input_id A = SliceFromString("Keyboard_A");
  CORE_API input_id B = SliceFromString("Keyboard_B");
  CORE_API input_id C = SliceFromString("Keyboard_C");
  CORE_API input_id D = SliceFromString("Keyboard_D");
  CORE_API input_id E = SliceFromString("Keyboard_E");
  CORE_API input_id F = SliceFromString("Keyboard_F");
  CORE_API input_id G = SliceFromString("Keyboard_G");
  CORE_API input_id H = SliceFromString("Keyboard_H");
  CORE_API input_id I = SliceFromString("Keyboard_I");
  CORE_API input_id J = SliceFromString("Keyboard_J");
  CORE_API input_id K = SliceFromString("Keyboard_K");
  CORE_API input_id L = SliceFromString("Keyboard_L");
  CORE_API input_id M = SliceFromString("Keyboard_M");
  CORE_API input_id N = SliceFromString("Keyboard_N");
  CORE_API input_id O = SliceFromString("Keyboard_O");
  CORE_API input_id P = SliceFromString("Keyboard_P");
  CORE_API input_id Q = SliceFromString("Keyboard_Q");
  CORE_API input_id R = SliceFromString("Keyboard_R");
  CORE_API input_id S = SliceFromString("Keyboard_S");
  CORE_API input_id T = SliceFromString("Keyboard_T");
  CORE_API input_id U = SliceFromString("Keyboard_U");
  CORE_API input_id V = SliceFromString("Keyboard_V");
  CORE_API input_id W = SliceFromString("Keyboard_W");
  CORE_API input_id X = SliceFromString("Keyboard_X");
  CORE_API input_id Y = SliceFromString("Keyboard_Y");
  CORE_API input_id Z = SliceFromString("Keyboard_Z");
}
