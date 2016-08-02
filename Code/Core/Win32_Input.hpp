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


class CORE_API win32_input_context : public input_context
{
public:
  fixed_block<XUSER_MAX_COUNT, XINPUT_STATE> XInputPreviousState;

  virtual ~win32_input_context();
};

CORE_API
void
Init(win32_input_context* Context, allocator_interface* Allocator);

CORE_API
void
Finalize(win32_input_context* Context);

CORE_API
bool
Win32ProcessInputMessage(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam,
                         win32_input_context* Input,
                         log_data* Log = nullptr);

CORE_API
slice<input_id>
Win32VirtualKeyToInputId(WPARAM VKCode, LPARAM lParam, slice<input_id> Buffer);

CORE_API
void
Win32PollXInput(x_input_dll* XInput, win32_input_context* Input);

CORE_API
void
Win32RegisterAllMouseSlots(win32_input_context* Context,
                           log_data* Log = nullptr);

CORE_API
void
Win32RegisterAllXInputSlots(win32_input_context* Context,
                            log_data* Log = nullptr);

CORE_API
void
Win32RegisterAllKeyboardSlots(win32_input_context* Context,
                              log_data* Log = nullptr);
