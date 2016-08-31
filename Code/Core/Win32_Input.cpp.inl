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

auto
::Win32ProcessInputMessage(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam,
                           input_context Input,
                           input_keyboard_slots* Keyboard,
                           input_mouse_slots* Mouse,
                           log_data* Log)
  -> bool
{
  //
  // Keyboard messages
  //
  if(Message == WM_CHAR || Message == WM_UNICHAR)
  {
    if(WParam == UNICODE_NOCHAR)
      return true;

    array<char>* CharacterBuffer = InputGetCharacterBuffer(Input);
    if(CharacterBuffer)
    {
      *CharacterBuffer += Cast<char>(WParam);
      return true;
    }

    return false;
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

    fixed_block<3, char const*> KeyIdBuffer;

    slice<char const*> KeyIds = Win32VirtualKeyToInputId(VKCode, LParam, Slice(KeyIdBuffer));

    if(KeyIds.Num == 0)
    {
      LogWarning(Log, "Unable to map virtual key code %d (Hex: 0x%x)", VKCode, VKCode);
      return true;
    }

    for(auto KeyId : KeyIds)
    {
      auto SlotHandle = InputGetSlot(Input, KeyId);
      InputSetButton(SlotHandle, IsDown);
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
      InputSetAxisValue(Mouse->XPosition, Cast<float>(XClientAreaMouse));

      auto YClientAreaMouse = GET_Y_LPARAM(LParam);
      InputSetAxisValue(Mouse->YPosition, Cast<float>(YClientAreaMouse));

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
        InputSetButton(Mouse->LeftButton, IsDown);
      } break;

      case WM_RBUTTONUP:
      case WM_RBUTTONDOWN:
      {
        bool IsDown = Message == WM_RBUTTONDOWN;
        InputSetButton(Mouse->RightButton, IsDown);
      } break;

      case WM_MBUTTONUP:
      case WM_MBUTTONDOWN:
      {
        bool IsDown = Message == WM_MBUTTONDOWN;
        InputSetButton(Mouse->MiddleButton, IsDown);
      } break;

      case WM_XBUTTONUP:
      case WM_XBUTTONDOWN:
      {
        bool IsDown = Message == WM_XBUTTONDOWN;
        auto XButtonNumber = GET_XBUTTON_WPARAM(WParam);
        switch(XButtonNumber)
        {
          case 1: InputSetButton(Mouse->ExtraButton1, IsDown); break;
          case 2: InputSetButton(Mouse->ExtraButton2, IsDown); break;
          default: break;
        }
      } break;

      case WM_MOUSEWHEEL:
      {
        auto RawValue = GET_WHEEL_DELTA_WPARAM(WParam);
        auto Value = Cast<float>(RawValue) / WHEEL_DELTA;
        InputSetActionValue(Mouse->VerticalWheelDelta, Value);
      } break;

      case WM_MOUSEHWHEEL:
      {
        auto RawValue = GET_WHEEL_DELTA_WPARAM(WParam);
        auto Value = Cast<float>(RawValue) / WHEEL_DELTA;
        InputSetActionValue(Mouse->HorizontalWheelDelta, Value);
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
    InputSetActionValue(Mouse->XDelta, XMovement);

    auto YMovement = Cast<float>(InputData.data.mouse.lLastY);
    InputSetActionValue(Mouse->YDelta, YMovement);

    return true;
  }

  return false;
}

auto
::Win32VirtualKeyToInputId(WPARAM VKCode, LPARAM lParam, slice<char const*> Buffer)
  -> slice<char const*>
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
      Buffer[0] = input_keyboard_slot_names::Shift;
      auto Result = Win32VirtualKeyToInputId(VKCode, lParam, SliceTrimFront(Buffer, 1));
      return Slice(Buffer, 0, Result.Num + 1);
    }

    case VK_CONTROL:
    {
      VKCode = IsExtended ? VK_RCONTROL : VK_LCONTROL;
      Buffer[0] = input_keyboard_slot_names::Control;
      auto Result = Win32VirtualKeyToInputId(VKCode, lParam, SliceTrimFront(Buffer, 1));
      return Slice(Buffer, 0, Result.Num + 1);
    }

    case VK_MENU:
    {
      VKCode = IsExtended ? VK_RMENU : VK_LMENU;
      Buffer[0] = input_keyboard_slot_names::Alt;
      auto Result = Win32VirtualKeyToInputId(VKCode, lParam, SliceTrimFront(Buffer, 1));
      return Slice(Buffer, 0, Result.Num + 1);
    }

    //
    // Common Keys
    //
    case VK_LSHIFT: Buffer[0] = input_keyboard_slot_names::LeftShift;  return Slice(Buffer, 0, 1);
    case VK_RSHIFT: Buffer[0] = input_keyboard_slot_names::RightShift; return Slice(Buffer, 0, 1);

    case VK_LMENU: Buffer[0] = input_keyboard_slot_names::LeftAlt;  return Slice(Buffer, 0, 1);
    case VK_RMENU: Buffer[0] = input_keyboard_slot_names::RightAlt; return Slice(Buffer, 0, 1);

    case VK_LCONTROL: Buffer[0] = input_keyboard_slot_names::LeftControl;  return Slice(Buffer, 0, 1);
    case VK_RCONTROL: Buffer[0] = input_keyboard_slot_names::RightControl; return Slice(Buffer, 0, 1);

    case VK_ESCAPE:   Buffer[0] = input_keyboard_slot_names::Escape;                                       return Slice(Buffer, 0, 1);
    case VK_SPACE:    Buffer[0] = input_keyboard_slot_names::Space;                                        return Slice(Buffer, 0, 1);
    case VK_TAB:      Buffer[0] = input_keyboard_slot_names::Tab;                                          return Slice(Buffer, 0, 1);
    case VK_LWIN:     Buffer[0] = input_keyboard_slot_names::LeftSystem;                                   return Slice(Buffer, 0, 1);
    case VK_RWIN:     Buffer[0] = input_keyboard_slot_names::RightSystem;                                  return Slice(Buffer, 0, 1);
    case VK_APPS:     Buffer[0] = input_keyboard_slot_names::Application;                                  return Slice(Buffer, 0, 1);
    case VK_BACK:     Buffer[0] = input_keyboard_slot_names::Backspace;                                    return Slice(Buffer, 0, 1);
    case VK_RETURN:   Buffer[0] = IsExtended ? input_keyboard_slot_names::Numpad_Enter : input_keyboard_slot_names::Return; return Slice(Buffer, 0, 1);

    case VK_INSERT: Buffer[0] = input_keyboard_slot_names::Insert;   return Slice(Buffer, 0, 1);
    case VK_DELETE: Buffer[0] = input_keyboard_slot_names::Delete;   return Slice(Buffer, 0, 1);
    case VK_HOME:   Buffer[0] = input_keyboard_slot_names::Home;     return Slice(Buffer, 0, 1);
    case VK_END:    Buffer[0] = input_keyboard_slot_names::End;      return Slice(Buffer, 0, 1);
    case VK_NEXT:   Buffer[0] = input_keyboard_slot_names::PageUp;   return Slice(Buffer, 0, 1);
    case VK_PRIOR:  Buffer[0] = input_keyboard_slot_names::PageDown; return Slice(Buffer, 0, 1);

    case VK_UP:    Buffer[0] = input_keyboard_slot_names::Up;    return Slice(Buffer, 0, 1);
    case VK_DOWN:  Buffer[0] = input_keyboard_slot_names::Down;  return Slice(Buffer, 0, 1);
    case VK_LEFT:  Buffer[0] = input_keyboard_slot_names::Left;  return Slice(Buffer, 0, 1);
    case VK_RIGHT: Buffer[0] = input_keyboard_slot_names::Right; return Slice(Buffer, 0, 1);

    //
    // Digit Keys
    //
    case '0': Buffer[0] = input_keyboard_slot_names::Digit_0; return Slice(Buffer, 0, 1);
    case '1': Buffer[0] = input_keyboard_slot_names::Digit_1; return Slice(Buffer, 0, 1);
    case '2': Buffer[0] = input_keyboard_slot_names::Digit_2; return Slice(Buffer, 0, 1);
    case '3': Buffer[0] = input_keyboard_slot_names::Digit_3; return Slice(Buffer, 0, 1);
    case '4': Buffer[0] = input_keyboard_slot_names::Digit_4; return Slice(Buffer, 0, 1);
    case '5': Buffer[0] = input_keyboard_slot_names::Digit_5; return Slice(Buffer, 0, 1);
    case '6': Buffer[0] = input_keyboard_slot_names::Digit_6; return Slice(Buffer, 0, 1);
    case '7': Buffer[0] = input_keyboard_slot_names::Digit_7; return Slice(Buffer, 0, 1);
    case '8': Buffer[0] = input_keyboard_slot_names::Digit_8; return Slice(Buffer, 0, 1);
    case '9': Buffer[0] = input_keyboard_slot_names::Digit_9; return Slice(Buffer, 0, 1);

    //
    // Numpad
    //
    case VK_MULTIPLY: Buffer[0] = input_keyboard_slot_names::Numpad_Multiply; return Slice(Buffer, 0, 1);
    case VK_ADD:      Buffer[0] = input_keyboard_slot_names::Numpad_Add;      return Slice(Buffer, 0, 1);
    case VK_SUBTRACT: Buffer[0] = input_keyboard_slot_names::Numpad_Subtract; return Slice(Buffer, 0, 1);
    case VK_DECIMAL:  Buffer[0] = input_keyboard_slot_names::Numpad_Decimal;  return Slice(Buffer, 0, 1);
    case VK_DIVIDE:   Buffer[0] = input_keyboard_slot_names::Numpad_Divide;   return Slice(Buffer, 0, 1);

    case VK_NUMPAD0: Buffer[0] = input_keyboard_slot_names::Numpad_0; return Slice(Buffer, 0, 1);
    case VK_NUMPAD1: Buffer[0] = input_keyboard_slot_names::Numpad_1; return Slice(Buffer, 0, 1);
    case VK_NUMPAD2: Buffer[0] = input_keyboard_slot_names::Numpad_2; return Slice(Buffer, 0, 1);
    case VK_NUMPAD3: Buffer[0] = input_keyboard_slot_names::Numpad_3; return Slice(Buffer, 0, 1);
    case VK_NUMPAD4: Buffer[0] = input_keyboard_slot_names::Numpad_4; return Slice(Buffer, 0, 1);
    case VK_NUMPAD5: Buffer[0] = input_keyboard_slot_names::Numpad_5; return Slice(Buffer, 0, 1);
    case VK_NUMPAD6: Buffer[0] = input_keyboard_slot_names::Numpad_6; return Slice(Buffer, 0, 1);
    case VK_NUMPAD7: Buffer[0] = input_keyboard_slot_names::Numpad_7; return Slice(Buffer, 0, 1);
    case VK_NUMPAD8: Buffer[0] = input_keyboard_slot_names::Numpad_8; return Slice(Buffer, 0, 1);
    case VK_NUMPAD9: Buffer[0] = input_keyboard_slot_names::Numpad_9; return Slice(Buffer, 0, 1);

    //
    // F-Keys
    //
    case VK_F1:  Buffer[0] = input_keyboard_slot_names::F1;  return Slice(Buffer, 0, 1);
    case VK_F2:  Buffer[0] = input_keyboard_slot_names::F2;  return Slice(Buffer, 0, 1);
    case VK_F3:  Buffer[0] = input_keyboard_slot_names::F3;  return Slice(Buffer, 0, 1);
    case VK_F4:  Buffer[0] = input_keyboard_slot_names::F4;  return Slice(Buffer, 0, 1);
    case VK_F5:  Buffer[0] = input_keyboard_slot_names::F5;  return Slice(Buffer, 0, 1);
    case VK_F6:  Buffer[0] = input_keyboard_slot_names::F6;  return Slice(Buffer, 0, 1);
    case VK_F7:  Buffer[0] = input_keyboard_slot_names::F7;  return Slice(Buffer, 0, 1);
    case VK_F8:  Buffer[0] = input_keyboard_slot_names::F8;  return Slice(Buffer, 0, 1);
    case VK_F9:  Buffer[0] = input_keyboard_slot_names::F9;  return Slice(Buffer, 0, 1);
    case VK_F10: Buffer[0] = input_keyboard_slot_names::F10; return Slice(Buffer, 0, 1);
    case VK_F11: Buffer[0] = input_keyboard_slot_names::F11; return Slice(Buffer, 0, 1);
    case VK_F12: Buffer[0] = input_keyboard_slot_names::F12; return Slice(Buffer, 0, 1);
    case VK_F13: Buffer[0] = input_keyboard_slot_names::F13; return Slice(Buffer, 0, 1);
    case VK_F14: Buffer[0] = input_keyboard_slot_names::F14; return Slice(Buffer, 0, 1);
    case VK_F15: Buffer[0] = input_keyboard_slot_names::F15; return Slice(Buffer, 0, 1);
    case VK_F16: Buffer[0] = input_keyboard_slot_names::F16; return Slice(Buffer, 0, 1);
    case VK_F17: Buffer[0] = input_keyboard_slot_names::F17; return Slice(Buffer, 0, 1);
    case VK_F18: Buffer[0] = input_keyboard_slot_names::F18; return Slice(Buffer, 0, 1);
    case VK_F19: Buffer[0] = input_keyboard_slot_names::F19; return Slice(Buffer, 0, 1);
    case VK_F20: Buffer[0] = input_keyboard_slot_names::F20; return Slice(Buffer, 0, 1);
    case VK_F21: Buffer[0] = input_keyboard_slot_names::F21; return Slice(Buffer, 0, 1);
    case VK_F22: Buffer[0] = input_keyboard_slot_names::F22; return Slice(Buffer, 0, 1);
    case VK_F23: Buffer[0] = input_keyboard_slot_names::F23; return Slice(Buffer, 0, 1);
    case VK_F24: Buffer[0] = input_keyboard_slot_names::F24; return Slice(Buffer, 0, 1);

    //
    // Keys
    //
    case 'A': Buffer[0] = input_keyboard_slot_names::A; return Slice(Buffer, 0, 1);
    case 'B': Buffer[0] = input_keyboard_slot_names::B; return Slice(Buffer, 0, 1);
    case 'C': Buffer[0] = input_keyboard_slot_names::C; return Slice(Buffer, 0, 1);
    case 'D': Buffer[0] = input_keyboard_slot_names::D; return Slice(Buffer, 0, 1);
    case 'E': Buffer[0] = input_keyboard_slot_names::E; return Slice(Buffer, 0, 1);
    case 'F': Buffer[0] = input_keyboard_slot_names::F; return Slice(Buffer, 0, 1);
    case 'G': Buffer[0] = input_keyboard_slot_names::G; return Slice(Buffer, 0, 1);
    case 'H': Buffer[0] = input_keyboard_slot_names::H; return Slice(Buffer, 0, 1);
    case 'I': Buffer[0] = input_keyboard_slot_names::I; return Slice(Buffer, 0, 1);
    case 'J': Buffer[0] = input_keyboard_slot_names::J; return Slice(Buffer, 0, 1);
    case 'K': Buffer[0] = input_keyboard_slot_names::K; return Slice(Buffer, 0, 1);
    case 'L': Buffer[0] = input_keyboard_slot_names::L; return Slice(Buffer, 0, 1);
    case 'M': Buffer[0] = input_keyboard_slot_names::M; return Slice(Buffer, 0, 1);
    case 'N': Buffer[0] = input_keyboard_slot_names::N; return Slice(Buffer, 0, 1);
    case 'O': Buffer[0] = input_keyboard_slot_names::O; return Slice(Buffer, 0, 1);
    case 'P': Buffer[0] = input_keyboard_slot_names::P; return Slice(Buffer, 0, 1);
    case 'Q': Buffer[0] = input_keyboard_slot_names::Q; return Slice(Buffer, 0, 1);
    case 'R': Buffer[0] = input_keyboard_slot_names::R; return Slice(Buffer, 0, 1);
    case 'S': Buffer[0] = input_keyboard_slot_names::S; return Slice(Buffer, 0, 1);
    case 'T': Buffer[0] = input_keyboard_slot_names::T; return Slice(Buffer, 0, 1);
    case 'U': Buffer[0] = input_keyboard_slot_names::U; return Slice(Buffer, 0, 1);
    case 'V': Buffer[0] = input_keyboard_slot_names::V; return Slice(Buffer, 0, 1);
    case 'W': Buffer[0] = input_keyboard_slot_names::W; return Slice(Buffer, 0, 1);
    case 'X': Buffer[0] = input_keyboard_slot_names::X; return Slice(Buffer, 0, 1);
    case 'Y': Buffer[0] = input_keyboard_slot_names::Y; return Slice(Buffer, 0, 1);
    case 'Z': Buffer[0] = input_keyboard_slot_names::Z; return Slice(Buffer, 0, 1);

    //
    // Mouse Buttons
    //
    case VK_LBUTTON:  Buffer[0] = input_mouse_slot_names::LeftButton;   return Slice(Buffer, 0, 1);
    case VK_MBUTTON:  Buffer[0] = input_mouse_slot_names::MiddleButton; return Slice(Buffer, 0, 1);
    case VK_RBUTTON:  Buffer[0] = input_mouse_slot_names::RightButton;  return Slice(Buffer, 0, 1);
    case VK_XBUTTON1: Buffer[0] = input_mouse_slot_names::ExtraButton1; return Slice(Buffer, 0, 1);
    case VK_XBUTTON2: Buffer[0] = input_mouse_slot_names::ExtraButton2; return Slice(Buffer, 0, 1);

    default: return {};
  }
}

