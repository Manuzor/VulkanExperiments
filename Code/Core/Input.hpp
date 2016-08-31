#pragma once

#include "CoreAPI.hpp"
#include "String.hpp"
#include <Backbone.hpp>


RESERVE_PREFIX(Input);

enum class input_type
{
  INVALID,

  /// Has a persistent floating point state.
  Axis,

  /// Only valid for a single frame.
  Action,

  /// Has a persistent on/off state.
  Button,
};

/// The handle to an input slot.
DefineOpaqueHandle(input_slot);

/// Sets the value of the given slot regardless of its type.
CORE_API
float
InputGetSlotValueUnchecked(input_slot SlotHandle);

/// Sets the value of the given slot regardless of its type.
CORE_API
bool
InputSetSlotValueUnchecked(input_slot SlotHandle,
                           float NewValue);

CORE_API
float
InputActionValue(input_slot SlotHandle);

CORE_API
void
InputSetActionValue(input_slot SlotHandle, float NewValue);

CORE_API
float
InputAxisValue(input_slot SlotHandle);

CORE_API
void
InputSetAxisValue(input_slot SlotHandle, float NewValue);

/// Whether the button is currently being held down.
CORE_API
bool
InputButtonIsDown(input_slot SlotHandle);

/// Whether the button is currently not held down.
CORE_API
bool
InputButtonIsUp(input_slot SlotHandle);

/// Whether the button was pressed in this input frame.
CORE_API
bool
InputButtonWasPressed(input_slot SlotHandle);

/// Whether the button was released in this input frame.
CORE_API
bool
InputButtonWasReleased(input_slot SlotHandle);

CORE_API
void
InputSetButton(input_slot SlotHandle, bool IsDown);


CORE_API
float
InputPositiveDeadZone(input_slot SlotHandle);

CORE_API
void
InputSetPositiveDeadZone(input_slot SlotHandle, float NewValue);

CORE_API
float
InputNegativeDeadZone(input_slot SlotHandle);

CORE_API
void
InputSetNegativeDeadZone(input_slot SlotHandle, float NewValue);

inline void
InputSetDeadZone(input_slot SlotHandle, float BothValues)
{
  InputSetPositiveDeadZone(SlotHandle, BothValues);
  InputSetNegativeDeadZone(SlotHandle, BothValues);
}

CORE_API
float
InputSensitivity(input_slot SlotHandle);

CORE_API
void
InputSetSensitivity(input_slot SlotHandle, float NewValue);

CORE_API
float
InputExponent(input_slot SlotHandle);

CORE_API
void
InputSetExponent(input_slot SlotHandle, float NewValue);



DefineOpaqueHandle(input_context);

CORE_API
input_context
InputCreateContext(allocator_interface* Allocator);

CORE_API
void
InputDestroyContext(allocator_interface* Allocator, input_context ContextHandle);

CORE_API
input_slot
InputGetSlot(input_context ContextHandle, arc_string SlotName);

CORE_API
input_slot
InputRegisterSlot(input_context ContextHandle, input_type SlotType, arc_string SlotName);

/// Maps changes done to \a SlotHandleFrom to \a SlotHandleTo, applying the given \a Scale.
CORE_API
bool
InputAddSlotMapping(input_slot SlotHandleFrom,
                    input_slot SlotHandleTo,
                    float Scale = 1.0f);

CORE_API
bool
InputRemoveSlotMapping(input_slot SlotHandleFrom,
                       input_slot SlotHandleTo);

CORE_API
array<char>*
InputGetCharacterBuffer(input_context ContextHandle);

CORE_API
void
InputSetUserIndex(input_context ContextHandle, int UserIndex);

CORE_API
int
InputGetUserIndex(input_context ContextHandle);

CORE_API
void
InputBeginFrame(input_context ContextHandle);

CORE_API
void
InputEndFrame(input_context ContextHandle);


//
// System Input Slots.
//
struct input_x_input_slots
{
  input_slot Unknown;

  //
  // Buttons
  //
  input_slot DPadUp;
  input_slot DPadDown;
  input_slot DPadLeft;
  input_slot DPadRight;
  input_slot Start;
  input_slot Back;
  input_slot LeftThumb;
  input_slot RightThumb;
  input_slot LeftBumper;
  input_slot RightBumper;
  input_slot A;
  input_slot B;
  input_slot X;
  input_slot Y;

  //
  // Axes
  //
  input_slot LeftTrigger;
  input_slot RightTrigger;
  input_slot XLeftStick;
  input_slot YLeftStick;
  input_slot XRightStick;
  input_slot YRightStick;
};

