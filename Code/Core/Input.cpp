#include "Input.hpp"

//
// input_id
//

auto
::InputId(char const* InputName)
  -> input_id
{
  return SliceFromString(InputName);
}

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
::AddInputSlotMapping(input_context* Context, input_id SourceSlotId, input_id TargetSlotId, float Scale)
  -> bool
{
  // TODO(Manu): Eliminate duplicates.
  auto NewSlotMapping = &Expand(&Context->SlotMappings);
  NewSlotMapping->SourceSlotId = SourceSlotId;
  NewSlotMapping->TargetSlotId = TargetSlotId;
  NewSlotMapping->Scale = Scale;

  return true;
}

auto
::RemoveInputTrigger(input_context* Context, input_id SourceSlotId, input_id TargetSlotId)
  -> bool
{
  // TODO(Manu): Implement
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
    {
      if(Slot->Type == input_type::Action)
      {
        Slot->Value = 0.0f;
      }

      continue;
    }

    auto Id = Ids[Index];
    Context->ChangeEvent(Id, Slot);
  }
}


//
// System Input Slots
//

namespace x_input
{
  CORE_API input_id Unknown = InputId("XInput_Unknown");

  //
  // Buttons
  //
  CORE_API input_id DPadUp      = InputId("XInput_DPadUp");
  CORE_API input_id DPadDown    = InputId("XInput_DPadDown");
  CORE_API input_id DPadLeft    = InputId("XInput_DPadLeft");
  CORE_API input_id DPadRight   = InputId("XInput_DPadRight");
  CORE_API input_id Start       = InputId("XInput_Start");
  CORE_API input_id Back        = InputId("XInput_Back");
  CORE_API input_id LeftThumb   = InputId("XInput_LeftThumb");
  CORE_API input_id RightThumb  = InputId("XInput_RightThumb");
  CORE_API input_id LeftBumper  = InputId("XInput_LeftBumper");
  CORE_API input_id RightBumper = InputId("XInput_RightBumper");
  CORE_API input_id A           = InputId("XInput_A");
  CORE_API input_id B           = InputId("XInput_B");
  CORE_API input_id X           = InputId("XInput_X");
  CORE_API input_id Y           = InputId("XInput_Y");

  //
  // Axes
  //
  CORE_API input_id LeftTrigger  = InputId("XInput_LeftTrigger");
  CORE_API input_id RightTrigger = InputId("XInput_RightTrigger");
  CORE_API input_id XLeftStick   = InputId("XInput_XLeftStick");
  CORE_API input_id YLeftStick   = InputId("XInput_YLeftStick");
  CORE_API input_id XRightStick  = InputId("XInput_XRightStick");
  CORE_API input_id YRightStick  = InputId("XInput_YRightStick");
}

namespace mouse
{
  CORE_API input_id Unknown = InputId("Mouse_Unknown");

  //
  // Buttons
  //
  CORE_API input_id LeftButton   = InputId("Mouse_LeftButton");
  CORE_API input_id MiddleButton = InputId("Mouse_MiddleButton");
  CORE_API input_id RightButton  = InputId("Mouse_RightButton");
  CORE_API input_id ExtraButton1 = InputId("Mouse_ExtraButton1");
  CORE_API input_id ExtraButton2 = InputId("Mouse_ExtraButton2");

  //
  // Axes
  //
  CORE_API input_id XPosition = InputId("Mouse_XPosition");
  CORE_API input_id YPosition = InputId("Mouse_YPosition");

  //
  // Actions
  //
  CORE_API input_id XDelta                   = InputId("Mouse_XDelta");
  CORE_API input_id YDelta                   = InputId("Mouse_YDelta");
  CORE_API input_id VerticalWheelDelta       = InputId("Mouse_VerticalWheelDelta");
  CORE_API input_id HorizontalWheelDelta     = InputId("Mouse_HorizontalWheelDelta");
  CORE_API input_id LeftButton_DoubleClick   = InputId("Mouse_LeftButton_DoubleClick");
  CORE_API input_id MiddleButton_DoubleClick = InputId("Mouse_MiddleButton_DoubleClick");
  CORE_API input_id RightButton_DoubleClick  = InputId("Mouse_RightButton_DoubleClick");
  CORE_API input_id ExtraButton1_DoubleClick = InputId("Mouse_ExtraButton1_DoubleClick");
  CORE_API input_id ExtraButton2_DoubleClick = InputId("Mouse_ExtraButton2_DoubleClick");
}

