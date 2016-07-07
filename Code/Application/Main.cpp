#include "VulkanHelper.hpp"

#include <Core/DynamicArray.hpp>
#include <Core/Log.hpp>
#include <Core/Input.hpp>
#include <Core/Win32_Input.hpp>
#include <Core/Time.hpp>

#include <Core/Image.hpp>
#include <Core/ImageLoader.hpp>

#include <Core/Math.hpp>

#include <Core/Camera.hpp>

#include <Backbone.hpp>

#include <Windows.h>

static bool GlobalRunning = false;
static bool GlobalIsResizeRequested = false;
static int GlobalResizeRequest_Width = 0;
static int GlobalResizeRequest_Height = 0;

static LRESULT WINAPI
Win32MainWindowCallback(HWND WindowHandle, UINT Message,
                        WPARAM WParam, LPARAM LParam);

struct window_setup
{
  HINSTANCE ProcessHandle;
  char const* WindowClassName;
  int ClientWidth;
  int ClientHeight;

  bool HasCustomWindowX;
  int WindowX;

  bool HasCustomWindowY;
  int WindowY;
};

struct window
{
  HWND WindowHandle;
  int ClientWidth;
  int ClientHeight;

  win32_input_context* Input;
  vulkan* Vulkan;
};


static window*
Win32CreateWindow(allocator_interface* Allocator, window_setup const* Args,
                  log_data* Log = nullptr)
{
  if(Args->ProcessHandle == INVALID_HANDLE_VALUE)
  {
    LogError("Need a valid process handle.");
    return nullptr;
  }

  if(Args->WindowClassName == nullptr)
  {
    LogError("Need a window class name.");
    return nullptr;
  }

  if(Args->ClientWidth == 0)
  {
    LogError("Need a window client width.");
    return nullptr;
  }

  if(Args->ClientHeight == 0)
  {
    LogError("Need a window client height.");
    return nullptr;
  }

  WNDCLASSA WindowClass = {};
  WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  WindowClass.lpfnWndProc = &Win32MainWindowCallback;
  WindowClass.hInstance = Args->ProcessHandle;
  WindowClass.lpszClassName = Args->WindowClassName;

  if(!RegisterClassA(&WindowClass))
  {
    LogError("Failed to register window class: %s", WindowClass.lpszClassName);
    return nullptr;
  }

  DWORD const WindowStyleEx = 0;
  DWORD const WindowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

  RECT WindowRect = {};
  WindowRect.right = Args->ClientWidth;
  WindowRect.bottom = Args->ClientHeight;
  AdjustWindowRectEx(&WindowRect, WindowStyle, FALSE, WindowStyleEx);

  WindowRect.right -= WindowRect.left;
  WindowRect.bottom -= WindowRect.top;

  // Apply user translation
  WindowRect.left = Args->HasCustomWindowX ? Cast<LONG>(Args->WindowX) : 0;
  WindowRect.right += WindowRect.left;
  WindowRect.top = Args->HasCustomWindowY ? Cast<LONG>(Args->WindowY) : 0;
  WindowRect.bottom += WindowRect.top;


  RECT WindowWorkArea = {};
  SystemParametersInfoW(SPI_GETWORKAREA, 0, &WindowWorkArea, 0);
  WindowRect.left   += WindowWorkArea.left;
  WindowRect.right  += WindowWorkArea.left;
  WindowRect.top    += WindowWorkArea.top;
  WindowRect.bottom += WindowWorkArea.top;

  HWND WindowHandle;
  auto const WindowX = Args->HasCustomWindowX ? WindowRect.left : CW_USEDEFAULT;
  auto const WindowY = Args->HasCustomWindowY ? WindowRect.top : CW_USEDEFAULT;
  auto const WindowWidth = WindowRect.right - WindowRect.left;
  auto const WindowHeight = WindowRect.bottom - WindowRect.top;
  WindowHandle = CreateWindowExA(WindowStyleEx,             // _In_     DWORD     dwExStyle
                                 WindowClass.lpszClassName, // _In_opt_ LPCWSTR   lpClassName
                                 "The Title Text",          // _In_opt_ LPCWSTR   lpWindowName
                                 WindowStyle,               // _In_     DWORD     dwStyle
                                 WindowX, WindowY,          // _In_     int       X, Y
                                 WindowWidth, WindowHeight, // _In_     int       nWidth, nHeight
                                 nullptr,                   // _In_opt_ HWND      hWndParent
                                 nullptr,                   // _In_opt_ HMENU     hMenu
                                 Args->ProcessHandle,       // _In_opt_ HINSTANCE hInstance
                                 nullptr);                  // _In_opt_ LPVOID    lpParam

  if(WindowHandle == nullptr)
  {
    LogError("Failed to create window.");
    return nullptr;
  }

  auto Window = Allocate<window>(Allocator);
  *Window = {};
  Window->WindowHandle = WindowHandle;
  Window->ClientWidth = Args->ClientWidth;
  Window->ClientHeight = Args->ClientHeight;

  SetWindowLongPtr(Window->WindowHandle, GWLP_USERDATA, Reinterpret<LONG_PTR>(Window));

  return Window;
}

