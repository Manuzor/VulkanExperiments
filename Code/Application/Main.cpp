#include "VulkanHelper.hpp"
#include "Stats.hpp"

#include <Core/Array.hpp>
#include <Core/Log.hpp>
#include <Core/Input.hpp>
#include <Core/Win32_Input.hpp>
#include <Core/Time.hpp>

#include <Core/Image.hpp>
#include <Core/ImageLoader.hpp>

#include <Core/Math.hpp>
#include <Core/Color.hpp>
#include <Core/String.hpp>

#include <Core/Camera.hpp>

#include <Backbone.hpp>

#include <ShaderCompiler/ShaderCompiler.hpp>

#include <Windows.h>


// TODO Get rid of this.
#include <cstdio>

static bool GlobalRunning = false;
static bool GlobalIsResizeRequested = false;
static extent2_<uint32> GlobalResizeRequest_Extent{};

static slice<char const>
ThisExeDir()
{
  static arc_string Buffer;
  if(StrIsEmpty(Buffer))
  {
    // TODO: Use GetModuleFileNameW and convert to UTF-8
    fixed_block<256, char> RawBuffer;

    // TODO: Handle return value
    DWORD Result = GetModuleFileNameA(nullptr, First(RawBuffer), RawBuffer.Num);
    auto FullPath = SliceFromString(First(RawBuffer));

    slice<char const> Directory;
    ExtractPathDirectoryAndFileName(FullPath, &Directory, nullptr);

    Buffer = Directory;
  }
  return Slice(Buffer);
}

static arc_string
DataPath(arc_string const& RelativePath)
{
  arc_string AbsolutePath;
  AbsolutePath += ThisExeDir();
  AbsolutePath += "/../Data/";
  AbsolutePath += Slice(RelativePath);
  return AbsolutePath;
}

static LRESULT WINAPI
Win32MainWindowCallback(HWND WindowHandle, UINT Message,
                        WPARAM WParam, LPARAM LParam);

struct window_setup
{
  HINSTANCE ProcessHandle;
  char const* WindowClassName;
  extent2_<uint> ClientExtents;

  bool HasCustomPosition;
  int WindowX;
  int WindowY;
};

struct window
{
  HWND WindowHandle;
  extent2_<uint32> ClientExtents;

  input_context Input;
  input_keyboard_slots* Keyboard;
  input_mouse_slots* Mouse;
  vulkan* Vulkan;
  uint64 DrawCount;
};

