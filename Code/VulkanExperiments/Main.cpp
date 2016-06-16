#include <Backbone.hpp>
#include <vulkan/vk_cpp.hpp>

#include <Windows.h>

#include "DynamicArray.hpp"
#include "Log.hpp"

struct vulkan_data
{
  bool IsPrepared;

  HMODULE DLL;
  slice<char> DLLName;

  VkInstance Instance;
};

struct window_construction_data
{
  HINSTANCE ProcessHandle;
  slice<char> WindowClassName;
  int ClientWidth;
  int ClientHeight;

  int WindowX;
  int WindowY;
};

struct window_data
{
  HWND Handle;
  int PositionX;
  int PositionY;
  int ClientWidth;
  int ClientHeight;

  vulkan_data* Vulkan;
};

window_data
Win32CreateWindow(allocator_interface* Allocator, const window_construction_data& Args,
                  log_data* Log = nullptr)
{
  // TODO
  return window_data();
}

void
Win32DestroyWindow(allocator_interface* Allocator, const window_data& Window)
{
  // TODO
}

bool
CreateVulkanInstance(vulkan_data* Vulkan)
{
  //
  // Load DLL
  //
  {
  }

  return true;
}

void Win32SetupConsole(char const* Title)
{
  AllocConsole();
  AttachConsole(GetCurrentProcessId());
  freopen("CON", "w", stdout);
  SetConsoleTitleA(Title);
}

int WinMain(HINSTANCE Instance, HINSTANCE PreviousINstance,
            LPSTR CommandLine, int ShowCode)
{
  Win32SetupConsole("Vulkan Experiments Console");

  mallocator Mallocator = {};
  log_data Log = {};
  LogInit(&Log, &Mallocator);
  ArrayPushBack(&Log.Sinks, log_sink(StdoutLogSink));
  ArrayPushBack(&Log.Sinks, log_sink(VisualStudioLogSink));

  LogInfo(&Log, "Hello logging world!");

  auto Message = CreateSliceFromString("Hello slicing world!");
  LogInfo(&Log, Message);

  return 0;
}
