#pragma once

#include "CoreAPI.hpp"
#include "Input.hpp"

#include <Backbone.hpp>

#include <Windows.h>
#include <XInput.h>


struct log_data;


struct CORE_API x_input_dll
{
  HMODULE DLL;
  slice<char const> DLLName;

  decltype(::XInputGetState)*              XInputGetState;
  decltype(::XInputSetState)*              XInputSetState;
  decltype(::XInputGetCapabilities)*       XInputGetCapabilities;
  decltype(::XInputGetAudioDeviceIds)*     XInputGetAudioDeviceIds;
  decltype(::XInputGetBatteryInformation)* XInputGetBatteryInformation;
  decltype(::XInputGetKeystroke)*          XInputGetKeystroke;
};

CORE_API
bool
Win32LoadXInput(x_input_dll* XInput, log_data* Log = nullptr);


CORE_API
bool
Win32ProcessInputMessage(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam,
                         input_context ContextHandle,
                         input_keyboard_slots* Keyboard,
                         input_mouse_slots* Mouse,
                         log_data* Log = nullptr);

CORE_API
slice<char const*>
Win32VirtualKeyToInputId(WPARAM VKCode, LPARAM lParam, slice<char const*> Buffer);

CORE_API
void
Win32PollXInput(x_input_dll* XInput, input_context ContextHandle, input_x_input_slots* Slots);

CORE_API
void
Win32RegisterMouseSlots(input_context ContextHandle,
                        input_mouse_slots* Mouse,
                        log_data* Log = nullptr);

CORE_API
void
Win32RegisterXInputSlots(input_context ContextHandle,
                         input_x_input_slots* XInput,
                         log_data* Log = nullptr);

CORE_API
void
Win32RegisterKeyboardSlots(input_context ContextHandle,
                           input_keyboard_slots* Keyboard,
                           log_data* Log = nullptr);
