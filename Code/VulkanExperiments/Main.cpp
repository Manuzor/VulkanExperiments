#include "DynamicArray.hpp"
#include "Log.hpp"
#include "Input.hpp"
#include "Win32_Input.hpp"

#include "VulkanHelper.hpp"

#include <Backbone.hpp>
#include <Backbone.cpp>

#include <Windows.h>


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

  vulkan* Vulkan;
};

#if 0

static window_data
Win32CreateWindow(allocator_interface* Allocator, const window_construction_data& Args,
                  log_data* Log = nullptr)
{
  // TODO
  return window_data();
}

static void
Win32DestroyWindow(allocator_interface* Allocator, const window_data& Window)
{
  // TODO
}

#endif

#if 0
static void
Verify(VkResult Result)
{
  Assert(Result == VK_SUCCESS);
}

static bool
CreateVulkanInstance(vulkan* Vulkan, allocator_interface* TempAllocator)
{
  Assert(Vulkan->DLL);
  Assert(Vulkan->vkCreateInstance);
  Assert(Vulkan->vkEnumerateInstanceLayerProperties);
  Assert(Vulkan->vkEnumerateInstanceExtensionProperties);

  //
  // Instance Layers
  //
  dynamic_array<char const*> LayerNames = {};
  Init(&LayerNames, TempAllocator);
  Defer(&LayerNames, Finalize(&LayerNames));
  {
    uint32 LayerCount;
    Verify(Vulkan->vkEnumerateInstanceLayerProperties(&LayerCount, nullptr));

    dynamic_array<VkLayerProperties> LayerProperties;
    Init(&LayerProperties, TempAllocator);
    Defer(&LayerProperties, Finalize(&LayerProperties));
    ExpandBy(&LayerProperties, LayerCount);
    Verify(Vulkan->vkEnumerateInstanceLayerProperties(&LayerCount, LayerProperties.Ptr));

    LogBeginScope("Explicitly enabled instance layers:");
    Defer(, LogEndScope("=========="));
    for(auto& Property : Slice(&LayerProperties))
    {
      auto LayerName = SliceFromString(Property.layerName);
      if(LayerName == SliceFromString("VK_LAYER_LUNARG_standard_validation"))
      {
        Expand(&LayerNames) = LayerName.Ptr;
      }
      else
      {
        LogInfo("[ ] %s", LayerName.Ptr);
        continue;
      }

      LogInfo("[x] %s", LayerName.Ptr);
    }
  }

  //
  // Instance Extensions
  //
  dynamic_array<char const*> ExtensionNames = {};
  Init(&ExtensionNames, TempAllocator);
  Defer(&LayerNames, Finalize(&LayerNames));
  {
    // Required extensions:
    bool SurfaceExtensionFound = false;
    bool PlatformSurfaceExtensionFound = false;

    uint ExtensionCount;
    Verify(Vulkan->vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    dynamic_array<VkExtensionProperties> ExtensionProperties;
    Init(&ExtensionProperties, TempAllocator);
    Defer(&ExtensionProperties, Finalize(&ExtensionProperties));
    ExpandBy(&ExtensionProperties, ExtensionCount);
    Verify(Vulkan->vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, ExtensionProperties.Ptr));

    LogBeginScope("Explicitly enabled instance extensions:");
    Defer(, LogEndScope("=========="));

    for(auto& Property : Slice(&ExtensionProperties))
    {
      auto ExtensionName = SliceFromString(Property.extensionName);
      if(ExtensionName == SliceFromString(VK_KHR_SURFACE_EXTENSION_NAME))
      {
        Expand(&ExtensionNames) = VK_KHR_SURFACE_EXTENSION_NAME;
        SurfaceExtensionFound = true;
      }
      else if(ExtensionName == SliceFromString(VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
      {
        Expand(&ExtensionNames) = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
        PlatformSurfaceExtensionFound = true;
      }
      else if(ExtensionName == SliceFromString(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
      {
        Expand(&ExtensionNames) = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
      }
      else
      {
        LogInfo("  [ ] %s", ExtensionName.Ptr);
        continue;
      }

      LogInfo("  [x] %s", ExtensionName.Ptr);
    }

    bool Success = true;

    if(!SurfaceExtensionFound)
    {
      LogError("Failed to load required extension: %s", VK_KHR_SURFACE_EXTENSION_NAME);
      Success = false;
    }

    if(!PlatformSurfaceExtensionFound)
    {
      LogError("Failed to load required extension: %s", VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
      Success = false;
    }

    if(!Success)
      return false;
  }

  //
  // Create Instance
  //
  {
    LogBeginScope("Creating Vulkan instance.");
    Defer(, LogEndScope(""));

    VkApplicationInfo ApplicationInfo = {};
    ApplicationInfo.pApplicationName = "Vulkan Experiments";
    ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);

    ApplicationInfo.pEngineName = "Backbone";
    ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);

    ApplicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 13);

    VkInstanceCreateInfo CreateInfo = {};
    CreateInfo.pApplicationInfo = &ApplicationInfo;

    CreateInfo.enabledExtensionCount = Cast<uint32>(ExtensionNames.Num);
    CreateInfo.ppEnabledExtensionNames = ExtensionNames.Ptr;

    CreateInfo.enabledLayerCount = Cast<uint32>(LayerNames.Num);
    CreateInfo.ppEnabledLayerNames = LayerNames.Ptr;

    Verify(vkCreateInstance(&CreateInfo, nullptr, &Vulkan->InstanceHandle));
  }

  return true;
}

#endif

static void
Win32SetupConsole(char const* Title)
{
  AllocConsole();
  AttachConsole(GetCurrentProcessId());
  freopen("CON", "w", stdout);
  SetConsoleTitleA(Title);
}

int
WinMain(HINSTANCE Instance, HINSTANCE PreviousINstance,
        LPSTR CommandLine, int ShowCode)
{
  Win32SetupConsole("Vulkan Experiments Console");

  mallocator Mallocator = {};
  allocator_interface* Allocator = &Mallocator;
  log_data Log = {};
  GlobalLog = &Log;
  Defer(=, GlobalLog = nullptr);

  Init(GlobalLog, Allocator);
  Defer(=, Finalize(GlobalLog));
  auto SinkSlots = ExpandBy(&GlobalLog->Sinks, 2);
  SinkSlots[0] = log_sink(StdoutLogSink);
  SinkSlots[1] = log_sink(VisualStudioLogSink);

  x_input_dll XInput;
  Win32LoadXInput(&XInput, GlobalLog);

  vulkan Vulkan;

  if(!LoadVulkanDLL(&Vulkan))
    return 1;

  // if(!CreateVulkanInstance(&Vulkan, Allocator))
  //   return 2;

  if(!LoadInstanceFunctions(&Vulkan))
    return 3;

  // TODO Create and open window.

  return 0;
}
