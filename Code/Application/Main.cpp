#include "VulkanHelper.hpp"

#include <Core/DynamicArray.hpp>
#include <Core/Log.hpp>
#include <Core/Input.hpp>
#include <Core/Win32_Input.hpp>

#include <Backbone.hpp>

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

static void
Verify(VkResult Result)
{
  Assert(Result == VK_SUCCESS);
}

static bool
VulkanCreateInstance(vulkan* Vulkan, allocator_interface* TempAllocator)
{
  Assert(Vulkan->DLL);
  Assert(Vulkan->F.vkCreateInstance);
  Assert(Vulkan->F.vkEnumerateInstanceLayerProperties);
  Assert(Vulkan->F.vkEnumerateInstanceExtensionProperties);

  //
  // Instance Layers
  //
  scoped_array<char const*> LayerNames{ TempAllocator };
  {
    uint32 LayerCount;
    Verify(Vulkan->F.vkEnumerateInstanceLayerProperties(&LayerCount, nullptr));

    scoped_array<VkLayerProperties> LayerProperties = { TempAllocator };
    ExpandBy(&LayerProperties, LayerCount);
    Verify(Vulkan->F.vkEnumerateInstanceLayerProperties(&LayerCount, LayerProperties.Ptr));

    LogBeginScope("Explicitly enabled instance layers:");
    Defer(, LogEndScope("=========="));
    for(auto& Property : Slice(&LayerProperties))
    {
      auto LayerName = SliceFromString(Property.layerName);
      if(LayerName == SliceFromString("VK_LAYER_LUNARG_standard_validation"))
      {
        Expand(&LayerNames) = "VK_LAYER_LUNARG_standard_validation";
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
  scoped_array<char const*> ExtensionNames{ TempAllocator };
  {
    // Required extensions:
    bool SurfaceExtensionFound = false;
    bool PlatformSurfaceExtensionFound = false;

    uint32_t ExtensionCount;
    Verify(Vulkan->F.vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    scoped_array<VkExtensionProperties> ExtensionProperties = { TempAllocator };
    ExpandBy(&ExtensionProperties, ExtensionCount);
    Verify(Vulkan->F.vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, ExtensionProperties.Ptr));

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

    CreateInfo.enabledExtensionCount = Cast<uint32_t>(ExtensionNames.Num);
    CreateInfo.ppEnabledExtensionNames = ExtensionNames.Ptr;

    CreateInfo.enabledLayerCount = Cast<uint32_t>(LayerNames.Num);
    CreateInfo.ppEnabledLayerNames = LayerNames.Ptr;

    Verify(Vulkan->F.vkCreateInstance(&CreateInfo, nullptr, &Vulkan->InstanceHandle));
  }

  return true;
}

static void
VulkanDestroyInstance(vulkan* Vulkan)
{
  VkAllocationCallbacks const* AllocationCallbacks = nullptr;
  Vulkan->F.vkDestroyInstance(Vulkan->InstanceHandle, AllocationCallbacks);
}

static void
Win32SetupConsole(char const* Title)
{
  AllocConsole();
  AttachConsole(GetCurrentProcessId());
  freopen("CON", "w", stdout);
  SetConsoleTitleA(Title);
}

VKAPI_ATTR
VkBool32 VKAPI_CALL
VulkanDebugCallback(
  VkDebugReportFlagsEXT      Flags,
  VkDebugReportObjectTypeEXT ObjectType,
  uint64_t                   Object,
  size_t                     Location,
  int32_t                    MessageCode,
  char const*                pLayerPrefix,
  char const*                pMessage,
  void*                      pUserData)
{
  if(Flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
  {
    LogError("[%s] Code %d: %s", pLayerPrefix, MessageCode, pMessage);
  }
  else if(Flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
  {
    LogWarning("[%s] Code %d: %s", pLayerPrefix, MessageCode, pMessage);
  }
  else
  {
    LogInfo("[%s] Code %d: %s", pLayerPrefix, MessageCode, pMessage);
  }

  return false;
}

static bool
VulkanSetupDebugging(vulkan* Vulkan)
{
  LogBeginScope("Setting up Vulkan debugging.");
  Defer(, LogEndScope("Finished debug setup."));

  if(Vulkan->F.vkCreateDebugReportCallbackEXT != nullptr)
  {
    VkDebugReportCallbackCreateInfoEXT DebugSetupInfo = {};
    DebugSetupInfo.pfnCallback = &VulkanDebugCallback;
    DebugSetupInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

    Verify(Vulkan->F.vkCreateDebugReportCallbackEXT(Vulkan->InstanceHandle,
                                                  &DebugSetupInfo,
                                                  nullptr,
                                                  &Vulkan->DebugCallbackHandle));
    return true;
  }
  else
  {
    LogWarning("Unable to set up debugging: vkCreateDebugReportCallbackEXT is nullptr");
    return false;
  }
}

static void
VulkanCleanupDebugging(vulkan* Vulkan)
{
  if(Vulkan->F.vkDestroyDebugReportCallbackEXT && Vulkan->DebugCallbackHandle)
  {
    Vulkan->F.vkDestroyDebugReportCallbackEXT(Vulkan->InstanceHandle, Vulkan->DebugCallbackHandle, nullptr);
  }
}


bool
VulkanChooseAndSetupPhysicalDevices(vulkan* Vulkan, allocator_interface* TempAllocator)
{
  uint32_t GpuCount;
  Verify(Vulkan->F.vkEnumeratePhysicalDevices(Vulkan->InstanceHandle,
                                            &GpuCount, nullptr));
  if(GpuCount == 0)
  {
    LogError("No GPUs found.");
    return false;
  }

  LogInfo("Found %u physical device(s).", GpuCount);

  scoped_array<VkPhysicalDevice> Gpus{ TempAllocator };
  ExpandBy(&Gpus, GpuCount);

  Verify(Vulkan->F.vkEnumeratePhysicalDevices(Vulkan->InstanceHandle,
                                            &GpuCount, Gpus.Ptr));

  // Use the first Physical Device for now.
  uint32_t const GpuIndex = 0;
  Vulkan->Gpu.GpuHandle = Gpus[GpuIndex];

  //
  // Properties
  //
  {
    LogBeginScope("Querying for physical device and queue properties.");
    Defer(, LogEndScope("Retrieved physical device and queue properties."));

    Vulkan->F.vkGetPhysicalDeviceProperties(Vulkan->Gpu.GpuHandle, &Vulkan->Gpu.Properties);
    Vulkan->F.vkGetPhysicalDeviceMemoryProperties(Vulkan->Gpu.GpuHandle, &Vulkan->Gpu.MemoryProperties);
    Vulkan->F.vkGetPhysicalDeviceFeatures(Vulkan->Gpu.GpuHandle, &Vulkan->Gpu.Features);

    uint32_t QueueCount;
    Vulkan->F.vkGetPhysicalDeviceQueueFamilyProperties(Vulkan->Gpu.GpuHandle, &QueueCount, nullptr);
    Clear(&Vulkan->Gpu.QueueProperties);
    ExpandBy(&Vulkan->Gpu.QueueProperties, QueueCount);
    Vulkan->F.vkGetPhysicalDeviceQueueFamilyProperties(Vulkan->Gpu.GpuHandle, &QueueCount, Vulkan->Gpu.QueueProperties.Ptr);
  }

  return true;
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

  {
    auto Vulkan = Allocate<vulkan>(Allocator);
    MemDefaultConstruct(1, Vulkan);
    Defer(=, Deallocate(Allocator, Vulkan));

    Init(Vulkan, Allocator);
    Defer(=, Finalize(Vulkan));

    if(!VulkanLoadDLL(Vulkan))
      return 1;

    if(!VulkanCreateInstance(Vulkan, Allocator))
      return 2;
    Defer(=, VulkanDestroyInstance(Vulkan));

    VulkanLoadInstanceFunctions(Vulkan);

    VulkanSetupDebugging(Vulkan);
    Defer(=, VulkanCleanupDebugging(Vulkan));

    if(!VulkanChooseAndSetupPhysicalDevices(Vulkan, Allocator))
      return 4;

    // TODO Create and open window.

    // if(!VulkanInitializeForGraphics(&Vulkan, Instance, Window.WindowHandle))
    //   return 5;
  }

  return 0;
}
