#include "VulkanHelper.hpp"

#include <Core/DynamicArray.hpp>
#include <Core/Log.hpp>
#include <Core/Input.hpp>
#include <Core/Win32_Input.hpp>
#include <Core/Time.hpp>

#include <Core/Image.hpp>
#include <Core/ImageLoader.hpp>

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
  MemDefaultConstruct(1, Window);
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

      Win32RegisterAllMouseSlots(&SystemInput);
      Win32RegisterAllXInputSlots(&SystemInput);
      Win32RegisterAllKeyboardSlots(&SystemInput);

      //
      // Input Mappings
      //
      RegisterInputSlot(&SystemInput, input_type::Button, InputId("Quit"));
      AddInputSlotMapping(&SystemInput, keyboard::Escape, InputId("Quit"));
      AddInputSlotMapping(&SystemInput, x_input::Start, InputId("Quit"));
    }
    Defer(&SystemInput, Finalize(&SystemInput));

    Window->Input = &SystemInput;

    // if(!VulkanInitializeForGraphics(&Vulkan, Instance, Window->WindowHandle))
    //   return 5;

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

      // "Game" logic
      {
        auto QuitId = InputId("Quit");
        if(ButtonIsDown(SystemInput[QuitId]))
        {
          ::GlobalRunning = false;
          break;
        }
      }

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
