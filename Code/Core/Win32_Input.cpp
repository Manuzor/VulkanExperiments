#include "Win32_Input.hpp"
#include "Log.hpp"

#include <WindowsX.h>


template<typename T>
bool
LoadHelper(HMODULE DLL, char const* FuncName, T** OutPtrToProcPtr)
{
  auto ProcPtr = GetProcAddress(DLL, FuncName);
  if(ProcPtr)
  {
    *OutPtrToProcPtr = Reinterpret<T*>(ProcPtr);
    return true;
  }
  return false;
}

auto
::Win32LoadXInput(x_input_dll* XInput, log_data* Log)
  -> bool
{
  char const* DLLNames[] =
  {
    XINPUT_DLL_A,
    "xinput1_4.dll",
    "xinput9_1_0.dll",
    "xinput1_3.dll",
  };

  LogBeginScope(Log, "Loading XInput");
  Defer [Log](){ LogEndScope(Log, ""); };

  for(auto DLLName : Slice(DLLNames))
  {
    auto DLL = LoadLibraryA(DLLName);
    if(!DLL)
    {
      LogWarning(Log, "Failed to load XInput DLL: %s", DLLName);
      continue;
    }

    LogInfo(Log, "Loaded XInput DLL: %s", DLLName);
    XInput->DLL = DLL;
    XInput->DLLName = SliceFromString(DLLName);

    #define TRY_LOAD(Name) if(!LoadHelper(DLL, #Name, &XInput->##Name)) \
    { \
      LogError(Log, "Failed to load procedure: %s", #Name); \
      return false; \
    }

    TRY_LOAD(XInputGetState);
    TRY_LOAD(XInputSetState);
    TRY_LOAD(XInputGetCapabilities);
    TRY_LOAD(XInputGetAudioDeviceIds);
    TRY_LOAD(XInputGetBatteryInformation);
    TRY_LOAD(XInputGetKeystroke);

    #undef TRY_LOAD

    return true;
  }

  LogError(Log, "Failed to load XInput: Couldn't find any of the known DLLs.");
  return false;
}


win32_input_context::~win32_input_context()
{
}

auto
::Init(win32_input_context* Context, allocator_interface* Allocator)
  -> void
{
  Init(Cast<input_context*>(Context), Allocator);
}

auto
::Finalize(win32_input_context* Context)
  -> void
{
  Finalize(Cast<input_context*>(Context));
}

auto
::Win32ProcessInputMessage(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam,
                         win32_input_context* Input, log_data* Log)
  -> bool
{
  //
  // Keyboard messages
  //
  if(Message == WM_CHAR || Message == WM_UNICHAR)
  {
    if(WParam == UNICODE_NOCHAR) return true;
    Expand(&Input->CharacterBuffer) = Cast<char>(WParam);
    return true;
  }

  if(Message >= WM_KEYFIRST && Message <= WM_KEYLAST)
  {
    //LogInfo(Log, "Keyboard message: %s", Win32MessageIdToString(Message));

    auto VKCode = WParam;
    bool WasDown = (LParam & (1 << 30)) != 0;
    bool IsDown = (LParam & (1 << 31)) == 0;

    if(WasDown == IsDown)
    {
      // No change.
      return true;
    }

    input_id KeyId = Win32VirtualKeyToInputId(VKCode, LParam);

    if(KeyId == nullptr)
    {
      LogWarning(Log, "Unable to map virtual key code %d (Hex: 0x%x)", VKCode, VKCode);
      return true;
    }

    UpdateInputSlotValue(Input, KeyId, IsDown);

    return true;
  }

  //
  // Mouse messages
  //
  if(Message >= WM_MOUSEFIRST && Message <= WM_MOUSELAST)
  {
    //
    // Mouse position
    //
    if(Message == WM_MOUSEMOVE)
    {
      auto XClientAreaMouse = GET_X_LPARAM(LParam);
      UpdateInputSlotValue(Input, mouse::XPosition, Cast<float>(XClientAreaMouse));

      auto YClientAreaMouse = GET_Y_LPARAM(LParam);
      UpdateInputSlotValue(Input, mouse::YPosition, Cast<float>(YClientAreaMouse));

      return true;
    }

    //
    // Mouse buttons and wheels
    //
    switch(Message)
    {
      case WM_LBUTTONUP:
      case WM_LBUTTONDOWN:
      {
        bool IsDown = Message == WM_LBUTTONDOWN;
        UpdateInputSlotValue(Input, mouse::LeftButton, IsDown);
      } break;

      case WM_RBUTTONUP:
      case WM_RBUTTONDOWN:
      {
        bool IsDown = Message == WM_RBUTTONDOWN;
        UpdateInputSlotValue(Input, mouse::RightButton, IsDown);
      } break;

      case WM_MBUTTONUP:
      case WM_MBUTTONDOWN:
      {
        bool IsDown = Message == WM_MBUTTONDOWN;
        UpdateInputSlotValue(Input, mouse::MiddleButton, IsDown);
      } break;

      case WM_XBUTTONUP:
      case WM_XBUTTONDOWN:
      {
        bool IsDown = Message == WM_XBUTTONDOWN;
        auto XButtonNumber = GET_XBUTTON_WPARAM(WParam);
        switch(XButtonNumber)
        {
          case 1: UpdateInputSlotValue(Input, mouse::ExtraButton1, IsDown); break;
          case 2: UpdateInputSlotValue(Input, mouse::ExtraButton2, IsDown); break;
          default: break;
        }
      } break;

      case WM_MOUSEWHEEL:
      {
        auto RawValue = GET_WHEEL_DELTA_WPARAM(WParam);
        auto Value = Cast<float>(RawValue) / WHEEL_DELTA;
        UpdateInputSlotValue(Input, mouse::VerticalWheelDelta, Value);
      } break;

      case WM_MOUSEHWHEEL:
      {
        auto RawValue = GET_WHEEL_DELTA_WPARAM(WParam);
        auto Value = Cast<float>(RawValue) / WHEEL_DELTA;
        UpdateInputSlotValue(Input, mouse::HorizontalWheelDelta, Value);
      } break;

      default: break;
    }

    return true;
  }

  //
  // Raw input messages.
  //
  if(Message == WM_INPUT)
  {
    UINT InputDataSize;
    GetRawInputData(Reinterpret<HRAWINPUT>(LParam), RID_INPUT, nullptr, &InputDataSize, sizeof(RAWINPUTHEADER));

    // Early out.
    if(InputDataSize == 0)
      return true;

    if(InputDataSize <= sizeof(RAWINPUT))
    {
      LogError(Log, "We are querying only for raw mouse input data, for which sizeof(RAWINPUT) should be enough.");
      Assert(false);
    }

    RAWINPUT InputData;
    const UINT BytesWritten = GetRawInputData(Reinterpret<HRAWINPUT>(LParam), RID_INPUT, &InputData, &InputDataSize, Cast<UINT>(sizeof(RAWINPUTHEADER)));
    if(BytesWritten != InputDataSize)
    {
      LogError(Log, "Failed to get raw input data.");
      // TODO: Win32LogErrorCode(Log, GetLastError());
      return true;
    }

    // We only care about mouse input (at the moment).
    if(InputData.header.dwType != RIM_TYPEMOUSE)
    {
      return true;
    }

    // Only process relative mouse movement (for now).
    bool IsAbsoluteMouseMovement = (InputData.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0;
    if(IsAbsoluteMouseMovement)
    {
      return true;
    }

    auto XMovement = Cast<float>(InputData.data.mouse.lLastX);
    UpdateInputSlotValue(Input, mouse::XDelta, XMovement);

    auto YMovement = Cast<float>(InputData.data.mouse.lLastY);
    UpdateInputSlotValue(Input, mouse::YDelta, YMovement);

    return true;
  }

  return false;
}

auto
::Win32VirtualKeyToInputId(WPARAM VKCode, LPARAM lParam)
  -> input_id
{
  const UINT ScanCode = Cast<UINT>((lParam & 0x00ff0000) >> 16);
  const bool IsExtended = (lParam & 0x01000000) != 0;

  switch(VKCode)
  {
    //
    // Special Key Handling
    //
    case VK_SHIFT:
    {
      VKCode = MapVirtualKey(ScanCode, MAPVK_VSC_TO_VK_EX);
      return Win32VirtualKeyToInputId(VKCode, lParam);
    }

    case VK_CONTROL:
    {
      VKCode = IsExtended ? VK_RCONTROL : VK_LCONTROL;
      return Win32VirtualKeyToInputId(VKCode, lParam);
    }

    case VK_MENU:
    {
      VKCode = IsExtended ? VK_RMENU : VK_LMENU;
      return Win32VirtualKeyToInputId(VKCode, lParam);
    }

    //
    // Common Keys
    //
    case VK_LSHIFT: return keyboard::LeftShift;
    case VK_RSHIFT: return keyboard::RightShift;

    case VK_LMENU: return keyboard::LeftAlt;
    case VK_RMENU: return keyboard::RightAlt;

    case VK_LCONTROL: return keyboard::LeftControl;
    case VK_RCONTROL: return keyboard::RightControl;

    case VK_ESCAPE:   return keyboard::Escape;
    case VK_SPACE:    return keyboard::Space;
    case VK_TAB:      return keyboard::Tab;
    case VK_LWIN:     return keyboard::LeftSystem;
    case VK_RWIN:     return keyboard::RightSystem;
    case VK_APPS:     return keyboard::Application;
    case VK_BACK:     return keyboard::Backspace;
    case VK_RETURN:   return IsExtended ? keyboard::Numpad_Enter : keyboard::Return;

    case VK_INSERT: return keyboard::Insert;
    case VK_DELETE: return keyboard::Delete;
    case VK_HOME:   return keyboard::Home;
    case VK_END:    return keyboard::End;
    case VK_NEXT:   return keyboard::PageUp;
    case VK_PRIOR:  return keyboard::PageDown;

    case VK_UP:    return keyboard::Up;
    case VK_DOWN:  return keyboard::Down;
    case VK_LEFT:  return keyboard::Left;
    case VK_RIGHT: return keyboard::Right;

    //
    // Digit Keys
    //
    case '0': return keyboard::Digit_0;
    case '1': return keyboard::Digit_1;
    case '2': return keyboard::Digit_2;
    case '3': return keyboard::Digit_3;
    case '4': return keyboard::Digit_4;
    case '5': return keyboard::Digit_5;
    case '6': return keyboard::Digit_6;
    case '7': return keyboard::Digit_7;
    case '8': return keyboard::Digit_8;
    case '9': return keyboard::Digit_9;

    //
    // Numpad
    //
    case VK_MULTIPLY: return keyboard::Numpad_Multiply;
    case VK_ADD:      return keyboard::Numpad_Add;
    case VK_SUBTRACT: return keyboard::Numpad_Subtract;
    case VK_DECIMAL:  return keyboard::Numpad_Decimal;
    case VK_DIVIDE:   return keyboard::Numpad_Divide;

    case VK_NUMPAD0: return keyboard::Numpad_0;
    case VK_NUMPAD1: return keyboard::Numpad_1;
    case VK_NUMPAD2: return keyboard::Numpad_2;
    case VK_NUMPAD3: return keyboard::Numpad_3;
    case VK_NUMPAD4: return keyboard::Numpad_4;
    case VK_NUMPAD5: return keyboard::Numpad_5;
    case VK_NUMPAD6: return keyboard::Numpad_6;
    case VK_NUMPAD7: return keyboard::Numpad_7;
    case VK_NUMPAD8: return keyboard::Numpad_8;
    case VK_NUMPAD9: return keyboard::Numpad_9;

    //
    // F-Keys
    //
    case VK_F1:  return keyboard::F1;
    case VK_F2:  return keyboard::F2;
    case VK_F3:  return keyboard::F3;
    case VK_F4:  return keyboard::F4;
    case VK_F5:  return keyboard::F5;
    case VK_F6:  return keyboard::F6;
    case VK_F7:  return keyboard::F7;
    case VK_F8:  return keyboard::F8;
    case VK_F9:  return keyboard::F9;
    case VK_F10: return keyboard::F10;
    case VK_F11: return keyboard::F11;
    case VK_F12: return keyboard::F12;
    case VK_F13: return keyboard::F13;
    case VK_F14: return keyboard::F14;
    case VK_F15: return keyboard::F15;
    case VK_F16: return keyboard::F16;
    case VK_F17: return keyboard::F17;
    case VK_F18: return keyboard::F18;
    case VK_F19: return keyboard::F19;
    case VK_F20: return keyboard::F20;
    case VK_F21: return keyboard::F21;
    case VK_F22: return keyboard::F22;
    case VK_F23: return keyboard::F23;
    case VK_F24: return keyboard::F24;

    //
    // Keys
    //
    case 'A': return keyboard::A;
    case 'B': return keyboard::B;
    case 'C': return keyboard::C;
    case 'D': return keyboard::D;
    case 'E': return keyboard::E;
    case 'F': return keyboard::F;
    case 'G': return keyboard::G;
    case 'H': return keyboard::H;
    case 'I': return keyboard::I;
    case 'J': return keyboard::J;
    case 'K': return keyboard::K;
    case 'L': return keyboard::L;
    case 'M': return keyboard::M;
    case 'N': return keyboard::N;
    case 'O': return keyboard::O;
    case 'P': return keyboard::P;
    case 'Q': return keyboard::Q;
    case 'R': return keyboard::R;
    case 'S': return keyboard::S;
    case 'T': return keyboard::T;
    case 'U': return keyboard::U;
    case 'V': return keyboard::V;
    case 'W': return keyboard::W;
    case 'X': return keyboard::X;
    case 'Y': return keyboard::Y;
    case 'Z': return keyboard::Z;

    //
    // Mouse Buttons
    //
    case VK_LBUTTON:  return mouse::LeftButton;
    case VK_MBUTTON:  return mouse::MiddleButton;
    case VK_RBUTTON:  return mouse::RightButton;
    case VK_XBUTTON1: return mouse::ExtraButton1;
    case VK_XBUTTON2: return mouse::ExtraButton2;

    default: return {};
  }
}

auto
::Win32PollXInput(x_input_dll* XInput, win32_input_context* Input)
  -> void
{
  if(Input->UserIndex < 0)
  {
    // Need a user index to poll for gamepad state.
    return;
  }

  auto UserIndex = Cast<DWORD>(Input->UserIndex);

  XINPUT_STATE NewControllerState;
  if(XInput->XInputGetState(UserIndex, &NewControllerState) != ERROR_SUCCESS)
  {
    // The gamepad seems to be disconnected.
    return;
  }

  if(Input->XInputPreviousState[UserIndex].dwPacketNumber == NewControllerState.dwPacketNumber)
  {
    // There are no updates for us.
    return;
  }

  auto NewControllerStatePtr = &NewControllerState;
  Defer [=](){ Input->XInputPreviousState[UserIndex] = *NewControllerStatePtr; };

  auto OldGamepad = &Input->XInputPreviousState[UserIndex].Gamepad;
  auto NewGamepad = &NewControllerState.Gamepad;

  //
  // Buttons
  //
  {
    auto UpdateButton = [](win32_input_context* Input, input_id Id, WORD OldButtons, WORD NewButtons, WORD ButtonMask)
    {
      bool WasDown = (OldButtons & ButtonMask) != 0;
      bool IsDown  = (NewButtons & ButtonMask) != 0;

      if(WasDown != IsDown)
      {
        UpdateInputSlotValue(Input, Id, IsDown);
      }
    };

    UpdateButton(Input, x_input::DPadUp,      OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_DPAD_UP);
    UpdateButton(Input, x_input::DPadDown,    OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
    UpdateButton(Input, x_input::DPadLeft,    OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
    UpdateButton(Input, x_input::DPadRight,   OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
    UpdateButton(Input, x_input::Start,       OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_START);
    UpdateButton(Input, x_input::Back,        OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_BACK);
    UpdateButton(Input, x_input::LeftThumb,   OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_LEFT_THUMB);
    UpdateButton(Input, x_input::RightThumb,  OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_RIGHT_THUMB);
    UpdateButton(Input, x_input::LeftBumper,  OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
    UpdateButton(Input, x_input::RightBumper, OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
    UpdateButton(Input, x_input::A,           OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_A);
    UpdateButton(Input, x_input::B,           OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_B);
    UpdateButton(Input, x_input::X,           OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_X);
    UpdateButton(Input, x_input::Y,           OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_Y);
  }

  //
  // Triggers
  //
  {
    auto UpdateTrigger = [](win32_input_context* Input, input_id Id, BYTE OldValue, BYTE NewValue)
    {
      if(OldValue != NewValue)
      {
        float NormalizedValue = NewValue / 255.0f;
        UpdateInputSlotValue(Input, Id, NormalizedValue);
      }
    };

    UpdateTrigger(Input, x_input::LeftTrigger, OldGamepad->bLeftTrigger, NewGamepad->bLeftTrigger);
    UpdateTrigger(Input, x_input::RightTrigger, OldGamepad->bRightTrigger, NewGamepad->bRightTrigger);
  }

  //
  // Thumbsticks
  //
  {
    auto UpdateThumbStick = [](win32_input_context* Input, input_id Id, SHORT OldValue, SHORT NewValue)
    {
      if(OldValue != NewValue)
      {
        float NormalizedValue;
        if(NewValue > 0) NormalizedValue = NewValue / 32767.0f;
        else             NormalizedValue = NewValue / 32768.0f;
        UpdateInputSlotValue(Input, Id, NormalizedValue);
      }
    };

    UpdateThumbStick(Input, x_input::XLeftStick, OldGamepad->sThumbLX, NewGamepad->sThumbLX);
    UpdateThumbStick(Input, x_input::YLeftStick, OldGamepad->sThumbLY, NewGamepad->sThumbLY);
    UpdateThumbStick(Input, x_input::XRightStick, OldGamepad->sThumbRX, NewGamepad->sThumbRX);
    UpdateThumbStick(Input, x_input::YRightStick, OldGamepad->sThumbRY, NewGamepad->sThumbRY);
  }
}

auto
::Win32RegisterAllMouseSlots(win32_input_context* Context, log_data* Log)
  -> void
{
  RegisterInputSlot(Context, input_type::Button, mouse::LeftButton);
  RegisterInputSlot(Context, input_type::Button, mouse::MiddleButton);
  RegisterInputSlot(Context, input_type::Button, mouse::RightButton);
  RegisterInputSlot(Context, input_type::Button, mouse::ExtraButton1);
  RegisterInputSlot(Context, input_type::Button, mouse::ExtraButton2);

  RegisterInputSlot(Context, input_type::Axis, mouse::XPosition);
  RegisterInputSlot(Context, input_type::Axis, mouse::YPosition);

  RegisterInputSlot(Context, input_type::Action, mouse::XDelta);
  RegisterInputSlot(Context, input_type::Action, mouse::YDelta);
  RegisterInputSlot(Context, input_type::Action, mouse::VerticalWheelDelta);
  RegisterInputSlot(Context, input_type::Action, mouse::HorizontalWheelDelta);
  RegisterInputSlot(Context, input_type::Action, mouse::LeftButton_DoubleClick);
  RegisterInputSlot(Context, input_type::Action, mouse::MiddleButton_DoubleClick);
  RegisterInputSlot(Context, input_type::Action, mouse::RightButton_DoubleClick);
  RegisterInputSlot(Context, input_type::Action, mouse::ExtraButton1_DoubleClick);
  RegisterInputSlot(Context, input_type::Action, mouse::ExtraButton2_DoubleClick);

  //
  // Register Mouse Raw Input
  //
  {
    RAWINPUTDEVICE Device;
    Device.usUsagePage = 0x01;
    Device.usUsage = 0x02;

    if(RegisterRawInputDevices(&Device, 1, sizeof(RAWINPUTDEVICE)))
    {
      LogInfo(Log, "Initialized raw input for mouse.");
    }
    else
    {
      LogError(Log, "Failed to initialize raw input for mouse.");
    }
  }
}

auto
::Win32RegisterAllXInputSlots(win32_input_context* Context, log_data* Log)
  -> void
{
  RegisterInputSlot(Context, input_type::Button, x_input::DPadUp);
  RegisterInputSlot(Context, input_type::Button, x_input::DPadDown);
  RegisterInputSlot(Context, input_type::Button, x_input::DPadLeft);
  RegisterInputSlot(Context, input_type::Button, x_input::DPadRight);
  RegisterInputSlot(Context, input_type::Button, x_input::Start);
  RegisterInputSlot(Context, input_type::Button, x_input::Back);
  RegisterInputSlot(Context, input_type::Button, x_input::LeftThumb);
  RegisterInputSlot(Context, input_type::Button, x_input::RightThumb);
  RegisterInputSlot(Context, input_type::Button, x_input::LeftBumper);
  RegisterInputSlot(Context, input_type::Button, x_input::RightBumper);
  RegisterInputSlot(Context, input_type::Button, x_input::A);
  RegisterInputSlot(Context, input_type::Button, x_input::B);
  RegisterInputSlot(Context, input_type::Button, x_input::X);
  RegisterInputSlot(Context, input_type::Button, x_input::Y);

  RegisterInputSlot(Context, input_type::Axis, x_input::LeftTrigger);
  {
    auto Props = GetOrCreate(&Context->ValueProperties, x_input::LeftTrigger);
    SetDeadZones(Props, XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f);
  }

  RegisterInputSlot(Context, input_type::Axis, x_input::RightTrigger);
  {
    auto Props = GetOrCreate(&Context->ValueProperties, x_input::RightTrigger);
    SetDeadZones(Props, XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f);
  }

  RegisterInputSlot(Context, input_type::Axis, x_input::XLeftStick);
  {
    auto Props = GetOrCreate(&Context->ValueProperties, x_input::XLeftStick);
    Props->PositiveDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
    Props->NegativeDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32768.0f;
  }

  RegisterInputSlot(Context, input_type::Axis, x_input::YLeftStick);
  {
    auto Props = GetOrCreate(&Context->ValueProperties, x_input::YLeftStick);
    Props->PositiveDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
    Props->NegativeDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32768.0f;
  }

  RegisterInputSlot(Context, input_type::Axis, x_input::XRightStick);
  {
    auto Props = GetOrCreate(&Context->ValueProperties, x_input::XRightStick);
    Props->PositiveDeadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f;
    Props->NegativeDeadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32768.0f;
  }

  RegisterInputSlot(Context, input_type::Axis, x_input::YRightStick);
  {
    auto Props = GetOrCreate(&Context->ValueProperties, x_input::YRightStick);
    Props->PositiveDeadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f;
    Props->NegativeDeadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32768.0f;
  }
}

auto
::Win32RegisterAllKeyboardSlots(win32_input_context* Context, log_data* Log)
  -> void
{
  RegisterInputSlot(Context, input_type::Button, keyboard::Escape);
  RegisterInputSlot(Context, input_type::Button, keyboard::Space);
  RegisterInputSlot(Context, input_type::Button, keyboard::Tab);
  RegisterInputSlot(Context, input_type::Button, keyboard::LeftShift);
  RegisterInputSlot(Context, input_type::Button, keyboard::LeftControl);
  RegisterInputSlot(Context, input_type::Button, keyboard::LeftAlt);
  RegisterInputSlot(Context, input_type::Button, keyboard::LeftSystem);
  RegisterInputSlot(Context, input_type::Button, keyboard::RightShift);
  RegisterInputSlot(Context, input_type::Button, keyboard::RightControl);
  RegisterInputSlot(Context, input_type::Button, keyboard::RightAlt);
  RegisterInputSlot(Context, input_type::Button, keyboard::RightSystem);
  RegisterInputSlot(Context, input_type::Button, keyboard::Application);
  RegisterInputSlot(Context, input_type::Button, keyboard::Backspace);
  RegisterInputSlot(Context, input_type::Button, keyboard::Return);

  RegisterInputSlot(Context, input_type::Button, keyboard::Insert);
  RegisterInputSlot(Context, input_type::Button, keyboard::Delete);
  RegisterInputSlot(Context, input_type::Button, keyboard::Home);
  RegisterInputSlot(Context, input_type::Button, keyboard::End);
  RegisterInputSlot(Context, input_type::Button, keyboard::PageUp);
  RegisterInputSlot(Context, input_type::Button, keyboard::PageDown);

  RegisterInputSlot(Context, input_type::Button, keyboard::Up);
  RegisterInputSlot(Context, input_type::Button, keyboard::Down);
  RegisterInputSlot(Context, input_type::Button, keyboard::Left);
  RegisterInputSlot(Context, input_type::Button, keyboard::Right);

  //
  // Digit Keys
  //
  RegisterInputSlot(Context, input_type::Button, keyboard::Digit_0);
  RegisterInputSlot(Context, input_type::Button, keyboard::Digit_1);
  RegisterInputSlot(Context, input_type::Button, keyboard::Digit_2);
  RegisterInputSlot(Context, input_type::Button, keyboard::Digit_3);
  RegisterInputSlot(Context, input_type::Button, keyboard::Digit_4);
  RegisterInputSlot(Context, input_type::Button, keyboard::Digit_5);
  RegisterInputSlot(Context, input_type::Button, keyboard::Digit_6);
  RegisterInputSlot(Context, input_type::Button, keyboard::Digit_7);
  RegisterInputSlot(Context, input_type::Button, keyboard::Digit_8);
  RegisterInputSlot(Context, input_type::Button, keyboard::Digit_9);

  //
  // Numpad
  //
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_Add);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_Subtract);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_Multiply);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_Divide);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_Decimal);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_Enter);

  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_0);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_1);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_2);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_3);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_4);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_5);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_6);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_7);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_8);
  RegisterInputSlot(Context, input_type::Button, keyboard::Numpad_9);

  //
  // F-Keys
  //
  RegisterInputSlot(Context, input_type::Button, keyboard::F1);
  RegisterInputSlot(Context, input_type::Button, keyboard::F2);
  RegisterInputSlot(Context, input_type::Button, keyboard::F3);
  RegisterInputSlot(Context, input_type::Button, keyboard::F4);
  RegisterInputSlot(Context, input_type::Button, keyboard::F5);
  RegisterInputSlot(Context, input_type::Button, keyboard::F6);
  RegisterInputSlot(Context, input_type::Button, keyboard::F7);
  RegisterInputSlot(Context, input_type::Button, keyboard::F8);
  RegisterInputSlot(Context, input_type::Button, keyboard::F9);
  RegisterInputSlot(Context, input_type::Button, keyboard::F10);
  RegisterInputSlot(Context, input_type::Button, keyboard::F11);
  RegisterInputSlot(Context, input_type::Button, keyboard::F12);
  RegisterInputSlot(Context, input_type::Button, keyboard::F13);
  RegisterInputSlot(Context, input_type::Button, keyboard::F14);
  RegisterInputSlot(Context, input_type::Button, keyboard::F15);
  RegisterInputSlot(Context, input_type::Button, keyboard::F16);
  RegisterInputSlot(Context, input_type::Button, keyboard::F17);
  RegisterInputSlot(Context, input_type::Button, keyboard::F18);
  RegisterInputSlot(Context, input_type::Button, keyboard::F19);
  RegisterInputSlot(Context, input_type::Button, keyboard::F20);
  RegisterInputSlot(Context, input_type::Button, keyboard::F21);
  RegisterInputSlot(Context, input_type::Button, keyboard::F22);
  RegisterInputSlot(Context, input_type::Button, keyboard::F23);
  RegisterInputSlot(Context, input_type::Button, keyboard::F24);

  //
  // Keys
  //
  RegisterInputSlot(Context, input_type::Button, keyboard::A);
  RegisterInputSlot(Context, input_type::Button, keyboard::B);
  RegisterInputSlot(Context, input_type::Button, keyboard::C);
  RegisterInputSlot(Context, input_type::Button, keyboard::D);
  RegisterInputSlot(Context, input_type::Button, keyboard::E);
  RegisterInputSlot(Context, input_type::Button, keyboard::F);
  RegisterInputSlot(Context, input_type::Button, keyboard::G);
  RegisterInputSlot(Context, input_type::Button, keyboard::H);
  RegisterInputSlot(Context, input_type::Button, keyboard::I);
  RegisterInputSlot(Context, input_type::Button, keyboard::J);
  RegisterInputSlot(Context, input_type::Button, keyboard::K);
  RegisterInputSlot(Context, input_type::Button, keyboard::L);
  RegisterInputSlot(Context, input_type::Button, keyboard::M);
  RegisterInputSlot(Context, input_type::Button, keyboard::N);
  RegisterInputSlot(Context, input_type::Button, keyboard::O);
  RegisterInputSlot(Context, input_type::Button, keyboard::P);
  RegisterInputSlot(Context, input_type::Button, keyboard::Q);
  RegisterInputSlot(Context, input_type::Button, keyboard::R);
  RegisterInputSlot(Context, input_type::Button, keyboard::S);
  RegisterInputSlot(Context, input_type::Button, keyboard::T);
  RegisterInputSlot(Context, input_type::Button, keyboard::U);
  RegisterInputSlot(Context, input_type::Button, keyboard::V);
  RegisterInputSlot(Context, input_type::Button, keyboard::W);
  RegisterInputSlot(Context, input_type::Button, keyboard::X);
  RegisterInputSlot(Context, input_type::Button, keyboard::Y);
  RegisterInputSlot(Context, input_type::Button, keyboard::Z);
}