namespace keyboard
{
  CORE_API input_id Unknown = InputId("Keyboard_Unknown");

  CORE_API input_id Escape       = InputId("Keyboard_Escape");
  CORE_API input_id Space        = InputId("Keyboard_Space");
  CORE_API input_id Tab          = InputId("Keyboard_Tab");
  CORE_API input_id LeftShift    = InputId("Keyboard_LeftShift");
  CORE_API input_id LeftControl  = InputId("Keyboard_LeftControl");
  CORE_API input_id LeftAlt      = InputId("Keyboard_LeftAlt");
  CORE_API input_id LeftSystem   = InputId("Keyboard_LeftSystem");
  CORE_API input_id RightShift   = InputId("Keyboard_RightShift");
  CORE_API input_id RightControl = InputId("Keyboard_RightControl");
  CORE_API input_id RightAlt     = InputId("Keyboard_RightAlt");
  CORE_API input_id RightSystem  = InputId("Keyboard_RightSystem");
  CORE_API input_id Application  = InputId("Keyboard_Application");
  CORE_API input_id Backspace    = InputId("Keyboard_Backspace");
  CORE_API input_id Return       = InputId("Keyboard_Return");

  CORE_API input_id Insert   = InputId("Keyboard_Insert");
  CORE_API input_id Delete   = InputId("Keyboard_Delete");
  CORE_API input_id Home     = InputId("Keyboard_Home");
  CORE_API input_id End      = InputId("Keyboard_End");
  CORE_API input_id PageUp   = InputId("Keyboard_PageUp");
  CORE_API input_id PageDown = InputId("Keyboard_PageDown");

  CORE_API input_id Up    = InputId("Keyboard_Up");
  CORE_API input_id Down  = InputId("Keyboard_Down");
  CORE_API input_id Left  = InputId("Keyboard_Left");
  CORE_API input_id Right = InputId("Keyboard_Right");

  //
  // Digit Keys
  //
  CORE_API input_id Digit_0  = InputId("Keyboard_Digit_0");
  CORE_API input_id Digit_1  = InputId("Keyboard_Digit_1");
  CORE_API input_id Digit_2  = InputId("Keyboard_Digit_2");
  CORE_API input_id Digit_3  = InputId("Keyboard_Digit_3");
  CORE_API input_id Digit_4  = InputId("Keyboard_Digit_4");
  CORE_API input_id Digit_5  = InputId("Keyboard_Digit_5");
  CORE_API input_id Digit_6  = InputId("Keyboard_Digit_6");
  CORE_API input_id Digit_7  = InputId("Keyboard_Digit_7");
  CORE_API input_id Digit_8  = InputId("Keyboard_Digit_8");
  CORE_API input_id Digit_9  = InputId("Keyboard_Digit_9");

  //
  // Numpad
  //
  CORE_API input_id Numpad_Add      = InputId("Keyboard_Numpad_Add");
  CORE_API input_id Numpad_Subtract = InputId("Keyboard_Numpad_Subtract");
  CORE_API input_id Numpad_Multiply = InputId("Keyboard_Numpad_Multiply");
  CORE_API input_id Numpad_Divide   = InputId("Keyboard_Numpad_Divide");
  CORE_API input_id Numpad_Decimal  = InputId("Keyboard_Numpad_Decimal");
  CORE_API input_id Numpad_Enter    = InputId("Keyboard_Numpad_Enter");

