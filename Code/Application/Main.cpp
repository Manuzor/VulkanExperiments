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
    SetNum(&Vulkan->Gpu.QueueProperties, QueueCount);
    Vulkan->vkGetPhysicalDeviceQueueFamilyProperties(Vulkan->Gpu.GpuHandle, &QueueCount, Vulkan->Gpu.QueueProperties.Ptr);
  }

  return true;
}

static bool
VulkanInitializeGraphics(vulkan* Vulkan, HINSTANCE ProcessHandle, HWND WindowHandle, allocator_interface* TempAllocator)
{
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

    float QueuePriority = 0.0f;

    auto QueueCreateInfo = InitStruct<VkDeviceQueueCreateInfo>();
    {
      QueueCreateInfo.queueFamilyIndex = Vulkan->Surface.PresentNode.Index;
      QueueCreateInfo.queueCount = 1;
      QueueCreateInfo.pQueuePriorities = &QueuePriority;
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

    VulkanLoadDeviceFunctions(*Vulkan, &Vulkan->Device);
  }

  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  //
  // Get device Queue
  //
  {
    Device.vkGetDeviceQueue(DeviceHandle, Vulkan->Surface.PresentNode.Index, 0, &Vulkan->Queue);
    Assert(Vulkan->Queue);
  }

  //
  // Create Command Pool
  //
  {
    LogBeginScope("Creating command pool.");
    Defer [](){ LogEndScope("Finished creating command pool."); };

    auto CreateInfo = InitStruct<VkCommandPoolCreateInfo>();
    {
      CreateInfo.queueFamilyIndex = Vulkan->Surface.PresentNode.Index;
      CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    }

    VulkanVerify(Device.vkCreateCommandPool(DeviceHandle, &CreateInfo, nullptr, &Vulkan->CommandPool));
    Assert(Vulkan->CommandPool);
  }

  //
  // Create Seamphores
  //
  {
    auto CreateInfo = InitStruct<VkSemaphoreCreateInfo>();
    VulkanVerify(Device.vkCreateSemaphore(DeviceHandle, &CreateInfo, nullptr, &Vulkan->PresentCompleteSemaphore));
    VulkanVerify(Device.vkCreateSemaphore(DeviceHandle, &CreateInfo, nullptr, &Vulkan->RenderCompleteSemaphore));
  }

  // Done.
  return true;
}

static void
VulkanFinalizeGraphics(vulkan* Vulkan)
{
  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  //
  // Destroy Seamphores
  //
  Device.vkDestroySemaphore(DeviceHandle, Vulkan->PresentCompleteSemaphore, nullptr);
  Device.vkDestroySemaphore(DeviceHandle, Vulkan->RenderCompleteSemaphore, nullptr);

  //
  // Destroy Command Pool
  //
  Device.vkDestroyCommandPool(DeviceHandle, Vulkan->CommandPool, nullptr);

  //
  // Destroy Logical Device
  //
  Vulkan->vkDestroyDevice(DeviceHandle, nullptr);
}