static void
Win32DestroyWindow(allocator_interface* Allocator, window* Window)
{
  // TODO: Close the window somehow?
  Deallocate(Allocator, Window);
}

static void
Verify(VkResult Result)
{
  Assert(Result == VK_SUCCESS);
}

static bool
VulkanCreateInstance(vulkan* Vulkan, allocator_interface* TempAllocator)
{
  Assert(Vulkan->DLL);
  Assert(Vulkan->vkCreateInstance);
  Assert(Vulkan->vkEnumerateInstanceLayerProperties);
  Assert(Vulkan->vkEnumerateInstanceExtensionProperties);

  //
  // Instance Layers
  //
  scoped_array<char const*> LayerNames{ TempAllocator };
  {
    uint32 LayerCount;
    Verify(Vulkan->vkEnumerateInstanceLayerProperties(&LayerCount, nullptr));

    scoped_array<VkLayerProperties> LayerProperties = { TempAllocator };
    ExpandBy(&LayerProperties, LayerCount);
    Verify(Vulkan->vkEnumerateInstanceLayerProperties(&LayerCount, LayerProperties.Ptr));

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
    Verify(Vulkan->vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    scoped_array<VkExtensionProperties> ExtensionProperties = { TempAllocator };
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
    {
      ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      ApplicationInfo.pApplicationName = "Vulkan Experiments";
      ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);

      ApplicationInfo.pEngineName = "Backbone";
      ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);

      ApplicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 13);
    }

    VkInstanceCreateInfo CreateInfo = {};
    {
      CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

      CreateInfo.pApplicationInfo = &ApplicationInfo;

      CreateInfo.enabledExtensionCount = Cast<uint32_t>(ExtensionNames.Num);
      CreateInfo.ppEnabledExtensionNames = ExtensionNames.Ptr;

      CreateInfo.enabledLayerCount = Cast<uint32_t>(LayerNames.Num);
      CreateInfo.ppEnabledLayerNames = LayerNames.Ptr;
    }

    Verify(Vulkan->vkCreateInstance(&CreateInfo, nullptr, &Vulkan->InstanceHandle));
  }

  return true;
}

