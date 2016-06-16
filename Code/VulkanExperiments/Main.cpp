#include <Backbone.hpp>

#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

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

void
Verify(VkResult Result)
{
  Assert(Result == VK_SUCCESS);
}

bool
CreateVulkanInstance(allocator_interface* TempAllocator, vulkan_data* Vulkan)
{
  //
  // Load DLL
  //
  {
    LogBeginScope("Loading Vulkan DLL.");
    Defer(=, LogEndScope(""));

    char const* FileName = "vulkan-1.dll";
    Vulkan->DLL = LoadLibraryA(FileName);
    if(Vulkan->DLL)
    {
      fixed_block<KiB(1), char> BufferMemory;
      auto Buffer = Slice(BufferMemory);
      auto CharCount = GetModuleFileNameA(Vulkan->DLL, Buffer.Data, Cast<DWORD>(Buffer.Num));
      Vulkan->DLLName = Slice(Buffer, DWORD(0), CharCount);
      LogInfo("Loaded Vulkan DLL: %*s", Vulkan->DLLName.Num, Vulkan->DLLName.Data);
    }
    else
    {
      LogError("Failed to load DLL: %s", FileName);
      return false;
    }
  }

  //
  // Load Crucial Function Pointers
  //
  PFN_vkCreateInstance vkCreateInstance = {};
  {
    LogBeginScope("First Stage of loading Vulkan function pointers.");
    Defer(=, LogEndScope(""));

    // vkCreateInstance
    {
      auto Func = GetProcAddress(Vulkan->DLL, "vkCreateInstance");
      if(Func)
      {
        // TODO Cast and save the function pointer somewhere.
        vkCreateInstance = Reinterpret<decltype(vkCreateInstance)>(Func);
      }
      else
      {
        LogError("Unable to load function Vulkan DLL: vkCreateInstance");
        return false;
      }
    }
  }

  //
  // Create Instance
  //
  {
    LogBeginScope("Creating Vulkan instance.");
    Defer(, LogEndScope(""));

    // Instance Layers
    dynamic_array<char const*> LayerNames = {};
    Init(&LayerNames, TempAllocator);
    Defer(&LayerNames, Finalize(&LayerNames));
    {
      // TODO
    }

    // Instance Extensions
    dynamic_array<char const*> ExtensionNames = {};
    Init(&ExtensionNames, TempAllocator);
    Defer(&LayerNames, Finalize(&LayerNames));
    {
      // TODO
    }

    VkApplicationInfo ApplicationInfo = {};
    ApplicationInfo.pApplicationName = "Vulkan Experiments";
    ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);

    ApplicationInfo.pEngineName = "Backbone";
    ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);

    ApplicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 13);

    VkInstanceCreateInfo CreateInfo = {};
    CreateInfo.pApplicationInfo = &ApplicationInfo;

    CreateInfo.enabledExtensionCount = Cast<uint>(ExtensionNames.Num);
    CreateInfo.ppEnabledExtensionNames = Data(&ExtensionNames).Data;

    CreateInfo.enabledLayerCount = Cast<uint>(LayerNames.Num);
    CreateInfo.ppEnabledLayerNames = Data(&LayerNames).Data;

    Verify(vkCreateInstance(&CreateInfo, nullptr, &Vulkan->Instance));
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
  GlobalLog = &Log;
  Defer(=, GlobalLog = nullptr);

  LogInit(GlobalLog, &Mallocator);
  PushBack(&GlobalLog->Sinks, log_sink(StdoutLogSink));
  PushBack(&GlobalLog->Sinks, log_sink(VisualStudioLogSink));

  vulkan_data Vulkan;

  if(!CreateVulkanInstance(&Mallocator, &Vulkan))
  {
    LogError("Failed to create vulkan instance.");
    return 1;
  }

  // TODO Create and open window.

  return 0;
}
