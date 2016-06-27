#pragma once

#include "CoreAPI.hpp"
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

struct CORE_API input_slot
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

bool CORE_API
ButtonIsUp(input_slot const* Slot);

void CORE_API
SetButtonIsUp(input_slot* Slot, bool Value);

bool CORE_API
ButtonIsDown(input_slot const* Slot);

void CORE_API
SetButtonIsDown(input_slot* Slot, bool NewValue);

float CORE_API
AxisValue(input_slot const* Slot);

void CORE_API
SetAxisValue(input_slot* Slot, float NewValue);

float CORE_API
ActionValue(input_slot const* Slot);

void CORE_API
SetActionValue(input_slot* Slot, float NewValue);

struct CORE_API input_value_properties
{
  float PositiveDeadZone = 0.0f; // [0 .. 1]
  float NegativeDeadZone = 0.0f; // [0 .. 1]
  float Sensitivity = 1.0f;
  float Exponent = 1.0f;
};

void CORE_API
SetDeadZones(input_value_properties* Properties, float BothValues);

// Sample signature: void Listener(input_id SlotId, input_slot Slot)
using input_event = event<input_id, input_slot*>;

// TODO(Manu): Implement ActionEvent so that users can listen to a specific action.
class CORE_API input_context
{
public:
  struct slot_mapping
  {
    input_id SourceSlotId; // When this slots' value is changed, the TargetSlotId will be changed.
    input_id TargetSlotId; // This is the slot which value will be changed if SourceSlotId's value changes.
    float Scale;           // A factor used when mapping slots.
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

  virtual ~input_context() = 0;
};

void CORE_API
Init(input_context* Context, allocator_interface* Allocator);

void CORE_API
Finalize(input_context* Context);

void CORE_API
RegisterInputSlot(input_context* Context, input_type Type, input_id SlotId);

bool CORE_API
AddInputSlotMapping(input_context* Context, input_id SlotId, input_id TriggerId, float Scale = 1.0f);

bool CORE_API
RemoveInputTrigger(input_context* Context, input_id SlotId, input_id TriggerId);

/// Overload for booleans.
///
/// A boolean value is treated as \c 0.0f for \c false and \c 1.0f for \c true
/// values.
bool CORE_API
UpdateInputSlotValue(input_context* Context, input_id TriggeringSlotId, bool NewValue);

/// Return: Will return $(D false) if the slot does not exist.
bool CORE_API
UpdateInputSlotValue(input_context* Context, input_id TriggeringSlotId, float NewValue);

/// Applies special settings for the given input slot, if there are any, and
/// returns an adjusted value.
float CORE_API
AttuneInputValue(input_context const* Context, input_id SlotId, float RawValue);

void CORE_API
BeginInputFrame(input_context* Context);

void CORE_API
EndInputFrame(input_context* Context);


//
// System Input Slots.
//

namespace x_input
{
  CORE_API extern input_id Unknown;

  //
  // Buttons
  //
  CORE_API extern input_id DPadUp;
  CORE_API extern input_id DPadDown;
  CORE_API extern input_id DPadLeft;
  CORE_API extern input_id DPadRight;
  CORE_API extern input_id Start;
  CORE_API extern input_id Back;
  CORE_API extern input_id LeftThumb;
  CORE_API extern input_id RightThumb;
  CORE_API extern input_id LeftBumper;
  CORE_API extern input_id RightBumper;
  CORE_API extern input_id A;
  CORE_API extern input_id B;
  CORE_API extern input_id X;
  CORE_API extern input_id Y;

  //
  // Axes
  //
  CORE_API extern input_id LeftTrigger;
  CORE_API extern input_id RightTrigger;
  CORE_API extern input_id XLeftStick;
  CORE_API extern input_id YLeftStick;
  CORE_API extern input_id XRightStick;
  CORE_API extern input_id YRightStick;
};

namespace mouse
{
  CORE_API extern input_id Unknown;

  //
  // Buttons
  //
  CORE_API extern input_id LeftButton;
  CORE_API extern input_id MiddleButton;
  CORE_API extern input_id RightButton;
  CORE_API extern input_id ExtraButton1;
  CORE_API extern input_id ExtraButton2;

  //
  // Axes
  //
  CORE_API extern input_id XPosition;
  CORE_API extern input_id YPosition;

