#pragma once

#include <Backbone.hpp>

#include "Dictionary.hpp"
#include "Event.hpp"

using input_id = slice<char const>;


enum class input_type
{
  INVALID,

  /// Only valid for a single frame.
  Action,

  /// Has a persistent on/off state.
  Button,

  /// Has a persistent floating point state.
  Axis,
};

struct input_slot
{
  /// The type of this slot.
  input_type Type;

  /// The value of the slot.
  ///
  /// Interpretation depends on the $(D Type) of this slot.
  /// Note(Manu): Consider using the property functions below as they do type
  /// checking for you and express the purpose more clearly.
  float Value;

  /// The frame in which this value was updated.
  uint64 Frame;
};

bool
ButtonIsUp(input_slot const* Slot);

void
SetButtonIsUp(input_slot* Slot, bool Value);

bool
ButtonIsDown(input_slot const* Slot);

void
SetButtonIsDown(input_slot* Slot, bool NewValue);

float
AxisValue(input_slot const* Slot);

void
SetAxisValue(input_slot* Slot, float NewValue);

float
ActionValue(input_slot const* Slot);

void
SetActionValue(input_slot* Slot, float NewValue);

struct input_value_properties
{
  float PositiveDeadZone = 0.0f; // [0 .. 1]
  float NegativeDeadZone = 0.0f; // [0 .. 1]
  float Sensitivity = 1.0f;
  float Exponent = 1.0f;
};

void
SetDeadZones(input_value_properties* Properties, float BothValues);

// Sample signature: void Listener(input_id SlotId, input_slot Slot)
using input_event = event<input_id, input_slot*>;

// TODO(Manu): Implement ActionEvent so that users can listen to a specific action.
struct input_context
{
  struct slot_mapping
  {
    input_id SourceSlotId; // When this slots' value is changed, the TargetSlotId will be changed.
    input_id TargetSlotId; // This is the slot which value will be changed if SourceSlotId's value changes.
    float Scale;          // A factor used when mapping slots.
  };

  input_context* Parent;
  int UserIndex = -1; // -1 for no associated user, >= 0 for a specific user.
  slice<char> Name; // Mostly for debugging.

  dictionary<input_id, input_slot> Slots;
  dictionary<input_id, input_value_properties> ValueProperties;
  dynamic_array<slot_mapping> SlotMappings;
  input_event ChangeEvent;

  uint64 CurrentFrame;

  dynamic_array<char> CharacterBuffer;

  // Convenient indexing operator that returns nullptr or the ptr to the
  // input_slot that corresponds to SlotId.
  input_slot* operator[](input_id SlotId);
};

void
Init(input_context* Context, allocator_interface* Allocator);

void
Finalize(input_context* Context);

void
RegisterInputSlot(input_context* Context, input_type Type, input_id SlotId);

bool
AddInputSlotMapping(input_context* Context, input_id SlotId, input_id TriggerId, float Scale = 1.0f);

bool
RemoveInputTrigger(input_context* Context, input_id SlotId, input_id TriggerId);

/// Overload for booleans.
///
/// A boolean value is treated as \c 0.0f for \c false and \c 1.0f for \c true
/// values.
bool
UpdateInputSlotValue(input_context* Context, input_id TriggeringSlotId, bool NewValue);

/// Return: Will return $(D false) if the slot does not exist.
bool
UpdateInputSlotValue(input_context* Context, input_id TriggeringSlotId, float NewValue);

/// Applies special settings for the given input slot, if there are any, and
/// returns an adjusted value.
float
AttuneInputValue(input_context const* Context, input_id SlotId, float RawValue);

void
BeginInputFrame(input_context* Context);

void
EndInputFrame(input_context* Context);


//
// System Input Slots.
//

namespace XInput
{
  extern input_id Unknown;

  //
  // Buttons
  //
  extern input_id DPadUp;
  extern input_id DPadDown;
  extern input_id DPadLeft;
  extern input_id DPadRight;
  extern input_id Start;
  extern input_id Back;
  extern input_id LeftThumb;
  extern input_id RightThumb;
  extern input_id LeftBumper;
  extern input_id RightBumper;
  extern input_id A;
  extern input_id B;
  extern input_id X;
  extern input_id Y;