static void
VulkanDestroyInstance(vulkan* Vulkan)
{
  VkAllocationCallbacks const* AllocationCallbacks = nullptr;
  Vulkan->vkDestroyInstance(Vulkan->InstanceHandle, AllocationCallbacks);
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

  if(Vulkan->vkCreateDebugReportCallbackEXT != nullptr)
  {
    VkDebugReportCallbackCreateInfoEXT DebugSetupInfo = {};
    DebugSetupInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    DebugSetupInfo.pfnCallback = &VulkanDebugCallback;
    DebugSetupInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

    Verify(Vulkan->vkCreateDebugReportCallbackEXT(Vulkan->InstanceHandle,
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
  if(Vulkan->vkDestroyDebugReportCallbackEXT && Vulkan->DebugCallbackHandle)
  {
    Vulkan->vkDestroyDebugReportCallbackEXT(Vulkan->InstanceHandle, Vulkan->DebugCallbackHandle, nullptr);
  }
}


static bool
VulkanChooseAndSetupPhysicalDevices(vulkan* Vulkan, allocator_interface* TempAllocator)
{
  uint32_t GpuCount;
  Verify(Vulkan->vkEnumeratePhysicalDevices(Vulkan->InstanceHandle,
                                            &GpuCount, nullptr));
  if(GpuCount == 0)
  {
    LogError("No GPUs found.");
    return false;
  }

  LogInfo("Found %u physical device(s).", GpuCount);

  scoped_array<VkPhysicalDevice> Gpus{ TempAllocator };
  ExpandBy(&Gpus, GpuCount);

  Verify(Vulkan->vkEnumeratePhysicalDevices(Vulkan->InstanceHandle,
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

    Vulkan->vkGetPhysicalDeviceProperties(Vulkan->Gpu.GpuHandle, &Vulkan->Gpu.Properties);
    Vulkan->vkGetPhysicalDeviceMemoryProperties(Vulkan->Gpu.GpuHandle, &Vulkan->Gpu.MemoryProperties);
    Vulkan->vkGetPhysicalDeviceFeatures(Vulkan->Gpu.GpuHandle, &Vulkan->Gpu.Features);

    uint32_t QueueCount;
    Vulkan->vkGetPhysicalDeviceQueueFamilyProperties(Vulkan->Gpu.GpuHandle, &QueueCount, nullptr);
    Clear(&Vulkan->Gpu.QueueProperties);
    ExpandBy(&Vulkan->Gpu.QueueProperties, QueueCount);
    Vulkan->vkGetPhysicalDeviceQueueFamilyProperties(Vulkan->Gpu.GpuHandle, &QueueCount, Vulkan->Gpu.QueueProperties.Ptr);
  }

  return true;
}

static bool
VulkanInitializeForGraphics(vulkan* Vulkan, HINSTANCE ProcessHandle, HWND WindowHandle, allocator_interface* TempAllocator)
{
  //
  // Create Win32 Surface
  //
  {
    LogBeginScope("Creating Win32 Surface.");
    Defer(, LogEndScope("Created Win32 Surface."));

    VkWin32SurfaceCreateInfoKHR CreateInfo = {};
    {
      CreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
      CreateInfo.hinstance = ProcessHandle;
      CreateInfo.hwnd = WindowHandle;
    }

    Verify(Vulkan->vkCreateWin32SurfaceKHR(Vulkan->InstanceHandle, &CreateInfo, nullptr, &Vulkan->Surface));
  }

  //
  // Find Queue for Graphics and Presenting
  //
  {
    LogBeginScope("Finding queue indices for graphics and presenting.");
    Defer(, LogEndScope("Done finding queue indices."));

    uint32 GraphicsIndex = IntMaxValue<uint32>();
    uint32 PresentIndex = IntMaxValue<uint32>();

    uint32 const NumQueuesProperties = Cast<uint32>(Vulkan->Gpu.QueueProperties.Num);
    for(uint32 Index = 0; Index < NumQueuesProperties; ++Index)
    {
      VkBool32 SupportsPresenting;
      Vulkan->vkGetPhysicalDeviceSurfaceSupportKHR(Vulkan->Gpu.GpuHandle, Index, Vulkan->Surface, &SupportsPresenting);

      auto& QueueProp = Vulkan->Gpu.QueueProperties[Index];
      if(QueueProp.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        if(GraphicsIndex == IntMaxValue<uint32>())
        {
          GraphicsIndex = Index;
        }

        if(SupportsPresenting)
        {
          GraphicsIndex = Index;
          PresentIndex = Index;
        }
      }
    }

    // TODO: Support for separate graphics and present queue?
    // See tri-demo 1.0.8 line 2200

    if(GraphicsIndex == IntMaxValue<uint>())
    {
      LogError("Unable to find Graphics queue.");
      return false;
    }

    if(PresentIndex == IntMaxValue<uint>())
    {
      LogError("Unable to find Present queue.");
      return false;
    }

    if(GraphicsIndex != PresentIndex)
    {
      LogError("Support for separate graphics and present queue not implemented.");
      return false;
    }

    Vulkan->QueueNodeIndex = GraphicsIndex;
  }

  //
  // Create Logical Device
  //
  {
    LogBeginScope("Creating Device.");
    Defer(, LogEndScope("Device created."));


    // Note: Device layers are deprecated.
    #if 0
    //
    // Device Layers
    //
    auto LayerNames = scoped_array<char const>(TempAllocator);
    {
      // Required extensions:
      bool SurfaceLayerFound;
      bool PlatformSurfaceLayerFound;

      uint LayerCount;
      Verify(vkEnumerateDeviceLayerProperties(Vulkan->Gpu.GpuHandle, &LayerCount, nullptr));

      auto LayerProperties = scoped_array<VkLayerProperties>(TempAllocator);
      ExpandBy(&LayerProperties, LayerCount);
      Verify(vkEnumerateDeviceLayerProperties(Vulkan->Gpu.GpuHandle, &LayerCount, LayerProperties.Ptr));

      LogBeginScope("Explicitly enabled device layers:");
      Defer(, LogEndScope("=========="));
      for(auto& Property : Slice(&LayerProperties))
      {
        auto LayerName = Property.layerName;
        if(LayerName == VK_LAYER_LUNARG_STANDARD_VALIDATION_NAME)
        {
          LayerNames ~= VK_LAYER_LUNARG_STANDARD_VALIDATION_NAME;
        }
        else
        {
          LogInfo("[ ] %s", LayerName);
          continue;
        }

        LogInfo("[x] %s", LayerName);
      }
    }
    #endif

    //
    // Device Extensions
    //
    auto ExtensionNames = scoped_array<char const*>(TempAllocator);
    {
      // Required extensions:
      bool SwapchainExtensionFound = {};

      uint ExtensionCount;
      Verify(Vulkan->vkEnumerateDeviceExtensionProperties(Vulkan->Gpu.GpuHandle, nullptr, &ExtensionCount, nullptr));

      scoped_array<VkExtensionProperties> ExtensionProperties(TempAllocator);
      ExpandBy(&ExtensionProperties, ExtensionCount);
      Verify(Vulkan->vkEnumerateDeviceExtensionProperties(Vulkan->Gpu.GpuHandle, nullptr, &ExtensionCount, ExtensionProperties.Ptr));

      LogBeginScope("Explicitly enabled device extensions:");
      Defer(, LogEndScope("=========="));
      for(auto& Property : Slice(&ExtensionProperties))
      {
        auto ExtensionName = Property.extensionName;
        if(SliceFromString(ExtensionName) == SliceFromString(VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        {
          Expand(&ExtensionNames) = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
          SwapchainExtensionFound = true;
        }
        else
        {
          LogInfo("[ ] %s", ExtensionName);
          continue;
        }

        LogInfo("[x] %s", ExtensionName);
      }

      bool Success = true;

      if(!SwapchainExtensionFound)
      {
        LogError("Failed to load required extension: %s", VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        Success = false;
      }

      if(!Success) return false;
    }

    fixed_block<1, float> QueuePriorities = {};
    QueuePriorities[0] = 0.0f;

    VkDeviceQueueCreateInfo QueueCreateInfo = {};
    {
      QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      QueueCreateInfo.queueFamilyIndex = Vulkan->QueueNodeIndex;
      QueueCreateInfo.queueCount = Cast<uint32>(QueuePriorities.Num);
      QueueCreateInfo.pQueuePriorities = &QueuePriorities[0];
    }

    VkPhysicalDeviceFeatures EnabledFeatures = {};
    {
      EnabledFeatures.shaderClipDistance = VK_TRUE;
      EnabledFeatures.shaderCullDistance = VK_TRUE;
      EnabledFeatures.textureCompressionBC = VK_TRUE;
    }

    VkDeviceCreateInfo DeviceCreateInfo = {};
    {
      DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

      DeviceCreateInfo.queueCreateInfoCount = 1;
      DeviceCreateInfo.pQueueCreateInfos = &QueueCreateInfo;

      DeviceCreateInfo.enabledExtensionCount = Cast<uint32>(ExtensionNames.Num);
      DeviceCreateInfo.ppEnabledExtensionNames = ExtensionNames.Ptr;

      DeviceCreateInfo.pEnabledFeatures = &EnabledFeatures;
    }

    Verify(Vulkan->vkCreateDevice(Vulkan->Gpu.GpuHandle, &DeviceCreateInfo, nullptr, &Vulkan->Device.DeviceHandle));
    Assert(Vulkan->Device.DeviceHandle);

    Vulkan->Device.Vulkan = Vulkan;
    Vulkan->Device.Gpu = &Vulkan->Gpu;

    VulkanLoadDeviceFunctions(&Vulkan->Device);

    Vulkan->Device.vkGetDeviceQueue(Vulkan->Device.DeviceHandle, Vulkan->QueueNodeIndex, 0, &Vulkan->Queue);
    Assert(Vulkan->Queue);
  }

  //
  // Get Physical Device Format and Color Space.
  //
  {
    LogBeginScope("Gathering physical device format and color space.");
    Defer(, LogEndScope("Got format and color space for the previously created Win32 surface."));

    uint FormatCount;
    Verify(Vulkan->vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan->Gpu.GpuHandle, Vulkan->Surface, &FormatCount, nullptr));
    Assert(FormatCount > 0);

    scoped_array<VkSurfaceFormatKHR> SurfaceFormats(TempAllocator);
    ExpandBy(&SurfaceFormats, FormatCount);

    Verify(Vulkan->vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan->Gpu.GpuHandle, Vulkan->Surface, &FormatCount, SurfaceFormats.Ptr));

    if(FormatCount == 1 && SurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
    {
      Vulkan->Format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
      Vulkan->Format = SurfaceFormats[0].format;
    }
    LogInfo("Format: %s", VulkanEnumToString(Vulkan->Format));

    Vulkan->ColorSpace = SurfaceFormats[0].colorSpace;
    LogInfo("Color Space: %s", VulkanEnumToString(Vulkan->ColorSpace));
  }

  // Done.
  return true;
}

void Win32MessagePump()
{
  MSG Message;
  while(PeekMessageA(&Message, nullptr, 0, 0, PM_REMOVE))
  {
    switch(Message.message)
    {
      case WM_QUIT:
      {
        ::GlobalRunning = false;
      } break;

      default:
      {
        TranslateMessage(&Message);
        DispatchMessageA(&Message);
      } break;
    }
  }
}

struct
{
  input_id const Quit  = InputId("Quit");
  input_id const Depth = InputId("Depth");

  struct
  {
    input_id const MoveForward = InputId("MoveForward");
    input_id const MoveRight   = InputId("MoveRight");
    input_id const MoveUp      = InputId("MoveUp");
    input_id const RelYaw      = InputId("RelYaw");
    input_id const RelPitch    = InputId("RelPitch");
    input_id const AbsYaw      = InputId("AbsYaw");
    input_id const AbsPitch    = InputId("AbsPitch");
  } Camera;
} MyInputSlots;

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

  {
    auto Vulkan = Allocate<vulkan>(Allocator);
    *Vulkan = {};
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

    window_setup WindowSetup = {};
    WindowSetup.ProcessHandle = Instance;
    WindowSetup.WindowClassName = "VulkanExperimentsWindowClass";
    WindowSetup.ClientWidth = 768;
    WindowSetup.ClientHeight = 768;
    auto Window = Win32CreateWindow(Allocator, &WindowSetup);

    if(Window == nullptr)
      return 5;

    Defer(=, Win32DestroyWindow(Allocator, Window));

    //
    // Input Setup
    //
    x_input_dll XInput = {};
    win32_input_context SystemInput = {};
    {
      Win32LoadXInput(&XInput, GlobalLog);
      Init(&SystemInput, Allocator);

      // Let's pretend the system is user 0 for now.
      SystemInput.UserIndex = 0;

      Win32RegisterAllMouseSlots(&SystemInput);
      Win32RegisterAllXInputSlots(&SystemInput);
      Win32RegisterAllKeyboardSlots(&SystemInput);

      //
      // Input Mappings
      //
      RegisterInputSlot(&SystemInput, input_type::Button, MyInputSlots.Quit);
      AddInputSlotMapping(&SystemInput, keyboard::Escape, MyInputSlots.Quit);
      AddInputSlotMapping(&SystemInput, x_input::Start, MyInputSlots.Quit);

      RegisterInputSlot(&SystemInput, input_type::Axis, MyInputSlots.Camera.MoveForward);
      AddInputSlotMapping(&SystemInput, x_input::YLeftStick, MyInputSlots.Camera.MoveForward);
      AddInputSlotMapping(&SystemInput, keyboard::W, MyInputSlots.Camera.MoveForward,  1);
      AddInputSlotMapping(&SystemInput, keyboard::S, MyInputSlots.Camera.MoveForward, -1);

      RegisterInputSlot(&SystemInput, input_type::Axis, MyInputSlots.Camera.MoveRight);
      AddInputSlotMapping(&SystemInput, x_input::XLeftStick, MyInputSlots.Camera.MoveRight);
      AddInputSlotMapping(&SystemInput, keyboard::D, MyInputSlots.Camera.MoveRight,  1);
      AddInputSlotMapping(&SystemInput, keyboard::A, MyInputSlots.Camera.MoveRight, -1);

      RegisterInputSlot(&SystemInput, input_type::Axis, MyInputSlots.Camera.MoveUp);
      AddInputSlotMapping(&SystemInput, x_input::LeftTrigger,  MyInputSlots.Camera.MoveUp,  1);
      AddInputSlotMapping(&SystemInput, x_input::RightTrigger, MyInputSlots.Camera.MoveUp, -1);
      AddInputSlotMapping(&SystemInput, keyboard::E, MyInputSlots.Camera.MoveUp,  1);
      AddInputSlotMapping(&SystemInput, keyboard::Q, MyInputSlots.Camera.MoveUp, -1);
      auto MoveUpSlotProperties = GetOrCreate(&SystemInput.ValueProperties, MyInputSlots.Camera.MoveUp);
      MoveUpSlotProperties->Exponent = 3.0f;

      RegisterInputSlot(&SystemInput, input_type::Axis, MyInputSlots.Camera.RelYaw);
      AddInputSlotMapping(&SystemInput, x_input::XRightStick, MyInputSlots.Camera.RelYaw);

      RegisterInputSlot(&SystemInput, input_type::Axis, MyInputSlots.Camera.RelPitch);
      AddInputSlotMapping(&SystemInput, x_input::YRightStick, MyInputSlots.Camera.RelPitch);

      RegisterInputSlot(&SystemInput, input_type::Axis, MyInputSlots.Camera.AbsYaw);
      AddInputSlotMapping(&SystemInput, mouse::YDelta, MyInputSlots.Camera.AbsYaw);

      RegisterInputSlot(&SystemInput, input_type::Axis, MyInputSlots.Camera.AbsPitch);
      AddInputSlotMapping(&SystemInput, mouse::XDelta, MyInputSlots.Camera.AbsPitch);
    }
    Defer(&SystemInput, Finalize(&SystemInput));

    Window->Input = &SystemInput;

    if(!VulkanInitializeForGraphics(Vulkan, Instance, Window->WindowHandle, Allocator))
      return 5;

    LogInfo("Vulkan initialization finished!");
    Defer(, LogInfo("Shutting down..."));

    free_horizon_camera Cam = {};
    Cam.FieldOfView = Degrees(90);
    Cam.Width = 1280;
    Cam.Height = 720;
    Cam.NearPlane = 0.1f;
    Cam.FarPlane = 1000.0f;
    Cam.Transform = Transform(Vec3(2, 0, 0),
                              IdentityQuaternion,
                              UnitScaleVector3);

    Cam.MovementSpeed = 7;
    Cam.RotationSpeed = 3;

    //
    // Main Loop
    //
    ::GlobalRunning = true;

    stopwatch FrameTimer = {};
    StopwatchStart(&FrameTimer);

    float DeltaSeconds = 0.016f; // Assume 16 milliseconds for the first frame.

    while(::GlobalRunning)
    {
      BeginInputFrame(&SystemInput);
      {
        Win32MessagePump();
        Win32PollXInput(&XInput, &SystemInput);
      }
      EndInputFrame(&SystemInput);

      //
      // Check for Quit requests
      //
      if(ButtonIsDown(SystemInput[MyInputSlots.Quit]))
      {
        ::GlobalRunning = false;
        break;
      }

      //
      // Update camera
      //
      auto ForwardMovement = AxisValue(SystemInput[MyInputSlots.Camera.MoveForward]);
      auto RightMovement = AxisValue(SystemInput[MyInputSlots.Camera.MoveRight]);
      auto UpMovement = AxisValue(SystemInput[MyInputSlots.Camera.MoveUp]);
      Cam.Transform.Translation += Cam.MovementSpeed * DeltaSeconds * (
        ForwardVector(Cam.Transform) * ForwardMovement +
        RightVector(Cam.Transform)   * RightMovement +
        UpVector(Cam.Transform)      * UpMovement
      );

      Cam.InputYaw += AxisValue(SystemInput[MyInputSlots.Camera.RelYaw]) * Cam.RotationSpeed * DeltaSeconds;
      Cam.InputYaw += AxisValue(SystemInput[MyInputSlots.Camera.AbsYaw]);
      Cam.InputPitch += AxisValue(SystemInput[MyInputSlots.Camera.RelPitch]) * Cam.RotationSpeed * DeltaSeconds;
      Cam.InputPitch += AxisValue(SystemInput[MyInputSlots.Camera.AbsPitch]);
      Cam.Transform.Rotation = Quaternion(UpVector3,    Radians(Cam.InputYaw)) *
                               Quaternion(RightVector3, Radians(Cam.InputPitch));
      SafeNormalize(&Cam.Transform.Rotation);

      auto const ViewProjectioMatrix = CameraViewProjectionMatrix(Cam, Cam.Transform);

      //
      // Handle resize requests
      //
      if(::GlobalIsResizeRequested)
      {
        LogBeginScope("Resizing swapchain");
        // TODO: Do the resizing here.
        ::GlobalIsResizeRequested = false;
        LogEndScope("Finished resizing swapchain");
      }

      RedrawWindow(Window->WindowHandle, nullptr, nullptr, RDW_INTERNALPAINT);

      // Update frame timer.
      {
        StopwatchStop(&FrameTimer);
        DeltaSeconds = Cast<float>(TimeAsSeconds(StopwatchTime(&FrameTimer)));
        StopwatchStart(&FrameTimer);
      }
    }
  }

  return 0;
}

LRESULT
Win32MainWindowCallback(HWND WindowHandle, UINT Message,
                        WPARAM WParam, LPARAM LParam)
{
  auto Window = Reinterpret<window*>(GetWindowLongPtr(WindowHandle, GWLP_USERDATA));
  if(Window == nullptr)
    return DefWindowProcA(WindowHandle, Message, WParam, LParam);

  Assert(Window->WindowHandle == WindowHandle);

  auto Vulkan = Window->Vulkan;


  LRESULT Result = 0;

  if(Message == WM_CLOSE || Message == WM_DESTROY)
  {
    PostQuitMessage(0);
  }
  else if(Message >= WM_KEYFIRST   && Message <= WM_KEYLAST ||
          Message >= WM_MOUSEFIRST && Message <= WM_MOUSELAST ||
          Message == WM_INPUT)
  {
    if(Window->Input)
    {
      // TODO(Manu): Deal with the return value?
      Win32ProcessInputMessage(WindowHandle, Message, WParam, LParam, Window->Input);
    }
  }
  else if(Message == WM_SIZE)
  {
    if(Vulkan && WParam != SIZE_MINIMIZED)
    {
      ::GlobalIsResizeRequested = true;
      ::GlobalResizeRequest_Width = LParam & 0xffff;
      ::GlobalResizeRequest_Height = (LParam & 0xffff0000) >> 16;
    }
  }
  else if(Message == WM_PAINT)
  {
    PAINTSTRUCT Paint;

    RECT Rect;
    bool MustBeginEndPaint = !!GetUpdateRect(WindowHandle, &Rect, FALSE);

    if(MustBeginEndPaint)
      BeginPaint(WindowHandle, &Paint);

    // TODO
    // if(Vulkan && Vulkan.IsPrepared)
    // {
    //   Vulkan.Draw();
    // }

    if(MustBeginEndPaint)
      EndPaint(WindowHandle, &Paint);
  }
  else
  {
    Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
  }

  return Result;
}