  //
  // Actions
  //
  CORE_API extern input_id XDelta;
  CORE_API extern input_id YDelta;
  CORE_API extern input_id VerticalWheelDelta;
  CORE_API extern input_id HorizontalWheelDelta;
  CORE_API extern input_id LeftButton_DoubleClick;
  CORE_API extern input_id MiddleButton_DoubleClick;
  CORE_API extern input_id RightButton_DoubleClick;
  CORE_API extern input_id ExtraButton1_DoubleClick;
  CORE_API extern input_id ExtraButton2_DoubleClick;
}

namespace keyboard
{
  CORE_API extern input_id Unknown;

  CORE_API extern input_id Escape;
  CORE_API extern input_id Space;
  CORE_API extern input_id Tab;
  CORE_API extern input_id LeftShift;
  CORE_API extern input_id LeftControl;
  CORE_API extern input_id LeftAlt;
  CORE_API extern input_id LeftSystem;
  CORE_API extern input_id RightShift;
  CORE_API extern input_id RightControl;
  CORE_API extern input_id RightAlt;
  CORE_API extern input_id RightSystem;
  CORE_API extern input_id Application;
  CORE_API extern input_id Backspace;
  CORE_API extern input_id Return;

  CORE_API extern input_id Insert;
  CORE_API extern input_id Delete;
  CORE_API extern input_id Home;
  CORE_API extern input_id End;
  CORE_API extern input_id PageUp;
  CORE_API extern input_id PageDown;

  CORE_API extern input_id Up;
  CORE_API extern input_id Down;
  CORE_API extern input_id Left;
  CORE_API extern input_id Right;

  //
  // Digit Keys
  //
  CORE_API extern input_id Digit_0;
  CORE_API extern input_id Digit_1;
  CORE_API extern input_id Digit_2;
  CORE_API extern input_id Digit_3;
  CORE_API extern input_id Digit_4;
  CORE_API extern input_id Digit_5;
  CORE_API extern input_id Digit_6;
  CORE_API extern input_id Digit_7;
  CORE_API extern input_id Digit_8;
  CORE_API extern input_id Digit_9;

  //
  // Numpad
  //
  CORE_API extern input_id Numpad_Add;
  CORE_API extern input_id Numpad_Subtract;
  CORE_API extern input_id Numpad_Multiply;
  CORE_API extern input_id Numpad_Divide;
  CORE_API extern input_id Numpad_Decimal;
  CORE_API extern input_id Numpad_Enter;

  CORE_API extern input_id Numpad_0;
  CORE_API extern input_id Numpad_1;
  CORE_API extern input_id Numpad_2;
  CORE_API extern input_id Numpad_3;
  CORE_API extern input_id Numpad_4;
  CORE_API extern input_id Numpad_5;
  CORE_API extern input_id Numpad_6;
  CORE_API extern input_id Numpad_7;
  CORE_API extern input_id Numpad_8;
  CORE_API extern input_id Numpad_9;

  //
  // F-Keys
  //
  CORE_API extern input_id F1;
  CORE_API extern input_id F2;
  CORE_API extern input_id F3;
  CORE_API extern input_id F4;
  CORE_API extern input_id F5;
  CORE_API extern input_id F6;
  CORE_API extern input_id F7;
  CORE_API extern input_id F8;
  CORE_API extern input_id F9;
  CORE_API extern input_id F10;
  CORE_API extern input_id F11;
  CORE_API extern input_id F12;
  CORE_API extern input_id F13;
  CORE_API extern input_id F14;
  CORE_API extern input_id F15;
  CORE_API extern input_id F16;
  CORE_API extern input_id F17;
  CORE_API extern input_id F18;
  CORE_API extern input_id F19;
  CORE_API extern input_id F20;
  CORE_API extern input_id F21;
  CORE_API extern input_id F22;
  CORE_API extern input_id F23;
  CORE_API extern input_id F24;

  //
  // Keys
  //
  CORE_API extern input_id A;
  CORE_API extern input_id B;
  CORE_API extern input_id C;
  CORE_API extern input_id D;
  CORE_API extern input_id E;
  CORE_API extern input_id F;
  CORE_API extern input_id G;
  CORE_API extern input_id H;
  CORE_API extern input_id I;
  CORE_API extern input_id J;
  CORE_API extern input_id K;
  CORE_API extern input_id L;
  CORE_API extern input_id M;
  CORE_API extern input_id N;
  CORE_API extern input_id O;
  CORE_API extern input_id P;
  CORE_API extern input_id Q;
  CORE_API extern input_id R;
  CORE_API extern input_id S;
  CORE_API extern input_id T;
  CORE_API extern input_id U;
  CORE_API extern input_id V;
  CORE_API extern input_id W;
  CORE_API extern input_id X;
  CORE_API extern input_id Y;
  CORE_API extern input_id Z;
}