struct input_mouse_slots
{
  input_slot Unknown;

  //
  // Buttons
  //
  input_slot LeftButton;
  input_slot MiddleButton;
  input_slot RightButton;
  input_slot ExtraButton1;
  input_slot ExtraButton2;

  //
  // Axes
  //
  input_slot XPosition;
  input_slot YPosition;

  //
  // Actions
  //
  input_slot XDelta;
  input_slot YDelta;
  input_slot VerticalWheelDelta;
  input_slot HorizontalWheelDelta;
  input_slot LeftButton_DoubleClick;
  input_slot MiddleButton_DoubleClick;
  input_slot RightButton_DoubleClick;
  input_slot ExtraButton1_DoubleClick;
  input_slot ExtraButton2_DoubleClick;
};

struct input_keyboard_slots
{
  input_slot Unknown;

  input_slot Escape;
  input_slot Space;
  input_slot Tab;
  input_slot LeftShift;
  input_slot LeftControl;
  input_slot LeftAlt;
  input_slot LeftSystem;
  input_slot RightShift;
  input_slot RightControl;
  input_slot RightAlt;
  input_slot RightSystem;
  input_slot Shift;
  input_slot Control;
  input_slot Alt;
  input_slot System;
  input_slot Application;
  input_slot Backspace;
  input_slot Return;

  input_slot Insert;
  input_slot Delete;
  input_slot Home;
  input_slot End;
  input_slot PageUp;
  input_slot PageDown;

  input_slot Up;
  input_slot Down;
  input_slot Left;
  input_slot Right;

  //
  // Digit Keys
  //
  input_slot Digit_0;
  input_slot Digit_1;
  input_slot Digit_2;
  input_slot Digit_3;
  input_slot Digit_4;
  input_slot Digit_5;
  input_slot Digit_6;
  input_slot Digit_7;
  input_slot Digit_8;
  input_slot Digit_9;

  //
  // Numpad
  //
  input_slot Numpad_Add;
  input_slot Numpad_Subtract;
  input_slot Numpad_Multiply;
  input_slot Numpad_Divide;
  input_slot Numpad_Decimal;
  input_slot Numpad_Enter;

  input_slot Numpad_0;
  input_slot Numpad_1;
  input_slot Numpad_2;
  input_slot Numpad_3;
  input_slot Numpad_4;
  input_slot Numpad_5;
  input_slot Numpad_6;
  input_slot Numpad_7;
  input_slot Numpad_8;
  input_slot Numpad_9;

  //
  // F-Keys
  //
  input_slot F1;
  input_slot F2;
  input_slot F3;
  input_slot F4;
  input_slot F5;
  input_slot F6;
  input_slot F7;
  input_slot F8;
  input_slot F9;
  input_slot F10;
  input_slot F11;
  input_slot F12;
  input_slot F13;
  input_slot F14;
  input_slot F15;
  input_slot F16;
  input_slot F17;
  input_slot F18;
  input_slot F19;
  input_slot F20;
  input_slot F21;
  input_slot F22;
  input_slot F23;
  input_slot F24;

  //
  // Keys
  //
  input_slot A;
  input_slot B;
  input_slot C;
  input_slot D;
  input_slot E;
  input_slot F;
  input_slot G;
  input_slot H;
  input_slot I;
  input_slot J;
  input_slot K;
  input_slot L;
  input_slot M;
  input_slot N;
  input_slot O;
  input_slot P;
  input_slot Q;
  input_slot R;
  input_slot S;
  input_slot T;
  input_slot U;
  input_slot V;
  input_slot W;
  input_slot X;
  input_slot Y;
  input_slot Z;
};

namespace input_x_input_slot_names
{
  constexpr char const* const Unknown = "XInput_Unknown";

  //
  // Buttons
  //
  constexpr char const* const DPadUp      = "XInput_DPadUp";
  constexpr char const* const DPadDown    = "XInput_DPadDown";
  constexpr char const* const DPadLeft    = "XInput_DPadLeft";
  constexpr char const* const DPadRight   = "XInput_DPadRight";
  constexpr char const* const Start       = "XInput_Start";
  constexpr char const* const Back        = "XInput_Back";
  constexpr char const* const LeftThumb   = "XInput_LeftThumb";
  constexpr char const* const RightThumb  = "XInput_RightThumb";
  constexpr char const* const LeftBumper  = "XInput_LeftBumper";
  constexpr char const* const RightBumper = "XInput_RightBumper";
  constexpr char const* const A           = "XInput_A";
  constexpr char const* const B           = "XInput_B";
  constexpr char const* const X           = "XInput_X";
  constexpr char const* const Y           = "XInput_Y";

