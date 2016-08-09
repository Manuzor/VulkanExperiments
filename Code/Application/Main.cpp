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

static LRESULT WINAPI
Win32MainWindowCallback(HWND WindowHandle, UINT Message,
                        WPARAM WParam, LPARAM LParam);

struct window_setup
{
  HINSTANCE ProcessHandle;
  char const* WindowClassName;
  extent2_<uint> ClientExtents;

  bool HasCustomWindowX;
  int WindowX;

  bool HasCustomWindowY;
  int WindowY;
};

struct window
{
  HWND WindowHandle;
  extent2_<uint32> ClientExtents;

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

  {
    auto const ClientArea = Args->ClientExtents.Width * Args->ClientExtents.Height;
    if(ClientArea == 0)
    {
      LogError("Invalid client extents.");
      return nullptr;
    }
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
  WindowRect.right = Args->ClientExtents.Width;
  WindowRect.bottom = Args->ClientExtents.Height;
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
  Window->ClientExtents = Args->ClientExtents;

  SetWindowLongPtr(Window->WindowHandle, GWLP_USERDATA, Reinterpret<LONG_PTR>(Window));

  return Window;
}

static void
Win32DestroyWindow(allocator_interface* Allocator, window* Window)
{
  // TODO: Close the window somehow?
  Deallocate(Allocator, Window);
}

enum class vulkan_enable_validation : bool { No = false, Yes = true };

static bool
VulkanCreateInstance(vulkan* Vulkan, vulkan_enable_validation EnableValidation)
{
  Assert(Vulkan->DLL);
  Assert(Vulkan->vkCreateInstance);
  Assert(Vulkan->vkEnumerateInstanceLayerProperties);
  Assert(Vulkan->vkEnumerateInstanceExtensionProperties);

  temp_allocator TempAllocator;
  allocator_interface* Allocator = *TempAllocator;

  //
  // Instance Layers
  //
  scoped_array<char const*> LayerNames{ Allocator };
  {
    scoped_array<char const*> DesiredLayerNames{ Allocator };

    if(EnableValidation == vulkan_enable_validation::Yes)
    {
      Expand(&DesiredLayerNames) = "VK_LAYER_LUNARG_standard_validation";
    }

    uint32 LayerCount;
    VulkanVerify(Vulkan->vkEnumerateInstanceLayerProperties(&LayerCount, nullptr));

    scoped_array<VkLayerProperties> LayerProperties = { Allocator };
    ExpandBy(&LayerProperties, LayerCount);
    VulkanVerify(Vulkan->vkEnumerateInstanceLayerProperties(&LayerCount, LayerProperties.Ptr));

    LogBeginScope("Explicitly enabled instance layers:");
    Defer [](){ LogEndScope("=========="); };
    {
      // Check for its existance.

    }
    for(auto& Property : Slice(&LayerProperties))
    {
      auto LayerName = SliceFromString(Property.layerName);
      bool WasAdded = false;

      for(auto DesiredLayerName : Slice(&DesiredLayerNames))
      {
        if(LayerName == SliceFromString(DesiredLayerName))
        {
          Expand(&LayerNames) = DesiredLayerName;
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
  scoped_array<char const*> ExtensionNames{ Allocator };
  {
    // Required extensions:
    bool SurfaceExtensionFound = false;
    bool PlatformSurfaceExtensionFound = false;

    uint32 ExtensionCount;
    VulkanVerify(Vulkan->vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    scoped_array<VkExtensionProperties> ExtensionProperties = { Allocator };
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
ImplVulkanCreateGraphicsPipeline(vulkan* Vulkan,
                             compiled_shader* CompiledShader,
                             VkPipelineLayout PipelineLayout,
                             vulkan_graphics_pipeline_desc& Desc,
                             size_t VertexDataStride)
{
  if(CompiledShader == nullptr)
  {
    LogError("Unable to create graphics pipeline without a shader.");
    return nullptr;
  }

  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  temp_allocator TempAllocator{};
  allocator_interface* Allocator = *TempAllocator;

  LogBeginScope("Creating graphics pipeline.");
  Defer [](){ LogEndScope("Finished creating graphics pipeline."); };

  auto const MaxNumShaderStages = (Cast<size_t>(shader_stage::Fragment) - Cast<size_t>(shader_stage::Vertex)) + 1;
  scoped_array<VkPipelineShaderStageCreateInfo> PipelineShaderStageInfos{ Allocator };
  Reserve(&PipelineShaderStageInfos, MaxNumShaderStages);

  for(size_t StageIndex = 0; StageIndex < MaxNumShaderStages; ++StageIndex)
  {
    auto Stage = Cast<shader_stage>(Cast<size_t>(shader_stage::Vertex) + StageIndex);
    if(!HasShaderStage(CompiledShader, Stage))
      continue;

    auto& StageInfo = Expand(&PipelineShaderStageInfos);
    StageInfo = InitStruct<VkPipelineShaderStageCreateInfo>();
    StageInfo.stage = ShaderStageToVulkan(Stage);
    StageInfo.pName = StrPtr(GetGlslShader(CompiledShader, Stage)->EntryPoint);

    auto SpirvShader = GetSpirvShader(CompiledShader, Stage);
    Assert(SpirvShader);

    auto ShaderModuleCreateInfo = InitStruct<VkShaderModuleCreateInfo>();
    ShaderModuleCreateInfo.codeSize = SliceByteSize(Slice(&SpirvShader->Code)); // In bytes, regardless of the fact that decltype(*pCode) == uint.
    ShaderModuleCreateInfo.pCode = SpirvShader->Code.Ptr;

    VulkanVerify(Device.vkCreateShaderModule(DeviceHandle, &ShaderModuleCreateInfo, nullptr, &StageInfo.module));
  }

  // Note(Manu): Can safely destroy the shader modules on our side once the pipeline was created.
  Defer [&]()
  {
    for(auto& StageInfo : Slice(&PipelineShaderStageInfos))
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

  scoped_array<VkVertexInputAttributeDescription> VertexInputAttributeDescs{ Allocator };
  GenerateVertexInputDescriptions(CompiledShader, VertexInputBinding, &VertexInputAttributeDescs);

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
    GraphicsPipelineCreateInfo.renderPass = Vulkan->RenderPass;
  }

  VkPipeline Pipeline{};
  VulkanVerify(Device.vkCreateGraphicsPipelines(DeviceHandle, Vulkan->PipelineCache,
                                                1, &GraphicsPipelineCreateInfo,
                                                nullptr,
                                                &Pipeline));
  return Pipeline;
}

template<typename VertexType>
inline VkPipeline
VulkanCreateGraphicsPipeline(vulkan* Vulkan,
                             compiled_shader* CompiledShader,
                             VkPipelineLayout PipelineLayout,
                             vulkan_graphics_pipeline_desc& Desc)
{
  return ImplVulkanCreateGraphicsPipeline(Vulkan, CompiledShader, PipelineLayout, Desc, SizeOf<VertexType>());
}

void
ImplVulkanPrepareRenderableFoo(vulkan* Vulkan, slice<char const> ShaderPath, vulkan_graphics_pipeline_desc PipelineDesc, size_t VertexDataStride, vulkan_renderable_foo* Foo)
{
  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  temp_allocator TempAllocator;
  allocator_interface* Allocator = *TempAllocator;

  //
  // Get compiled shader
  //
  Foo->Shader = GetCompiledShader(Vulkan->ShaderManager, ShaderPath, GlobalLog);
  Assert(Foo->Shader != nullptr);

  //
  // Prepare Descriptor Set Layout
  //
  {
    LogBeginScope("Preparing descriptor set layout.");
    Defer [](){ LogEndScope("Finished preparing descriptor set layout."); };

    scoped_array<VkDescriptorSetLayoutBinding> LayoutBindings{ Allocator };

    GetDescriptorSetLayoutBindings(Foo->Shader, &LayoutBindings);

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
  if(Vulkan->PipelineCache == nullptr)
  {
    auto PipelineCacheCreateInfo = InitStruct<VkPipelineCacheCreateInfo>();
    VulkanVerify(Device.vkCreatePipelineCache(DeviceHandle, &PipelineCacheCreateInfo, nullptr, &Vulkan->PipelineCache));
  }

  //
  // Create Pipeline
  //
  {
    Foo->Pipeline = ImplVulkanCreateGraphicsPipeline(Vulkan, Foo->Shader, Foo->PipelineLayout, PipelineDesc, VertexDataStride);
    Assert(Foo->Pipeline);
  }

  //
  // Allocate the UBO for Globals
  //
  {
    VulkanCreateShaderBuffer(Vulkan, &Foo->UboGlobals, is_read_only_for_shader::Yes);
  }
}

template<typename VertexDataType>
void
VulkanPrepareRenderableFoo(vulkan* Vulkan, slice<char const> ShaderPath, vulkan_graphics_pipeline_desc PipelineDesc, vulkan_renderable_foo* Foo)
{
  ImplVulkanPrepareRenderableFoo(Vulkan, ShaderPath, PipelineDesc, SizeOf<VertexDataType>(), Foo);
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
  // Prepare Foos
  //
  {
    // Debug Grids
    {
      auto PipelineDesc = InitStruct<vulkan_graphics_pipeline_desc>();
      PipelineDesc.RasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
      PipelineDesc.InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
      VulkanPrepareRenderableFoo<vulkan_debug_grid::vertex>(Vulkan, "Data/Shader/DebugGrid.shader"_S, PipelineDesc, &Vulkan->DebugGridsFoo);
    }

    // Scene Objects
    {
      auto PipelineDesc = InitStruct<vulkan_graphics_pipeline_desc>();
      PipelineDesc.RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
      PipelineDesc.RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
      PipelineDesc.InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      VulkanPrepareRenderableFoo<vulkan_scene_object::vertex>(Vulkan, "Data/Shader/SceneObject.shader"_S, PipelineDesc, &Vulkan->SceneObjectsFoo);
    }
  }


  //
  // Create Descriptor Pool
  //
  {
    LogBeginScope("Preparing descriptor pool.");
    Defer [](){ LogEndScope("Finished preparing descriptor pool."); };

    // Get descriptor counts
    scoped_dictionary<VkDescriptorType, uint32> DescriptorCounts{ Allocator };
    GetDescriptorTypeCounts(Vulkan->DebugGridsFoo.Shader, &DescriptorCounts);
    GetDescriptorTypeCounts(Vulkan->SceneObjectsFoo.Shader, &DescriptorCounts);

    uint32 const MaxNumInstances = 50;

    // Now collect them in the proper structs.
    scoped_array<VkDescriptorPoolSize> PoolSizes{ Allocator };
    for(auto Key : Keys(&DescriptorCounts))
    {
      auto& Size = Expand(&PoolSizes);
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
                                               &Vulkan->DescriptorPool));
  }

  return true;
}

static void VulkanCreateFramebuffers(vulkan* Vulkan)
{
  LogBeginScope("Creating framebuffers.");
  Defer [](){ LogEndScope("Finished creating framebuffers."); };

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
    VulkanVerify(Vulkan->Device.vkCreateFramebuffer(Vulkan->Device.DeviceHandle,
                                                    &FramebufferCreateInfo,
                                                    nullptr,
                                                    &Vulkan->Framebuffers[Index]));
  }
}

static void VulkanDestroyFramebuffers(vulkan* Vulkan)
{
  auto const ImageCount = Vulkan->Swapchain.ImageCount;
  for(uint32 Index = 0; Index < ImageCount; ++Index)
  {
    Vulkan->Device.vkDestroyFramebuffer(Vulkan->Device.DeviceHandle,
                                                     Vulkan->Framebuffers[Index],
                                                     nullptr);
  }
}

static void
VulkanCleanupRenderPass(vulkan* Vulkan)
{
  // TODO: Cleanup

  #if 0
  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  // Framebuffers
  for(auto Framebuffer : Slice(&Vulkan->Framebuffers))
  {
    Device.vkDestroyFramebuffer(DeviceHandle, Framebuffer, nullptr);
  }
  Clear(&Vulkan->Framebuffers);

  // UBOs
  Device.vkFreeMemory(DeviceHandle, Vulkan->SceneObjectGraphicsState.GlobalsUBO.MemoryHandle, nullptr);
  Device.vkDestroyBuffer(DeviceHandle, Vulkan->SceneObjectGraphicsState.GlobalsUBO.BufferHandle, nullptr);

  // Descriptor Sets
  Device.vkFreeDescriptorSets(DeviceHandle, Vulkan->DescriptorPool, 1, &Vulkan->SceneObjectGraphicsState.DescriptorSet);

  // Descriptor Pool
  Device.vkDestroyDescriptorPool(DeviceHandle, Vulkan->DescriptorPool, nullptr);

  // Pipeline
  Device.vkDestroyPipeline(DeviceHandle, Vulkan->SceneObjectGraphicsState.Pipeline, nullptr);

  // Pipeline Cache
  Device.vkDestroyPipelineCache(DeviceHandle, Vulkan->PipelineCache, nullptr);

  // Render Pass
  Device.vkDestroyRenderPass(DeviceHandle, Vulkan->RenderPass, nullptr);

  // Pipeline Layout
  Device.vkDestroyPipelineLayout(DeviceHandle, Vulkan->SceneObjectGraphicsState.PipelineLayout, nullptr);

  // Descriptor Set Layout
  Device.vkDestroyDescriptorSetLayout(DeviceHandle, Vulkan->SceneObjectGraphicsState.DescriptorSetLayout, nullptr);
  #endif
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

  SetNum(&Vulkan->DrawCommandBuffers, Num);
  SetNum(&Vulkan->PrePresentCommandBuffers, Num);
  SetNum(&Vulkan->PostPresentCommandBuffers, Num);

  VulkanVerify(Device.vkAllocateCommandBuffers(DeviceHandle, &AllocateInfo, Vulkan->DrawCommandBuffers.Ptr));
  VulkanVerify(Device.vkAllocateCommandBuffers(DeviceHandle, &AllocateInfo, Vulkan->PrePresentCommandBuffers.Ptr));
  VulkanVerify(Device.vkAllocateCommandBuffers(DeviceHandle, &AllocateInfo, Vulkan->PostPresentCommandBuffers.Ptr));
}

static void VulkanDestroyCommandBuffers(vulkan* Vulkan, VkCommandPool CommandPool)
{
  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  Device.vkFreeCommandBuffers(DeviceHandle, CommandPool, Cast<uint32>(Vulkan->PostPresentCommandBuffers.Num), Vulkan->PostPresentCommandBuffers.Ptr);
  Device.vkFreeCommandBuffers(DeviceHandle, CommandPool, Cast<uint32>(Vulkan->PrePresentCommandBuffers.Num), Vulkan->PrePresentCommandBuffers.Ptr);
  Device.vkFreeCommandBuffers(DeviceHandle, CommandPool, Cast<uint32>(Vulkan->DrawCommandBuffers.Num), Vulkan->DrawCommandBuffers.Ptr);

  Clear(&Vulkan->DrawCommandBuffers);
  Clear(&Vulkan->PrePresentCommandBuffers);
  Clear(&Vulkan->PostPresentCommandBuffers);
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
    VulkanDestroyAndDeallocateSceneObject(Vulkan, &SceneObject);
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
VulkanResize(vulkan* Vulkan, extent2_<uint32> NewExtent)
{
  LogError("Resizing is not implemented yet!");
  return;

  #if 0

  // Don't react to resize until after first initialization.
  if(!Vulkan->IsPrepared)
    return;

  LogInfo("Resizing to %ux%u.", NewExtent.Width, NewExtent.Height);

  // In order to properly resize the window, we must re-create the swapchain
  // AND redo the command buffers, etc.

  //
  // Perform some cleanup
  //
  VulkanCleanupDepth(*Vulkan, &Vulkan->Depth);
  VulkanDestroyCommandBuffers(Vulkan, Vulkan->CommandPool);
  VulkanDestroyFramebuffers(Vulkan);

  //
  // Re-create the swapchain
  //
  VulkanPrepareSetupCommandBuffer(Vulkan);
  VulkanPrepareSwapchain(*Vulkan, &Vulkan->Swapchain, NewExtent, Vulkan->Swapchain.VSync);
  VulkanPrepareDepth(Vulkan, &Vulkan->Depth, Vulkan->Swapchain.Extent);
  VulkanCreateFramebuffers(Vulkan);
  VulkanCreateCommandBuffers(Vulkan, Vulkan->CommandPool, Vulkan->Swapchain.ImageCount);
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

      // Draw scene objects
      VkDeviceSize NoOffset = {};

      for(auto Renderable : Slice(&Vulkan.Renderables))
      {
        VulkanEnsureIsReadyForDrawing(&Vulkan, Renderable);

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
        Renderable->Draw(&Vulkan, DrawCommandBuffer);
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

    VulkanVerify(Error);
    #if 0
    switch(Error)
    {
      case VK_ERROR_OUT_OF_DATE_KHR:
      {
        // Swapchain is out of date (e.g. the window was resized) and must be
        // recreated:
        VulkanResize(Vulkan, Vulkan->Swapchain.Extent);
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
  // Submit post-present commands.
  //
  {
    auto SubmitInfo = InitStruct<VkSubmitInfo>();
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &Vulkan->PostPresentCommandBuffers[Vulkan->CurrentSwapchainImage.Index];
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
    SubmitInfo.pCommandBuffers = &Vulkan->DrawCommandBuffers[Vulkan->CurrentSwapchainImage.Index];
    VulkanVerify(Device.vkQueueSubmit(Vulkan->Queue, 1, &SubmitInfo, NullFence));
  }


  //
  // Submit pre-present commands.
  //
  {
    auto SubmitInfo = InitStruct<VkSubmitInfo>();
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &Vulkan->PrePresentCommandBuffers[Vulkan->CurrentSwapchainImage.Index];
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
        VulkanResize(Vulkan, Vulkan->Swapchain.Extent);
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
    input_id const Reset       = InputId("Reset");
  } Camera;
} MyInputSlots;

struct frame_time_sample
{
  time CPU;
  time GPU;
  time Total;
};

struct frame_time_stats
{
  struct sample
  {
    time CpuTime;
    time GpuTime;
    time FrameTime;
  };

  sample MinSample{};
  sample MaxSample{};
  scoped_array<sample> Samples;

  size_t MaxNumSamples{};
  size_t NumSamples{};
  size_t SampleIndex{};

  frame_time_stats(allocator_interface* Allocator)
    : Samples{ Allocator }
  {
  }
};

static void
PrintFrameTimeStats(frame_time_stats const& FrameTimeStats, log_data* Log)
{
  frame_time_stats::sample MinSample{ Seconds(1e20f), Seconds(1e20f), Seconds(1e20f) };
  frame_time_stats::sample MaxSample{};
  frame_time_stats::sample AverageSample{};

  for(auto& Sample : Slice(&FrameTimeStats.Samples))
  {
    AverageSample.CpuTime += Sample.CpuTime;
    AverageSample.GpuTime += Sample.GpuTime;
    AverageSample.FrameTime += Sample.FrameTime;

    MinSample.CpuTime = Min(Sample.CpuTime, MinSample.CpuTime);
    MinSample.GpuTime = Min(Sample.GpuTime, MinSample.GpuTime);
    MinSample.FrameTime = Min(Sample.FrameTime, MinSample.FrameTime);
    MaxSample.CpuTime = Max(Sample.CpuTime, MaxSample.CpuTime);
    MaxSample.GpuTime = Max(Sample.GpuTime, MaxSample.GpuTime);
    MaxSample.FrameTime = Max(Sample.FrameTime, MaxSample.FrameTime);
  }

  AverageSample.CpuTime /= Convert<double>(FrameTimeStats.Samples.Num);
  AverageSample.GpuTime /= Convert<double>(FrameTimeStats.Samples.Num);
  AverageSample.FrameTime /= Convert<double>(FrameTimeStats.Samples.Num);

  LogBeginScope("Frame Time Stats (%u samples)", FrameTimeStats.Samples.Num);
  {
    LogInfo(Log, "Min     Frame|CPU|GPU: %f|%f|%f",
            TimeAsSeconds(MinSample.FrameTime),
            TimeAsSeconds(MinSample.CpuTime),
            TimeAsSeconds(MinSample.GpuTime));

    LogInfo(Log, "Max     Frame|CPU|GPU: %f|%f|%f",
            TimeAsSeconds(MaxSample.FrameTime),
            TimeAsSeconds(MaxSample.CpuTime),
            TimeAsSeconds(MaxSample.GpuTime));

    LogInfo(Log, "Average Frame|CPU|GPU: %f|%f|%f",
            TimeAsSeconds(AverageSample.FrameTime),
            TimeAsSeconds(AverageSample.CpuTime),
            TimeAsSeconds(AverageSample.GpuTime));
  }
  LogEndScope("Frame Time Stats");
}

static void
AddFrameTimeSample(frame_time_stats& Stats, frame_time_stats::sample Sample)
{
  auto const RequiredNumArrayElements = Stats.SampleIndex + 1;
  if(RequiredNumArrayElements > Stats.Samples.Num)
  {
    ExpandBy(&Stats.Samples, RequiredNumArrayElements - Stats.Samples.Num);
  }

  Stats.Samples[Stats.SampleIndex] = Sample;
  Stats.SampleIndex = Wrap(Stats.SampleIndex + 1, 0, Stats.MaxNumSamples);
  Stats.NumSamples = Min(Stats.NumSamples + 1, Stats.MaxNumSamples);
}

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
  SinkSlots[0] = GetStdoutLogSink(stdout_log_sink_enable_prefixes::Yes);
  SinkSlots[1] = log_sink(VisualStudioLogSink);

  image_loader_registry* ImageLoaderRegistry = CreateImageLoaderRegistry(Allocator);
  Defer [=](){ DestroyImageLoaderRegistry(Allocator, ImageLoaderRegistry); };

  {
    auto ImageLoaderModule = RegisterImageLoaderModule(ImageLoaderRegistry,
                                                       "DDS"_S,
                                                       "Core.dll"_S,
                                                       "CreateImageLoader_DDS"_S,
                                                       "DestroyImageLoader_DDS"_S);
    AssociateImageLoaderModuleWithFileExtension(ImageLoaderModule, ".dds"_S);
  }

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
    if(!VulkanCreateInstance(Vulkan, vulkan_enable_validation::Yes))
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
    WindowSetup.ClientExtents.Width = 1280;
    WindowSetup.ClientExtents.Height = 720;
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
      if(!VulkanPrepareSwapchain(*Vulkan, &Vulkan->Swapchain, WindowSetup.ClientExtents, vsync::On))
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

    //
    // Create framebuffers
    //
    {
      VulkanCreateFramebuffers(Vulkan);
    }
    Defer [Vulkan](){ VulkanDestroyFramebuffers(Vulkan); };

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

      Win32RegisterAllMouseSlots(&SystemInput, GlobalLog);
      Win32RegisterAllXInputSlots(&SystemInput, GlobalLog);
      Win32RegisterAllKeyboardSlots(&SystemInput, GlobalLog);

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
      AddInputSlotMapping(&SystemInput, keyboard::Left,  MyInputSlots.Camera.RelYaw, 1);
      AddInputSlotMapping(&SystemInput, keyboard::Right, MyInputSlots.Camera.RelYaw, -1);

      RegisterInputSlot(&SystemInput, input_type::Axis, MyInputSlots.Camera.RelPitch);
      AddInputSlotMapping(&SystemInput, x_input::YRightStick, MyInputSlots.Camera.RelPitch, -1);
      AddInputSlotMapping(&SystemInput, keyboard::Up,   MyInputSlots.Camera.RelPitch, -1);
      AddInputSlotMapping(&SystemInput, keyboard::Down, MyInputSlots.Camera.RelPitch, 1);

      float const CameraMouseSensitivity = 0.01f;

      RegisterInputSlot(&SystemInput, input_type::Axis, MyInputSlots.Camera.AbsYaw);
      AddInputSlotMapping(&SystemInput, mouse::XDelta, MyInputSlots.Camera.AbsYaw);
      {
        auto SlotProperties = GetOrCreate(&SystemInput.ValueProperties, MyInputSlots.Camera.AbsYaw);
        SlotProperties->Sensitivity = CameraMouseSensitivity;
      }

      RegisterInputSlot(&SystemInput, input_type::Axis, MyInputSlots.Camera.AbsPitch);
      AddInputSlotMapping(&SystemInput, mouse::YDelta, MyInputSlots.Camera.AbsPitch);
      {
        auto SlotProperties = GetOrCreate(&SystemInput.ValueProperties, MyInputSlots.Camera.AbsPitch);
        SlotProperties->Sensitivity = CameraMouseSensitivity;
      }

      RegisterInputSlot(&SystemInput, input_type::Button, MyInputSlots.Camera.Reset);
      AddInputSlotMapping(&SystemInput, keyboard::R, MyInputSlots.Camera.Reset);

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

      VkCommandBuffer TextureUploadCommandBuffer = {};
      {
        auto CommandBufferInfo = InitStruct<VkCommandBufferAllocateInfo>();
        CommandBufferInfo.commandPool = Vulkan->CommandPool;
        CommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        CommandBufferInfo.commandBufferCount = 1;
        VulkanVerify(Vulkan->Device.vkAllocateCommandBuffers(Vulkan->Device.DeviceHandle,
                                                             &CommandBufferInfo,
                                                             &TextureUploadCommandBuffer));
      }

      Defer [Vulkan, TextureUploadCommandBuffer]()
      {
        Vulkan->Device.vkFreeCommandBuffers(Vulkan->Device.DeviceHandle,
                                            Vulkan->CommandPool,
                                            1,
                                            &TextureUploadCommandBuffer);
      };

      arc_string KittenImageFilePath = "Data/Kitten_DXT1_Mipmaps.dds";
      auto KittenImageFileExtension = AsConst(FindFileExtension(Slice(KittenImageFilePath)));
      auto KittenImageLoaderFactory = GetImageLoaderFactoryByFileExtension(ImageLoaderRegistry, KittenImageFileExtension);
      image KittenImage{};
      Init(&KittenImage, Allocator);
      Defer [&](){ Finalize(&KittenImage); };


      //
      // Load kitten image.
      //
      {
        bool UseFallbackImage = false;

        if(KittenImageLoaderFactory)
        {
          auto ImageLoader = CreateImageLoader(KittenImageLoaderFactory);

          if(LoadImageFromFile(ImageLoader, &KittenImage, Slice(KittenImageFilePath)))
          {
            LogInfo("Loaded image file: %s", StrPtr(KittenImageFilePath));
          }
          else
          {
            LogWarning("Failed to load image file: %s", StrPtr(KittenImageFilePath));
            UseFallbackImage = true;
          }

          DestroyImageLoader(KittenImageLoaderFactory, ImageLoader);
        }
        else
        {
          LogError("Failed to find image loader for: %s", StrPtr(KittenImageFilePath));
          UseFallbackImage = true;
        }

        if(UseFallbackImage)
        {
          ImageSetAsSolidColor(&KittenImage, color::Pink, image_format::R32G32B32A32_FLOAT);
        }
      }


      //
      // Kitten 1
      //
      {
        auto Kitten = VulkanCreateSceneObject(Vulkan, "Kitten 1"_S);
        // TODO: Cleanup

        Kitten->Transform.Translation = Vec3(0, 0, 2);

        Copy(&Kitten->Texture.Image, KittenImage);
        VulkanUploadTexture(*Vulkan,
                            TextureUploadCommandBuffer,
                            &Kitten->Texture);

        VulkanSetQuadGeometry(Vulkan, &Kitten->VertexBuffer, &Kitten->IndexBuffer);
      }


      //
      // Kitten 2
      //
      {
        auto Kitten = VulkanCreateSceneObject(Vulkan, "Kitten 2"_S);
        // TODO: Cleanup

        Kitten->Transform.Translation = Vec3(0.5f, 2, 1);

        Copy(&Kitten->Texture.Image, KittenImage);
        VulkanUploadTexture(*Vulkan,
                            TextureUploadCommandBuffer,
                            &Kitten->Texture);

        VulkanSetBoxGeometry(Vulkan, &Kitten->VertexBuffer, &Kitten->IndexBuffer);
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

        Copy(&Kitten->Texture.Image, KittenImage);
        VulkanUploadTexture(*Vulkan,
                            TextureUploadCommandBuffer,
                            &Kitten->Texture);

        VulkanSetBoxGeometry(Vulkan, &Kitten->VertexBuffer, &Kitten->IndexBuffer);
      }

      //
      // Debug Grid 1
      //
      {
        auto DebugGrid = VulkanCreateDebugGrid(Vulkan, "Debug Grid 1"_S);
        // TODO: Cleanup

        VulkanSetDebugGridGeometry(Vulkan, { 10, 10, 5 }, { 10, 10, 10 },
                                   &DebugGrid->VertexBuffer,
                                   &DebugGrid->IndexBuffer);
      }
    }

    //
    // Build Command Buffers
    //
    {
      VulkanBuildDrawCommands(*Vulkan,
                              Slice(&Vulkan->DrawCommandBuffers),
                              Slice(&Vulkan->Framebuffers),
                              // color::CornflowerBlue,
                              color::Gray,
                              Vulkan->DepthStencilValue,
                              0);
      VulkanBuildPrePresentCommands(Vulkan->Device,
                                    Slice(&Vulkan->PrePresentCommandBuffers),
                                    Slice(&Vulkan->Swapchain.Images));
      VulkanBuildPostPresentCommands(Vulkan->Device,
                                     Slice(&Vulkan->PostPresentCommandBuffers),
                                     Slice(&Vulkan->Swapchain.Images));
    }

    //
    // Main Loop
    //
    Vulkan->DepthStencilValue = 1.0f;
    ::GlobalRunning = true;

    stopwatch FrameTimer = {};
    stopwatch PerfTimer = {};

    time CurrentFrameTime = Milliseconds(16); // Assume 16 milliseconds for the first frame.
    float DeltaSeconds = Convert<float>(TimeAsSeconds(CurrentFrameTime));

    frame_time_stats FrameTimeStats{ Allocator };
    FrameTimeStats.MaxNumSamples = 256;

    while(::GlobalRunning)
    {
      StopwatchStart(&FrameTimer);
      StopwatchStart(&PerfTimer);

      BeginInputFrame(&SystemInput);
      {
        Win32MessagePump();
        Win32PollXInput(&XInput, &SystemInput);
      }
      EndInputFrame(&SystemInput);

      //
      // Check for Quit requests
      //
      if(ButtonIsDown(SystemInput[keyboard::Alt]) &&
         ButtonIsDown(SystemInput[keyboard::F4]))
      {
        quick_exit(0);
      }

      if(ButtonIsDown(SystemInput[MyInputSlots.Quit]))
      {
        ::GlobalRunning = false;
        break;
      }

      if(ButtonIsDown(SystemInput[keyboard::P]))
      {
        PrintFrameTimeStats(FrameTimeStats, GlobalLog);
      }

      //Vulkan->DepthStencilValue = Clamp(Vulkan->DepthStencilValue + AxisValue(SystemInput[MyInputSlots.Depth]), 0.8f, 1.0f);

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

        if(ButtonIsDown(SystemInput[MyInputSlots.Camera.Reset]))
        {
          Cam.Transform = IdentityTransform;
          Cam.InputYaw = 0;
          Cam.InputPitch = 0;
        }
      }

      auto const ViewProjectionMatrix = CameraViewProjectionMatrix(Cam, Cam.Transform);

      StopwatchStop(&PerfTimer);
      auto FrameTimeOnCpu = StopwatchTime(&PerfTimer);

      StopwatchStart(&PerfTimer);


      //
      // Upload shader buffers
      //
      {
        Vulkan->DebugGridsFoo.UboGlobals.Data.ViewProjectionMatrix = ViewProjectionMatrix;
        VulkanUploadShaderBufferData(Vulkan, Vulkan->DebugGridsFoo.UboGlobals);

        Vulkan->SceneObjectsFoo.UboGlobals.Data.ViewProjectionMatrix = ViewProjectionMatrix;
        VulkanUploadShaderBufferData(Vulkan, Vulkan->SceneObjectsFoo.UboGlobals);

        // TODO: Don't do this every frame?
        for(auto SceneObject : Slice(&Vulkan->SceneObjects))
        {
          SceneObject->UboModel.Data.ViewProjectionMatrix = Mat4x4(SceneObject->Transform) * ViewProjectionMatrix;
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
      auto FrameTimeOnGpu = StopwatchTime(&PerfTimer);

      // Update frame timer.
      {
        StopwatchStop(&FrameTimer);
        CurrentFrameTime = StopwatchTime(&FrameTimer);
        DeltaSeconds = Convert<float>(TimeAsSeconds(CurrentFrameTime));
      }

      // Capture timing data.
      {
        frame_time_stats::sample Sample;
        Sample.CpuTime = FrameTimeOnCpu;
        Sample.GpuTime = FrameTimeOnGpu;
        Sample.FrameTime = CurrentFrameTime;
        AddFrameTimeSample(FrameTimeStats, Sample);
      }
    }

    PrintFrameTimeStats(FrameTimeStats, GlobalLog);
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
