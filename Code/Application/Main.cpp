#include "VulkanHelper.hpp"

#include <Core/DynamicArray.hpp>
#include <Core/Log.hpp>
#include <Core/Input.hpp>
#include <Core/Win32_Input.hpp>
#include <Core/Time.hpp>

#include <Core/Image.hpp>
#include <Core/ImageLoader.hpp>

#include <Core/Math.hpp>
#include <Core/Color.hpp>

#include <Core/Camera.hpp>

#include <Backbone.hpp>

#include <Windows.h>


// TODO Get rid of this.
#include <cstdio>

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

/// Note: Clears the array entirely.
static bool
ReadFileContentIntoArray(dynamic_array<uint8>* Array, char const* FileName)
{
  Clear(Array);

  auto File = std::fopen(FileName, "rb");
  if(File == nullptr)
    return false;

  Defer [File](){ std::fclose(File); };

  size_t const ChunkSize = KiB(4);

  while(true)
  {
    auto NewSlice = ExpandBy(Array, ChunkSize);
    auto const NumBytesRead = std::fread(NewSlice.Ptr, 1, ChunkSize, File);
    auto const Delta = ChunkSize - NumBytesRead;

    if(std::feof(File))
    {
      // Correct the internal array value in case we didn't exactly read a
      // ChunkSize worth of bytes last time.
      Array->Num -= Delta;

      return true;
    }

    if(Delta > 0)
    {
      LogError("Didn't reach the end of file but failed to read any more bytes: %s", FileName);
      return false;
    }
  }
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
    VulkanVerify(Vulkan->vkEnumerateInstanceLayerProperties(&LayerCount, nullptr));

    scoped_array<VkLayerProperties> LayerProperties = { TempAllocator };
    ExpandBy(&LayerProperties, LayerCount);
    VulkanVerify(Vulkan->vkEnumerateInstanceLayerProperties(&LayerCount, LayerProperties.Ptr));

    LogBeginScope("Explicitly enabled instance layers:");
    Defer [](){ LogEndScope("=========="); };
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

    uint32 ExtensionCount;
    VulkanVerify(Vulkan->vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    scoped_array<VkExtensionProperties> ExtensionProperties = { TempAllocator };
    ExpandBy(&ExtensionProperties, ExtensionCount);
    VulkanVerify(Vulkan->vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, ExtensionProperties.Ptr));

    LogBeginScope("Explicitly enabled instance extensions:");
    Defer [](){ LogEndScope("=========="); };

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
    Defer [](){ LogEndScope(""); };

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

      CreateInfo.enabledExtensionCount = Cast<uint32>(ExtensionNames.Num);
      CreateInfo.ppEnabledExtensionNames = ExtensionNames.Ptr;

      CreateInfo.enabledLayerCount = Cast<uint32>(LayerNames.Num);
      CreateInfo.ppEnabledLayerNames = LayerNames.Ptr;
    }

    VulkanVerify(Vulkan->vkCreateInstance(&CreateInfo, nullptr, &Vulkan->InstanceHandle));
  }

  return true;
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
  Defer [](){ LogEndScope("Finished debug setup."); };

  if(Vulkan->vkCreateDebugReportCallbackEXT != nullptr)
  {
    VkDebugReportCallbackCreateInfoEXT DebugSetupInfo = {};
    DebugSetupInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    DebugSetupInfo.pfnCallback = &VulkanDebugCallback;
    DebugSetupInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

    VulkanVerify(Vulkan->vkCreateDebugReportCallbackEXT(Vulkan->InstanceHandle,
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
  uint32 GpuCount;
  VulkanVerify(Vulkan->vkEnumeratePhysicalDevices(Vulkan->InstanceHandle,
                                            &GpuCount, nullptr));
  if(GpuCount == 0)
  {
    LogError("No GPUs found.");
    return false;
  }

  LogInfo("Found %u physical device(s).", GpuCount);

  scoped_array<VkPhysicalDevice> Gpus{ TempAllocator };
  ExpandBy(&Gpus, GpuCount);

  VulkanVerify(Vulkan->vkEnumeratePhysicalDevices(Vulkan->InstanceHandle,
                                            &GpuCount, Gpus.Ptr));

  // Use the first Physical Device for now.
  uint32 const GpuIndex = 0;
  Vulkan->Gpu.GpuHandle = Gpus[GpuIndex];

  //
  // Properties
  //
  {
    LogBeginScope("Querying for physical device and queue properties.");
    Defer [](){ LogEndScope("Retrieved physical device and queue properties."); };

    Vulkan->vkGetPhysicalDeviceProperties(Vulkan->Gpu.GpuHandle, &Vulkan->Gpu.Properties);
    Vulkan->vkGetPhysicalDeviceMemoryProperties(Vulkan->Gpu.GpuHandle, &Vulkan->Gpu.MemoryProperties);
    Vulkan->vkGetPhysicalDeviceFeatures(Vulkan->Gpu.GpuHandle, &Vulkan->Gpu.Features);

    uint32 QueueCount;
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
    Defer [](){ LogEndScope("Created Win32 Surface."); };

    auto CreateInfo = InitStruct<VkWin32SurfaceCreateInfoKHR>();
    {
      CreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
      CreateInfo.hinstance = ProcessHandle;
      CreateInfo.hwnd = WindowHandle;
    }

    VulkanVerify(Vulkan->vkCreateWin32SurfaceKHR(Vulkan->InstanceHandle, &CreateInfo, nullptr, &Vulkan->Surface));
  }

  //
  // Find Queue for Graphics and Presenting
  //
  {
    LogBeginScope("Finding queue indices for graphics and presenting.");
    Defer [](){ LogEndScope("Done finding queue indices."); };

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
    Defer [](){ LogEndScope("Device created."); };

    //
    // Device Extensions
    //
    scoped_array<char const*> ExtensionNames(TempAllocator);
    {
      // Required extensions:
      bool SwapchainExtensionFound = {};

      uint ExtensionCount;
      VulkanVerify(Vulkan->vkEnumerateDeviceExtensionProperties(Vulkan->Gpu.GpuHandle, nullptr, &ExtensionCount, nullptr));

      scoped_array<VkExtensionProperties> ExtensionProperties(TempAllocator);
      ExpandBy(&ExtensionProperties, ExtensionCount);
      VulkanVerify(Vulkan->vkEnumerateDeviceExtensionProperties(Vulkan->Gpu.GpuHandle, nullptr, &ExtensionCount, ExtensionProperties.Ptr));

      LogBeginScope("Explicitly enabled device extensions:");
      Defer [](){ LogEndScope("=========="); };
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

    auto QueueCreateInfo = InitStruct<VkDeviceQueueCreateInfo>();
    {
      QueueCreateInfo.queueFamilyIndex = Vulkan->QueueNodeIndex;
      QueueCreateInfo.queueCount = Cast<uint32>(QueuePriorities.Num);
      QueueCreateInfo.pQueuePriorities = &QueuePriorities[0];
    }

    auto EnabledFeatures = InitStruct<VkPhysicalDeviceFeatures>();
    {
      EnabledFeatures.shaderClipDistance = VK_TRUE;
      EnabledFeatures.shaderCullDistance = VK_TRUE;
      EnabledFeatures.textureCompressionBC = VK_TRUE;
    }

    auto DeviceCreateInfo = InitStruct<VkDeviceCreateInfo>();
    {
      DeviceCreateInfo.queueCreateInfoCount = 1;
      DeviceCreateInfo.pQueueCreateInfos = &QueueCreateInfo;

      DeviceCreateInfo.enabledExtensionCount = Cast<uint32>(ExtensionNames.Num);
      DeviceCreateInfo.ppEnabledExtensionNames = ExtensionNames.Ptr;

      DeviceCreateInfo.pEnabledFeatures = &EnabledFeatures;
    }

    VulkanVerify(Vulkan->vkCreateDevice(Vulkan->Gpu.GpuHandle, &DeviceCreateInfo, nullptr, &Vulkan->Device.DeviceHandle));
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
    Defer [](){ LogEndScope("Got format and color space for the previously created Win32 surface."); };

    uint FormatCount;
    VulkanVerify(Vulkan->vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan->Gpu.GpuHandle, Vulkan->Surface, &FormatCount, nullptr));
    Assert(FormatCount > 0);

    scoped_array<VkSurfaceFormatKHR> SurfaceFormats(TempAllocator);
    ExpandBy(&SurfaceFormats, FormatCount);

    VulkanVerify(Vulkan->vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan->Gpu.GpuHandle, Vulkan->Surface, &FormatCount, SurfaceFormats.Ptr));

    if(FormatCount == 1 && SurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
    {
      Vulkan->Format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
      Vulkan->Format = SurfaceFormats[0].format;
    }
    LogInfo("Format: %s", VulkanEnumName(Vulkan->Format));

    Vulkan->ColorSpace = SurfaceFormats[0].colorSpace;
    LogInfo("Color Space: %s", VulkanEnumName(Vulkan->ColorSpace));
  }

  // Done.
  return true;
}

static bool
VulkanPrepareSwapchain(vulkan* Vulkan, uint32 NewWidth, uint32 NewHeight, allocator_interface* TempAllocator)
{
  //
  // Create Command Pool
  //
  {
    LogBeginScope("Creating command pool.");
    Defer [](){ LogEndScope("Finished creating command pool."); };

    auto CreateInfo = InitStruct<VkCommandPoolCreateInfo>();
    {
      CreateInfo.queueFamilyIndex = Vulkan->QueueNodeIndex;
      CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    }

    VulkanVerify(Vulkan->Device.vkCreateCommandPool(Vulkan->Device.DeviceHandle, &CreateInfo, nullptr, &Vulkan->CommandPool));
    Assert(Vulkan->CommandPool);
  }

  //
  // Create Command Buffer
  //
  {
    LogBeginScope("Creating command buffer.");
    Defer [](){ LogEndScope("Finished creating command buffer."); };

    auto AllocateInfo = InitStruct<VkCommandBufferAllocateInfo>();
    {
      AllocateInfo.commandPool = Vulkan->CommandPool;
      AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      AllocateInfo.commandBufferCount = 1;
    }
    VulkanVerify(Vulkan->Device.vkAllocateCommandBuffers(Vulkan->Device.DeviceHandle, &AllocateInfo, &Vulkan->DrawCommand));
  }

  //
  // Prepare Buffers
  //
  {
    LogBeginScope("Creating swapchain buffers.");
    Defer [](){ LogEndScope("Finished creating swapchain buffers."); };

    Defer [=](){ Vulkan->CurrentBufferIndex = 0; };

    VkSwapchainKHR OldSwapchain = Vulkan->Swapchain;

    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    VulkanVerify(Vulkan->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vulkan->Gpu.GpuHandle, Vulkan->Surface, &SurfaceCapabilities));

    uint PresentModeCount;
    VulkanVerify(Vulkan->vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan->Gpu.GpuHandle, Vulkan->Surface, &PresentModeCount, nullptr));

    scoped_array<VkPresentModeKHR> PresentModes(TempAllocator);
    ExpandBy(&PresentModes, PresentModeCount);

    VulkanVerify(Vulkan->vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan->Gpu.GpuHandle, Vulkan->Surface, &PresentModeCount, PresentModes.Ptr));

    auto SwapchainExtent = InitStruct<VkExtent2D>();

    if(SurfaceCapabilities.currentExtent.width == IntMaxValue<uint32>())
    {
      Assert(SurfaceCapabilities.currentExtent.height == IntMaxValue<uint32>());

      SwapchainExtent.width = NewWidth;
      SwapchainExtent.height = NewHeight;
    }
    else
    {
      SwapchainExtent = SurfaceCapabilities.currentExtent;
    }
    Vulkan->Width = SwapchainExtent.width;
    Vulkan->Height = SwapchainExtent.height;
    LogInfo("Swapchain extents: { width=%u, height=%u }", SwapchainExtent.width, SwapchainExtent.height);


    // Try to find the best present mode, preferring Mailbox or Immediate and
    // fallback to FIFO (which is always available).
    VkPresentModeKHR SwapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(size_t Index = 0; Index < PresentModeCount; ++Index)
    {
      auto const PresentMode = PresentModes[Index];

      if(PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        SwapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }

      Assert(SwapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR);
      if(PresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
      {
        SwapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      }
    }

    // Determine the number of VkImage's to use in the swapchain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint DesiredNumberOfSwapchainImages = SurfaceCapabilities.minImageCount + 1;

    if (SurfaceCapabilities.maxImageCount > 0 &&
        (DesiredNumberOfSwapchainImages > SurfaceCapabilities.maxImageCount))
    {
      // Application must settle for fewer images than desired:
      DesiredNumberOfSwapchainImages = SurfaceCapabilities.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR PreTransform;
    if(SurfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
      PreTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
      PreTransform = SurfaceCapabilities.currentTransform;
    }

    auto SwapchainCreateInfo = InitStruct<VkSwapchainCreateInfoKHR>();
    {
      SwapchainCreateInfo.surface = Vulkan->Surface;
      SwapchainCreateInfo.minImageCount = DesiredNumberOfSwapchainImages;
      SwapchainCreateInfo.imageFormat = Vulkan->Format;
      SwapchainCreateInfo.imageColorSpace = Vulkan->ColorSpace;
      SwapchainCreateInfo.imageExtent = SwapchainExtent;
      SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      SwapchainCreateInfo.preTransform = PreTransform;
      SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      SwapchainCreateInfo.imageArrayLayers = 1;
      SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      SwapchainCreateInfo.queueFamilyIndexCount = 0;
      SwapchainCreateInfo.pQueueFamilyIndices = nullptr;
      SwapchainCreateInfo.presentMode = SwapchainPresentMode;
      SwapchainCreateInfo.oldSwapchain = OldSwapchain;
      SwapchainCreateInfo.clipped = true;
    }

    VulkanVerify(Vulkan->Device.vkCreateSwapchainKHR(Vulkan->Device.DeviceHandle, &SwapchainCreateInfo, nullptr, &Vulkan->Swapchain));
    Assert(Vulkan->Swapchain);
    LogInfo("Created Swapchain.");

    if(OldSwapchain)
    {
      Vulkan->Device.vkDestroySwapchainKHR(Vulkan->Device.DeviceHandle, OldSwapchain, nullptr);
      LogInfo("Destroyed previous swapchain.");
    }

    VulkanVerify(Vulkan->Device.vkGetSwapchainImagesKHR(Vulkan->Device.DeviceHandle, Vulkan->Swapchain, &Vulkan->SwapchainImageCount, nullptr));

    scoped_array<VkImage> SwapchainImages(TempAllocator);
    ExpandBy(&SwapchainImages, Vulkan->SwapchainImageCount);
    VulkanVerify(Vulkan->Device.vkGetSwapchainImagesKHR(Vulkan->Device.DeviceHandle, Vulkan->Swapchain, &Vulkan->SwapchainImageCount, SwapchainImages.Ptr));

    // TODO: Check if we can just set the size to Vulkan->SwapchainImageCount.
    Clear(&Vulkan->SwapchainBuffers);
    ExpandBy(&Vulkan->SwapchainBuffers, Vulkan->SwapchainImageCount);

    for(uint32 Index = 0; Index < Vulkan->SwapchainImageCount; ++Index)
    {
      auto ColorAttachmentCreateInfo = InitStruct<VkImageViewCreateInfo>();
      {
        ColorAttachmentCreateInfo.format = Vulkan->Format;

        ColorAttachmentCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        ColorAttachmentCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        ColorAttachmentCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        ColorAttachmentCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

        ColorAttachmentCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ColorAttachmentCreateInfo.subresourceRange.baseMipLevel = 0;
        ColorAttachmentCreateInfo.subresourceRange.levelCount = 1;
        ColorAttachmentCreateInfo.subresourceRange.baseArrayLayer = 0;
        ColorAttachmentCreateInfo.subresourceRange.layerCount = 1;

        ColorAttachmentCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ColorAttachmentCreateInfo.flags = 0;
      }

      Vulkan->SwapchainBuffers[Index].Image = SwapchainImages[Index];

      // Render loop will expect image to have been used before and in
      // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR layout and will change to
      // COLOR_ATTACHMENT_OPTIMAL, so init the image to that state.
      VulkanSetImageLayout(Vulkan->Device, Vulkan->CommandPool, &Vulkan->SetupCommand, Vulkan->SwapchainBuffers[Index].Image,
                           VkImageAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT),
                           VkImageLayout(VK_IMAGE_LAYOUT_UNDEFINED),
                           VkImageLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR),
                           VkAccessFlags(0));

      ColorAttachmentCreateInfo.image = Vulkan->SwapchainBuffers[Index].Image;

      VulkanVerify(Vulkan->Device.vkCreateImageView(Vulkan->Device.DeviceHandle, &ColorAttachmentCreateInfo, nullptr,
                                              &Vulkan->SwapchainBuffers[Index].View));
    }
  }

  //
  // Prepare Depth
  //
  {
    LogBeginScope("Preparing depth.");
    Defer [](){ LogEndScope("Finished preparing depth."); };

    Vulkan->Depth.Format = VK_FORMAT_D16_UNORM;
    auto ImageCreateInfo = InitStruct<VkImageCreateInfo>();
    {
      ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
      ImageCreateInfo.format = Vulkan->Depth.Format;
      ImageCreateInfo.extent = { Vulkan->Width, Vulkan->Height, 1 };
      ImageCreateInfo.mipLevels = 1;
      ImageCreateInfo.arrayLayers = 1;
      ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
      ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
      ImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    // Create image.
    VulkanVerify(Vulkan->Device.vkCreateImage(Vulkan->Device.DeviceHandle, &ImageCreateInfo, nullptr, &Vulkan->Depth.Image));

    // Get memory requirements for this object.
    VkMemoryRequirements MemoryRequirements;
    Vulkan->Device.vkGetImageMemoryRequirements(Vulkan->Device.DeviceHandle, Vulkan->Depth.Image, &MemoryRequirements);

    // Select memory size and type.
    auto MemoryAllocateInfo = InitStruct<VkMemoryAllocateInfo>();
    {
      MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
      MemoryAllocateInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan->Gpu.MemoryProperties,
                                                                          MemoryRequirements.memoryTypeBits,
                                                                          0); // No requirements.
      Assert(MemoryAllocateInfo.memoryTypeIndex != IntMaxValue<uint32>());
    }

    // Allocate memory.
    VulkanVerify(Vulkan->Device.vkAllocateMemory(Vulkan->Device.DeviceHandle, &MemoryAllocateInfo, nullptr, &Vulkan->Depth.Memory));

    // Bind memory.
    VulkanVerify(Vulkan->Device.vkBindImageMemory(Vulkan->Device.DeviceHandle, Vulkan->Depth.Image, Vulkan->Depth.Memory, 0));

    VulkanSetImageLayout(Vulkan->Device, Vulkan->CommandPool, &Vulkan->SetupCommand, Vulkan->Depth.Image,
                         VkImageAspectFlags(VK_IMAGE_ASPECT_DEPTH_BIT),
                         VkImageLayout(VK_IMAGE_LAYOUT_UNDEFINED),
                         VkImageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
                         VkAccessFlags(0));

    // Create image view.
    auto ImageViewCreateInfo = InitStruct<VkImageViewCreateInfo>();
    {
      ImageViewCreateInfo.image = Vulkan->Depth.Image;
      ImageViewCreateInfo.format = Vulkan->Depth.Format;
      ImageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
    }
    VulkanVerify(Vulkan->Device.vkCreateImageView(Vulkan->Device.DeviceHandle, &ImageViewCreateInfo, nullptr, &Vulkan->Depth.View));
  }

  //
  // Prepare Descriptor Set Layout
  //
  {
    LogBeginScope("Preparing descriptor set layout.");
    Defer [](){ LogEndScope("Finished preparing descriptor set layout."); };

    scoped_array<VkDescriptorSetLayoutBinding> LayoutBindings{ TempAllocator };

    {
      auto& Layout = Expand(&LayoutBindings);
      Layout.binding = 1;
      Layout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      Layout.descriptorCount = 1;
      Layout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    // Model View Projection matrix
    {
      auto& Layout = Expand(&LayoutBindings);
      Layout.binding = 0;
      Layout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      Layout.descriptorCount = 1;
      Layout.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
    }

    auto DescriptorSetLayoutCreateInfo = InitStruct<VkDescriptorSetLayoutCreateInfo>();
    {
      DescriptorSetLayoutCreateInfo.bindingCount = Cast<uint32>(LayoutBindings.Num);
      DescriptorSetLayoutCreateInfo.pBindings = LayoutBindings.Ptr;
    }

    VulkanVerify(Vulkan->Device.vkCreateDescriptorSetLayout(Vulkan->Device.DeviceHandle, &DescriptorSetLayoutCreateInfo, nullptr, &Vulkan->DescriptorSetLayout));

    auto PipelineLayoutCreateInfo = InitStruct<VkPipelineLayoutCreateInfo>();
    {
      PipelineLayoutCreateInfo.setLayoutCount = 1;
      PipelineLayoutCreateInfo.pSetLayouts = &Vulkan->DescriptorSetLayout;
    }
    VulkanVerify(Vulkan->Device.vkCreatePipelineLayout(Vulkan->Device.DeviceHandle, &PipelineLayoutCreateInfo, nullptr, &Vulkan->PipelineLayout));
  }

  //
  // Prepare Render Pass
  //
  {
    LogBeginScope("Preparing render pass.");
    Defer [](){ LogEndScope("Finished preparing render pass."); };

    fixed_block<2, VkAttachmentDescription> AttachmentsBlock;
    auto Attachments = Slice(AttachmentsBlock);
    {
      auto& Attachment = Attachments[0];
      Attachment = InitStruct<decltype(Attachment)>();

      Attachment.format = Vulkan->Format;
      Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
      Attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      Attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      Attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      Attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    {
      auto& Attachment = Attachments[1];
      Attachment = InitStruct<decltype(Attachment)>();

      Attachment.format = Vulkan->Depth.Format;
      Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
      Attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      Attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      Attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      Attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    auto ColorReference = InitStruct<VkAttachmentReference>();
    {
      ColorReference.attachment = 0;
      ColorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    auto DepthReference = InitStruct<VkAttachmentReference>();
    {
      DepthReference.attachment = 1;
      DepthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    auto SubpassDesc = InitStruct<VkSubpassDescription>();
    {
      SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      SubpassDesc.colorAttachmentCount = 1;
      SubpassDesc.pColorAttachments = &ColorReference;
      SubpassDesc.pDepthStencilAttachment = &DepthReference;
    }

    auto RenderPassCreateInfo = InitStruct<VkRenderPassCreateInfo>();
    {
      RenderPassCreateInfo.attachmentCount = Cast<uint32>(Attachments.Num);
      RenderPassCreateInfo.pAttachments = Attachments.Ptr;
      RenderPassCreateInfo.subpassCount = 1;
      RenderPassCreateInfo.pSubpasses = &SubpassDesc;
    }

    VulkanVerify(Vulkan->Device.vkCreateRenderPass(Vulkan->Device.DeviceHandle, &RenderPassCreateInfo, nullptr, &Vulkan->RenderPass));
  }

  //
  // Prepare Pipeline
  //
  {
    LogBeginScope("Preparing pipeline.");
    Defer [](){ LogEndScope("Finished preparing pipeline."); };

    // Two shader stages: vs and fs
    fixed_block<2, VkPipelineShaderStageCreateInfo> StagesBlock;
    auto Stages = Slice(StagesBlock);
    for(auto& Stage : Stages)
    {
      Stage = InitStruct<decltype(Stage)>();
    }

    //
    // Vertex Shader
    //
    auto& VertexShaderStage = Stages[0];
    {
      VertexShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
      VertexShaderStage.pName = "main";

      char const* FileName = "Build/Shader-Simple.vert.spv";

      LogBeginScope("Loading vertex shader from file: %s", FileName);
      Defer [](){ LogEndScope(""); };

      scoped_array<uint8> ShaderCode{ TempAllocator };
      ReadFileContentIntoArray(&ShaderCode, FileName);

      auto ShaderModuleCreateInfo = InitStruct<VkShaderModuleCreateInfo>();
      ShaderModuleCreateInfo.codeSize = Cast<uint32>(ShaderCode.Num); // In bytes, regardless of the fact that typeof(*pCode) == uint.
      ShaderModuleCreateInfo.pCode = Reinterpret<uint32*>(ShaderCode.Ptr); // Is a const(uint)*, for some reason...

      VulkanVerify(Vulkan->Device.vkCreateShaderModule(Vulkan->Device.DeviceHandle, &ShaderModuleCreateInfo, nullptr, &VertexShaderStage.module));
    }
    // Note(Manu): Can safely destroy the shader modules on our side once the pipeline was created.
    Defer [&](){ Vulkan->Device.vkDestroyShaderModule(Vulkan->Device.DeviceHandle, VertexShaderStage.module, nullptr); };

    //
    // Fragment Shader
    //
    auto& FragmentShaderStage = Stages[1];
    {
      FragmentShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      FragmentShaderStage.pName = "main";

      char const* FileName = "Build/Shader-Simple.frag.spv";

      LogBeginScope("Loading fragment shader from file: %s", FileName);
      Defer [](){ LogEndScope(""); };

      scoped_array<uint8> ShaderCode{ TempAllocator };
      ReadFileContentIntoArray(&ShaderCode, FileName);

      auto ShaderModuleCreateInfo = InitStruct<VkShaderModuleCreateInfo>();
      ShaderModuleCreateInfo.codeSize = Cast<uint32>(ShaderCode.Num); // In bytes, regardless of the fact that typeof(*pCode) == uint.
      ShaderModuleCreateInfo.pCode = Reinterpret<uint32*>(ShaderCode.Ptr); // Is a const(uint)*, for some reason...

      VulkanVerify(Vulkan->Device.vkCreateShaderModule(Vulkan->Device.DeviceHandle, &ShaderModuleCreateInfo, nullptr, &FragmentShaderStage.module));
    }
    Defer [&](){ Vulkan->Device.vkDestroyShaderModule(Vulkan->Device.DeviceHandle, FragmentShaderStage.module, nullptr); };

    fixed_block<1, VkVertexInputBindingDescription> VertexInputBindingDescs;
    fixed_block<2, VkVertexInputAttributeDescription> VertexInputAttributeDescs;

    {
      auto& Desc = VertexInputBindingDescs[0];
      Desc.binding = 0;
      Desc.stride = Cast<uint32>(SizeOf<vertex>());
      Desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }

    {
      auto& Desc = VertexInputAttributeDescs[0];
      Desc.binding = 0;
      Desc.location = 0;
      Desc.format = VK_FORMAT_R32G32B32_SFLOAT;
      Desc.offset = 0;
    }

    {
      auto& Desc = VertexInputAttributeDescs[1];
      Desc.binding = 0;
      Desc.location = 1;
      Desc.format = VK_FORMAT_R32G32_SFLOAT;
      Desc.offset = Cast<uint32>(SizeOf<decltype(vertex::Position)>());
    }

    auto VertexInputState = InitStruct<VkPipelineVertexInputStateCreateInfo>();
    {
      VertexInputState.vertexBindingDescriptionCount = Cast<uint32>(VertexInputBindingDescs.Num);
      VertexInputState.pVertexBindingDescriptions    = &VertexInputBindingDescs[0];
      VertexInputState.vertexAttributeDescriptionCount = Cast<uint32>(VertexInputAttributeDescs.Num);
      VertexInputState.pVertexAttributeDescriptions    = &VertexInputAttributeDescs[0];
    }

    auto InputAssemblyState = InitStruct<VkPipelineInputAssemblyStateCreateInfo>();
    {
      InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    auto ViewportState = InitStruct<VkPipelineViewportStateCreateInfo>();
    {
      ViewportState.viewportCount = 1;
      ViewportState.scissorCount = 1;
    }

    auto RasterizationState = InitStruct<VkPipelineRasterizationStateCreateInfo>();
    {
      RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
      RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
      RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
      RasterizationState.depthClampEnable = VK_FALSE;
      RasterizationState.rasterizerDiscardEnable = VK_FALSE;
      RasterizationState.depthBiasEnable = VK_FALSE;
      RasterizationState.lineWidth = 1.0f;
    }

    auto MultisampleState = InitStruct<VkPipelineMultisampleStateCreateInfo>();
    {
      MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    }

    auto DepthStencilState = InitStruct<VkPipelineDepthStencilStateCreateInfo>();
    {
      DepthStencilState.depthTestEnable = VK_TRUE;
      DepthStencilState.depthWriteEnable = VK_TRUE;
      DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      DepthStencilState.depthBoundsTestEnable = VK_FALSE;
      DepthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
      DepthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
      DepthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
      DepthStencilState.stencilTestEnable = VK_FALSE;
      DepthStencilState.front = DepthStencilState.back;
    }

    fixed_block<1, VkPipelineColorBlendAttachmentState> ColorBlendStateAttachmentsBlock;
    auto ColorBlendStateAttachments = Slice(ColorBlendStateAttachmentsBlock);
    {
      auto& Attachment = ColorBlendStateAttachments[0];
      Attachment = InitStruct<decltype(Attachment)>();

      Attachment.colorWriteMask = 0xf;
      Attachment.blendEnable = VK_FALSE;
    }

    auto ColorBlendState = InitStruct<VkPipelineColorBlendStateCreateInfo>();
    {
      ColorBlendState.attachmentCount = Cast<uint32>(ColorBlendStateAttachments.Num);
      ColorBlendState.pAttachments = ColorBlendStateAttachments.Ptr;
    }

    fixed_block<VK_DYNAMIC_STATE_RANGE_SIZE, VkDynamicState> DynamicStates;
    auto DynamicState = InitStruct<VkPipelineDynamicStateCreateInfo>();
    {
      DynamicStates[DynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
      DynamicStates[DynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
      DynamicState.pDynamicStates = &DynamicStates[0];
    }

    auto GraphicsPipelineCreateInfo = InitStruct<VkGraphicsPipelineCreateInfo>();
    {
      GraphicsPipelineCreateInfo.stageCount = Cast<uint32>(Stages.Num);
      GraphicsPipelineCreateInfo.pStages = Stages.Ptr;
      GraphicsPipelineCreateInfo.pVertexInputState = &VertexInputState;
      GraphicsPipelineCreateInfo.pInputAssemblyState = &InputAssemblyState;
      GraphicsPipelineCreateInfo.pViewportState = &ViewportState;
      GraphicsPipelineCreateInfo.pRasterizationState = &RasterizationState;
      GraphicsPipelineCreateInfo.pMultisampleState = &MultisampleState;
      GraphicsPipelineCreateInfo.pDepthStencilState = &DepthStencilState;
      GraphicsPipelineCreateInfo.pColorBlendState = &ColorBlendState;
      GraphicsPipelineCreateInfo.pDynamicState = &DynamicState;
      GraphicsPipelineCreateInfo.layout = Vulkan->PipelineLayout;
      GraphicsPipelineCreateInfo.renderPass = Vulkan->RenderPass;
    }

    auto PipelineCacheCreateInfo = InitStruct<VkPipelineCacheCreateInfo>();
    VkPipelineCache PipelineCache;
    VulkanVerify(Vulkan->Device.vkCreatePipelineCache(Vulkan->Device.DeviceHandle,
                                                      &PipelineCacheCreateInfo,
                                                      nullptr,
                                                      &PipelineCache));

    VulkanVerify(Vulkan->Device.vkCreateGraphicsPipelines(Vulkan->Device.DeviceHandle, PipelineCache,
                                                          1, &GraphicsPipelineCreateInfo,
                                                          nullptr,
                                                          &Vulkan->Pipeline));

    Vulkan->Device.vkDestroyPipelineCache(Vulkan->Device.DeviceHandle, PipelineCache, nullptr);
  }

  //
  // Prepare Descriptor Pool
  //
  {
    LogBeginScope("Preparing descriptor pool.");
    Defer [](){ LogEndScope("Finished preparing descriptor pool."); };

    scoped_array<VkDescriptorPoolSize> PoolSizes{ TempAllocator };

    {
      auto& Size = Expand(&PoolSizes);
      Size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      Size.descriptorCount = 1;
    }

    {
      auto& Size = Expand(&PoolSizes);
      Size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      Size.descriptorCount = 1;
    }

    auto DescriptorPoolCreateInfo = InitStruct<VkDescriptorPoolCreateInfo>();
    {
      DescriptorPoolCreateInfo.maxSets = 1;
      DescriptorPoolCreateInfo.poolSizeCount = Cast<uint32>(PoolSizes.Num);
      DescriptorPoolCreateInfo.pPoolSizes = PoolSizes.Ptr;
    }

    VulkanVerify(Vulkan->Device.vkCreateDescriptorPool(Vulkan->Device.DeviceHandle,
                                                       &DescriptorPoolCreateInfo,
                                                       nullptr,
                                                       &Vulkan->DescriptorPool));
  }

  //
  // Prepare Descriptor Set
  //
  {
    LogBeginScope("Preparing descriptor set.");
    Defer [](){ LogEndScope("Finished preparing descriptor set."); };

    auto DescriptorSetAllocateInfo = InitStruct<VkDescriptorSetAllocateInfo>();
    {
      DescriptorSetAllocateInfo.descriptorPool = Vulkan->DescriptorPool;
      DescriptorSetAllocateInfo.descriptorSetCount = 1;
      DescriptorSetAllocateInfo.pSetLayouts = &Vulkan->DescriptorSetLayout;
    }
    VulkanVerify(Vulkan->Device.vkAllocateDescriptorSets(Vulkan->Device.DeviceHandle,
                                                         &DescriptorSetAllocateInfo,
                                                         &Vulkan->DescriptorSet));

    //
    // GlobalUBO
    //
    {
      auto BufferCreateInfo = InitStruct<VkBufferCreateInfo>();
      {
        BufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        BufferCreateInfo.size = SizeOf<mat4x4>();
      }

      VulkanVerify(Vulkan->Device.vkCreateBuffer(Vulkan->Device.DeviceHandle, &BufferCreateInfo, nullptr,
                                                 &Vulkan->GlobalUBO_BufferHandle));

      VkMemoryRequirements Temp_MemoryRequirements;
      Vulkan->Device.vkGetBufferMemoryRequirements(Vulkan->Device.DeviceHandle, Vulkan->GlobalUBO_BufferHandle,
                                                   &Temp_MemoryRequirements);

      auto Temp_MemoryAllocationInfo = InitStruct<VkMemoryAllocateInfo>();
      {
        Temp_MemoryAllocationInfo.allocationSize = Temp_MemoryRequirements.size;
        Temp_MemoryAllocationInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan->Device.Gpu->MemoryProperties,
                                                                                   Temp_MemoryRequirements.memoryTypeBits,
                                                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        Assert(Temp_MemoryAllocationInfo.memoryTypeIndex != IntMaxValue<uint32>());
      }

      VulkanVerify(Vulkan->Device.vkAllocateMemory(Vulkan->Device.DeviceHandle, &Temp_MemoryAllocationInfo, nullptr, &Vulkan->GlobalUBO_MemoryHandle));

      VulkanVerify(Vulkan->Device.vkBindBufferMemory(Vulkan->Device.DeviceHandle, Vulkan->GlobalUBO_BufferHandle, Vulkan->GlobalUBO_MemoryHandle, 0));
    }

    //
    // Associate the buffer with the descriptor set.
    //
    auto DescriptorBufferInfo = InitStruct<VkDescriptorBufferInfo>();
    {
      DescriptorBufferInfo.buffer = Vulkan->GlobalUBO_BufferHandle;
      DescriptorBufferInfo.offset = 0;
      DescriptorBufferInfo.range = VK_WHOLE_SIZE;
    }

    auto GlobalUBOUpdateInfo = InitStruct<VkWriteDescriptorSet>();
    {
      GlobalUBOUpdateInfo.dstSet = Vulkan->DescriptorSet;
      GlobalUBOUpdateInfo.dstBinding = 0;
      GlobalUBOUpdateInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      GlobalUBOUpdateInfo.descriptorCount = 1;
      GlobalUBOUpdateInfo.pBufferInfo = &DescriptorBufferInfo;
    }

    Vulkan->Device.vkUpdateDescriptorSets(Vulkan->Device.DeviceHandle,
                                          1, &GlobalUBOUpdateInfo,
                                          0, nullptr);
  }

  //
  // Prepare Framebuffers
  //
  {
    LogBeginScope("Preparing framebuffers.");
    Defer [](){ LogEndScope("Finished preparing framebuffers."); };

    fixed_block<2, VkImageView> Attachments = {};
    Attachments[1] = Vulkan->Depth.View;

    auto FramebufferCreateInfo = InitStruct<VkFramebufferCreateInfo>();
    {
      FramebufferCreateInfo.renderPass = Vulkan->RenderPass;
      FramebufferCreateInfo.attachmentCount = Cast<uint32>(Attachments.Num);
      FramebufferCreateInfo.pAttachments = &Attachments[0];
      FramebufferCreateInfo.width = Vulkan->Width;
      FramebufferCreateInfo.height = Vulkan->Height;
      FramebufferCreateInfo.layers = 1;
    }

    Clear(&Vulkan->Framebuffers);
    ExpandBy(&Vulkan->Framebuffers, Vulkan->SwapchainImageCount);

    for(size_t Index = 0; Index < Vulkan->SwapchainImageCount; ++Index)
    {
      Attachments[0] = Vulkan->SwapchainBuffers[Index].View;
      VulkanVerify(Vulkan->Device.vkCreateFramebuffer(Vulkan->Device.DeviceHandle,
                                                      &FramebufferCreateInfo,
                                                      nullptr,
                                                      &Vulkan->Framebuffers[Index]));
    }
  }

  // Done preparing swapchain...
  return true;
}

static void
VulkanDestroySwapchain(vulkan* Vulkan)
{
  Assert(Vulkan->IsPrepared);

  Vulkan->IsPrepared = false;

  for(auto Framebuffer : Slice(&Vulkan->Framebuffers))
  {
    Vulkan->Device.vkDestroyFramebuffer(Vulkan->Device.DeviceHandle, Framebuffer, nullptr);
  }
  Clear(&Vulkan->Framebuffers);

  Vulkan->Device.vkDestroyDescriptorPool(Vulkan->Device.DeviceHandle, Vulkan->DescriptorPool, nullptr);

  if(Vulkan->SetupCommand)
  {
    Vulkan->Device.vkFreeCommandBuffers(Vulkan->Device.DeviceHandle, Vulkan->CommandPool, 1, &Vulkan->SetupCommand);
  }
  Vulkan->Device.vkFreeCommandBuffers(Vulkan->Device.DeviceHandle, Vulkan->CommandPool, 1, &Vulkan->DrawCommand);
  Vulkan->Device.vkDestroyCommandPool(Vulkan->Device.DeviceHandle, Vulkan->CommandPool, nullptr);

  Vulkan->Device.vkDestroyPipeline(Vulkan->Device.DeviceHandle, Vulkan->Pipeline, nullptr);
  Vulkan->Device.vkDestroyRenderPass(Vulkan->Device.DeviceHandle, Vulkan->RenderPass, nullptr);
  Vulkan->Device.vkDestroyPipelineLayout(Vulkan->Device.DeviceHandle, Vulkan->PipelineLayout, nullptr);
  Vulkan->Device.vkDestroyDescriptorSetLayout(Vulkan->Device.DeviceHandle, Vulkan->DescriptorSetLayout, nullptr);

  for(auto& SceneObject : Slice(&Vulkan->SceneObjects))
  {
    VulkanDestroySceneObject(Vulkan, &SceneObject);
  }

  for(auto& Buffer : Slice(&Vulkan->SwapchainBuffers))
  {
    Vulkan->Device.vkDestroyImageView(Vulkan->Device.DeviceHandle, Buffer.View, nullptr);
  }
  Clear(&Vulkan->SwapchainBuffers);

  Vulkan->Device.vkDestroyImageView(Vulkan->Device.DeviceHandle, Vulkan->Depth.View, nullptr);
  Vulkan->Device.vkDestroyImage(Vulkan->Device.DeviceHandle,     Vulkan->Depth.Image, nullptr);
  Vulkan->Device.vkFreeMemory(Vulkan->Device.DeviceHandle,       Vulkan->Depth.Memory, nullptr);
}

static void
VulkanCleanup(vulkan* Vulkan)
{
  LogBeginScope("Vulkan cleanup.");
  Defer [](){ LogEndScope("Finished Vulkan cleanup."); };

  if(Vulkan->IsPrepared)
  {
    VulkanDestroySwapchain(Vulkan);
  }

  Vulkan->Device.vkDestroySwapchainKHR(Vulkan->Device.DeviceHandle, Vulkan->Swapchain, nullptr);
  Vulkan->Device.vkDestroyDevice(Vulkan->Device.DeviceHandle, nullptr);

  Vulkan->vkDestroySurfaceKHR(Vulkan->InstanceHandle, Vulkan->Surface, nullptr);
  Vulkan->vkDestroyInstance(Vulkan->InstanceHandle, nullptr);

  Clear(&Vulkan->Gpu.QueueProperties);
}

static void
VulkanResize(vulkan* Vulkan, uint32 NewWidth, uint32 NewHeight, allocator_interface* TempAllocator)
{
  // Don't react to resize until after first initialization.
  if(!Vulkan->IsPrepared) return;

  LogInfo("Resizing to %ux%u.", NewWidth, NewHeight);

  // In order to properly resize the window, we must re-create the swapchain
  // AND redo the command buffers, etc.

  // First, perform part of the VulkanCleanup() function:
  VulkanDestroySwapchain(Vulkan);

  // Second, re-perform the Prepare() function, which will re-create the
  // swapchain:
  Vulkan->IsPrepared = VulkanPrepareSwapchain(Vulkan, NewWidth, NewHeight, TempAllocator);
}

static void
VulkanRenderPass(vulkan* Vulkan)
{
  // Begin render pass.
  {
    fixed_block<2, VkClearValue> ClearValuesBlock;
    auto ClearValues = Slice(ClearValuesBlock);

    // First color
    {
      SliceCopy(Slice(ClearValues[0].color.float32),
                AsConst(Slice(color::CornflowerBlue.Data)));
    }

    // Second color
    {
      ClearValues[1].depthStencil.depth = Vulkan->DepthStencilValue;
      ClearValues[1].depthStencil.stencil = 0;
    }

    auto RenderPassBeginInfo = InitStruct<VkRenderPassBeginInfo>();
    {
      RenderPassBeginInfo.renderPass = Vulkan->RenderPass;
      RenderPassBeginInfo.framebuffer = Vulkan->Framebuffers[Vulkan->CurrentBufferIndex];
      RenderPassBeginInfo.renderArea.extent.width = Vulkan->Width;
      RenderPassBeginInfo.renderArea.extent.height = Vulkan->Height;
      RenderPassBeginInfo.clearValueCount = Cast<uint32>(ClearValues.Num);
      RenderPassBeginInfo.pClearValues = ClearValues.Ptr;
    }
    Vulkan->Device.vkCmdBeginRenderPass(Vulkan->DrawCommand,
                                        &RenderPassBeginInfo,
                                        VK_SUBPASS_CONTENTS_INLINE);
  }
  Defer [=](){ Vulkan->Device.vkCmdEndRenderPass(Vulkan->DrawCommand); };

  Vulkan->Device.vkCmdBindPipeline(Vulkan->DrawCommand, VK_PIPELINE_BIND_POINT_GRAPHICS, Vulkan->Pipeline);
  Vulkan->Device.vkCmdBindDescriptorSets(Vulkan->DrawCommand, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          Vulkan->PipelineLayout, 0,
                          1, &Vulkan->DescriptorSet,
                          0, nullptr);

  // Set Viewport
  {
    auto Viewport = InitStruct<VkViewport>();
    {
      Viewport.height = Cast<float>(Vulkan->Height);
      Viewport.width = Cast<float>(Vulkan->Width);
      Viewport.minDepth = 0.0f;
      Viewport.maxDepth = 1.0f;
    }
    Vulkan->Device.vkCmdSetViewport(Vulkan->DrawCommand, 0, 1, &Viewport);
  }

  // Set Scissor
  {
    auto Scissor = InitStruct<VkRect2D>();
    {
      Scissor.extent.width = Vulkan->Width;
      Scissor.extent.height = Vulkan->Height;
    }
    Vulkan->Device.vkCmdSetScissor(Vulkan->DrawCommand, 0, 1, &Scissor);
  }

  for(auto& SceneObject : Slice(&Vulkan->SceneObjects))
  {
    //
    // Bind vertex buffer
    //
    {
      auto VertexBufferOffset = InitStruct<VkDeviceSize>();
      Vulkan->Device.vkCmdBindVertexBuffers(Vulkan->DrawCommand, SceneObject.Vertices.BindID,
                                            1, &SceneObject.Vertices.Buffer, &VertexBufferOffset);
    }

    //
    // Bind index buffer
    //
    {
      auto IndexBufferOffset = InitStruct<VkDeviceSize>();
      Vulkan->Device.vkCmdBindIndexBuffer(Vulkan->DrawCommand,
                                          SceneObject.Indices.Buffer,
                                          IndexBufferOffset,
                                          VK_INDEX_TYPE_UINT32);
    }

    //
    // Draw
    //
    Vulkan->Device.vkCmdDrawIndexed(Vulkan->DrawCommand,
                                    SceneObject.Indices.NumIndices, // indexCount
                                    1,                              // instanceCount
                                    0,                              // firstIndex
                                    0,                              // vertexOffset
                                    0);                             // firstInstance
  }
}

static bool GlobalIsDrawing = false;

static void
VulkanDraw(vulkan* Vulkan, allocator_interface* TempAllocator)
{
  ::GlobalIsDrawing = true;
  Defer [](){ ::GlobalIsDrawing = false; };

  VkFence NullFence = {};
  VkResult Error = {};

  auto PresentCompleteSemaphore = InitStruct<VkSemaphore>();
  {
    auto PresentCompleteSemaphoreCreateInfo = InitStruct<VkSemaphoreCreateInfo>();
    VulkanVerify(Vulkan->Device.vkCreateSemaphore(Vulkan->Device.DeviceHandle, &PresentCompleteSemaphoreCreateInfo, nullptr, &PresentCompleteSemaphore));
  }
  Defer [=](){ Vulkan->Device.vkDestroySemaphore(Vulkan->Device.DeviceHandle, PresentCompleteSemaphore, nullptr); };

  // Get the index of the next available swapchain image:
  {
    auto Timeout = IntMaxValue<uint64>();
    Error = Vulkan->Device.vkAcquireNextImageKHR(Vulkan->Device.DeviceHandle, Vulkan->Swapchain, Timeout,
                                                 PresentCompleteSemaphore, NullFence,
                                                 &Vulkan->CurrentBufferIndex);
  }

  switch(Error)
  {
    case VK_ERROR_OUT_OF_DATE_KHR:
    {
      // Swapchain is out of date (e.g. the window was resized) and must be
      // recreated:
      VulkanResize(Vulkan, Vulkan->Width, Vulkan->Height, TempAllocator);
      VulkanDraw(Vulkan, TempAllocator);
    } break;
    case VK_SUBOPTIMAL_KHR:
    {
      // Swapchain is not as optimal as it could be, but the platform's
      // presentation engine will still present the image correctly.
    } break;
    default: VulkanVerify(Error);
  }

  auto const CurrentSwapchainImage = Vulkan->SwapchainBuffers[Vulkan->CurrentBufferIndex].Image;

  // Assume the command buffer has been run on current_buffer before so
  // we need to set the image layout back to COLOR_ATTACHMENT_OPTIMAL
  {
    VulkanEnsureSetupCommandIsReady(Vulkan->Device, Vulkan->CommandPool, &Vulkan->SetupCommand);

    auto ImageMemoryBarrier = InitStruct<VkImageMemoryBarrier>();
    {
      ImageMemoryBarrier.srcAccessMask = 0;
      ImageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      ImageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      ImageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      {
        auto& Range = ImageMemoryBarrier.subresourceRange;
        Range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Range.baseMipLevel = 0;
        Range.levelCount = 1;
        Range.baseArrayLayer = 0;
        Range.layerCount = 1;
      }
      ImageMemoryBarrier.image = CurrentSwapchainImage;
    }

    VkPipelineStageFlags SourceStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags DestinationStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    Vulkan->Device.vkCmdPipelineBarrier(Vulkan->SetupCommand,            // commandBuffer
                                        SourceStages, DestinationStages, // srcStageMask, dstStageMask
                                        0,                               // dependencyFlags
                                        0, nullptr,                      // memoryBarrierCount, pMemoryBarriers
                                        0, nullptr,                      // bufferMemoryBarrierCount, pBufferMemoryBarriers
                                        1, &ImageMemoryBarrier);         // imageMemoryBarrierCount, pImageMemoryBarriers
  }

  VulkanFlushSetupCommand(Vulkan->Device, Vulkan->Queue, Vulkan->CommandPool, &Vulkan->SetupCommand);


  {
    auto CommandBufferInheritanceInfo = InitStruct<VkCommandBufferInheritanceInfo>();
    auto CommandBufferBeginInfo = InitStruct<VkCommandBufferBeginInfo>();
    CommandBufferBeginInfo.pInheritanceInfo = &CommandBufferInheritanceInfo;
    VulkanVerify(Vulkan->Device.vkBeginCommandBuffer(Vulkan->DrawCommand, &CommandBufferBeginInfo));
  }
  // FIXME/TODO: Deal with VK_IMAGE_LAYOUT_PRESENT_SRC_KHR?!?!?
  VulkanRenderPass(Vulkan);

  {
    auto PrePresentBarrier = InitStruct<VkImageMemoryBarrier>();
    {
      PrePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      PrePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      PrePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      PrePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      {
        auto& Range = PrePresentBarrier.subresourceRange;
        Range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Range.baseMipLevel = 0;
        Range.levelCount = 1;
        Range.baseArrayLayer = 0;
        Range.layerCount = 1;
      }
      PrePresentBarrier.image = CurrentSwapchainImage;
    }

    Vulkan->Device.vkCmdPipelineBarrier(Vulkan->DrawCommand,                  // commandBuffer
                                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,   // srcStageMask
                                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dstStageMask
                                        0,                                    // dependencyFlags
                                        0, nullptr,                           // memoryBarrierCount, pMemoryBarriers
                                        0, nullptr,                           // bufferMemoryBarrierCount, pBufferMemoryBarriers
                                        1, &PrePresentBarrier);               // imageMemoryBarrierCount, pImageMemoryBarriers
  }

  VulkanVerify(Vulkan->Device.vkEndCommandBuffer(Vulkan->DrawCommand));

  auto DrawFence = InitStruct<VkFence>();
  {
    auto FenceInfo = InitStruct<VkFenceCreateInfo>();
    VulkanVerify(Vulkan->Device.vkCreateFence(Vulkan->Device.DeviceHandle, &FenceInfo, nullptr, &DrawFence));
  }
  Defer [=](){ Vulkan->Device.vkDestroyFence(Vulkan->Device.DeviceHandle, DrawFence, nullptr); };

  // Submit the draw command to the queue.
  {
    VkPipelineStageFlags PipelineStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    auto SubmitInfo = InitStruct<VkSubmitInfo>();
    {
      SubmitInfo.waitSemaphoreCount = 1;
      SubmitInfo.pWaitSemaphores = &PresentCompleteSemaphore;
      SubmitInfo.pWaitDstStageMask = &PipelineStageFlags;
      SubmitInfo.commandBufferCount = 1;
      SubmitInfo.pCommandBuffers = &Vulkan->DrawCommand;
      SubmitInfo.signalSemaphoreCount = 0;
      SubmitInfo.pSignalSemaphores = nullptr;
    }

    VulkanVerify(Vulkan->Device.vkQueueSubmit(Vulkan->Queue, 1, &SubmitInfo, DrawFence));
  }

  // Wait for the present complete semaphore to be signaled to ensure
  // that the image won't be rendered to until the presentation
  // engine has fully released ownership to the application, and it is
  // okay to render to the image.

  // Make sure the command buffer is finished before presenting.
  {
    VkResult WaitResult = {};
    auto Timeout = Milliseconds(100);
    while(true)
    {
      WaitResult = Vulkan->Device.vkWaitForFences(Vulkan->Device.DeviceHandle,
                                                  1, &DrawFence,
                                                  VK_TRUE,
                                                  Convert<uint64>(TimeAsNanoseconds(Timeout)));
      if(WaitResult == VK_TIMEOUT)
        continue;
      VulkanVerify(WaitResult);
      break;
    }
  }

  // Present the results.
  {
    auto Present = InitStruct<VkPresentInfoKHR>();
    {
      Present.swapchainCount = 1;
      Present.pSwapchains = &Vulkan->Swapchain;
      Present.pImageIndices = &Vulkan->CurrentBufferIndex;
    }

    Error = Vulkan->Device.vkQueuePresentKHR(Vulkan->Queue, &Present);
    switch(Error)
    {
      case VK_ERROR_OUT_OF_DATE_KHR:
      {
        // Swapchain is out of date (e.g. the window was resized) and must be
        // recreated:
        VulkanResize(Vulkan, Vulkan->Width, Vulkan->Height, TempAllocator);
      } break;
      case VK_SUBOPTIMAL_KHR:
      {
        // Swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
      } break;
      default: VulkanVerify(Error);
    }
  }

  // Wait for the queue to complete.
  VulkanVerify(Vulkan->Device.vkQueueWaitIdle(Vulkan->Queue));
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
  Defer [=](){ GlobalLog = nullptr; };

  Init(GlobalLog, Allocator);
  Defer [=](){ Finalize(GlobalLog); };
  auto SinkSlots = ExpandBy(&GlobalLog->Sinks, 2);
  SinkSlots[0] = log_sink(StdoutLogSink);
  SinkSlots[1] = log_sink(VisualStudioLogSink);

  {
    auto Vulkan = Allocate<vulkan>(Allocator);
    *Vulkan = {};
    Defer [=](){ Deallocate(Allocator, Vulkan); };

    Init(Vulkan, Allocator);
    Defer [=](){ Finalize(Vulkan); };

    if(!VulkanLoadDLL(Vulkan))
      return 1;

    if(!VulkanCreateInstance(Vulkan, Allocator))
      return 2;
    Defer [=](){ VulkanCleanup(Vulkan); };

    VulkanLoadInstanceFunctions(Vulkan);

    VulkanSetupDebugging(Vulkan);
    Defer [=](){ VulkanCleanupDebugging(Vulkan); };

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

    Defer [=](){ Win32DestroyWindow(Allocator, Window); };

    //
    // Input Setup
    //
    x_input_dll XInput = {};
    Win32LoadXInput(&XInput, GlobalLog);

    win32_input_context SystemInput = {};
    Init(&SystemInput, Allocator);
    Defer [&](){ Finalize(&SystemInput); };
    {
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

      RegisterInputSlot(&SystemInput, input_type::Axis, MyInputSlots.Depth);
      AddInputSlotMapping(&SystemInput, keyboard::Up, MyInputSlots.Depth, -1);
      AddInputSlotMapping(&SystemInput, keyboard::Down, MyInputSlots.Depth, 1);
      auto DepthSlotProperties = GetOrCreate(&SystemInput.ValueProperties, MyInputSlots.Depth);
      DepthSlotProperties->Sensitivity = 0.005f;

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

    Window->Input = &SystemInput;

    {
      LogBeginScope("Initializing Vulkan for graphics.");

      if(!VulkanInitializeForGraphics(Vulkan, Instance, Window->WindowHandle, Allocator))
      {
        LogEndScope("Failed initialization.");
        return 5;
      }

      LogEndScope("Initialized successfully.");
    }

    {
      LogBeginScope("Preparing swapchain for the first time.");

      if(!VulkanPrepareSwapchain(Vulkan, 1200, 720, Allocator))
      {
        LogEndScope("Failed to prepare initial swapchain.");
        return 6;
      }

      Vulkan->IsPrepared = true;
      LogEndScope("Initial Swapchain is prepared.");
    }

    LogInfo("Vulkan initialization finished!");
    Defer [](){ LogInfo("Shutting down..."); };

    Window->Vulkan = Vulkan;

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
    // Add scene objects
    //
    {
      LogBeginScope("Creating scene objects");
      Defer [](){ LogEndScope("Finished creating scene objects."); };

      auto KittenQuad = VulkanCreateSceneObject(Vulkan);
      auto KittenFileName = "Data/Kitten_DXT1_Mipmaps.dds";
      VulkanSetTextureFromFile(Vulkan, KittenFileName, &KittenQuad->Texture);
      VulkanSetQuadGeometry(Vulkan, Vec2(1, 1), &KittenQuad->Vertices, &KittenQuad->Indices);

      // TODO: This should be done per object in the render loop, but somehow
      // the command buffer recording is stopped by the call to
      // vkUpdateDescriptorSets. For now this is hardcoded to only use the
      // kitten's data.
      if(Vulkan->SceneObjects.Num)
      {
        //
        // Update the texture and sampler in use by the shader.
        //
        auto TextureDescriptor = InitStruct<VkDescriptorImageInfo>();
        {
          TextureDescriptor.sampler = KittenQuad->Texture.SamplerHandle;
          TextureDescriptor.imageView = KittenQuad->Texture.ImageViewHandle;
          TextureDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        }

        auto TextureAndSamplerUpdateInfo = InitStruct<VkWriteDescriptorSet>();
        {
          TextureAndSamplerUpdateInfo.dstSet = Vulkan->DescriptorSet;
          TextureAndSamplerUpdateInfo.dstBinding = 1;
          TextureAndSamplerUpdateInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
          TextureAndSamplerUpdateInfo.descriptorCount = 1;
          TextureAndSamplerUpdateInfo.pImageInfo = &TextureDescriptor;
        }

        Vulkan->Device.vkUpdateDescriptorSets(Vulkan->Device.DeviceHandle,
                                              1, &TextureAndSamplerUpdateInfo,
                                              0, nullptr);
      }
    }

    //
    // Main Loop
    //
    Vulkan->DepthStencilValue = 1.0f;
    ::GlobalRunning = true;

    stopwatch FrameTimer = {};

    float DeltaSeconds = 0.016f; // Assume 16 milliseconds for the first frame.

    while(::GlobalRunning)
    {
      StopwatchStart(&FrameTimer);

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

      Vulkan->DepthStencilValue = Clamp(Vulkan->DepthStencilValue + AxisValue(SystemInput[MyInputSlots.Depth]), 0.8f, 1.0f);

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

      // auto const ViewProjectionMatrix = CameraViewProjectionMatrix(Cam, Cam.Transform);
      auto const ViewProjectionMatrix = IdentityMatrix4x4;

      //
      // Upload shader globals
      //
      {
        void* RawData;
        VulkanVerify(Vulkan->Device.vkMapMemory(Vulkan->Device.DeviceHandle,
                                                Vulkan->GlobalUBO_MemoryHandle,
                                                0, // offset
                                                VK_WHOLE_SIZE,
                                                0, // flags
                                                &RawData));

        auto const Target = Reinterpret<mat4x4*>(RawData);
        MemCopy(1, Target, &ViewProjectionMatrix);

        Vulkan->Device.vkUnmapMemory(Vulkan->Device.DeviceHandle, Vulkan->GlobalUBO_MemoryHandle);
      }

      //
      // Handle resize requests
      //
      if(::GlobalIsResizeRequested)
      {
        LogBeginScope("Resizing swapchain");
        VulkanResize(Vulkan, ::GlobalResizeRequest_Width, ::GlobalResizeRequest_Height, Allocator);
        ::GlobalIsResizeRequested = false;
        LogEndScope("Finished resizing swapchain");
      }

      RedrawWindow(Window->WindowHandle, nullptr, nullptr, RDW_INTERNALPAINT);

      // Update frame timer.
      {
        StopwatchStop(&FrameTimer);
        DeltaSeconds = Cast<float>(TimeAsSeconds(StopwatchTime(&FrameTimer)));
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
    bool MustBeginAndEndPaint = !!GetUpdateRect(WindowHandle, &Rect, FALSE);

    if(MustBeginAndEndPaint)
      BeginPaint(WindowHandle, &Paint);

    if(Vulkan && Vulkan->IsPrepared)
    {
      // TODO: More appropriate temp allocator?
      mallocator TempAllocator = {};
      VulkanDraw(Vulkan, &TempAllocator);
    }

    if(MustBeginAndEndPaint)
      EndPaint(WindowHandle, &Paint);
  }
  else
  {
    Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
  }

  return Result;
}