  //
  // Axes
  //
  constexpr char const* const LeftTrigger  = "XInput_LeftTrigger";
  constexpr char const* const RightTrigger = "XInput_RightTrigger";
  constexpr char const* const XLeftStick   = "XInput_XLeftStick";
  constexpr char const* const YLeftStick   = "XInput_YLeftStick";
  constexpr char const* const XRightStick  = "XInput_XRightStick";
  constexpr char const* const YRightStick  = "XInput_YRightStick";
}

namespace input_mouse_slot_names
{
  constexpr char const* const Unknown = "Mouse_Unknown";

  //
  // Buttons
  //
  constexpr char const* const LeftButton   = "Mouse_LeftButton";
  constexpr char const* const MiddleButton = "Mouse_MiddleButton";
  constexpr char const* const RightButton  = "Mouse_RightButton";
  constexpr char const* const ExtraButton1 = "Mouse_ExtraButton1";
  constexpr char const* const ExtraButton2 = "Mouse_ExtraButton2";

  //
  // Axes
  //
  constexpr char const* const XPosition = "Mouse_XPosition";
  constexpr char const* const YPosition = "Mouse_YPosition";

  //
  // Actions
  //
  constexpr char const* const XDelta                   = "Mouse_XDelta";
  constexpr char const* const YDelta                   = "Mouse_YDelta";
  constexpr char const* const VerticalWheelDelta       = "Mouse_VerticalWheelDelta";
  constexpr char const* const HorizontalWheelDelta     = "Mouse_HorizontalWheelDelta";
  constexpr char const* const LeftButton_DoubleClick   = "Mouse_LeftButton_DoubleClick";
  constexpr char const* const MiddleButton_DoubleClick = "Mouse_MiddleButton_DoubleClick";
  constexpr char const* const RightButton_DoubleClick  = "Mouse_RightButton_DoubleClick";
  constexpr char const* const ExtraButton1_DoubleClick = "Mouse_ExtraButton1_DoubleClick";
  constexpr char const* const ExtraButton2_DoubleClick = "Mouse_ExtraButton2_DoubleClick";
}

namespace input_keyboard_slot_names
{
  constexpr char const* const Unknown = "Keyboard_Unknown";

  constexpr char const* const Escape       = "Keyboard_Escape";
  constexpr char const* const Space        = "Keyboard_Space";
  constexpr char const* const Tab          = "Keyboard_Tab";
  constexpr char const* const LeftShift    = "Keyboard_LeftShift";
  constexpr char const* const LeftControl  = "Keyboard_LeftControl";
  constexpr char const* const LeftAlt      = "Keyboard_LeftAlt";
  constexpr char const* const LeftSystem   = "Keyboard_LeftSystem";
  constexpr char const* const RightShift   = "Keyboard_RightShift";
  constexpr char const* const RightControl = "Keyboard_RightControl";
  constexpr char const* const RightAlt     = "Keyboard_RightAlt";
  constexpr char const* const RightSystem  = "Keyboard_RightSystem";
  constexpr char const* const Shift        = "Keyboard_Shift";   // Either left or right
  constexpr char const* const Control      = "Keyboard_Control"; // Either left or right
  constexpr char const* const Alt          = "Keyboard_Alt";     // Either left or right
  constexpr char const* const System       = "Keyboard_System";  // Either left or right
  constexpr char const* const Application  = "Keyboard_Application";
  constexpr char const* const Backspace    = "Keyboard_Backspace";
  constexpr char const* const Return       = "Keyboard_Return";

  constexpr char const* const Insert   = "Keyboard_Insert";
  constexpr char const* const Delete   = "Keyboard_Delete";
  constexpr char const* const Home     = "Keyboard_Home";
  constexpr char const* const End      = "Keyboard_End";
  constexpr char const* const PageUp   = "Keyboard_PageUp";
  constexpr char const* const PageDown = "Keyboard_PageDown";

  constexpr char const* const Up    = "Keyboard_Up";
  constexpr char const* const Down  = "Keyboard_Down";
  constexpr char const* const Left  = "Keyboard_Left";
  constexpr char const* const Right = "Keyboard_Right";

  //
  // Digit Keys
  //
  constexpr char const* const Digit_0  = "Keyboard_Digit_0";
  constexpr char const* const Digit_1  = "Keyboard_Digit_1";
  constexpr char const* const Digit_2  = "Keyboard_Digit_2";
  constexpr char const* const Digit_3  = "Keyboard_Digit_3";
  constexpr char const* const Digit_4  = "Keyboard_Digit_4";
  constexpr char const* const Digit_5  = "Keyboard_Digit_5";
  constexpr char const* const Digit_6  = "Keyboard_Digit_6";
  constexpr char const* const Digit_7  = "Keyboard_Digit_7";
  constexpr char const* const Digit_8  = "Keyboard_Digit_8";
  constexpr char const* const Digit_9  = "Keyboard_Digit_9";

