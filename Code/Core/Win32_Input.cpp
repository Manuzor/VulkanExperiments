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

    fixed_block<3, input_id> KeyIdBuffer;

    slice<input_id> KeyIds = Win32VirtualKeyToInputId(VKCode, LParam, Slice(KeyIdBuffer));

    if(KeyIds.Num == 0)
    {
      LogWarning(Log, "Unable to map virtual key code %d (Hex: 0x%x)", VKCode, VKCode);
      return true;
    }

    for(auto KeyId : KeyIds)
    {
      UpdateInputSlotValue(Input, KeyId, IsDown);
    }

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

    if(InputDataSize > sizeof(RAWINPUT))
    {
      LogError(Log, "We are querying only for raw mouse input data, for which sizeof(RAWINPUT) should be enough.");
      Assert(false);
    }

    RAWINPUT InputData;
    const UINT BytesWritten = GetRawInputData(Reinterpret<HRAWINPUT>(LParam), RID_INPUT, &InputData, &InputDataSize, Cast<UINT>(sizeof(RAWINPUTHEADER)));
    if(BytesWritten != InputDataSize)
    {
      LogError(Log, "Failed to get raw input data.");
      Win32LogErrorCode(GetLastError());
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
::Win32VirtualKeyToInputId(WPARAM VKCode, LPARAM lParam, slice<input_id> Buffer)
  -> slice<input_id>
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
      Buffer[0] = keyboard::Shift;
      auto Result = Win32VirtualKeyToInputId(VKCode, lParam, SliceTrimFront(Buffer, 1));
      return Slice(Buffer, 0, Result.Num + 1);
    }

    case VK_CONTROL:
    {
      VKCode = IsExtended ? VK_RCONTROL : VK_LCONTROL;
      Buffer[0] = keyboard::Control;
      auto Result = Win32VirtualKeyToInputId(VKCode, lParam, SliceTrimFront(Buffer, 1));
      return Slice(Buffer, 0, Result.Num + 1);
    }

    case VK_MENU:
    {
      VKCode = IsExtended ? VK_RMENU : VK_LMENU;
      Buffer[0] = keyboard::Alt;
      auto Result = Win32VirtualKeyToInputId(VKCode, lParam, SliceTrimFront(Buffer, 1));
      return Slice(Buffer, 0, Result.Num + 1);
    }

    //
    // Common Keys
    //
    case VK_LSHIFT: Buffer[0] = keyboard::LeftShift;  return Slice(Buffer, 0, 1);
    case VK_RSHIFT: Buffer[0] = keyboard::RightShift; return Slice(Buffer, 0, 1);

    case VK_LMENU: Buffer[0] = keyboard::LeftAlt;  return Slice(Buffer, 0, 1);
    case VK_RMENU: Buffer[0] = keyboard::RightAlt; return Slice(Buffer, 0, 1);

    case VK_LCONTROL: Buffer[0] = keyboard::LeftControl;  return Slice(Buffer, 0, 1);
    case VK_RCONTROL: Buffer[0] = keyboard::RightControl; return Slice(Buffer, 0, 1);

    case VK_ESCAPE:   Buffer[0] = keyboard::Escape;                                       return Slice(Buffer, 0, 1);
    case VK_SPACE:    Buffer[0] = keyboard::Space;                                        return Slice(Buffer, 0, 1);
    case VK_TAB:      Buffer[0] = keyboard::Tab;                                          return Slice(Buffer, 0, 1);
    case VK_LWIN:     Buffer[0] = keyboard::LeftSystem;                                   return Slice(Buffer, 0, 1);
    case VK_RWIN:     Buffer[0] = keyboard::RightSystem;                                  return Slice(Buffer, 0, 1);
    case VK_APPS:     Buffer[0] = keyboard::Application;                                  return Slice(Buffer, 0, 1);
    case VK_BACK:     Buffer[0] = keyboard::Backspace;                                    return Slice(Buffer, 0, 1);
    case VK_RETURN:   Buffer[0] = IsExtended ? keyboard::Numpad_Enter : keyboard::Return; return Slice(Buffer, 0, 1);

    case VK_INSERT: Buffer[0] = keyboard::Insert;   return Slice(Buffer, 0, 1);
    case VK_DELETE: Buffer[0] = keyboard::Delete;   return Slice(Buffer, 0, 1);
    case VK_HOME:   Buffer[0] = keyboard::Home;     return Slice(Buffer, 0, 1);
    case VK_END:    Buffer[0] = keyboard::End;      return Slice(Buffer, 0, 1);
    case VK_NEXT:   Buffer[0] = keyboard::PageUp;   return Slice(Buffer, 0, 1);
    case VK_PRIOR:  Buffer[0] = keyboard::PageDown; return Slice(Buffer, 0, 1);

    case VK_UP:    Buffer[0] = keyboard::Up;    return Slice(Buffer, 0, 1);
    case VK_DOWN:  Buffer[0] = keyboard::Down;  return Slice(Buffer, 0, 1);
    case VK_LEFT:  Buffer[0] = keyboard::Left;  return Slice(Buffer, 0, 1);
    case VK_RIGHT: Buffer[0] = keyboard::Right; return Slice(Buffer, 0, 1);

    //
    // Digit Keys
    //
    case '0': Buffer[0] = keyboard::Digit_0; return Slice(Buffer, 0, 1);
    case '1': Buffer[0] = keyboard::Digit_1; return Slice(Buffer, 0, 1);
    case '2': Buffer[0] = keyboard::Digit_2; return Slice(Buffer, 0, 1);
    case '3': Buffer[0] = keyboard::Digit_3; return Slice(Buffer, 0, 1);
    case '4': Buffer[0] = keyboard::Digit_4; return Slice(Buffer, 0, 1);
    case '5': Buffer[0] = keyboard::Digit_5; return Slice(Buffer, 0, 1);
    case '6': Buffer[0] = keyboard::Digit_6; return Slice(Buffer, 0, 1);
    case '7': Buffer[0] = keyboard::Digit_7; return Slice(Buffer, 0, 1);
    case '8': Buffer[0] = keyboard::Digit_8; return Slice(Buffer, 0, 1);
    case '9': Buffer[0] = keyboard::Digit_9; return Slice(Buffer, 0, 1);

    //
    // Numpad
    //
    case VK_MULTIPLY: Buffer[0] = keyboard::Numpad_Multiply; return Slice(Buffer, 0, 1);
    case VK_ADD:      Buffer[0] = keyboard::Numpad_Add;      return Slice(Buffer, 0, 1);
    case VK_SUBTRACT: Buffer[0] = keyboard::Numpad_Subtract; return Slice(Buffer, 0, 1);
    case VK_DECIMAL:  Buffer[0] = keyboard::Numpad_Decimal;  return Slice(Buffer, 0, 1);
    case VK_DIVIDE:   Buffer[0] = keyboard::Numpad_Divide;   return Slice(Buffer, 0, 1);

    case VK_NUMPAD0: Buffer[0] = keyboard::Numpad_0; return Slice(Buffer, 0, 1);
    case VK_NUMPAD1: Buffer[0] = keyboard::Numpad_1; return Slice(Buffer, 0, 1);
    case VK_NUMPAD2: Buffer[0] = keyboard::Numpad_2; return Slice(Buffer, 0, 1);
    case VK_NUMPAD3: Buffer[0] = keyboard::Numpad_3; return Slice(Buffer, 0, 1);
    case VK_NUMPAD4: Buffer[0] = keyboard::Numpad_4; return Slice(Buffer, 0, 1);
    case VK_NUMPAD5: Buffer[0] = keyboard::Numpad_5; return Slice(Buffer, 0, 1);
    case VK_NUMPAD6: Buffer[0] = keyboard::Numpad_6; return Slice(Buffer, 0, 1);
    case VK_NUMPAD7: Buffer[0] = keyboard::Numpad_7; return Slice(Buffer, 0, 1);
    case VK_NUMPAD8: Buffer[0] = keyboard::Numpad_8; return Slice(Buffer, 0, 1);
    case VK_NUMPAD9: Buffer[0] = keyboard::Numpad_9; return Slice(Buffer, 0, 1);

    //
    // F-Keys
    //
    case VK_F1:  Buffer[0] = keyboard::F1;  return Slice(Buffer, 0, 1);
    case VK_F2:  Buffer[0] = keyboard::F2;  return Slice(Buffer, 0, 1);
    case VK_F3:  Buffer[0] = keyboard::F3;  return Slice(Buffer, 0, 1);
    case VK_F4:  Buffer[0] = keyboard::F4;  return Slice(Buffer, 0, 1);
    case VK_F5:  Buffer[0] = keyboard::F5;  return Slice(Buffer, 0, 1);
    case VK_F6:  Buffer[0] = keyboard::F6;  return Slice(Buffer, 0, 1);
    case VK_F7:  Buffer[0] = keyboard::F7;  return Slice(Buffer, 0, 1);
    case VK_F8:  Buffer[0] = keyboard::F8;  return Slice(Buffer, 0, 1);
    case VK_F9:  Buffer[0] = keyboard::F9;  return Slice(Buffer, 0, 1);
    case VK_F10: Buffer[0] = keyboard::F10; return Slice(Buffer, 0, 1);
    case VK_F11: Buffer[0] = keyboard::F11; return Slice(Buffer, 0, 1);
    case VK_F12: Buffer[0] = keyboard::F12; return Slice(Buffer, 0, 1);
    case VK_F13: Buffer[0] = keyboard::F13; return Slice(Buffer, 0, 1);
    case VK_F14: Buffer[0] = keyboard::F14; return Slice(Buffer, 0, 1);
    case VK_F15: Buffer[0] = keyboard::F15; return Slice(Buffer, 0, 1);
    case VK_F16: Buffer[0] = keyboard::F16; return Slice(Buffer, 0, 1);
    case VK_F17: Buffer[0] = keyboard::F17; return Slice(Buffer, 0, 1);
    case VK_F18: Buffer[0] = keyboard::F18; return Slice(Buffer, 0, 1);
    case VK_F19: Buffer[0] = keyboard::F19; return Slice(Buffer, 0, 1);
    case VK_F20: Buffer[0] = keyboard::F20; return Slice(Buffer, 0, 1);
    case VK_F21: Buffer[0] = keyboard::F21; return Slice(Buffer, 0, 1);
    case VK_F22: Buffer[0] = keyboard::F22; return Slice(Buffer, 0, 1);
    case VK_F23: Buffer[0] = keyboard::F23; return Slice(Buffer, 0, 1);
    case VK_F24: Buffer[0] = keyboard::F24; return Slice(Buffer, 0, 1);

    //
    // Keys
    //
    case 'A': Buffer[0] = keyboard::A; return Slice(Buffer, 0, 1);
    case 'B': Buffer[0] = keyboard::B; return Slice(Buffer, 0, 1);
    case 'C': Buffer[0] = keyboard::C; return Slice(Buffer, 0, 1);
    case 'D': Buffer[0] = keyboard::D; return Slice(Buffer, 0, 1);
    case 'E': Buffer[0] = keyboard::E; return Slice(Buffer, 0, 1);
    case 'F': Buffer[0] = keyboard::F; return Slice(Buffer, 0, 1);
    case 'G': Buffer[0] = keyboard::G; return Slice(Buffer, 0, 1);
    case 'H': Buffer[0] = keyboard::H; return Slice(Buffer, 0, 1);
    case 'I': Buffer[0] = keyboard::I; return Slice(Buffer, 0, 1);
    case 'J': Buffer[0] = keyboard::J; return Slice(Buffer, 0, 1);
    case 'K': Buffer[0] = keyboard::K; return Slice(Buffer, 0, 1);
    case 'L': Buffer[0] = keyboard::L; return Slice(Buffer, 0, 1);
    case 'M': Buffer[0] = keyboard::M; return Slice(Buffer, 0, 1);
    case 'N': Buffer[0] = keyboard::N; return Slice(Buffer, 0, 1);
    case 'O': Buffer[0] = keyboard::O; return Slice(Buffer, 0, 1);
    case 'P': Buffer[0] = keyboard::P; return Slice(Buffer, 0, 1);
    case 'Q': Buffer[0] = keyboard::Q; return Slice(Buffer, 0, 1);
    case 'R': Buffer[0] = keyboard::R; return Slice(Buffer, 0, 1);
    case 'S': Buffer[0] = keyboard::S; return Slice(Buffer, 0, 1);
    case 'T': Buffer[0] = keyboard::T; return Slice(Buffer, 0, 1);
    case 'U': Buffer[0] = keyboard::U; return Slice(Buffer, 0, 1);
    case 'V': Buffer[0] = keyboard::V; return Slice(Buffer, 0, 1);
    case 'W': Buffer[0] = keyboard::W; return Slice(Buffer, 0, 1);
    case 'X': Buffer[0] = keyboard::X; return Slice(Buffer, 0, 1);
    case 'Y': Buffer[0] = keyboard::Y; return Slice(Buffer, 0, 1);
    case 'Z': Buffer[0] = keyboard::Z; return Slice(Buffer, 0, 1);

    //
    // Mouse Buttons
    //
    case VK_LBUTTON:  Buffer[0] = mouse::LeftButton;   return Slice(Buffer, 0, 1);
    case VK_MBUTTON:  Buffer[0] = mouse::MiddleButton; return Slice(Buffer, 0, 1);
    case VK_RBUTTON:  Buffer[0] = mouse::RightButton;  return Slice(Buffer, 0, 1);
    case VK_XBUTTON1: Buffer[0] = mouse::ExtraButton1; return Slice(Buffer, 0, 1);
    case VK_XBUTTON2: Buffer[0] = mouse::ExtraButton2; return Slice(Buffer, 0, 1);

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
    RAWINPUTDEVICE Device = {};
    Device.usUsagePage = 0x01;
    Device.usUsage = 0x02;

    if(RegisterRawInputDevices(&Device, 1, sizeof(RAWINPUTDEVICE)))
    {
      LogInfo(Log, "Initialized raw input for mouse.");
    }
    else
    {
      LogError(Log, "Failed to initialize raw input for mouse.");
      Win32LogErrorCode(Log, GetLastError());
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
  RegisterInputSlot(Context, input_type::Button, keyboard::Shift);
  RegisterInputSlot(Context, input_type::Button, keyboard::Control);
  RegisterInputSlot(Context, input_type::Button, keyboard::Alt);
  RegisterInputSlot(Context, input_type::Button, keyboard::System);
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