  CORE_API input_id Numpad_0 = InputId("Keyboard_Numpad_0");
  CORE_API input_id Numpad_1 = InputId("Keyboard_Numpad_1");
  CORE_API input_id Numpad_2 = InputId("Keyboard_Numpad_2");
  CORE_API input_id Numpad_3 = InputId("Keyboard_Numpad_3");
  CORE_API input_id Numpad_4 = InputId("Keyboard_Numpad_4");
  CORE_API input_id Numpad_5 = InputId("Keyboard_Numpad_5");
  CORE_API input_id Numpad_6 = InputId("Keyboard_Numpad_6");
  CORE_API input_id Numpad_7 = InputId("Keyboard_Numpad_7");
  CORE_API input_id Numpad_8 = InputId("Keyboard_Numpad_8");
  CORE_API input_id Numpad_9 = InputId("Keyboard_Numpad_9");

  //
  // F-Keys
  //
  CORE_API input_id F1  = InputId("Keyboard_F1");
  CORE_API input_id F2  = InputId("Keyboard_F2");
  CORE_API input_id F3  = InputId("Keyboard_F3");
  CORE_API input_id F4  = InputId("Keyboard_F4");
  CORE_API input_id F5  = InputId("Keyboard_F5");
  CORE_API input_id F6  = InputId("Keyboard_F6");
  CORE_API input_id F7  = InputId("Keyboard_F7");
  CORE_API input_id F8  = InputId("Keyboard_F8");
  CORE_API input_id F9  = InputId("Keyboard_F9");
  CORE_API input_id F10 = InputId("Keyboard_F10");
  CORE_API input_id F11 = InputId("Keyboard_F11");
  CORE_API input_id F12 = InputId("Keyboard_F12");
  CORE_API input_id F13 = InputId("Keyboard_F13");
  CORE_API input_id F14 = InputId("Keyboard_F14");
  CORE_API input_id F15 = InputId("Keyboard_F15");
  CORE_API input_id F16 = InputId("Keyboard_F16");
  CORE_API input_id F17 = InputId("Keyboard_F17");
  CORE_API input_id F18 = InputId("Keyboard_F18");
  CORE_API input_id F19 = InputId("Keyboard_F19");
  CORE_API input_id F20 = InputId("Keyboard_F20");
  CORE_API input_id F21 = InputId("Keyboard_F21");
  CORE_API input_id F22 = InputId("Keyboard_F22");
  CORE_API input_id F23 = InputId("Keyboard_F23");
  CORE_API input_id F24 = InputId("Keyboard_F24");

  //
  // Keys
  //
  CORE_API input_id A = InputId("Keyboard_A");
  CORE_API input_id B = InputId("Keyboard_B");
  CORE_API input_id C = InputId("Keyboard_C");
  CORE_API input_id D = InputId("Keyboard_D");
  CORE_API input_id E = InputId("Keyboard_E");
  CORE_API input_id F = InputId("Keyboard_F");
  CORE_API input_id G = InputId("Keyboard_G");
  CORE_API input_id H = InputId("Keyboard_H");
  CORE_API input_id I = InputId("Keyboard_I");
  CORE_API input_id J = InputId("Keyboard_J");
  CORE_API input_id K = InputId("Keyboard_K");
  CORE_API input_id L = InputId("Keyboard_L");
  CORE_API input_id M = InputId("Keyboard_M");
  CORE_API input_id N = InputId("Keyboard_N");
  CORE_API input_id O = InputId("Keyboard_O");
  CORE_API input_id P = InputId("Keyboard_P");
  CORE_API input_id Q = InputId("Keyboard_Q");
  CORE_API input_id R = InputId("Keyboard_R");
  CORE_API input_id S = InputId("Keyboard_S");
  CORE_API input_id T = InputId("Keyboard_T");
  CORE_API input_id U = InputId("Keyboard_U");
  CORE_API input_id V = InputId("Keyboard_V");
  CORE_API input_id W = InputId("Keyboard_W");
  CORE_API input_id X = InputId("Keyboard_X");
  CORE_API input_id Y = InputId("Keyboard_Y");
  CORE_API input_id Z = InputId("Keyboard_Z");
}