  //
  // Numpad
  //
  constexpr char const* const Numpad_Add      = "Keyboard_Numpad_Add";
  constexpr char const* const Numpad_Subtract = "Keyboard_Numpad_Subtract";
  constexpr char const* const Numpad_Multiply = "Keyboard_Numpad_Multiply";
  constexpr char const* const Numpad_Divide   = "Keyboard_Numpad_Divide";
  constexpr char const* const Numpad_Decimal  = "Keyboard_Numpad_Decimal";
  constexpr char const* const Numpad_Enter    = "Keyboard_Numpad_Enter";

  constexpr char const* const Numpad_0 = "Keyboard_Numpad_0";
  constexpr char const* const Numpad_1 = "Keyboard_Numpad_1";
  constexpr char const* const Numpad_2 = "Keyboard_Numpad_2";
  constexpr char const* const Numpad_3 = "Keyboard_Numpad_3";
  constexpr char const* const Numpad_4 = "Keyboard_Numpad_4";
  constexpr char const* const Numpad_5 = "Keyboard_Numpad_5";
  constexpr char const* const Numpad_6 = "Keyboard_Numpad_6";
  constexpr char const* const Numpad_7 = "Keyboard_Numpad_7";
  constexpr char const* const Numpad_8 = "Keyboard_Numpad_8";
  constexpr char const* const Numpad_9 = "Keyboard_Numpad_9";

  //
  // F-Keys
  //
  constexpr char const* const F1  = "Keyboard_F1";
  constexpr char const* const F2  = "Keyboard_F2";
  constexpr char const* const F3  = "Keyboard_F3";
  constexpr char const* const F4  = "Keyboard_F4";
  constexpr char const* const F5  = "Keyboard_F5";
  constexpr char const* const F6  = "Keyboard_F6";
  constexpr char const* const F7  = "Keyboard_F7";
  constexpr char const* const F8  = "Keyboard_F8";
  constexpr char const* const F9  = "Keyboard_F9";
  constexpr char const* const F10 = "Keyboard_F10";
  constexpr char const* const F11 = "Keyboard_F11";
  constexpr char const* const F12 = "Keyboard_F12";
  constexpr char const* const F13 = "Keyboard_F13";
  constexpr char const* const F14 = "Keyboard_F14";
  constexpr char const* const F15 = "Keyboard_F15";
  constexpr char const* const F16 = "Keyboard_F16";
  constexpr char const* const F17 = "Keyboard_F17";
  constexpr char const* const F18 = "Keyboard_F18";
  constexpr char const* const F19 = "Keyboard_F19";
  constexpr char const* const F20 = "Keyboard_F20";
  constexpr char const* const F21 = "Keyboard_F21";
  constexpr char const* const F22 = "Keyboard_F22";
  constexpr char const* const F23 = "Keyboard_F23";
  constexpr char const* const F24 = "Keyboard_F24";

  //
  // Keys
  //
  constexpr char const* const A = "Keyboard_A";
  constexpr char const* const B = "Keyboard_B";
  constexpr char const* const C = "Keyboard_C";
  constexpr char const* const D = "Keyboard_D";
  constexpr char const* const E = "Keyboard_E";
  constexpr char const* const F = "Keyboard_F";
  constexpr char const* const G = "Keyboard_G";
  constexpr char const* const H = "Keyboard_H";
  constexpr char const* const I = "Keyboard_I";
  constexpr char const* const J = "Keyboard_J";
  constexpr char const* const K = "Keyboard_K";
  constexpr char const* const L = "Keyboard_L";
  constexpr char const* const M = "Keyboard_M";
  constexpr char const* const N = "Keyboard_N";
  constexpr char const* const O = "Keyboard_O";
  constexpr char const* const P = "Keyboard_P";
  constexpr char const* const Q = "Keyboard_Q";
  constexpr char const* const R = "Keyboard_R";
  constexpr char const* const S = "Keyboard_S";
  constexpr char const* const T = "Keyboard_T";
  constexpr char const* const U = "Keyboard_U";
  constexpr char const* const V = "Keyboard_V";
  constexpr char const* const W = "Keyboard_W";
  constexpr char const* const X = "Keyboard_X";
  constexpr char const* const Y = "Keyboard_Y";
  constexpr char const* const Z = "Keyboard_Z";
}