static bool
VulkanPrepareRenderPass(vulkan* Vulkan)
{
  // TODO: Put this somewhere else?

  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  temp_allocator TempAllocator;
  allocator_interface* Allocator = *TempAllocator;

  //
  // Prepare Descriptor Set Layout
  //
  {
    LogBeginScope("Preparing descriptor set layout.");
    Defer [](){ LogEndScope("Finished preparing descriptor set layout."); };

    scoped_array<VkDescriptorSetLayoutBinding> LayoutBindings{ Allocator };

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

    VulkanVerify(Device.vkCreateDescriptorSetLayout(DeviceHandle, &DescriptorSetLayoutCreateInfo, nullptr, &Vulkan->DescriptorSetLayout));

    auto PipelineLayoutCreateInfo = InitStruct<VkPipelineLayoutCreateInfo>();
    {
      PipelineLayoutCreateInfo.setLayoutCount = 1;
      PipelineLayoutCreateInfo.pSetLayouts = &Vulkan->DescriptorSetLayout;
    }
    VulkanVerify(Device.vkCreatePipelineLayout(DeviceHandle, &PipelineLayoutCreateInfo, nullptr, &Vulkan->PipelineLayout));
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

      Attachment.format = Vulkan->Surface.Format;
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

    VulkanVerify(Device.vkCreateRenderPass(DeviceHandle, &RenderPassCreateInfo, nullptr, &Vulkan->RenderPass));
  }

  //
  // Create Pipeline Cache
  //
  {
    auto CreateInfo = InitStruct<VkPipelineCacheCreateInfo>();
    VulkanVerify(Device.vkCreatePipelineCache(DeviceHandle, &CreateInfo, nullptr, &Vulkan->PipelineCache));
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

      scoped_array<uint8> ShaderCode{ Allocator };
      ReadFileContentIntoArray(&ShaderCode, FileName);

      auto ShaderModuleCreateInfo = InitStruct<VkShaderModuleCreateInfo>();
      ShaderModuleCreateInfo.codeSize = Cast<uint32>(ShaderCode.Num); // In bytes, regardless of the fact that typeof(*pCode) == uint.
      ShaderModuleCreateInfo.pCode = Reinterpret<uint32*>(ShaderCode.Ptr); // Is a const(uint)*, for some reason...

      VulkanVerify(Device.vkCreateShaderModule(DeviceHandle, &ShaderModuleCreateInfo, nullptr, &VertexShaderStage.module));
    }
    // Note(Manu): Can safely destroy the shader modules on our side once the pipeline was created.
    Defer [&](){ Device.vkDestroyShaderModule(DeviceHandle, VertexShaderStage.module, nullptr); };

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

      scoped_array<uint8> ShaderCode{ Allocator };
      ReadFileContentIntoArray(&ShaderCode, FileName);

      auto ShaderModuleCreateInfo = InitStruct<VkShaderModuleCreateInfo>();
      ShaderModuleCreateInfo.codeSize = Cast<uint32>(ShaderCode.Num); // In bytes, regardless of the fact that typeof(*pCode) == uint.
      ShaderModuleCreateInfo.pCode = Reinterpret<uint32*>(ShaderCode.Ptr); // Is a const(uint)*, for some reason...

      VulkanVerify(Device.vkCreateShaderModule(DeviceHandle, &ShaderModuleCreateInfo, nullptr, &FragmentShaderStage.module));
    }
    Defer [&](){ Device.vkDestroyShaderModule(DeviceHandle, FragmentShaderStage.module, nullptr); };

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
      DepthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
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

    fixed_block<2, VkDynamicState> DynamicStatesBlock;
    DynamicStatesBlock[0] = VK_DYNAMIC_STATE_VIEWPORT;
    DynamicStatesBlock[1] = VK_DYNAMIC_STATE_SCISSOR;
    auto DynamicState = InitStruct<VkPipelineDynamicStateCreateInfo>();
    {
      DynamicState.dynamicStateCount = Cast<uint32>(DynamicStatesBlock.Num);
      DynamicState.pDynamicStates = &DynamicStatesBlock[0];
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

    VulkanVerify(Device.vkCreateGraphicsPipelines(DeviceHandle, Vulkan->PipelineCache,
                                                  1, &GraphicsPipelineCreateInfo,
                                                  nullptr,
                                                  &Vulkan->Pipeline));
  }

  //
  // Prepare Descriptor Pool
  //
  {
    LogBeginScope("Preparing descriptor pool.");
    Defer [](){ LogEndScope("Finished preparing descriptor pool."); };

    scoped_array<VkDescriptorPoolSize> PoolSizes{ Allocator };

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

    VulkanVerify(Device.vkCreateDescriptorPool(DeviceHandle,
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
    VulkanVerify(Device.vkAllocateDescriptorSets(DeviceHandle,
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

      VulkanVerify(Device.vkCreateBuffer(DeviceHandle, &BufferCreateInfo, nullptr,
                                         &Vulkan->GlobalsUBO.Buffer));

      VkMemoryRequirements Temp_MemoryRequirements;
      Device.vkGetBufferMemoryRequirements(DeviceHandle, Vulkan->GlobalsUBO.Buffer,
                                           &Temp_MemoryRequirements);

      auto Temp_MemoryAllocationInfo = InitStruct<VkMemoryAllocateInfo>();
      {
        Temp_MemoryAllocationInfo.allocationSize = Temp_MemoryRequirements.size;
        Temp_MemoryAllocationInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan->Device.Gpu->MemoryProperties,
                                                                                   Temp_MemoryRequirements.memoryTypeBits,
                                                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        Assert(Temp_MemoryAllocationInfo.memoryTypeIndex != IntMaxValue<uint32>());
      }

      VulkanVerify(Device.vkAllocateMemory(DeviceHandle, &Temp_MemoryAllocationInfo, nullptr, &Vulkan->GlobalsUBO.Memory));

      VulkanVerify(Device.vkBindBufferMemory(DeviceHandle, Vulkan->GlobalsUBO.Buffer, Vulkan->GlobalsUBO.Memory, 0));
    }

    //
    // Associate the buffer with the descriptor set.
    //
    auto DescriptorBufferInfo = InitStruct<VkDescriptorBufferInfo>();
    {
      DescriptorBufferInfo.buffer = Vulkan->GlobalsUBO.Buffer;
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

    Device.vkUpdateDescriptorSets(DeviceHandle,
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
      FramebufferCreateInfo.width = Vulkan->Swapchain.Extent.Width;
      FramebufferCreateInfo.height = Vulkan->Swapchain.Extent.Height;
      FramebufferCreateInfo.layers = 1;
    }

    SetNum(&Vulkan->Framebuffers, Vulkan->Swapchain.ImageCount);

    auto const ImageCount = Vulkan->Swapchain.ImageCount;
    for(uint32 Index = 0; Index < ImageCount; ++Index)
    {
      Attachments[0] = Vulkan->Swapchain.ImageViews[Index];
      VulkanVerify(Device.vkCreateFramebuffer(DeviceHandle,
                                              &FramebufferCreateInfo,
                                              nullptr,
                                              &Vulkan->Framebuffers[Index]));
    }
  }

  // Done preparing swapchain...
  return true;
}

static void
VulkanCleanupRenderPass(vulkan* Vulkan)
{
  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  // Framebuffers
  for(auto Framebuffer : Slice(&Vulkan->Framebuffers))
  {
    Device.vkDestroyFramebuffer(DeviceHandle, Framebuffer, nullptr);
  }
  Clear(&Vulkan->Framebuffers);

  // UBOs
  Device.vkFreeMemory(DeviceHandle, Vulkan->GlobalsUBO.Memory, nullptr);
  Device.vkDestroyBuffer(DeviceHandle, Vulkan->GlobalsUBO.Buffer, nullptr);

  // Descriptor Sets
  Device.vkFreeDescriptorSets(DeviceHandle, Vulkan->DescriptorPool, 1, &Vulkan->DescriptorSet);

  // Descriptor Pool
  Device.vkDestroyDescriptorPool(DeviceHandle, Vulkan->DescriptorPool, nullptr);

  // Pipeline
  Device.vkDestroyPipeline(DeviceHandle, Vulkan->Pipeline, nullptr);

  // Pipeline Cache
  Device.vkDestroyPipelineCache(DeviceHandle, Vulkan->PipelineCache, nullptr);

  // Render Pass
  Device.vkDestroyRenderPass(DeviceHandle, Vulkan->RenderPass, nullptr);

  // Pipeline Layout
  Device.vkDestroyPipelineLayout(DeviceHandle, Vulkan->PipelineLayout, nullptr);

  // Descriptor Set Layout
  Device.vkDestroyDescriptorSetLayout(DeviceHandle, Vulkan->DescriptorSetLayout, nullptr);
}

static void
VulkanCreateCommandBuffers(vulkan* Vulkan, VkCommandPool CommandPool, uint32 Num)
{
  LogBeginScope("Creating command buffers.");
  Defer [](){ LogEndScope("Finished creating command buffers."); };

  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  auto AllocateInfo = InitStruct<VkCommandBufferAllocateInfo>();
  {
    AllocateInfo.commandPool = Vulkan->CommandPool;
    AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocateInfo.commandBufferCount = Num;
  }

  SetNum(&Vulkan->DrawCommands, Num);
  SetNum(&Vulkan->PrePresentCommands, Num);
  SetNum(&Vulkan->PostPresentCommands, Num);

  VulkanVerify(Device.vkAllocateCommandBuffers(DeviceHandle, &AllocateInfo, Vulkan->DrawCommands.Ptr));
  VulkanVerify(Device.vkAllocateCommandBuffers(DeviceHandle, &AllocateInfo, Vulkan->PrePresentCommands.Ptr));
  VulkanVerify(Device.vkAllocateCommandBuffers(DeviceHandle, &AllocateInfo, Vulkan->PostPresentCommands.Ptr));
}

static void VulkanDestroyCommandBuffers(vulkan* Vulkan, VkCommandPool CommandPool)
{
  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  Device.vkFreeCommandBuffers(DeviceHandle, CommandPool, Cast<uint32>(Vulkan->PostPresentCommands.Num), Vulkan->PostPresentCommands.Ptr);
  Device.vkFreeCommandBuffers(DeviceHandle, CommandPool, Cast<uint32>(Vulkan->PrePresentCommands.Num), Vulkan->PrePresentCommands.Ptr);
  Device.vkFreeCommandBuffers(DeviceHandle, CommandPool, Cast<uint32>(Vulkan->DrawCommands.Num), Vulkan->DrawCommands.Ptr);

  Clear(&Vulkan->DrawCommands);
  Clear(&Vulkan->PrePresentCommands);
  Clear(&Vulkan->PostPresentCommands);
}

// TODO
#if 0
static void
VulkanDestroySwapchain(vulkan* Vulkan)
{
  Assert(Vulkan->IsPrepared);

  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  Vulkan->IsPrepared = false;

  Device.vkDestroyDescriptorPool(DeviceHandle, Vulkan->DescriptorPool, nullptr);

  if(Vulkan->SetupCommand)
  {
    Device.vkFreeCommandBuffers(DeviceHandle, Vulkan->CommandPool, 1, &Vulkan->SetupCommand);
  }
  Device.vkFreeCommandBuffers(DeviceHandle, Vulkan->CommandPool, 1, &Vulkan->DrawCommand);
  Device.vkDestroyCommandPool(DeviceHandle, Vulkan->CommandPool, nullptr);

  Device.vkDestroyPipeline(DeviceHandle, Vulkan->Pipeline, nullptr);
  Device.vkDestroyRenderPass(DeviceHandle, Vulkan->RenderPass, nullptr);
  Device.vkDestroyPipelineLayout(DeviceHandle, Vulkan->PipelineLayout, nullptr);
  Device.vkDestroyDescriptorSetLayout(DeviceHandle, Vulkan->DescriptorSetLayout, nullptr);

  for(auto& SceneObject : Slice(&Vulkan->SceneObjects))
  {
    VulkanDestroySceneObject(Vulkan, &SceneObject);
  }

  Device.vkDestroyImageView(DeviceHandle, Vulkan->Depth.View, nullptr);
  Device.vkDestroyImage(DeviceHandle,     Vulkan->Depth.Image, nullptr);
  Device.vkFreeMemory(DeviceHandle,       Vulkan->Depth.Memory, nullptr);
}
#endif

static void
VulkanCleanup(vulkan* Vulkan)
{
  // TODO

  #if 0
  LogBeginScope("Vulkan cleanup.");
  Defer [](){ LogEndScope("Finished Vulkan cleanup."); };

  if(Vulkan->IsPrepared)
  {
    VulkanDestroySwapchain(Vulkan);
  }

  Vulkan->Device.vkDestroyDevice(Vulkan->Device.DeviceHandle, nullptr);

  Vulkan->vkDestroySurfaceKHR(Vulkan->InstanceHandle, Vulkan->Surface.SurfaceHandle, nullptr);
  Vulkan->vkDestroyInstance(Vulkan->InstanceHandle, nullptr);

  Clear(&Vulkan->Gpu.QueueProperties);
  #endif
}

static void
VulkanResize(vulkan* Vulkan, uint32 NewWidth, uint32 NewHeight)
{
  // TODO

  #if 0
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
  #endif
}

static void
VulkanBuildDrawCommands(vulkan const&          Vulkan,
                        slice<VkCommandBuffer> DrawCommandBuffers,
                        slice<VkFramebuffer>   Framebuffers,
                        color_linear const&    ClearColor,
                        float                  ClearDepth,
                        uint32                 ClearStencil)
{
  BoundsCheck(DrawCommandBuffers.Num == Framebuffers.Num);

  auto const& Device = Vulkan.Device;

  fixed_block<2, VkClearValue> ClearValuesBlock;
  auto ClearValues = Slice(ClearValuesBlock);

  // Regular clear color.
  SliceCopy(Slice(ClearValues[0].color.float32),
            AsConst(Slice(ClearColor.Data)));

  // DepthStencil clear values.
  ClearValues[1].depthStencil.depth = ClearDepth;
  ClearValues[1].depthStencil.stencil = ClearStencil;

  auto RenderPassBeginInfo = InitStruct<VkRenderPassBeginInfo>();
  {
    RenderPassBeginInfo.renderPass = Vulkan.RenderPass;
    RenderPassBeginInfo.renderArea.extent.width = Vulkan.Swapchain.Extent.Width;
    RenderPassBeginInfo.renderArea.extent.height = Vulkan.Swapchain.Extent.Height;
    RenderPassBeginInfo.clearValueCount = Cast<uint32>(ClearValues.Num);
    RenderPassBeginInfo.pClearValues = ClearValues.Ptr;
  }

  auto BeginCommandBufferInfo = InitStruct<VkCommandBufferBeginInfo>();

  auto const Num = DrawCommandBuffers.Num;
  for(size_t Index = 0; Index < Num; ++Index)
  {
    RenderPassBeginInfo.framebuffer = Framebuffers[Index];

    auto DrawCommandBuffer = DrawCommandBuffers[Index];

    VulkanVerify(Device.vkBeginCommandBuffer(DrawCommandBuffer, &BeginCommandBufferInfo));
    Device.vkCmdBeginRenderPass(DrawCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
      // Set viewport
      {
        auto Viewport = InitStruct<VkViewport>();
        {
          Viewport.height = Cast<float>(Vulkan.Swapchain.Extent.Height);
          Viewport.width = Cast<float>(Vulkan.Swapchain.Extent.Width);
          Viewport.minDepth = 0.0f;
          Viewport.maxDepth = 1.0f;
        }
        Device.vkCmdSetViewport(DrawCommandBuffer, 0, 1, &Viewport);
      }

      // Set scissor
      {
        auto Scissor = InitStruct<VkRect2D>();
        {
          Scissor.extent.width = Vulkan.Swapchain.Extent.Width;
          Scissor.extent.height = Vulkan.Swapchain.Extent.Height;
        }
        Device.vkCmdSetScissor(DrawCommandBuffer, 0, 1, &Scissor);
      }

      // Bind descriptor set
      Device.vkCmdBindDescriptorSets(DrawCommandBuffer,
                                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                                     Vulkan.PipelineLayout,
                                     0, // Descriptor set offset
                                     1, &Vulkan.DescriptorSet,
                                     0, nullptr); // Dynamic offsets

      // Draw scene objects
      VkDeviceSize NoOffset = {};

      for(auto& SceneObject : Slice(&Vulkan.SceneObjects))
      {
        Device.vkCmdBindPipeline(DrawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, SceneObject.Pipeline);
        Device.vkCmdBindVertexBuffers(DrawCommandBuffer, SceneObject.Vertices.BindID, 1, &SceneObject.Vertices.Buffer, &NoOffset);
        Device.vkCmdBindIndexBuffer(DrawCommandBuffer, SceneObject.Indices.Buffer, NoOffset, VK_INDEX_TYPE_UINT32);
        Device.vkCmdDrawIndexed(DrawCommandBuffer, SceneObject.Indices.NumIndices, 1, 0, 0, 0);
      }
    }
    Device.vkCmdEndRenderPass(DrawCommandBuffer);
    VulkanVerify(Device.vkEndCommandBuffer(DrawCommandBuffer));
  }
}

static void
VulkanBuildPrePresentCommands(vulkan_device const& Device,
                              slice<VkCommandBuffer> PrePresentCommandBuffers,
                              slice<VkImage> PresentationImages)
{
  BoundsCheck(PrePresentCommandBuffers.Num == PresentationImages.Num);

  auto const DeviceHandle = Device.DeviceHandle;

  auto BeginCommandBufferInfo = InitStruct<VkCommandBufferBeginInfo>();

  size_t const Num = PrePresentCommandBuffers.Num;
  for(size_t Index = 0; Index < Num; ++Index)
  {
    auto const PrePresentCommandBuffer = PrePresentCommandBuffers[Index];
    auto const PresentationImage = PresentationImages[Index];

    VulkanVerify(Device.vkBeginCommandBuffer(PrePresentCommandBuffer, &BeginCommandBufferInfo));

    {
      auto ImageBarrier = InitStruct<VkImageMemoryBarrier>();
      ImageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      ImageBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      ImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      ImageBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
      ImageBarrier.image = PresentationImage;

      Device.vkCmdPipelineBarrier(PrePresentCommandBuffer,
                                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                  0,
                                  0, nullptr, // memory barriers
                                  0, nullptr, // buffer barriers
                                  1, &ImageBarrier);
    }

    VulkanVerify(Device.vkEndCommandBuffer(PrePresentCommandBuffer));
  }
}

static void
VulkanBuildPostPresentCommands(vulkan_device const& Device,
                               slice<VkCommandBuffer> PostPresentCommandBuffers,
                               slice<VkImage> PresentationImages)
{
  BoundsCheck(PostPresentCommandBuffers.Num == PresentationImages.Num);

  auto const DeviceHandle = Device.DeviceHandle;

  auto BeginCommandBufferInfo = InitStruct<VkCommandBufferBeginInfo>();

  size_t const Num = PostPresentCommandBuffers.Num;
  for(size_t Index = 0; Index < Num; ++Index)
  {
    auto const PostPresentCommandBuffer = PostPresentCommandBuffers[Index];
    auto const PresentationImage = PresentationImages[Index];

    VulkanVerify(Device.vkBeginCommandBuffer(PostPresentCommandBuffer, &BeginCommandBufferInfo));

    {
      auto ImageBarrier = InitStruct<VkImageMemoryBarrier>();
      ImageBarrier.srcAccessMask = 0;
      ImageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      ImageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      ImageBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
      ImageBarrier.image = PresentationImage;

      Device.vkCmdPipelineBarrier(PostPresentCommandBuffer,
                                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                  0,
                                  0, nullptr, // memory barriers
                                  0, nullptr, // buffer barriers
                                  1, &ImageBarrier);
    }

    VulkanVerify(Device.vkEndCommandBuffer(PostPresentCommandBuffer));
  }
}

static bool GlobalIsDrawing = false;

static void
VulkanDraw(vulkan* Vulkan)
{
  ::GlobalIsDrawing = true;
  Defer [](){ ::GlobalIsDrawing = false; };

  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  VkFence NullFence = {};
  VkResult Error = {};


  //
  // Get next swapchain image.
  //
  {
    Error = VulkanAcquireNextSwapchainImage(Vulkan->Swapchain,
                                            Vulkan->PresentCompleteSemaphore,
                                            &Vulkan->CurrentSwapchainImage);

    // TODO: Check if this ever happens.
    switch(Error)
    {
      case VK_ERROR_OUT_OF_DATE_KHR:
      {
        // Swapchain is out of date (e.g. the window was resized) and must be
        // recreated:
        VulkanResize(Vulkan, Vulkan->Swapchain.Extent.Width, Vulkan->Swapchain.Extent.Height);
        VulkanDraw(Vulkan);
      } break;
      case VK_SUBOPTIMAL_KHR:
      {
        // Swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
      } break;
      default: VulkanVerify(Error);
    }
  }


  //
  // Submit post-present commands.
  //
  {
    auto SubmitInfo = InitStruct<VkSubmitInfo>();
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &Vulkan->PostPresentCommands[Vulkan->CurrentSwapchainImage.Index];
    VulkanVerify(Device.vkQueueSubmit(Vulkan->Queue, 1, &SubmitInfo, NullFence));
  }


  //
  // Submit draw commands.
  //
  {
    VkPipelineStageFlags const SubmitPipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    auto SubmitInfo = InitStruct<VkSubmitInfo>();
    SubmitInfo.pWaitDstStageMask = &SubmitPipelineStages;
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores = &Vulkan->PresentCompleteSemaphore;
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores = &Vulkan->RenderCompleteSemaphore;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &Vulkan->DrawCommands[Vulkan->CurrentSwapchainImage.Index];
    VulkanVerify(Device.vkQueueSubmit(Vulkan->Queue, 1, &SubmitInfo, NullFence));
  }


  //
  // Submit pre-present commands.
  //
  {
    auto SubmitInfo = InitStruct<VkSubmitInfo>();
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &Vulkan->PrePresentCommands[Vulkan->CurrentSwapchainImage.Index];
    VulkanVerify(Device.vkQueueSubmit(Vulkan->Queue, 1, &SubmitInfo, NullFence));
  }


  //
  // Present
  //
  {
    Error = VulkanQueuePresent(&Vulkan->Swapchain,
                               Vulkan->Queue,
                               Vulkan->CurrentSwapchainImage,
                               Vulkan->RenderCompleteSemaphore);

    // TODO: See if we need to handle the cases in the switch below.
    VulkanVerify(Error);

    #if 0
    switch(Error)
    {
      case VK_ERROR_OUT_OF_DATE_KHR:
      {
        // Swapchain is out of date (e.g. the window was resized) and must be
        // recreated:
        VulkanResize(Vulkan, Vulkan->Swapchain.Extent.Width, Vulkan->Swapchain.Extent.Height);
        VulkanDraw(Vulkan);
      } break;
      case VK_SUBOPTIMAL_KHR:
      {
        // Swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
      } break;
      default: VulkanVerify(Error);
    }
    #endif
  }

  //
  // Wait for the presenting to finish.
  //
  Device.vkQueueWaitIdle(Vulkan->Queue);
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
  int CurrentExitCode = 0;

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

    ++CurrentExitCode;
    if(!VulkanLoadDLL(Vulkan))
      return CurrentExitCode;

    ++CurrentExitCode;
    if(!VulkanCreateInstance(Vulkan, Allocator))
      return CurrentExitCode;
    Defer [=](){ VulkanCleanup(Vulkan); };

    VulkanLoadInstanceFunctions(Vulkan);

    VulkanSetupDebugging(Vulkan);
    Defer [=](){ VulkanCleanupDebugging(Vulkan); };

    ++CurrentExitCode;
    if(!VulkanChooseAndSetupPhysicalDevices(Vulkan, Allocator))
      return CurrentExitCode;

    window_setup WindowSetup = {};
    WindowSetup.ProcessHandle = Instance;
    WindowSetup.WindowClassName = "VulkanExperimentsWindowClass";
    WindowSetup.ClientWidth = 768;
    WindowSetup.ClientHeight = 768;
    auto Window = Win32CreateWindow(Allocator, &WindowSetup);

    ++CurrentExitCode;
    if(Window == nullptr)
      return CurrentExitCode;

    Defer [=](){ Win32DestroyWindow(Allocator, Window); };

    //
    // Surface
    //
    {
      LogBeginScope("Preparing OS surface.");

      ++CurrentExitCode;
      if(!VulkanPrepareSurface(*Vulkan, &Vulkan->Surface, Instance, Window->WindowHandle))
      {
        LogEndScope("Surface creation failed.");
        return CurrentExitCode;
      }

      LogEndScope("OS surface successfully created.");
    }
    Defer [Vulkan](){ VulkanCleanupSurface(*Vulkan, &Vulkan->Surface); };


    //
    // Device creation
    //
    {
      LogBeginScope("Initializing Vulkan for graphics.");

      ++CurrentExitCode;
      if(!VulkanInitializeGraphics(Vulkan, Instance, Window->WindowHandle, Allocator))
      {
        LogEndScope("Vulkan initialization failed.");
        return CurrentExitCode;
      }

      LogEndScope("Vulkan successfully initialized.");
    }
    Defer [Vulkan](){ VulkanFinalizeGraphics(Vulkan); };

    VulkanSwapchainConnect(&Vulkan->Swapchain, &Vulkan->Device, &Vulkan->Surface);

    // Now that we have a command pool, we can create the setup command
    VulkanPrepareSetupCommandBuffer(Vulkan);

    //
    // Swapchain
    //
    {
      LogBeginScope("Preparing swapchain for the first time.");

      ++CurrentExitCode;
      if(!VulkanPrepareSwapchain(*Vulkan, &Vulkan->Swapchain, { 1280, 720 }, vsync::On))
      {
        LogEndScope("Failed to prepare initial swapchain.");
        return CurrentExitCode;
      }

      Vulkan->IsPrepared = true;
      LogEndScope("Initial Swapchain is prepared.");
    }
    Defer [Vulkan](){ VulkanCleanupSwapchain(*Vulkan, &Vulkan->Swapchain); };


    //
    // Create Command Buffers
    //
    {
      VulkanCreateCommandBuffers(Vulkan, Vulkan->CommandPool, Vulkan->Swapchain.ImageCount);
    }
    Defer [Vulkan](){ VulkanDestroyCommandBuffers(Vulkan, Vulkan->CommandPool); };


    //
    // Depth
    //
    {
      LogBeginScope("Preparing depth.");

      ++CurrentExitCode;
      if(!VulkanPrepareDepth(Vulkan, &Vulkan->Depth, Vulkan->Swapchain.Extent))
      {
        LogEndScope("Failed preparing depth.");
        return CurrentExitCode;
      }

      LogEndScope("Finished preparing depth.");
    }
    Defer [Vulkan](){ VulkanCleanupDepth(*Vulkan, &Vulkan->Depth); };


    //
    // Render Pass
    //
    {
      VulkanPrepareRenderPass(Vulkan);
    }
    Defer [Vulkan](){ VulkanCleanupRenderPass(Vulkan); };

    // Flush the setup command once now to finalize initialization but prepare it for further use also.
    VulkanCleanupSetupCommandBuffer(Vulkan, flush_command_buffer::Yes);
    VulkanPrepareSetupCommandBuffer(Vulkan);
    Defer [Vulkan](){ VulkanCleanupSetupCommandBuffer(Vulkan, flush_command_buffer::No); };

    Window->Vulkan = Vulkan;

    LogInfo("Vulkan initialization finished!");
    Defer [](){ LogInfo("Shutting down..."); };


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

      Window->Input = &SystemInput;
    }


    //
    // Camera Setup
    //
    free_horizon_camera Cam = {};
    {
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
    }

    //
    // Add scene objects
    //
    {
      LogBeginScope("Creating scene objects");
      Defer [](){ LogEndScope("Finished creating scene objects."); };

      auto Kitten = VulkanCreateSceneObject(Vulkan);
      auto KittenFileName = "Data/Kitten_DXT1_Mipmaps.dds";
      VulkanSetTextureFromFile(Vulkan, KittenFileName, &Kitten->Texture);
      VulkanSetQuadGeometry(Vulkan, { 1, 1 }, &Kitten->Vertices, &Kitten->Indices);

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
          TextureDescriptor.sampler = Kitten->Texture.SamplerHandle;
          TextureDescriptor.imageView = Kitten->Texture.ImageViewHandle;
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

      Kitten->Pipeline = Vulkan->Pipeline;
    }

    //
    // Build Command Buffers
    //
    {
      VulkanBuildDrawCommands(*Vulkan,
                              Slice(&Vulkan->DrawCommands),
                              Slice(&Vulkan->Framebuffers),
                              color::CornflowerBlue,
                              Vulkan->DepthStencilValue,
                              0);
      VulkanBuildPrePresentCommands(Vulkan->Device,
                                    Slice(&Vulkan->PrePresentCommands),
                                    Slice(&Vulkan->Swapchain.Images));
      VulkanBuildPostPresentCommands(Vulkan->Device,
                                     Slice(&Vulkan->PostPresentCommands),
                                     Slice(&Vulkan->Swapchain.Images));
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
      {
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
      }

      // auto const ViewProjectionMatrix = CameraViewProjectionMatrix(Cam, Cam.Transform);
      auto const ViewProjectionMatrix = IdentityMatrix4x4;

      //
      // Upload shader globals
      //
      {
        void* RawData;
        VulkanVerify(Vulkan->Device.vkMapMemory(Vulkan->Device.DeviceHandle,
                                                Vulkan->GlobalsUBO.Memory,
                                                0, // offset
                                                VK_WHOLE_SIZE,
                                                0, // flags
                                                &RawData));

        auto const Target = Reinterpret<mat4x4*>(RawData);
        MemCopy(1, Target, &ViewProjectionMatrix);

        Vulkan->Device.vkUnmapMemory(Vulkan->Device.DeviceHandle, Vulkan->GlobalsUBO.Memory);
      }

      //
      // Handle resize requests
      //
      if(::GlobalIsResizeRequested)
      {
        LogBeginScope("Resizing swapchain");
        VulkanResize(Vulkan, ::GlobalResizeRequest_Width, ::GlobalResizeRequest_Height);
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
      temp_allocator TempAllocator;
      VulkanDraw(Vulkan);
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