auto
::Win32PollXInput(x_input_dll* XInput, input_context ContextHandle, input_x_input_slots* Slots)
  -> void
{
  impl_context* Context{};
  DisassembleContextHandle(ContextHandle, nullptr, &Context);

  if(Context == nullptr)
  {
    // Bogus argument.
    Assert(0);
    return;
  }

  DWORD UserIndex = Convert<DWORD>(Context->UserIndex);

  XINPUT_STATE NewControllerState;
  if(XInput->XInputGetState(UserIndex, &NewControllerState) != ERROR_SUCCESS)
  {
    // The gamepad seems to be disconnected.
    return;
  }

  if(Context->XInputPreviousState[UserIndex].dwPacketNumber == NewControllerState.dwPacketNumber)
  {
    // There are no updates for us.
    return;
  }

  auto NewControllerStatePtr = &NewControllerState;
  Defer [=](){ Context->XInputPreviousState[UserIndex] = *NewControllerStatePtr; };

  auto OldGamepad = &Context->XInputPreviousState[UserIndex].Gamepad;
  auto NewGamepad = &NewControllerState.Gamepad;

  //
  // Buttons
  //
  {
    auto UpdateButton = [](input_slot Slot, WORD OldButtons, WORD NewButtons, WORD ButtonMask)
    {
      bool WasDown = (OldButtons & ButtonMask) != 0;
      bool IsDown  = (NewButtons & ButtonMask) != 0;

      if(WasDown != IsDown)
      {
        InputSetButton(Slot, IsDown);
      }
    };

    UpdateButton(Slots->DPadUp,      OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_DPAD_UP);
    UpdateButton(Slots->DPadDown,    OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
    UpdateButton(Slots->DPadLeft,    OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
    UpdateButton(Slots->DPadRight,   OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
    UpdateButton(Slots->Start,       OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_START);
    UpdateButton(Slots->Back,        OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_BACK);
    UpdateButton(Slots->LeftThumb,   OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_LEFT_THUMB);
    UpdateButton(Slots->RightThumb,  OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_RIGHT_THUMB);
    UpdateButton(Slots->LeftBumper,  OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
    UpdateButton(Slots->RightBumper, OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
    UpdateButton(Slots->A,           OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_A);
    UpdateButton(Slots->B,           OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_B);
    UpdateButton(Slots->X,           OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_X);
    UpdateButton(Slots->Y,           OldGamepad->wButtons, NewGamepad->wButtons, XINPUT_GAMEPAD_Y);
  }

  //
  // Triggers
  //
  {
    auto UpdateTrigger = [](input_slot Slot, BYTE OldValue, BYTE NewValue)
    {
      if(OldValue != NewValue)
      {
        float NormalizedValue = NewValue / 255.0f;
        InputSetAxisValue(Slot, NormalizedValue);
      }
    };

    UpdateTrigger(Slots->LeftTrigger, OldGamepad->bLeftTrigger, NewGamepad->bLeftTrigger);
    UpdateTrigger(Slots->RightTrigger, OldGamepad->bRightTrigger, NewGamepad->bRightTrigger);
  }

  //
  // Thumbsticks
  //
  {
    auto UpdateThumbStick = [](input_slot Slot, SHORT OldValue, SHORT NewValue)
    {
      if(OldValue != NewValue)
      {
        float NormalizedValue;
        if(NewValue > 0) NormalizedValue = NewValue / 32767.0f;
        else             NormalizedValue = NewValue / 32768.0f;
        InputSetAxisValue(Slot, NormalizedValue);
      }
    };

    UpdateThumbStick(Slots->XLeftStick, OldGamepad->sThumbLX, NewGamepad->sThumbLX);
    UpdateThumbStick(Slots->YLeftStick, OldGamepad->sThumbLY, NewGamepad->sThumbLY);
    UpdateThumbStick(Slots->XRightStick, OldGamepad->sThumbRX, NewGamepad->sThumbRX);
    UpdateThumbStick(Slots->YRightStick, OldGamepad->sThumbRY, NewGamepad->sThumbRY);
  }
}

auto
::Win32RegisterMouseSlots(input_context Context, input_mouse_slots* Mouse, log_data* Log)
  -> void
{
  #define REG_BUTTON(Name) Mouse->Name = InputRegisterSlot(Context, input_type::Button, input_mouse_slot_names::Name);
  #define REG_AXIS(Name)   Mouse->Name = InputRegisterSlot(Context, input_type::Axis, input_mouse_slot_names::Name);
  #define REG_ACTION(Name) Mouse->Name = InputRegisterSlot(Context, input_type::Action, input_mouse_slot_names::Name);

  REG_BUTTON(LeftButton);
  REG_BUTTON(MiddleButton);
  REG_BUTTON(RightButton);
  REG_BUTTON(ExtraButton1);
  REG_BUTTON(ExtraButton2);

  REG_AXIS(XPosition);
  REG_AXIS(YPosition);

  REG_ACTION(XDelta);
  REG_ACTION(YDelta);
  REG_ACTION(VerticalWheelDelta);
  REG_ACTION(HorizontalWheelDelta);
  REG_ACTION(LeftButton_DoubleClick);
  REG_ACTION(MiddleButton_DoubleClick);
  REG_ACTION(RightButton_DoubleClick);
  REG_ACTION(ExtraButton1_DoubleClick);
  REG_ACTION(ExtraButton2_DoubleClick);

  #undef REG_ACTION
  #undef REG_AXIS
  #undef REG_BUTTON

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
::Win32RegisterXInputSlots(input_context Context, input_x_input_slots* XInput, log_data* Log)
  -> void
{
  #define REG_BUTTON(Name) XInput->Name = InputRegisterSlot(Context, input_type::Button, input_x_input_slot_names::Name);
  #define REG_AXIS(Name, PosValue, NegValue) \
  XInput->Name = InputRegisterSlot(Context, input_type::Axis, input_x_input_slot_names::Name); \
  InputSetPositiveDeadZone(XInput->Name, PosValue); \
  InputSetNegativeDeadZone(XInput->Name, NegValue);

  REG_BUTTON(DPadUp);
  REG_BUTTON(DPadDown);
  REG_BUTTON(DPadLeft);
  REG_BUTTON(DPadRight);
  REG_BUTTON(Start);
  REG_BUTTON(Back);
  REG_BUTTON(LeftThumb);
  REG_BUTTON(RightThumb);
  REG_BUTTON(LeftBumper);
  REG_BUTTON(RightBumper);
  REG_BUTTON(A);
  REG_BUTTON(B);
  REG_BUTTON(X);
  REG_BUTTON(Y);

  REG_AXIS(LeftTrigger,  XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f,      XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f);
  REG_AXIS(RightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f,      XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f);
  REG_AXIS(XLeftStick,   XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f,  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32768.0f);
  REG_AXIS(YLeftStick,   XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f,  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32768.0f);
  REG_AXIS(XRightStick,  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32768.0f);
  REG_AXIS(YRightStick,  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32768.0f);

  #undef REG_BUTTON
  #undef REG_AXIS
}

auto
::Win32RegisterKeyboardSlots(input_context Context, input_keyboard_slots* Keyboard, log_data* Log)
  -> void
{
  #define REG_BUTTON(Name) Keyboard->Name = InputRegisterSlot(Context, input_type::Button, input_keyboard_slot_names::Name)

  REG_BUTTON(Escape);
  REG_BUTTON(Space);
  REG_BUTTON(Tab);
  REG_BUTTON(LeftShift);
  REG_BUTTON(LeftControl);
  REG_BUTTON(LeftAlt);
  REG_BUTTON(LeftSystem);
  REG_BUTTON(RightShift);
  REG_BUTTON(RightControl);
  REG_BUTTON(RightAlt);
  REG_BUTTON(RightSystem);
  REG_BUTTON(Shift);
  REG_BUTTON(Control);
  REG_BUTTON(Alt);
  REG_BUTTON(System);
  REG_BUTTON(Application);
  REG_BUTTON(Backspace);
  REG_BUTTON(Return);

  REG_BUTTON(Insert);
  REG_BUTTON(Delete);
  REG_BUTTON(Home);
  REG_BUTTON(End);
  REG_BUTTON(PageUp);
  REG_BUTTON(PageDown);

  REG_BUTTON(Up);
  REG_BUTTON(Down);
  REG_BUTTON(Left);
  REG_BUTTON(Right);

  //
  // Digit Keys
  //
  REG_BUTTON(Digit_0);
  REG_BUTTON(Digit_1);
  REG_BUTTON(Digit_2);
  REG_BUTTON(Digit_3);
  REG_BUTTON(Digit_4);
  REG_BUTTON(Digit_5);
  REG_BUTTON(Digit_6);
  REG_BUTTON(Digit_7);
  REG_BUTTON(Digit_8);
  REG_BUTTON(Digit_9);

  //
  // Numpad
  //
  REG_BUTTON(Numpad_Add);
  REG_BUTTON(Numpad_Subtract);
  REG_BUTTON(Numpad_Multiply);
  REG_BUTTON(Numpad_Divide);
  REG_BUTTON(Numpad_Decimal);
  REG_BUTTON(Numpad_Enter);

  REG_BUTTON(Numpad_0);
  REG_BUTTON(Numpad_1);
  REG_BUTTON(Numpad_2);
  REG_BUTTON(Numpad_3);
  REG_BUTTON(Numpad_4);
  REG_BUTTON(Numpad_5);
  REG_BUTTON(Numpad_6);
  REG_BUTTON(Numpad_7);
  REG_BUTTON(Numpad_8);
  REG_BUTTON(Numpad_9);

  //
  // F-Keys
  //
  REG_BUTTON(F1);
  REG_BUTTON(F2);
  REG_BUTTON(F3);
  REG_BUTTON(F4);
  REG_BUTTON(F5);
  REG_BUTTON(F6);
  REG_BUTTON(F7);
  REG_BUTTON(F8);
  REG_BUTTON(F9);
  REG_BUTTON(F10);
  REG_BUTTON(F11);
  REG_BUTTON(F12);
  REG_BUTTON(F13);
  REG_BUTTON(F14);
  REG_BUTTON(F15);
  REG_BUTTON(F16);
  REG_BUTTON(F17);
  REG_BUTTON(F18);
  REG_BUTTON(F19);
  REG_BUTTON(F20);
  REG_BUTTON(F21);
  REG_BUTTON(F22);
  REG_BUTTON(F23);
  REG_BUTTON(F24);

  //
  // Keys
  //
  REG_BUTTON(A);
  REG_BUTTON(B);
  REG_BUTTON(C);
  REG_BUTTON(D);
  REG_BUTTON(E);
  REG_BUTTON(F);
  REG_BUTTON(G);
  REG_BUTTON(H);
  REG_BUTTON(I);
  REG_BUTTON(J);
  REG_BUTTON(K);
  REG_BUTTON(L);
  REG_BUTTON(M);
  REG_BUTTON(N);
  REG_BUTTON(O);
  REG_BUTTON(P);
  REG_BUTTON(Q);
  REG_BUTTON(R);
  REG_BUTTON(S);
  REG_BUTTON(T);
  REG_BUTTON(U);
  REG_BUTTON(V);
  REG_BUTTON(W);
  REG_BUTTON(X);
  REG_BUTTON(Y);
  REG_BUTTON(Z);

  #undef REG_BUTTON
}