  //
  // Axes
  //
  extern input_id LeftTrigger;
  extern input_id RightTrigger;
  extern input_id XLeftStick;
  extern input_id YLeftStick;
  extern input_id XRightStick;
  extern input_id YRightStick;
};

namespace Mouse
{
  extern input_id Unknown;

  //
  // Buttons
  //
  extern input_id LeftButton;
  extern input_id MiddleButton;
  extern input_id RightButton;
  extern input_id ExtraButton1;
  extern input_id ExtraButton2;

  //
  // Axes
  //
  extern input_id XPosition;
  extern input_id YPosition;

  //
  // Actions
  //
  extern input_id XDelta;
  extern input_id YDelta;
  extern input_id VerticalWheelDelta;
  extern input_id HorizontalWheelDelta;
  extern input_id LeftButton_DoubleClick;
  extern input_id MiddleButton_DoubleClick;
  extern input_id RightButton_DoubleClick;
  extern input_id ExtraButton1_DoubleClick;
  extern input_id ExtraButton2_DoubleClick;
}

namespace Keyboard
{
  extern input_id Unknown;

  extern input_id Escape;
  extern input_id Space;
  extern input_id Tab;
  extern input_id LeftShift;
  extern input_id LeftControl;
  extern input_id LeftAlt;
  extern input_id LeftSystem;
  extern input_id RightShift;
  extern input_id RightControl;
  extern input_id RightAlt;
  extern input_id RightSystem;
  extern input_id Application;
  extern input_id Backspace;
  extern input_id Return;

  extern input_id Insert;
  extern input_id Delete;
  extern input_id Home;
  extern input_id End;
  extern input_id PageUp;
  extern input_id PageDown;

  extern input_id Up;
  extern input_id Down;
  extern input_id Left;
  extern input_id Right;

  //
  // Digit Keys
  //
  extern input_id Digit_0;
  extern input_id Digit_1;
  extern input_id Digit_2;
  extern input_id Digit_3;
  extern input_id Digit_4;
  extern input_id Digit_5;
  extern input_id Digit_6;
  extern input_id Digit_7;
  extern input_id Digit_8;
  extern input_id Digit_9;

  //
  // Numpad
  //
  extern input_id Numpad_Add;
  extern input_id Numpad_Subtract;
  extern input_id Numpad_Multiply;
  extern input_id Numpad_Divide;
  extern input_id Numpad_Decimal;
  extern input_id Numpad_Enter;

  extern input_id Numpad_0;
  extern input_id Numpad_1;
  extern input_id Numpad_2;
  extern input_id Numpad_3;
  extern input_id Numpad_4;
  extern input_id Numpad_5;
  extern input_id Numpad_6;
  extern input_id Numpad_7;
  extern input_id Numpad_8;
  extern input_id Numpad_9;

  //
  // F-Keys
  //
  extern input_id F1;
  extern input_id F2;
  extern input_id F3;
  extern input_id F4;
  extern input_id F5;
  extern input_id F6;
  extern input_id F7;
  extern input_id F8;
  extern input_id F9;
  extern input_id F10;
  extern input_id F11;
  extern input_id F12;
  extern input_id F13;
  extern input_id F14;
  extern input_id F15;
  extern input_id F16;
  extern input_id F17;
  extern input_id F18;
  extern input_id F19;
  extern input_id F20;
  extern input_id F21;
  extern input_id F22;
  extern input_id F23;
  extern input_id F24;

  //
  // Keys
  //
  extern input_id A;
  extern input_id B;
  extern input_id C;
  extern input_id D;
  extern input_id E;
  extern input_id F;
  extern input_id G;
  extern input_id H;
  extern input_id I;
  extern input_id J;
  extern input_id K;
  extern input_id L;
  extern input_id M;
  extern input_id N;
  extern input_id O;
  extern input_id P;
  extern input_id Q;
  extern input_id R;
  extern input_id S;
  extern input_id T;
  extern input_id U;
  extern input_id V;
  extern input_id W;
  extern input_id X;
  extern input_id Y;
  extern input_id Z;
}