static window*
Win32CreateWindow(allocator_interface& Allocator, window_setup const* Args,
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

  {
    auto const ClientArea = Args->ClientExtents.Width * Args->ClientExtents.Height;
    if(ClientArea == 0)
    {
      LogError("Invalid client extents.");
      return nullptr;
    }
  }

  WNDCLASSA WindowClass{};
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

  RECT WindowRect{};
  WindowRect.right = Args->ClientExtents.Width;
  WindowRect.bottom = Args->ClientExtents.Height;
  AdjustWindowRectEx(&WindowRect, WindowStyle, FALSE, WindowStyleEx);

  WindowRect.right -= WindowRect.left;
  WindowRect.bottom -= WindowRect.top;

  // Apply user translation
  WindowRect.left = Args->HasCustomPosition ? Cast<LONG>(Args->WindowX) : 0;
  WindowRect.right += WindowRect.left;
  WindowRect.top = Args->HasCustomPosition ? Cast<LONG>(Args->WindowY) : 0;
  WindowRect.bottom += WindowRect.top;


  RECT WindowWorkArea{};
  SystemParametersInfoW(SPI_GETWORKAREA, 0, &WindowWorkArea, 0);
  WindowRect.left   += WindowWorkArea.left;
  WindowRect.right  += WindowWorkArea.left;
  WindowRect.top    += WindowWorkArea.top;
  WindowRect.bottom += WindowWorkArea.top;

  HWND WindowHandle;
  auto const WindowX = Args->HasCustomPosition ? WindowRect.left : CW_USEDEFAULT;
  auto const WindowY = Args->HasCustomPosition ? WindowRect.top : CW_USEDEFAULT;
  auto const WindowWidth = WindowRect.right - WindowRect.left;
  auto const WindowHeight = WindowRect.bottom - WindowRect.top;
  WindowHandle = CreateWindowExA(WindowStyleEx,             // _In_     DWORD     dwExStyle
                                 WindowClass.lpszClassName, // _In_opt_ LPCWSTR   lpClassName
                                 "Vulkan Experiments",      // _In_opt_ LPCWSTR   lpWindowName
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

  auto Window = New<window>(Allocator);
  Window->WindowHandle = WindowHandle;
  Window->ClientExtents = Args->ClientExtents;

  SetWindowLongPtr(Window->WindowHandle, GWLP_USERDATA, Reinterpret<LONG_PTR>(Window));

  return Window;
}

static void
Win32DestroyWindow(allocator_interface& Allocator, window* Window)
{
  // TODO: Close the window somehow?
  Deallocate(Allocator, Window);
}

enum class vulkan_enable_validation : bool { No = false, Yes = true };

static bool
VulkanCreateInstance(vulkan& Vulkan, vulkan_enable_validation EnableValidation)
{
  Assert(Vulkan.DLL);
  Assert(Vulkan.vkCreateInstance);
  Assert(Vulkan.vkEnumerateInstanceLayerProperties);
  Assert(Vulkan.vkEnumerateInstanceExtensionProperties);

  //
  // Instance Layers
  //
  array<char const*> LayerNames;
  {
    array<char const*> DesiredLayerNames;

    if(EnableValidation == vulkan_enable_validation::Yes)
    {
      DesiredLayerNames += "VK_LAYER_LUNARG_standard_validation";
    }

    uint32 LayerCount;
    VulkanVerify(Vulkan.vkEnumerateInstanceLayerProperties(&LayerCount, nullptr));

    array<VkLayerProperties> LayerProperties;
    ExpandBy(LayerProperties, LayerCount);
    VulkanVerify(Vulkan.vkEnumerateInstanceLayerProperties(&LayerCount, LayerProperties.Ptr));

    LogBeginScope("Explicitly enabled instance layers:");
    Defer [](){ LogEndScope("=========="); };
    {
      // Check for its existance.

    }
    for(auto& Property : Slice(LayerProperties))
    {
      auto LayerName = SliceFromString(Property.layerName);
      bool WasAdded = false;

      for(auto DesiredLayerName : Slice(DesiredLayerNames))
      {
        if(LayerName == SliceFromString(DesiredLayerName))
        {
          LayerNames += DesiredLayerName;
          WasAdded = true;
          break;
        }
      }

      if(WasAdded) LogInfo("[x] %s", LayerName.Ptr);
      else         LogInfo("[ ] %s", LayerName.Ptr);
    }
  }

  //
  // Instance Extensions
  //
  array<char const*> ExtensionNames;
  {
    // Required extensions:
    bool SurfaceExtensionFound = false;
    bool PlatformSurfaceExtensionFound = false;

    uint32 ExtensionCount;
    VulkanVerify(Vulkan.vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    array<VkExtensionProperties> ExtensionProperties;
    ExpandBy(ExtensionProperties, ExtensionCount);
    VulkanVerify(Vulkan.vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, ExtensionProperties.Ptr));

    LogBeginScope("Explicitly enabled instance extensions:");
    Defer [](){ LogEndScope("=========="); };

    for(auto& Property : Slice(ExtensionProperties))
    {
      auto ExtensionName = SliceFromString(Property.extensionName);
      if(ExtensionName == SliceFromString(VK_KHR_SURFACE_EXTENSION_NAME))
      {
        ExtensionNames += VK_KHR_SURFACE_EXTENSION_NAME;
        SurfaceExtensionFound = true;
      }
      else if(ExtensionName == SliceFromString(VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
      {
        ExtensionNames += VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
        PlatformSurfaceExtensionFound = true;
      }
      else if(ExtensionName == SliceFromString(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
      {
        ExtensionNames += VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
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
    Defer [](){ LogEndScope("=========="); };

    VkApplicationInfo ApplicationInfo{};
    {
      ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      ApplicationInfo.pApplicationName = "Vulkan Experiments";
      ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);

      ApplicationInfo.pEngineName = "Backbone";
      ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);

      ApplicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 13);
    }

    VkInstanceCreateInfo CreateInfo{};
    {
      CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

      CreateInfo.pApplicationInfo = &ApplicationInfo;

      CreateInfo.enabledExtensionCount = Cast<uint32>(ExtensionNames.Num);
      CreateInfo.ppEnabledExtensionNames = ExtensionNames.Ptr;

      CreateInfo.enabledLayerCount = Cast<uint32>(LayerNames.Num);
      CreateInfo.ppEnabledLayerNames = LayerNames.Ptr;
    }

    if(Vulkan.vkCreateInstance(&CreateInfo, nullptr, &Vulkan.InstanceHandle) != VK_SUCCESS)
    {
      LogError("Unable to create Vulkan instance.");
      return false;
    }
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
VulkanSetupDebugging(vulkan& Vulkan)
{
  LogBeginScope("Setting up Vulkan debugging.");
  Defer [](){ LogEndScope("Finished debug setup."); };

  if(Vulkan.vkCreateDebugReportCallbackEXT != nullptr)
  {
    VkDebugReportCallbackCreateInfoEXT DebugSetupInfo{};
    DebugSetupInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    DebugSetupInfo.pfnCallback = &VulkanDebugCallback;
    DebugSetupInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

    VulkanVerify(Vulkan.vkCreateDebugReportCallbackEXT(Vulkan.InstanceHandle,
                                                       &DebugSetupInfo,
                                                       nullptr,
                                                       &Vulkan.DebugCallbackHandle));
    return true;
  }
  else
  {
    LogWarning("Unable to set up debugging: vkCreateDebugReportCallbackEXT is nullptr");
    return false;
  }
}

static void
VulkanCleanupDebugging(vulkan& Vulkan)
{
  if(Vulkan.vkDestroyDebugReportCallbackEXT && Vulkan.DebugCallbackHandle)
  {
    Vulkan.vkDestroyDebugReportCallbackEXT(Vulkan.InstanceHandle, Vulkan.DebugCallbackHandle, nullptr);
  }
}

static bool
VulkanChooseAndSetupPhysicalDevices(vulkan& Vulkan)
{
  uint32 GpuCount;
  VulkanVerify(Vulkan.vkEnumeratePhysicalDevices(Vulkan.InstanceHandle,
                                            &GpuCount, nullptr));
  if(GpuCount == 0)
  {
    LogError("No GPUs found.");
    return false;
  }

  LogInfo("Found %u physical device(s).", GpuCount);

  array<VkPhysicalDevice> Gpus;
  ExpandBy(Gpus, GpuCount);

  VulkanVerify(Vulkan.vkEnumeratePhysicalDevices(Vulkan.InstanceHandle,
                                            &GpuCount, Gpus.Ptr));

  // Use the first Physical Device for now.
  uint32 const GpuIndex = 0;
  Vulkan.Gpu.GpuHandle = Gpus[GpuIndex];

  //
  // Properties
  //
  {
    LogBeginScope("Querying for physical device and queue properties.");
    Defer [](){ LogEndScope("Retrieved physical device and queue properties."); };

    Vulkan.vkGetPhysicalDeviceProperties(Vulkan.Gpu.GpuHandle, &Vulkan.Gpu.Properties);
    Vulkan.vkGetPhysicalDeviceMemoryProperties(Vulkan.Gpu.GpuHandle, &Vulkan.Gpu.MemoryProperties);
    Vulkan.vkGetPhysicalDeviceFeatures(Vulkan.Gpu.GpuHandle, &Vulkan.Gpu.Features);

    uint32 QueueCount;
    Vulkan.vkGetPhysicalDeviceQueueFamilyProperties(Vulkan.Gpu.GpuHandle, &QueueCount, nullptr);
    SetNum(Vulkan.Gpu.QueueProperties, QueueCount);
    Vulkan.vkGetPhysicalDeviceQueueFamilyProperties(Vulkan.Gpu.GpuHandle, &QueueCount, Vulkan.Gpu.QueueProperties.Ptr);
  }

  return true;
}

static bool
VulkanInitializeGraphics(vulkan& Vulkan)
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
    array<char const*> ExtensionNames;
    {
      // Required extensions:
      bool SwapchainExtensionFound{};

      uint ExtensionCount;
      VulkanVerify(Vulkan.vkEnumerateDeviceExtensionProperties(Vulkan.Gpu.GpuHandle, nullptr, &ExtensionCount, nullptr));

      array<VkExtensionProperties> ExtensionProperties;
      ExpandBy(ExtensionProperties, ExtensionCount);
      VulkanVerify(Vulkan.vkEnumerateDeviceExtensionProperties(Vulkan.Gpu.GpuHandle, nullptr, &ExtensionCount, ExtensionProperties.Ptr));

      LogBeginScope("Explicitly enabled device extensions:");
      Defer [](){ LogEndScope("=========="); };
      for(auto& Property : Slice(ExtensionProperties))
      {
        auto ExtensionName = Property.extensionName;
        if(SliceFromString(ExtensionName) == SliceFromString(VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        {
          ExtensionNames += VK_KHR_SWAPCHAIN_EXTENSION_NAME;
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
      QueueCreateInfo.queueFamilyIndex = Vulkan.Surface.PresentNode.Index;
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

    VulkanVerify(Vulkan.vkCreateDevice(Vulkan.Gpu.GpuHandle, &DeviceCreateInfo, nullptr, &Vulkan.Device.DeviceHandle));
    Assert(Vulkan.Device.DeviceHandle);

    VulkanLoadDeviceFunctions(Vulkan, Vulkan.Device);
  }

  auto const& Device = Vulkan.Device;
  auto const DeviceHandle = Device.DeviceHandle;

  //
  // Get device Queue
  //
  {
    Device.vkGetDeviceQueue(DeviceHandle, Vulkan.Surface.PresentNode.Index, 0, &Vulkan.Queue);
    Assert(Vulkan.Queue);
  }

  //
  // Create Command Pool
  //
  {
    LogBeginScope("Creating command pool.");
    Defer [](){ LogEndScope("Finished creating command pool."); };

    auto CreateInfo = InitStruct<VkCommandPoolCreateInfo>();
    {
      CreateInfo.queueFamilyIndex = Vulkan.Surface.PresentNode.Index;
      CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    }

    VulkanVerify(Device.vkCreateCommandPool(DeviceHandle, &CreateInfo, nullptr, &Vulkan.CommandPool));
    Assert(Vulkan.CommandPool);
  }

  //
  // Create Seamphores
  //
  {
    auto CreateInfo = InitStruct<VkSemaphoreCreateInfo>();
    VulkanVerify(Device.vkCreateSemaphore(DeviceHandle, &CreateInfo, nullptr, &Vulkan.PresentCompleteSemaphore));
    VulkanVerify(Device.vkCreateSemaphore(DeviceHandle, &CreateInfo, nullptr, &Vulkan.RenderCompleteSemaphore));
  }

  // Done.
  return true;
}

static void
VulkanFinalizeGraphics(vulkan& Vulkan)
{
  auto const& Device = Vulkan.Device;
  auto const DeviceHandle = Device.DeviceHandle;

  //
  // Destroy Seamphores
  //
  Device.vkDestroySemaphore(DeviceHandle, Vulkan.PresentCompleteSemaphore, nullptr);
  Device.vkDestroySemaphore(DeviceHandle, Vulkan.RenderCompleteSemaphore, nullptr);

  //
  // Destroy Command Pool
  //
  Device.vkDestroyCommandPool(DeviceHandle, Vulkan.CommandPool, nullptr);

  //
  // Destroy Logical Device
  //
  Vulkan.vkDestroyDevice(DeviceHandle, nullptr);
}

struct vulkan_graphics_pipeline_desc
{
  VkPipelineRasterizationStateCreateInfo RasterizationState
  {
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // sType
    nullptr,                                                    // pNext
    0,                                                          // flags
    VK_FALSE,                                                   // depthClampEnable
    VK_FALSE,                                                   // rasterizerDiscardEnable
    VK_POLYGON_MODE_MAX_ENUM,                                   // ! polygonMode
    VK_CULL_MODE_NONE,                                          // cullMode
    VK_FRONT_FACE_COUNTER_CLOCKWISE,                            // frontFace
    VK_FALSE,                                                   // depthBiasEnable
    0,                                                          // depthBiasConstantFactor
    0,                                                          // depthBiasClamp
    0,                                                          // depthBiasSlopeFactor
    1,                                                          // lineWidth
  };

  VkPipelineInputAssemblyStateCreateInfo InputAssemblyState
  {
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, // sType
    nullptr,                                                     // pNext
    0,                                                           // flags
    VK_PRIMITIVE_TOPOLOGY_MAX_ENUM,                              // ! topology
    VK_FALSE,                                                    // primitiveRestartEnable
  };

  VkPipelineMultisampleStateCreateInfo MultisampleState
  {
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, // sType
    nullptr,                                                  // pNext
    0,                                                        // flags
    VK_SAMPLE_COUNT_1_BIT,                                    // rasterizationSamples
    VK_FALSE,                                                 // sampleShadingEnable
    0.0f,                                                     // minSampleShading
    nullptr,                                                  // pSampleMask
    VK_FALSE,                                                 // alphaToCoverageEnable
    VK_FALSE,                                                 // alphaToOneEnable
  };

  VkPipelineDepthStencilStateCreateInfo DepthStencilState
  {
    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, // sType
    nullptr,                                                    // pNext
    0,                                                          // flags
    VK_TRUE,                                                    // depthTestEnable
    VK_TRUE,                                                    // depthWriteEnable
    VK_COMPARE_OP_LESS_OR_EQUAL,                                // depthCompareOp
    VK_FALSE,                                                   // depthBoundsTestEnable
    VK_FALSE,                                                   // stencilTestEnable
    {                                                           // front
      VK_STENCIL_OP_KEEP,   // failOp
      VK_STENCIL_OP_KEEP,   // passOp
      VK_STENCIL_OP_KEEP,   // depthFailOp
      VK_COMPARE_OP_NEVER,  // compareOp
      0,                    // compareMask
      0,                    // writeMask
      0,                    // reference
    },
    {                                                           // back
      VK_STENCIL_OP_KEEP,   // failOp
      VK_STENCIL_OP_KEEP,   // passOp
      VK_STENCIL_OP_KEEP,   // depthFailOp
      VK_COMPARE_OP_ALWAYS, // compareOp
      0,                    // compareMask
      0,                    // writeMask
      0,                    // reference
    },
    0,                                                          // minDepthBounds
    0,                                                          // maxDepthBounds
  };
};

static VkPipeline
VulkanCreateGraphicsPipeline(vulkan& Vulkan,
                             compiled_shader& CompiledShader,
                             VkPipelineLayout PipelineLayout,
                             vulkan_graphics_pipeline_desc& Desc,
                             size_t VertexDataStride)
{
  auto const& Device = Vulkan.Device;
  auto const DeviceHandle = Device.DeviceHandle;

  temp_allocator Allocator;

  LogBeginScope("Creating graphics pipeline.");
  Defer [](){ LogEndScope("Finished creating graphics pipeline."); };

  auto const MaxNumShaderStages = (Cast<size_t>(shader_stage::Fragment) - Cast<size_t>(shader_stage::Vertex)) + 1;
  array<VkPipelineShaderStageCreateInfo> PipelineShaderStageInfos{ Allocator };
  Reserve(PipelineShaderStageInfos, MaxNumShaderStages);

  for(size_t StageIndex = 0; StageIndex < MaxNumShaderStages; ++StageIndex)
  {
    auto Stage = Cast<shader_stage>(Cast<size_t>(shader_stage::Vertex) + StageIndex);
    if(!HasShaderStage(CompiledShader, Stage))
      continue;

    auto& StageInfo = Expand(PipelineShaderStageInfos);
    StageInfo = InitStruct<VkPipelineShaderStageCreateInfo>();
    StageInfo.stage = ShaderStageToVulkan(Stage);
    StageInfo.pName = StrPtr(GetGlslShader(CompiledShader, Stage)->EntryPoint);

    auto SpirvShader = GetSpirvShader(CompiledShader, Stage);
    Assert(SpirvShader);

    auto ShaderModuleCreateInfo = InitStruct<VkShaderModuleCreateInfo>();
    ShaderModuleCreateInfo.codeSize = SliceByteSize(Slice(SpirvShader->Code)); // In bytes, regardless of the fact that decltype(*pCode) == uint.
    ShaderModuleCreateInfo.pCode = SpirvShader->Code.Ptr;

    VulkanVerify(Device.vkCreateShaderModule(DeviceHandle, &ShaderModuleCreateInfo, nullptr, &StageInfo.module));
  }

  // Note(Manu): Can safely destroy the shader modules on our side once the pipeline was created.
  Defer [&]()
  {
    for(auto& StageInfo : Slice(PipelineShaderStageInfos))
    {
      Device.vkDestroyShaderModule(DeviceHandle, StageInfo.module, nullptr);
    }
  };

  VkVertexInputBindingDescription VertexInputBinding = InitStruct<VkVertexInputBindingDescription>();
  {
    VertexInputBinding.binding = 0;
    VertexInputBinding.stride = Convert<uint32>(VertexDataStride);
    VertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  }

  array<VkVertexInputAttributeDescription> VertexInputAttributeDescs{ Allocator };
  GenerateVertexInputDescriptions(CompiledShader, VertexInputBinding, VertexInputAttributeDescs);

  auto VertexInputState = InitStruct<VkPipelineVertexInputStateCreateInfo>();
  {
    VertexInputState.vertexBindingDescriptionCount = 1;
    VertexInputState.pVertexBindingDescriptions    = &VertexInputBinding;
    VertexInputState.vertexAttributeDescriptionCount = Cast<uint32>(VertexInputAttributeDescs.Num);
    VertexInputState.pVertexAttributeDescriptions    = &VertexInputAttributeDescs[0];
  }

  auto ViewportState = InitStruct<VkPipelineViewportStateCreateInfo>();
  {
    ViewportState.viewportCount = 1;
    ViewportState.scissorCount = 1;
  }

  VkPipelineColorBlendAttachmentState ColorBlendStateAttachment = InitStruct<VkPipelineColorBlendAttachmentState>();
  {
    ColorBlendStateAttachment.colorWriteMask = 0xf;
    ColorBlendStateAttachment.blendEnable = VK_FALSE;
  }

  auto ColorBlendState = InitStruct<VkPipelineColorBlendStateCreateInfo>();
  {
    ColorBlendState.attachmentCount = 1;
    ColorBlendState.pAttachments = &ColorBlendStateAttachment;
  }

  VkDynamicState DynamicStates[2]{
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  auto DynamicState = InitStruct<VkPipelineDynamicStateCreateInfo>();
  {
    DynamicState.dynamicStateCount = (uint32_t)ArrayCount(DynamicStates);
    DynamicState.pDynamicStates = DynamicStates;
  }

  auto GraphicsPipelineCreateInfo = InitStruct<VkGraphicsPipelineCreateInfo>();
  {
    GraphicsPipelineCreateInfo.stageCount = Cast<uint32>(PipelineShaderStageInfos.Num);
    GraphicsPipelineCreateInfo.pStages = PipelineShaderStageInfos.Ptr;
    GraphicsPipelineCreateInfo.pVertexInputState = &VertexInputState;
    GraphicsPipelineCreateInfo.pInputAssemblyState = &Desc.InputAssemblyState;
    GraphicsPipelineCreateInfo.pViewportState = &ViewportState;
    GraphicsPipelineCreateInfo.pRasterizationState = &Desc.RasterizationState;
    GraphicsPipelineCreateInfo.pMultisampleState = &Desc.MultisampleState;
    GraphicsPipelineCreateInfo.pDepthStencilState = &Desc.DepthStencilState;
    GraphicsPipelineCreateInfo.pColorBlendState = &ColorBlendState;
    GraphicsPipelineCreateInfo.pDynamicState = &DynamicState;
    GraphicsPipelineCreateInfo.layout = PipelineLayout;
    GraphicsPipelineCreateInfo.renderPass = Vulkan.RenderPass;
  }

  VkPipeline Pipeline{};
  VulkanVerify(Device.vkCreateGraphicsPipelines(DeviceHandle, Vulkan.PipelineCache,
                                                1, &GraphicsPipelineCreateInfo,
                                                nullptr,
                                                &Pipeline));
  return Pipeline;
}

void
ImplVulkanPrepareRenderableFoo(vulkan& Vulkan, arc_string const& ShaderPath, vulkan_graphics_pipeline_desc PipelineDesc, size_t VertexDataStride, vulkan_renderable_foo* Foo)
{
  auto const& Device = Vulkan.Device;
  auto const DeviceHandle = Device.DeviceHandle;

  temp_allocator Allocator;

  //
  // Get compiled shader
  //
  {
    restore_global_log Restore_{ nullptr };
    Foo->Shader = GetCompiledShader(*Vulkan.ShaderManager, Slice(ShaderPath));
    Assert(Foo->Shader != nullptr);
  }

  //
  // Prepare Descriptor Set Layout
  //
  {
    LogBeginScope("Preparing descriptor set layout.");
    Defer [](){ LogEndScope("Finished preparing descriptor set layout."); };

    array<VkDescriptorSetLayoutBinding> LayoutBindings{ Allocator };

    GetDescriptorSetLayoutBindings(*Foo->Shader, LayoutBindings);

    auto DescriptorSetLayoutCreateInfo = InitStruct<VkDescriptorSetLayoutCreateInfo>();
    {
      DescriptorSetLayoutCreateInfo.bindingCount = Cast<uint32>(LayoutBindings.Num);
      DescriptorSetLayoutCreateInfo.pBindings = LayoutBindings.Ptr;
    }

    VulkanVerify(Device.vkCreateDescriptorSetLayout(DeviceHandle, &DescriptorSetLayoutCreateInfo, nullptr, &Foo->DescriptorSetLayout));

    auto PipelineLayoutCreateInfo = InitStruct<VkPipelineLayoutCreateInfo>();
    {
      PipelineLayoutCreateInfo.setLayoutCount = 1;
      PipelineLayoutCreateInfo.pSetLayouts = &Foo->DescriptorSetLayout;
    }
    VulkanVerify(Device.vkCreatePipelineLayout(DeviceHandle, &PipelineLayoutCreateInfo, nullptr, &Foo->PipelineLayout));
  }

  //
  // Create Pipeline Cache (if necessary)
  //
  if(Vulkan.PipelineCache == nullptr)
  {
    auto PipelineCacheCreateInfo = InitStruct<VkPipelineCacheCreateInfo>();
    VulkanVerify(Device.vkCreatePipelineCache(DeviceHandle, &PipelineCacheCreateInfo, nullptr, &Vulkan.PipelineCache));
  }

  //
  // Create Pipeline
  //
  {
    Foo->Pipeline = VulkanCreateGraphicsPipeline(Vulkan, *Foo->Shader, Foo->PipelineLayout, PipelineDesc, VertexDataStride);
    Assert(Foo->Pipeline);
  }

  //
  // Allocate the UBO for Globals
  //
  {
    VulkanCreateShaderBuffer(Vulkan, Foo->UboGlobals, is_read_only_for_shader::Yes);
  }
}

template<typename VertexDataType>
void
VulkanPrepareRenderableFoo(vulkan& Vulkan, arc_string const& ShaderPath, vulkan_graphics_pipeline_desc PipelineDesc, vulkan_renderable_foo* Foo)
{
  ImplVulkanPrepareRenderableFoo(Vulkan, ShaderPath, PipelineDesc, SizeOf<VertexDataType>(), Foo);
}

static bool
VulkanPrepareRenderPass(vulkan& Vulkan)
{
  // TODO: Put this somewhere else?

  auto const& Device = Vulkan.Device;
  auto const DeviceHandle = Device.DeviceHandle;

  temp_allocator Allocator;


  //
  // Prepare Render Pass
  //
  {
    LogBeginScope("Preparing render pass.");
    Defer [](){ LogEndScope("Finished preparing render pass."); };

    VkAttachmentDescription Attachments[2]{ InitStruct<VkAttachmentDescription>() };
    {
      Attachments[0].format = Vulkan.Surface.Format;
      Attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
      Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      Attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      Attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    {
      Attachments[1].format = Vulkan.Depth.Format;
      Attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
      Attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      Attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      Attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
      RenderPassCreateInfo.attachmentCount = (uint32)ArrayCount(Attachments);
      RenderPassCreateInfo.pAttachments = Attachments;
      RenderPassCreateInfo.subpassCount = 1;
      RenderPassCreateInfo.pSubpasses = &SubpassDesc;
    }

    VulkanVerify(Device.vkCreateRenderPass(DeviceHandle, &RenderPassCreateInfo, nullptr, &Vulkan.RenderPass));

    Attachments[0].format = Vulkan.RenderTarget2.ImageFormat;
    Attachments[1].format = Vulkan.RenderTarget2.Depth.Format;
    VulkanVerify(Device.vkCreateRenderPass(DeviceHandle, &RenderPassCreateInfo, nullptr, &Vulkan.RenderTarget2.RenderPass));
  }


  //
  // Prepare Foos
  //
  {
    // Debug Grids
    {
      auto PipelineDesc = InitStruct<vulkan_graphics_pipeline_desc>();
      PipelineDesc.RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
      PipelineDesc.InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
      VulkanPrepareRenderableFoo<vulkan_debug_grid::vertex>(Vulkan, DataPath("Shader/DebugGrid.shader"), PipelineDesc, &Vulkan.DebugGridsFoo);
    }

    // Scene Objects
    {
      auto PipelineDesc = InitStruct<vulkan_graphics_pipeline_desc>();
      PipelineDesc.RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
      PipelineDesc.RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
      PipelineDesc.InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      VulkanPrepareRenderableFoo<vulkan_scene_object::vertex>(Vulkan, DataPath("Shader/SceneObject.shader"), PipelineDesc, &Vulkan.SceneObjectsFoo);
    }
  }


  //
  // Create Descriptor Pool
  //
  {
    LogBeginScope("Preparing descriptor pool.");
    Defer [](){ LogEndScope("Finished preparing descriptor pool."); };

    // Get descriptor counts
    scoped_dictionary<VkDescriptorType, uint32> DescriptorCounts{ &Allocator };
    {
      restore_global_log Restore_{ nullptr };
      GetDescriptorTypeCounts(*Vulkan.DebugGridsFoo.Shader, DescriptorCounts);
      GetDescriptorTypeCounts(*Vulkan.SceneObjectsFoo.Shader, DescriptorCounts);
    }

    uint32 const MaxNumInstances = 50;

    // Now collect them in the proper structs.
    array<VkDescriptorPoolSize> PoolSizes{ Allocator };
    for(auto Key : Keys(&DescriptorCounts))
    {
      auto& Size = Expand(PoolSizes);
      Size.type = Key;
      Size.descriptorCount = MaxNumInstances * *Get(&DescriptorCounts, Key);
    }

    auto DescriptorPoolCreateInfo = InitStruct<VkDescriptorPoolCreateInfo>();
    {
      DescriptorPoolCreateInfo.maxSets = MaxNumInstances;
      DescriptorPoolCreateInfo.poolSizeCount = Cast<uint32>(PoolSizes.Num);
      DescriptorPoolCreateInfo.pPoolSizes = PoolSizes.Ptr;
    }

    VulkanVerify(Device.vkCreateDescriptorPool(DeviceHandle,
                                               &DescriptorPoolCreateInfo,
                                               nullptr,
                                               &Vulkan.DescriptorPool));
  }

  return true;
}

static void VulkanCreateFramebuffers(vulkan& Vulkan)
{
  LogBeginScope("Creating framebuffers.");
  Defer [](){ LogEndScope("Finished creating framebuffers."); };

  fixed_block<2, VkImageView> Attachments{};
  Attachments[1] = Vulkan.Depth.View;

  auto FramebufferCreateInfo = InitStruct<VkFramebufferCreateInfo>();
  {
    FramebufferCreateInfo.renderPass = Vulkan.RenderPass;
    FramebufferCreateInfo.attachmentCount = Cast<uint32>(Attachments.Num);
    FramebufferCreateInfo.pAttachments = First(Attachments);
    FramebufferCreateInfo.width = Vulkan.Swapchain.Extent.Width;
    FramebufferCreateInfo.height = Vulkan.Swapchain.Extent.Height;
    FramebufferCreateInfo.layers = 1;
  }

  SetNum(Vulkan.Framebuffers, Vulkan.Swapchain.ImageCount);

  auto const ImageCount = Vulkan.Swapchain.ImageCount;
  for(uint32 Index = 0; Index < ImageCount; ++Index)
  {
    Attachments[0] = Vulkan.Swapchain.ImageViews[Index];
    VulkanVerify(Vulkan.Device.vkCreateFramebuffer(Vulkan.Device.DeviceHandle,
                                                    &FramebufferCreateInfo,
                                                    nullptr,
                                                    &Vulkan.Framebuffers[Index]));
  }
  auto& RT2 = Vulkan.RenderTarget2;
  Attachments[1] = RT2.Depth.View;
  Attachments[0] = RT2.ImageView;
  FramebufferCreateInfo.renderPass = RT2.RenderPass;
  VulkanVerify(Vulkan.Device.vkCreateFramebuffer(Vulkan.Device.DeviceHandle,
                                                  &FramebufferCreateInfo,
                                                  nullptr,
                                                  &RT2.Framebuffer));
}

static void VulkanDestroyFramebuffers(vulkan& Vulkan)
{
  auto const ImageCount = Vulkan.Swapchain.ImageCount;
  for(uint32 Index = 0; Index < ImageCount; ++Index)
  {
    Vulkan.Device.vkDestroyFramebuffer(Vulkan.Device.DeviceHandle,
                                                     Vulkan.Framebuffers[Index],
                                                     nullptr);
  }
}

static void
VulkanCleanupRenderPass(vulkan& Vulkan)
{
  // TODO: Cleanup

  #if 0
  auto const& Device = Vulkan.Device;
  auto const DeviceHandle = Device.DeviceHandle;

  // Framebuffers
  for(auto Framebuffer : Slice(Vulkan.Framebuffers))
  {
    Device.vkDestroyFramebuffer(DeviceHandle, Framebuffer, nullptr);
  }
  Clear(Vulkan.Framebuffers);

  // UBOs
  Device.vkFreeMemory(DeviceHandle, Vulkan.SceneObjectGraphicsState.GlobalsUBO.MemoryHandle, nullptr);
  Device.vkDestroyBuffer(DeviceHandle, Vulkan.SceneObjectGraphicsState.GlobalsUBO.BufferHandle, nullptr);

  // Descriptor Sets
  Device.vkFreeDescriptorSets(DeviceHandle, Vulkan.DescriptorPool, 1, &Vulkan.SceneObjectGraphicsState.DescriptorSet);

  // Descriptor Pool
  Device.vkDestroyDescriptorPool(DeviceHandle, Vulkan.DescriptorPool, nullptr);

  // Pipeline
  Device.vkDestroyPipeline(DeviceHandle, Vulkan.SceneObjectGraphicsState.Pipeline, nullptr);

  // Pipeline Cache
  Device.vkDestroyPipelineCache(DeviceHandle, Vulkan.PipelineCache, nullptr);

  // Render Pass
  Device.vkDestroyRenderPass(DeviceHandle, Vulkan.RenderPass, nullptr);

  // Pipeline Layout
  Device.vkDestroyPipelineLayout(DeviceHandle, Vulkan.SceneObjectGraphicsState.PipelineLayout, nullptr);

  // Descriptor Set Layout
  Device.vkDestroyDescriptorSetLayout(DeviceHandle, Vulkan.SceneObjectGraphicsState.DescriptorSetLayout, nullptr);
  #endif
}

static void
VulkanCreateCommandBuffers(vulkan& Vulkan, VkCommandPool CommandPool, uint32 Num)
{
  LogBeginScope("Creating command buffers.");
  Defer [](){ LogEndScope("Finished creating command buffers."); };

  auto const& Device = Vulkan.Device;
  auto const DeviceHandle = Device.DeviceHandle;

  auto AllocateInfo = InitStruct<VkCommandBufferAllocateInfo>();
  {
    AllocateInfo.commandPool = Vulkan.CommandPool;
    AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocateInfo.commandBufferCount = Num;
  }

  SetNum(Vulkan.DrawCommandBuffers, Num);

  VulkanVerify(Device.vkAllocateCommandBuffers(DeviceHandle, &AllocateInfo, Vulkan.DrawCommandBuffers.Ptr));

  AllocateInfo.commandBufferCount = 1;
  VulkanVerify(Device.vkAllocateCommandBuffers(DeviceHandle, &AllocateInfo, &Vulkan.RenderTarget2.DrawCommandBuffer));
}

static void VulkanDestroyCommandBuffers(vulkan& Vulkan, VkCommandPool CommandPool)
{
  auto const& Device = Vulkan.Device;
  auto const DeviceHandle = Device.DeviceHandle;

  Device.vkFreeCommandBuffers(DeviceHandle, CommandPool, Cast<uint32>(Vulkan.DrawCommandBuffers.Num), Vulkan.DrawCommandBuffers.Ptr);

  Clear(Vulkan.DrawCommandBuffers);
}

// TODO
#if 0
static void
VulkanDestroySwapchain(vulkan& Vulkan)
{
  Assert(Vulkan.IsPrepared);

  auto const& Device = Vulkan.Device;
  auto const DeviceHandle = Device.DeviceHandle;

  Vulkan.IsPrepared = false;

  Device.vkDestroyDescriptorPool(DeviceHandle, Vulkan.DescriptorPool, nullptr);

  if(Vulkan.SetupCommand)
  {
    Device.vkFreeCommandBuffers(DeviceHandle, Vulkan.CommandPool, 1, &Vulkan.SetupCommand);
  }
  Device.vkFreeCommandBuffers(DeviceHandle, Vulkan.CommandPool, 1, &Vulkan.DrawCommand);
  Device.vkDestroyCommandPool(DeviceHandle, Vulkan.CommandPool, nullptr);

  Device.vkDestroyPipeline(DeviceHandle, Vulkan.Pipeline, nullptr);
  Device.vkDestroyRenderPass(DeviceHandle, Vulkan.RenderPass, nullptr);
  Device.vkDestroyPipelineLayout(DeviceHandle, Vulkan.PipelineLayout, nullptr);
  Device.vkDestroyDescriptorSetLayout(DeviceHandle, Vulkan.DescriptorSetLayout, nullptr);

  for(auto& SceneObject : Slice(Vulkan.SceneObjects))
  {
    VulkanDestroyAndDeallocateSceneObject(Vulkan, &SceneObject);
  }

  Device.vkDestroyImageView(DeviceHandle, Vulkan.Depth.View, nullptr);
  Device.vkDestroyImage(DeviceHandle,     Vulkan.Depth.Image, nullptr);
  Device.vkFreeMemory(DeviceHandle,       Vulkan.Depth.Memory, nullptr);
}
#endif

#if 0
static void
VulkanCleanup(vulkan& Vulkan)
{
  // TODO

  LogBeginScope("Vulkan cleanup.");
  Defer [](){ LogEndScope("Finished Vulkan cleanup."); };

  if(Vulkan.IsPrepared)
  {
    VulkanDestroySwapchain(Vulkan);
  }

  Vulkan.Device.vkDestroyDevice(Vulkan.Device.DeviceHandle, nullptr);

  Vulkan.vkDestroySurfaceKHR(Vulkan.InstanceHandle, Vulkan.Surface.SurfaceHandle, nullptr);
  Vulkan.vkDestroyInstance(Vulkan.InstanceHandle, nullptr);

  Clear(Vulkan.Gpu.QueueProperties);
}
#endif

static void
VulkanDestroyInstance(vulkan& Vulkan)
{
  Vulkan.vkDestroyInstance(Vulkan.InstanceHandle, nullptr);
}

static void
VulkanResize(vulkan& Vulkan, extent2_<uint32> NewExtent)
{
  LogError("Resizing is not implemented yet!");
  return;

  #if 0

  // Don't react to resize until after first initialization.
  if(!Vulkan.IsPrepared)
    return;

  LogInfo("Resizing to %ux%u.", NewExtent.Width, NewExtent.Height);

  // In order to properly resize the window, we must re-create the swapchain
  // AND redo the command buffers, etc.

  //
  // Perform some cleanup
  //
  VulkanCleanupDepth(*Vulkan, &Vulkan.Depth);
  VulkanDestroyCommandBuffers(Vulkan, Vulkan.CommandPool);
  VulkanDestroyFramebuffers(Vulkan);

  //
  // Re-create the swapchain
  //
  VulkanPrepareSetupCommandBuffer(Vulkan);
  VulkanPrepareSwapchain(*Vulkan, &Vulkan.Swapchain, NewExtent, Vulkan.Swapchain.VSync);
  VulkanPrepareDepth(Vulkan, &Vulkan.Depth, Vulkan.Swapchain.Extent);
  VulkanCreateFramebuffers(Vulkan);
  VulkanCreateCommandBuffers(Vulkan, Vulkan.CommandPool, Vulkan.Swapchain.ImageCount);
  VulkanCleanupSetupCommandBuffer(Vulkan, flush_command_buffer::Yes);

  #endif
}

static void
VulkanBuildDrawCommands(vulkan&                Vulkan,
                        slice<VkCommandBuffer> DrawCommandBuffers,
                        slice<VkFramebuffer>   Framebuffers,
                        color_linear const&    ClearColor,
                        float                  ClearDepth,
                        uint32                 ClearStencil)
{
  BoundsCheck(DrawCommandBuffers.Num == Framebuffers.Num);

  auto RecordingHelper = [](vulkan& Vulkan,
                            VkRenderPass RenderPass,
                            VkFramebuffer Framebuffer,
                            VkCommandBuffer DrawCommandBuffer,
                            slice<VkClearValue> ClearValues)
  {
    auto const& Device = Vulkan.Device;

    // Set viewport
    auto Viewport = InitStruct<VkViewport>();
    {
      Viewport.height = Cast<float>(Vulkan.Swapchain.Extent.Height);
      Viewport.width = Cast<float>(Vulkan.Swapchain.Extent.Width);
      Viewport.minDepth = 0.0f;
      Viewport.maxDepth = 1.0f;
    }
    Device.vkCmdSetViewport(DrawCommandBuffer, 0, 1, &Viewport);

    // Set scissor
    auto Scissor = InitStruct<VkRect2D>();
    {
      Scissor.extent.width = Vulkan.Swapchain.Extent.Width;
      Scissor.extent.height = Vulkan.Swapchain.Extent.Height;
    }
    Device.vkCmdSetScissor(DrawCommandBuffer, 0, 1, &Scissor);

    auto RenderPassBeginInfo = InitStruct<VkRenderPassBeginInfo>();
    {
      RenderPassBeginInfo.renderPass = RenderPass;
      RenderPassBeginInfo.framebuffer = Framebuffer;
      RenderPassBeginInfo.renderArea = Scissor;
      RenderPassBeginInfo.clearValueCount = Cast<uint32>(ClearValues.Num);
      RenderPassBeginInfo.pClearValues = First(ClearValues);
    }
    Device.vkCmdBeginRenderPass(DrawCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Draw scene objects
    for(auto Renderable : Slice(Vulkan.Renderables))
    {
      VulkanEnsureIsReadyForDrawing(Vulkan, *Renderable);

      auto Shader = Renderable->Foo->Shader;
      if(Shader == nullptr)
      {
        LogError("No shader for renderable: %s", StrPtr(Renderable->Name));
      }

      Device.vkCmdBindDescriptorSets(DrawCommandBuffer,
                                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                                     Renderable->Foo->PipelineLayout,
                                     0, // Descriptor set offset
                                     1, &Renderable->DescriptorSet,
                                     0, nullptr); // Dynamic offsets
      Device.vkCmdBindPipeline(DrawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Renderable->Foo->Pipeline);
      Renderable->Draw(Vulkan, DrawCommandBuffer);
    }
    Device.vkCmdEndRenderPass(DrawCommandBuffer);
  };

  VkClearValue ClearValues_[2];

  // Regular clear color.
  SliceCopy(Slice(ClearValues_[0].color.float32),
            Slice(ClearColor.Data));

  // DepthStencil clear values.
  ClearValues_[1].depthStencil.depth = ClearDepth;
  ClearValues_[1].depthStencil.stencil = ClearStencil;

  slice<VkClearValue> ClearValues = Slice(ClearValues_);

  auto const Num = DrawCommandBuffers.Num;
  for(size_t Index = 0; Index < Num; ++Index)
  {
    auto BeginCommandBufferInfo = InitStruct<VkCommandBufferBeginInfo>();

    auto& RT2 = Vulkan.RenderTarget2;
    VulkanVerify(Vulkan.Device.vkBeginCommandBuffer(RT2.DrawCommandBuffer, &BeginCommandBufferInfo));
    {
      RecordingHelper(Vulkan, RT2.RenderPass, RT2.Framebuffer, RT2.DrawCommandBuffer, ClearValues);
    }
    VulkanVerify(Vulkan.Device.vkEndCommandBuffer(RT2.DrawCommandBuffer));

    VulkanVerify(Vulkan.Device.vkBeginCommandBuffer(DrawCommandBuffers[Index], &BeginCommandBufferInfo));
    {
      RecordingHelper(Vulkan, Vulkan.RenderPass, Framebuffers[Index], DrawCommandBuffers[Index], ClearValues);
    }
    VulkanVerify(Vulkan.Device.vkEndCommandBuffer(DrawCommandBuffers[Index]));
  }
}

static void
VulkanDraw(vulkan& Vulkan)
{
  auto const& Device = Vulkan.Device;
  auto const DeviceHandle = Device.DeviceHandle;

  VkFence NullFence{};
  VkResult Error{};


  //
  // Get next swapchain image.
  //
  {
    Error = VulkanAcquireNextSwapchainImage(Vulkan.Swapchain,
                                            Vulkan.PresentCompleteSemaphore,
                                            Vulkan.CurrentSwapchainImage);

    VulkanVerify(Error);
    #if 0
    switch(Error)
    {
      case VK_ERROR_OUT_OF_DATE_KHR:
      {
        // Swapchain is out of date (e.g. the window was resized) and must be
        // recreated:
        VulkanResize(Vulkan, Vulkan.Swapchain.Extent);
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
  // Submit draw commands.
  //
  {
    VkPipelineStageFlags const SubmitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkCommandBuffer DrawCommandBuffers[2];
    DrawCommandBuffers[0] = Vulkan.RenderTarget2.DrawCommandBuffer;
    DrawCommandBuffers[1] = Vulkan.DrawCommandBuffers[Vulkan.CurrentSwapchainImage.Index];

    auto SubmitInfo = InitStruct<VkSubmitInfo>();
    SubmitInfo.pWaitDstStageMask = &SubmitPipelineStages;
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores = &Vulkan.PresentCompleteSemaphore;
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores = &Vulkan.RenderCompleteSemaphore;
    SubmitInfo.commandBufferCount = (uint32)ArrayCount(DrawCommandBuffers);
    SubmitInfo.pCommandBuffers = DrawCommandBuffers;
    VulkanVerify(Device.vkQueueSubmit(Vulkan.Queue, 1, &SubmitInfo, NullFence));
  }

  //
  // Present
  //
  {
    Error = VulkanQueuePresent(Vulkan.Swapchain,
                               Vulkan.Queue,
                               Vulkan.CurrentSwapchainImage,
                               Vulkan.RenderCompleteSemaphore);

    // TODO: See if we need to handle the cases in the switch below.
    VulkanVerify(Error);

    #if 0
    switch(Error)
    {
      case VK_ERROR_OUT_OF_DATE_KHR:
      {
        // Swapchain is out of date (e.g. the window was resized) and must be
        // recreated:
        VulkanResize(Vulkan, Vulkan.Swapchain.Extent);
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
  Device.vkQueueWaitIdle(Vulkan.Queue);
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

struct my_input_slots
{
  input_slot Quit;
  input_slot Depth;

  input_slot CamMoveForward;
  input_slot CamMoveRight;
  input_slot CamMoveUp;
  input_slot CamRelYaw;
  input_slot CamRelPitch;
  input_slot CamAbsYaw;
  input_slot CamAbsPitch;
  input_slot CamReset;
};

void
PrepareInputSlots(input_context         Context,
                  my_input_slots*       MyInputSlots,
                  input_x_input_slots*  XInput,
                  input_mouse_slots*    Mouse,
                  input_keyboard_slots* Keyboard)
{
  // Let's pretend the system is user 0 for now.
  InputSetUserIndex(Context, 0);

  Win32RegisterXInputSlots(Context, XInput, GlobalLog);
  Win32RegisterMouseSlots(Context, Mouse, GlobalLog);
  Win32RegisterKeyboardSlots(Context, Keyboard, GlobalLog);

  //
  // Input Mappings
  //
  MyInputSlots->Quit = InputRegisterSlot(Context, input_type::Button, "Quit");
  InputAddSlotMapping(Keyboard->Escape, MyInputSlots->Quit);
  InputAddSlotMapping(XInput->Start, MyInputSlots->Quit);

  MyInputSlots->Depth = InputRegisterSlot(Context, input_type::Axis, "Depth");
  InputAddSlotMapping(Keyboard->Up, MyInputSlots->Depth, -1);
  InputAddSlotMapping(Keyboard->Down, MyInputSlots->Depth, 1);
  InputSetSensitivity(MyInputSlots->Depth, 0.005f);

  MyInputSlots->CamMoveForward = InputRegisterSlot(Context, input_type::Axis, "CamMoveForward");
  InputAddSlotMapping(XInput->YLeftStick, MyInputSlots->CamMoveForward);
  InputAddSlotMapping(Keyboard->W, MyInputSlots->CamMoveForward,  1);
  InputAddSlotMapping(Keyboard->S, MyInputSlots->CamMoveForward, -1);

  MyInputSlots->CamMoveRight = InputRegisterSlot(Context, input_type::Axis, "CamMoveRight");
  InputAddSlotMapping(XInput->XLeftStick, MyInputSlots->CamMoveRight);
  InputAddSlotMapping(Keyboard->D, MyInputSlots->CamMoveRight,  1);
  InputAddSlotMapping(Keyboard->A, MyInputSlots->CamMoveRight, -1);

  MyInputSlots->CamMoveUp = InputRegisterSlot(Context, input_type::Axis, "CamMoveUp");
  InputAddSlotMapping(XInput->LeftTrigger,  MyInputSlots->CamMoveUp,  1);
  InputAddSlotMapping(XInput->RightTrigger, MyInputSlots->CamMoveUp, -1);
  InputAddSlotMapping(Keyboard->E, MyInputSlots->CamMoveUp,  1);
  InputAddSlotMapping(Keyboard->Q, MyInputSlots->CamMoveUp, -1);
  InputSetExponent(MyInputSlots->CamMoveUp, 3.0f);

  MyInputSlots->CamRelYaw = InputRegisterSlot(Context, input_type::Axis, "CamRelYaw");
  InputAddSlotMapping(XInput->XRightStick, MyInputSlots->CamRelYaw);
  InputAddSlotMapping(Keyboard->Left,  MyInputSlots->CamRelYaw, -1);
  InputAddSlotMapping(Keyboard->Right, MyInputSlots->CamRelYaw, 1);

  MyInputSlots->CamRelPitch = InputRegisterSlot(Context, input_type::Axis, "CamRelPitch");
  InputAddSlotMapping(XInput->YRightStick, MyInputSlots->CamRelPitch, -1);
  InputAddSlotMapping(Keyboard->Up,   MyInputSlots->CamRelPitch, -1);
  InputAddSlotMapping(Keyboard->Down, MyInputSlots->CamRelPitch, 1);

  float const CameraMouseSensitivity = 0.01f;

  MyInputSlots->CamAbsYaw = InputRegisterSlot(Context, input_type::Action, "CamAbsYaw");
  InputAddSlotMapping(Mouse->XDelta, MyInputSlots->CamAbsYaw);
  InputSetSensitivity(MyInputSlots->CamAbsYaw, CameraMouseSensitivity);

  MyInputSlots->CamAbsPitch = InputRegisterSlot(Context, input_type::Action, "CamAbsPitch");
  InputAddSlotMapping(Mouse->YDelta, MyInputSlots->CamAbsPitch);
  InputSetSensitivity(MyInputSlots->CamAbsPitch, CameraMouseSensitivity);

  MyInputSlots->CamReset = InputRegisterSlot(Context, input_type::Button, "CamReset");
  InputAddSlotMapping(Keyboard->R, MyInputSlots->CamReset);
}

enum class exit_code : int
{
  Success = 0,

  NoVulkanDll = 1,
  FailedCreatingVulkanInstance = 2,
  NoVulkanGpu = 3,
  FailedCreatingWindow = 4,
  FailedPreparingVulkanSurface = 5,
  FailedInitializingVulkanForGraphics = 6,
  FailedPreparingVulkanSwapchain = 7,
  FailedPreparingVulkanDepth = 8,
};

static exit_code
ApplicationEntryPoint(HINSTANCE ProcessHandle)
{
  //
  // Settings
  //
  // Whether to show the current FPS in the window title text.
  bool ShowFpsInWindowTitle = false;
  DebugCode(ShowFpsInWindowTitle = true);

  // Whether to enable vsync.
  vsync VSync = vsync::On;

  // Whether to enable Vulkan validation layers or not.
  // Note that validation drains quite some performance.
  vulkan_enable_validation VulkanValidation = vulkan_enable_validation::Yes;


  //
  // =============
  //
  int CurrentExitCode = 0;

  bool HasConsole{};
  Win32SetupConsole("Vulkan Experiments Console");
  RECT InitialConsoleRect{};
  HasConsole = !!GetWindowRect(GetConsoleWindow(), &InitialConsoleRect);

  mallocator Mallocator{};
  allocator_interface& Allocator = Mallocator;
  allocator_interface* AllocatorPtr = &Allocator;

  log_data Log{};
  {
    auto SinkSlots = ExpandBy(Log.Sinks, 2);
    SinkSlots[0] = GetStdoutLogSink(stdout_log_sink_enable_prefixes::Yes);
    SinkSlots[1] = log_sink(VisualStudioLogSink);
  }

  GlobalLog = &Log;
  Defer [=](){ GlobalLog = nullptr; };

  image_loader_registry* ImageLoaderRegistry = CreateImageLoaderRegistry(Allocator);
  Defer [&](){ DestroyImageLoaderRegistry(Allocator, ImageLoaderRegistry); };

  {
    auto ImageLoaderModule = RegisterImageLoaderModule(*ImageLoaderRegistry,
                                                       "DDS"_S,
                                                       "Core.dll"_S,
                                                       "CreateImageLoader_DDS"_S,
                                                       "DestroyImageLoader_DDS"_S);
    if(ImageLoaderModule)
      AssociateImageLoaderModuleWithFileExtension(*ImageLoaderModule, ".dds"_S);
  }

  {
    auto VulkanPtr = New<vulkan>(Allocator);
    Defer [&](){ Delete(Allocator, VulkanPtr); };
    auto& Vulkan = *VulkanPtr;

    Init(Vulkan, Allocator);
    Defer [=](){ Finalize(*VulkanPtr); };

    if(!VulkanLoadDLL(Vulkan))
      return exit_code::NoVulkanDll;

    if(!VulkanCreateInstance(Vulkan, VulkanValidation))
      return exit_code::FailedCreatingVulkanInstance;
    Defer [=](){ VulkanDestroyInstance(*VulkanPtr); };

    VulkanLoadInstanceFunctions(Vulkan);

    VulkanSetupDebugging(Vulkan);
    Defer [=](){ VulkanCleanupDebugging(*VulkanPtr); };

    if(!VulkanChooseAndSetupPhysicalDevices(Vulkan))
      return exit_code::NoVulkanGpu;


    //
    // Create a Win32 window
    //
    window_setup WindowSetup{};
    WindowSetup.ProcessHandle = ProcessHandle;
    WindowSetup.WindowClassName = "VulkanExperimentsWindowClass";
    WindowSetup.HasCustomPosition = HasConsole;
    WindowSetup.WindowX = InitialConsoleRect.right - InitialConsoleRect.left;
    WindowSetup.WindowY = InitialConsoleRect.top + 50;
    WindowSetup.ClientExtents.Width = 1280;
    WindowSetup.ClientExtents.Height = 720;
    auto Window = Win32CreateWindow(Allocator, &WindowSetup);

    if(Window == nullptr)
      return exit_code::FailedCreatingWindow;

    Defer [&](){ Win32DestroyWindow(Allocator, Window); };


    //
    // Surface
    //
    {
      LogBeginScope("Preparing OS surface.");

      if(!VulkanPrepareSurface(Vulkan, Vulkan.Surface, ProcessHandle, Window->WindowHandle))
      {
        LogEndScope("Surface creation failed.");
        return exit_code::FailedPreparingVulkanSurface;
      }

      LogEndScope("OS surface successfully created.");
    }
    Defer [VulkanPtr](){ VulkanCleanupSurface(*VulkanPtr, VulkanPtr->Surface); };


    //
    // Device creation
    //
    {
      LogBeginScope("Initializing Vulkan for graphics.");

      if(!VulkanInitializeGraphics(Vulkan))
      {
        LogEndScope("Vulkan initialization failed.");
        return exit_code::FailedInitializingVulkanForGraphics;
      }

      LogEndScope("Vulkan successfully initialized.");
    }
    Defer [VulkanPtr](){ VulkanFinalizeGraphics(*VulkanPtr); };

    VulkanSwapchainConnect(Vulkan.Swapchain, Vulkan.Device, Vulkan.Surface);

    // Now that we have a command pool, we can create the setup command
    VulkanPrepareSetupCommandBuffer(Vulkan);

    //
    // Swapchain
    //
    {
      LogBeginScope("Preparing swapchain for the first duration.");

      if(!VulkanPrepareSwapchain(Vulkan, Vulkan.Swapchain, WindowSetup.ClientExtents, VSync))
      {
        LogEndScope("Failed to prepare initial swapchain.");
        return exit_code::FailedPreparingVulkanSwapchain;
      }

      Vulkan.IsPrepared = true;
      LogEndScope("Initial Swapchain is prepared.");
    }
    Defer [VulkanPtr](){ VulkanCleanupSwapchain(*VulkanPtr, VulkanPtr->Swapchain); };

    //
    // RenderTarget2 image setup.
    //
    {
      Vulkan.RenderTarget2.ImageFormat = VK_FORMAT_R8G8B8A8_UNORM; // TODO: Check for best format here.

      auto const& Device = Vulkan.Device;
      auto const DeviceHandle = Device.DeviceHandle;
      auto& RT2 = Vulkan.RenderTarget2;

      // Create the image.
      auto ImageCreateInfo = InitStruct<VkImageCreateInfo>();
      {
        ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        ImageCreateInfo.format = RT2.ImageFormat;
        ImageCreateInfo.extent = { Vulkan.Swapchain.Extent.Width, Vulkan.Swapchain.Extent.Height, 1 };
        ImageCreateInfo.mipLevels = 1;
        ImageCreateInfo.arrayLayers = 1;
        ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        ImageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      }
      VulkanVerify(Device.vkCreateImage(DeviceHandle, &ImageCreateInfo, nullptr, &RT2.Image));

      // Allocate memory for the image.
      VkMemoryRequirements MemoryRequirements;
      Device.vkGetImageMemoryRequirements(DeviceHandle, RT2.Image, &MemoryRequirements);

      auto MemoryAllocateInfo = InitStruct<VkMemoryAllocateInfo>();
      MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
      MemoryAllocateInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan.Gpu.MemoryProperties,
                                                                          MemoryRequirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      VulkanVerify(Device.vkAllocateMemory(DeviceHandle,
                                           &MemoryAllocateInfo,
                                           nullptr,
                                           &RT2.ImageMemory));

      // Bind the image to the allocated memory.
      VulkanVerify(Device.vkBindImageMemory(DeviceHandle,
                                            RT2.Image,
                                            RT2.ImageMemory,
                                            0)); // Offset

      // Create a view to that image.
      auto ImageViewCreateInfo = InitStruct<VkImageViewCreateInfo>();
      {
        ImageViewCreateInfo.image = RT2.Image;
        ImageViewCreateInfo.format = RT2.ImageFormat;
        ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ImageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
      }
      VulkanVerify(Device.vkCreateImageView(DeviceHandle, &ImageViewCreateInfo, nullptr, &RT2.ImageView));

      VulkanPrepareDepth(Vulkan, RT2.Depth, Vulkan.Swapchain.Extent);
    }


    //
    // Create Command Buffers
    //
    {
      VulkanCreateCommandBuffers(Vulkan, Vulkan.CommandPool, Vulkan.Swapchain.ImageCount);
    }
    Defer [VulkanPtr](){ VulkanDestroyCommandBuffers(*VulkanPtr, VulkanPtr->CommandPool); };


    //
    // Depth
    //
    {
      LogBeginScope("Preparing depth.");

      if(!VulkanPrepareDepth(Vulkan, Vulkan.Depth, Vulkan.Swapchain.Extent))
      {
        LogEndScope("Failed preparing depth.");
        return exit_code::FailedPreparingVulkanDepth;
      }

      LogEndScope("Finished preparing depth.");
    }
    Defer [VulkanPtr](){ VulkanCleanupDepth(*VulkanPtr, VulkanPtr->Depth); };


    //
    // Render Pass
    //
    {
      VulkanPrepareRenderPass(Vulkan);
    }
    Defer [VulkanPtr](){ VulkanCleanupRenderPass(*VulkanPtr); };

    //
    // Create framebuffers
    //
    {
      VulkanCreateFramebuffers(Vulkan);
    }
    Defer [VulkanPtr](){ VulkanDestroyFramebuffers(*VulkanPtr); };

    // Flush the setup command once now to finalize initialization but prepare it for further use also.
    VulkanCleanupSetupCommandBuffer(Vulkan, flush_command_buffer::Yes);
    VulkanPrepareSetupCommandBuffer(Vulkan);
    Defer [VulkanPtr](){ VulkanCleanupSetupCommandBuffer(*VulkanPtr, flush_command_buffer::No); };

    Window->Vulkan = &Vulkan;

    LogInfo("Vulkan initialization finished!");
    Defer [](){ LogInfo("Shutting down..."); };

    //
    // Input Setup
    //
    my_input_slots MyInputSlots;
    x_input_dll XInputDll{};
    Win32LoadXInput(&XInputDll, GlobalLog);

    auto SystemInput = InputCreateContext(&Allocator);
    Defer [&](){ InputDestroyContext(&Allocator, SystemInput); };

    input_x_input_slots XInput{};
    input_mouse_slots Mouse{};
    input_keyboard_slots Keyboard{};
    PrepareInputSlots(SystemInput, &MyInputSlots, &XInput, &Mouse, &Keyboard);

    Window->Input = SystemInput;
    Window->Mouse = &Mouse;
    Window->Keyboard = &Keyboard;


    //
    // Camera Setup
    //
    free_horizon_camera Cam{};
    {
      Cam.VerticalFieldOfView = Degrees(60);
      Cam.Width = 1280;
      Cam.Height = 720;
      Cam.NearPlane = 0.1f;
      Cam.FarPlane = 100.0f;
      Cam.Transform = IdentityTransform;
      Cam.Transform.Translation = Vec3(-5, 0, 3);

      Cam.MovementSpeed = 7;
      Cam.RotationSpeed = 3;
    }

    //
    // Add scene objects
    //
    {
      LogBeginScope("Creating scene objects");
      Defer [](){ LogEndScope("Finished creating scene objects."); };

      VkCommandBuffer TextureUploadCommandBuffer{};
      {
        auto CommandBufferInfo = InitStruct<VkCommandBufferAllocateInfo>();
        CommandBufferInfo.commandPool = Vulkan.CommandPool;
        CommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        CommandBufferInfo.commandBufferCount = 1;
        VulkanVerify(Vulkan.Device.vkAllocateCommandBuffers(Vulkan.Device.DeviceHandle,
                                                             &CommandBufferInfo,
                                                             &TextureUploadCommandBuffer));
      }

      Defer [VulkanPtr, TextureUploadCommandBuffer]()
      {
        auto& Vulkan = *VulkanPtr;
        Vulkan.Device.vkFreeCommandBuffers(Vulkan.Device.DeviceHandle,
                                           Vulkan.CommandPool,
                                           1,
                                           &TextureUploadCommandBuffer);
      };

      arc_string KittenImageFilePath = DataPath("Kitten_DXT1_Mipmaps.dds");
      auto KittenImageFileExtension = AsConst(FindFileExtension(Slice(KittenImageFilePath)));
      auto KittenImageLoaderFactory = GetImageLoaderFactoryByFileExtension(*ImageLoaderRegistry, KittenImageFileExtension);
      image KittenImage{};
      Init(KittenImage, Allocator);
      Defer [&](){ Finalize(KittenImage); };


      //
      // Load kitten image.
      //
      {
        bool UseFallbackImage = false;

        if(KittenImageLoaderFactory)
        {
          auto ImageLoader = CreateImageLoader(*KittenImageLoaderFactory);
          Assert(ImageLoader);

          if(LoadImageFromFile(*ImageLoader, KittenImage, Slice(KittenImageFilePath)))
          {
            LogInfo("Loaded image file: %s", StrPtr(KittenImageFilePath));
          }
          else
          {
            LogWarning("Failed to load image file: %s", StrPtr(KittenImageFilePath));
            UseFallbackImage = true;
          }

          DestroyImageLoader(*KittenImageLoaderFactory, ImageLoader);
        }
        else
        {
          LogError("Failed to find image loader for: %s", StrPtr(KittenImageFilePath));
          UseFallbackImage = true;
        }

        if(UseFallbackImage)
        {
          ImageSetAsSolidColor(KittenImage, color::Pink, image_format::R32G32B32A32_FLOAT);
        }
      }


      //
      // Kitten 1
      //
      {
        auto Kitten = VulkanCreateSceneObject(Vulkan, "Kitten 1"_S);
        // TODO: Cleanup

        Kitten->Transform.Translation = Vec3(0, 0, 2);

        #if 0
        Copy(Kitten->Texture.Image, KittenImage);
        VulkanUploadTexture(Vulkan,
                            TextureUploadCommandBuffer,
                            Kitten->Texture);
        #else
        Kitten->Texture.ImageViewHandle = Vulkan.RenderTarget2.ImageView;
        Kitten->Texture.ImageHandle = Vulkan.RenderTarget2.Image;
        Kitten->Texture.ImageFormat = Vulkan.RenderTarget2.ImageFormat;
        Kitten->Texture.ImageTiling = VK_IMAGE_TILING_OPTIMAL;
        Kitten->Texture.ImageLayout = VK_IMAGE_LAYOUT_GENERAL; // TODO: Get the proper layout?

        //
        // Create sampler.
        //
        auto SamplerCreateInfo = InitStruct<VkSamplerCreateInfo>();
        {
          SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
          SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
          SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
          SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
          SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
          SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
          SamplerCreateInfo.anisotropyEnable = VK_TRUE;
          SamplerCreateInfo.maxLod = 1;
          SamplerCreateInfo.maxAnisotropy = 8;
          SamplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
          SamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
          SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
        }
        VulkanVerify(Vulkan.Device.vkCreateSampler(Vulkan.Device.DeviceHandle,
                                                   &SamplerCreateInfo,
                                                   nullptr,
                                                   &Kitten->Texture.SamplerHandle));
        #endif

        VulkanSetQuadGeometry(Vulkan, Kitten->VertexBuffer, Kitten->IndexBuffer);
      }


      //
      // Kitten 2
      //
      {
        auto Kitten = VulkanCreateSceneObject(Vulkan, "Kitten 2"_S);
        // TODO: Cleanup

        Kitten->Transform.Translation = Vec3(0.5f, 2, 1);

        Copy(Kitten->Texture.Image, KittenImage);
        VulkanUploadTexture(Vulkan,
                            TextureUploadCommandBuffer,
                            Kitten->Texture);

        VulkanSetBoxGeometry(Vulkan, Kitten->VertexBuffer, Kitten->IndexBuffer);
      }


      //
      // Kitten 3
      //
      {
        auto Kitten = VulkanCreateSceneObject(Vulkan, "Kitten 3"_S);
        // TODO: Cleanup

        Kitten->Transform.Translation = Vec3(-1, -4, 1);
        Kitten->Transform.Scale = Vec3(2, 2, 2);
        Kitten->Transform.Rotation = Quaternion(UpVector3, Degrees(30)) * Quaternion(RightVector3, Degrees(45));

        Copy(Kitten->Texture.Image, KittenImage);
        VulkanUploadTexture(Vulkan,
                            TextureUploadCommandBuffer,
                            Kitten->Texture);

        VulkanSetBoxGeometry(Vulkan, Kitten->VertexBuffer, Kitten->IndexBuffer);
      }

      //
      // Debug Grid 1
      //
      {
        auto DebugGrid = VulkanCreateDebugGrid(Vulkan, "Debug Grid 1"_S);
        // TODO: Cleanup

        VulkanSetDebugGridGeometry(Vulkan, { 10, 10, 5 }, { 10, 10, 10 },
                                   DebugGrid->VertexBuffer,
                                   DebugGrid->IndexBuffer);
      }
    }

    //
    // Build Command Buffers
    //
    {
      VulkanBuildDrawCommands(Vulkan,
                              Slice(Vulkan.DrawCommandBuffers),
                              Slice(Vulkan.Framebuffers),
                              // color::CornflowerBlue,
                              color::Gray,
                              Vulkan.DepthStencilValue,
                              0);
    }

    //
    // Main Loop | *mainloop*
    //
    Vulkan.DepthStencilValue = 1.0f;
    ::GlobalRunning = true;

    stopwatch FrameTimer{};
    stopwatch PerfTimer{};

    duration CurrentFrameTime = Milliseconds(16); // Assume 16 milliseconds for the first frame.
    float DeltaSeconds = Convert<float>(DurationAsSeconds(CurrentFrameTime));

    frame_stats_registry FrameStatsRegistry;

    frame_stats* FrameStats = FrameRegistryGetStats(&FrameStatsRegistry, "FrameStats");
    FrameStats->MaxNumSamples = 256;

    uint64 FrameCount = 0;
    int LastKnownFrameDrawCountDelta = 0;
    bool AutoCamera = true;

    while(::GlobalRunning)
    {
      StopwatchStart(&FrameTimer);
      StopwatchStart(&PerfTimer);

      InputBeginFrame(SystemInput);
      {
        Win32MessagePump();
        Win32PollXInput(&XInputDll, SystemInput, &XInput);
      }
      InputEndFrame(SystemInput);

      //
      // Check for Quit requests
      //
      if(InputButtonIsDown(Keyboard.Alt) &&
         InputButtonWasPressed(Keyboard.F4))
      {
        quick_exit(0);
      }

      if(InputButtonWasPressed(MyInputSlots.Quit))
      {
        ::GlobalRunning = false;
        break;
      }

      if(InputButtonWasPressed(Keyboard.P))
      {
        FrameStatsPrintEvaluated(FrameEvaluateStats(FrameStats), GlobalLog);
      }

      if(InputButtonWasPressed(Keyboard.Digit_1))
      {
        // TODO: Copy RenderTarget2 image?
      }

      //Vulkan.DepthStencilValue = Clamp(Vulkan.DepthStencilValue + InputAxisValue(SystemInput[MyInputSlots.Depth]), 0.8f, 1.0f);

      //
      // Update camera
      //
      {
        auto ForwardMovement = InputAxisValue(MyInputSlots.CamMoveForward);
        auto RightMovement = InputAxisValue(MyInputSlots.CamMoveRight);
        auto UpMovement = InputAxisValue(MyInputSlots.CamMoveUp);
        Cam.Transform.Translation += Cam.MovementSpeed * DeltaSeconds * (
          ForwardVector(Cam.Transform) * ForwardMovement +
          RightVector(Cam.Transform)   * RightMovement +
          UpVector(Cam.Transform)      * UpMovement
        );


        float InputYawDelta = InputActionValue(MyInputSlots.CamAbsYaw) +
                              InputAxisValue(MyInputSlots.CamRelYaw) * Cam.RotationSpeed * DeltaSeconds;
        float InputPitchDelta = InputActionValue(MyInputSlots.CamAbsPitch) +
                                InputAxisValue(MyInputSlots.CamRelPitch) * Cam.RotationSpeed * DeltaSeconds;

        Cam.InputYaw += InputYawDelta;
        Cam.InputPitch += InputPitchDelta;

        if(!IsNearlyZero(InputYawDelta) ||
           !IsNearlyZero(InputYawDelta))
        {
          AutoCamera = false;
        }

        if(InputButtonWasPressed(Keyboard.T))
        {
          AutoCamera = !AutoCamera;
        }

        if(AutoCamera)
        {
          Cam.InputYaw += 0.1f * Cam.RotationSpeed * DeltaSeconds;
        }

        Cam.Transform.Rotation = Quaternion(UpVector3,    Radians(Cam.InputYaw)) *
                                 Quaternion(RightVector3, Radians(Cam.InputPitch));
        SafeNormalize(&Cam.Transform.Rotation);

        if(InputButtonWasPressed(MyInputSlots.CamReset))
        {
          Cam.Transform = IdentityTransform;
          Cam.InputYaw = 0;
          Cam.InputPitch = 0;
        }
      }

      auto const ViewProjectionMatrix = CameraViewProjectionMatrix(Cam, Cam.Transform);

      StopwatchStop(&PerfTimer);
      auto FrameTimeOnCpu = StopwatchDuration(&PerfTimer);

      StopwatchStart(&PerfTimer);


      //
      // Upload shader buffers
      //
      {
        Vulkan.DebugGridsFoo.UboGlobals.Data.ViewProjectionMatrix = ViewProjectionMatrix;
        VulkanUploadShaderBufferData(Vulkan, Vulkan.DebugGridsFoo.UboGlobals);

        Vulkan.SceneObjectsFoo.UboGlobals.Data.ViewProjectionMatrix = ViewProjectionMatrix;
        VulkanUploadShaderBufferData(Vulkan, Vulkan.SceneObjectsFoo.UboGlobals);

        // TODO: Don't do this every frame?
        for(auto SceneObject : Slice(Vulkan.SceneObjects))
        {
          SceneObject->UboModel.Data.ModelViewProjectionMatrix = Mat4x4(SceneObject->Transform) * ViewProjectionMatrix;
          VulkanUploadShaderBufferData(Vulkan, SceneObject->UboModel);
        }
      }


      //
      // Handle resize requests
      //
      if(::GlobalIsResizeRequested)
      {
        LogBeginScope("Resizing swapchain");
        VulkanResize(Vulkan, ::GlobalResizeRequest_Extent);
        ::GlobalIsResizeRequested = false;
        LogEndScope("Finished resizing swapchain");
      }

      RedrawWindow(Window->WindowHandle, nullptr, nullptr, RDW_INTERNALPAINT);

      StopwatchStop(&PerfTimer);
      auto FrameTimeOnGpu = StopwatchDuration(&PerfTimer);

      // Update frame timer.
      {
        StopwatchStop(&FrameTimer);
        CurrentFrameTime = StopwatchDuration(&FrameTimer);
        DeltaSeconds = Convert<float>(DurationAsSeconds(CurrentFrameTime));
      }

      // Capture timing data.
      {
        frame_sample Sample;
        Sample.CpuTime = FrameTimeOnCpu;
        Sample.GpuTime = FrameTimeOnGpu;
        Sample.FrameTime = CurrentFrameTime;
        FrameStatsAddSample(FrameStats, Sample);
      }

      duration const IdealFrameTime(Seconds(1 / 60.0f));
      duration const AcceptableFrameTime(2 * IdealFrameTime);
      duration const BadFrameTime(2 * AcceptableFrameTime);

      double const CurrentSeconds = DurationAsSeconds(CurrentFrameTime);
      uint64 const CurrentFramesPerSecond = Round<uint64>(1.0 / CurrentSeconds);

      if(CurrentFrameTime > IdealFrameTime)
      {
        if(CurrentFrameTime > BadFrameTime)
        {
          LogWarning("Unacceptably slow frame: %fs (%u FPS)", CurrentSeconds, CurrentFramesPerSecond);
        }
        else if(CurrentFrameTime > AcceptableFrameTime)
        {
          LogInfo("Unusually slow frame: %fs (%u FPS)", CurrentSeconds, CurrentFramesPerSecond);
        }
        else
        {
          LogInfo("Slow frame: %fs (%u FPS)", CurrentSeconds, CurrentFramesPerSecond);
        }
      }

      if(ShowFpsInWindowTitle)
      {
        char WindowTextBuffer[sizeof("Vulkan Experiments (0000FPS)")];
        sprintf_s(WindowTextBuffer, "Vulkan Experiments (%4lluFPS)", CurrentFramesPerSecond);
        SetWindowTextA(Window->WindowHandle, WindowTextBuffer);
      }

      ++FrameCount;
      int FrameDrawCountDelta = Convert<int>(FrameCount - Window->DrawCount);
      if(FrameDrawCountDelta != LastKnownFrameDrawCountDelta)
      {
        LogWarning("Draw/Frame count mismatch: %u/%u", Window->DrawCount, FrameCount);
        LastKnownFrameDrawCountDelta = FrameDrawCountDelta;
      }
    }

    FrameStatsPrintEvaluated(FrameEvaluateStats(FrameStats), GlobalLog);
  }

  return exit_code::Success;
}

int
WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance,
        LPSTR CommandLine, int ShowCode)
{
  exit_code ExitCode = ApplicationEntryPoint(Instance);
  return (int)ExitCode;
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
      Win32ProcessInputMessage(WindowHandle, Message, WParam, LParam,
                               Window->Input, Window->Keyboard, Window->Mouse);
    }
  }
  else if(Message == WM_SIZE)
  {
    if(Vulkan && WParam != SIZE_MINIMIZED)
    {
      ::GlobalIsResizeRequested = true;
      ::GlobalResizeRequest_Extent.Width = Convert<uint32>(LParam & 0xffff);
      ::GlobalResizeRequest_Extent.Height = Convert<uint32>((LParam & 0xffff0000) >> 16);
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
      VulkanDraw(*Vulkan);
      ++Window->DrawCount;
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
