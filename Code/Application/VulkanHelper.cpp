#include "VulkanHelper.hpp"

#include <Core/Log.hpp>
#include <Core/Image.hpp>
#include <Core/ImageLoader.hpp>
#include <Core/Time.hpp>

void
vulkan_renderable::PrepareForDrawing(struct vulkan* Vulkan)
{
  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  //
  // Ensure foo is valid.
  //
  Assert(this->Foo != nullptr && this->Foo->Pipeline != nullptr);


  //
  // Allocate DescriptorSet
  //
  if(this->DescriptorSet == nullptr)
  {
    auto DescriptorSetAllocateInfo = InitStruct<VkDescriptorSetAllocateInfo>();
    {
      DescriptorSetAllocateInfo.descriptorPool = Vulkan->DescriptorPool;
      DescriptorSetAllocateInfo.descriptorSetCount = 1;
      DescriptorSetAllocateInfo.pSetLayouts = &this->Foo->DescriptorSetLayout;
    }
    VulkanVerify(Device.vkAllocateDescriptorSets(DeviceHandle,
                                                 &DescriptorSetAllocateInfo,
                                                 &this->DescriptorSet));
  }

  //
  // Associate the Globals UBO with the DescriptorSet
  //
  auto DescriptorBufferInfo = InitStruct<VkDescriptorBufferInfo>();
  {
    DescriptorBufferInfo.buffer = this->Foo->UboGlobals.BufferHandle;
    DescriptorBufferInfo.offset = 0;
    DescriptorBufferInfo.range = VK_WHOLE_SIZE;
  }

  auto GlobalUBOUpdateInfo = InitStruct<VkWriteDescriptorSet>();
  {
    GlobalUBOUpdateInfo.dstSet = this->DescriptorSet;
    GlobalUBOUpdateInfo.dstBinding = 0;
    GlobalUBOUpdateInfo.descriptorType = this->Foo->UboGlobals.DescriptorType;
    GlobalUBOUpdateInfo.descriptorCount = 1;
    GlobalUBOUpdateInfo.pBufferInfo = &DescriptorBufferInfo;
  }

  //
  // Push descriptor set update.
  //
  Device.vkUpdateDescriptorSets(DeviceHandle,
                                1, &GlobalUBOUpdateInfo,
                                0, nullptr);
}

void
vulkan_scene_object::PrepareForDrawing(struct vulkan* Vulkan)
{
  vulkan_renderable::PrepareForDrawing(Vulkan);

  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  temp_allocator TempAllocator{};
  allocator_interface* Allocator = *TempAllocator;

  //
  // Create UboModel
  //
  {
    VulkanCreateShaderBuffer(Vulkan, &this->UboModel, is_read_only_for_shader::Yes);
  }

  scoped_array<VkWriteDescriptorSet> DescriptorSetWrites{ Allocator };

  //
  // Associate UboModel with the DescriptorSet
  //
  auto DescriptorBufferInfo = InitStruct<VkDescriptorBufferInfo>();
  {
    DescriptorBufferInfo.buffer = this->UboModel.BufferHandle;
    DescriptorBufferInfo.offset = 0;
    DescriptorBufferInfo.range = VK_WHOLE_SIZE;
  }

  {
    auto UboModelUpdateInfo = InitStruct<VkWriteDescriptorSet>();

    UboModelUpdateInfo.dstSet = this->DescriptorSet;
    UboModelUpdateInfo.dstBinding = 1;
    UboModelUpdateInfo.descriptorType = this->UboModel.DescriptorType;
    UboModelUpdateInfo.descriptorCount = 1;
    UboModelUpdateInfo.pBufferInfo = &DescriptorBufferInfo;

    Expand(&DescriptorSetWrites) = UboModelUpdateInfo;
  }

  //
  // Update the texture and sampler in use by the shader.
  //
  auto TextureDescriptor = InitStruct<VkDescriptorImageInfo>();
  {
    TextureDescriptor.sampler = this->Texture.SamplerHandle;
    TextureDescriptor.imageView = this->Texture.ImageViewHandle;
    TextureDescriptor.imageLayout = this->Texture.ImageLayout;
  }

  {
    auto TextureAndSamplerUpdateInfo = InitStruct<VkWriteDescriptorSet>();

    TextureAndSamplerUpdateInfo.dstSet = this->DescriptorSet;
    TextureAndSamplerUpdateInfo.dstBinding = 10;
    TextureAndSamplerUpdateInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    TextureAndSamplerUpdateInfo.descriptorCount = 1;
    TextureAndSamplerUpdateInfo.pImageInfo = &TextureDescriptor;

    Expand(&DescriptorSetWrites) = TextureAndSamplerUpdateInfo;
  }


  //
  // Push descriptor set updates.
  //
  Device.vkUpdateDescriptorSets(DeviceHandle,
                                Convert<uint32>(DescriptorSetWrites.Num), DescriptorSetWrites.Ptr,
                                0, nullptr);
}

void
vulkan_scene_object::Draw(vulkan* Vulkan, VkCommandBuffer CommandBuffer)
{
  auto const& Device = Vulkan->Device;
  VkDeviceSize NoOffset{};

  Device.vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &this->VertexBuffer.BufferHandle, &NoOffset);
  Device.vkCmdBindIndexBuffer(CommandBuffer, this->IndexBuffer.BufferHandle, NoOffset, VK_INDEX_TYPE_UINT32);
  Device.vkCmdDrawIndexed(CommandBuffer, this->IndexBuffer.NumIndices, 1, 0, 0, 0);
}

void
vulkan_debug_grid::PrepareForDrawing(struct vulkan* Vulkan)
{
  vulkan_renderable::PrepareForDrawing(Vulkan);
}

void
vulkan_debug_grid::Draw(vulkan* Vulkan, VkCommandBuffer CommandBuffer)
{
  auto const& Device = Vulkan->Device;
  VkDeviceSize NoOffset{};

  Device.vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &this->VertexBuffer.BufferHandle, &NoOffset);
  Device.vkCmdBindIndexBuffer(CommandBuffer, this->IndexBuffer.BufferHandle, NoOffset, VK_INDEX_TYPE_UINT32);
  Device.vkCmdDrawIndexed(CommandBuffer, this->IndexBuffer.NumIndices, 1, 0, 0, 0);
}

auto
::Init(vulkan* Vulkan,
       allocator_interface* Allocator)
  -> void
{
  Vulkan->RenderableAllocator = Allocator;
  Vulkan->ShaderManagerAllocator = Allocator;
  Vulkan->ShaderManager = CreateShaderManager(Vulkan->ShaderManagerAllocator);
  Init(&Vulkan->Gpu.QueueProperties, Allocator);
  Init(&Vulkan->Framebuffers, Allocator);
  Init(&Vulkan->SceneObjects, Allocator);
  Init(&Vulkan->DebugGrids, Allocator);
  Init(&Vulkan->Renderables, Allocator);
  Init(&Vulkan->Swapchain, &Vulkan->Device, Allocator);
  Init(&Vulkan->DrawCommandBuffers, Allocator);
  Init(&Vulkan->PrePresentCommandBuffers, Allocator);
  Init(&Vulkan->PostPresentCommandBuffers, Allocator);
  Vulkan->Gpu.Vulkan = Vulkan;
  Vulkan->Device.Gpu = &Vulkan->Gpu;
}

auto
::Finalize(vulkan* Vulkan)
  -> void
{
  Finalize(&Vulkan->PostPresentCommandBuffers);
  Finalize(&Vulkan->PrePresentCommandBuffers);
  Finalize(&Vulkan->DrawCommandBuffers);
  Finalize(&Vulkan->Swapchain);
  Finalize(&Vulkan->SceneObjects);
  Finalize(&Vulkan->Renderables);
  Finalize(&Vulkan->DebugGrids);
  Finalize(&Vulkan->Framebuffers);
  Finalize(&Vulkan->Gpu.QueueProperties);
  DestroyShaderManager(Vulkan->ShaderManagerAllocator, Vulkan->ShaderManager);
}

auto
::Init(vulkan_swapchain* Swapchain, vulkan_device* Device, allocator_interface* Allocator)
  -> void
{
  Init(&Swapchain->Images, Allocator);
  Init(&Swapchain->ImageViews, Allocator);
  Swapchain->Device = Device;
}

auto
::Finalize(vulkan_swapchain* Swapchain)
  -> void
{
  Finalize(&Swapchain->ImageViews);
  Finalize(&Swapchain->Images);
}

auto
::VulkanPrepareSurface(vulkan const&   Vulkan,
                       vulkan_surface* Surface,
                       HINSTANCE       ProcessHandle,
                       HWND            WindowHandle)
  -> bool
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

    VulkanVerify(Vulkan.vkCreateWin32SurfaceKHR(Vulkan.InstanceHandle, &CreateInfo, nullptr, &Surface->SurfaceHandle));
  }

  //
  // Find Queue for Graphics and Presenting
  //
  {
    LogBeginScope("Finding queue indices for graphics and presenting.");
    Defer [](){ LogEndScope("Done finding queue indices."); };

    uint32 GraphicsIndex = IntMaxValue<uint32>();
    uint32 PresentIndex = IntMaxValue<uint32>();

    uint32 const NumQueuesProperties = Cast<uint32>(Vulkan.Gpu.QueueProperties.Num);
    for(uint32 Index = 0; Index < NumQueuesProperties; ++Index)
    {
      VkBool32 SupportsPresenting;
      Vulkan.vkGetPhysicalDeviceSurfaceSupportKHR(Vulkan.Gpu.GpuHandle, Index, Surface->SurfaceHandle, &SupportsPresenting);

      auto& QueueProp = Vulkan.Gpu.QueueProperties[Index];
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

    Surface->PresentNode.Index = GraphicsIndex;
  }

  //
  // Get Physical Device Format and Color Space.
  //
  {
    LogBeginScope("Gathering surface format and color space.");
    Defer [](){ LogEndScope("Got format and color space for the previously created Win32 surface."); };

    uint32 FormatCount;
    VulkanVerify(Vulkan.vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan.Gpu.GpuHandle, Surface->SurfaceHandle, &FormatCount, nullptr));
    Assert(FormatCount > 0);

    temp_allocator TempAllocator;
    scoped_array<VkSurfaceFormatKHR> SurfaceFormats(*TempAllocator);
    ExpandBy(&SurfaceFormats, FormatCount);

    VulkanVerify(Vulkan.vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan.Gpu.GpuHandle, Surface->SurfaceHandle, &FormatCount, SurfaceFormats.Ptr));

    if(FormatCount == 1 && SurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
    {
      Surface->Format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
      Surface->Format = SurfaceFormats[0].format;
    }
    LogInfo("Format: %s", VulkanEnumName(Surface->Format));

    Surface->ColorSpace = SurfaceFormats[0].colorSpace;
    LogInfo("Color Space: %s", VulkanEnumName(Surface->ColorSpace));
  }

  return true;
}

auto
::VulkanCleanupSurface(vulkan const&   Vulkan,
                       vulkan_surface* Surface)
  -> void
{
  Vulkan.vkDestroySurfaceKHR(Vulkan.InstanceHandle, Surface->SurfaceHandle, nullptr);
}

auto
::VulkanSwapchainConnect(vulkan_swapchain* Swapchain,
                       vulkan_device*    Device,
                       vulkan_surface*   Surface)
  -> void
{
  Swapchain->Device = Device;
  Swapchain->Surface = Surface;
}

auto
::VulkanPrepareSwapchain(vulkan const&     Vulkan,
                         vulkan_swapchain* Swapchain,
                         extent2_<uint32>  Extents,
                         vsync             VSync)
  -> bool
{
  auto const& Device = *Swapchain->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  auto const OldSwapchainHandle = Swapchain->SwapchainHandle;

  auto const Surface = Swapchain->Surface;
  auto const SurfaceHandle = Surface->SurfaceHandle;


  //
  // Fetch surface capabilites
  //
  VkSurfaceCapabilitiesKHR SurfaceCapabilities;
  VulkanVerify(Vulkan.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vulkan.Gpu.GpuHandle,
                                                                SurfaceHandle,
                                                                &SurfaceCapabilities));


  //
  // Determine the final swapchain image extents
  //
  auto SwapchainExtent = SurfaceCapabilities.currentExtent;
  if(SwapchainExtent.width == -1 && SwapchainExtent.height == -1)
  {
    // The extent is currently not defined, so we can just set our desired
    // extent.
    SwapchainExtent.width = Extents.Width;
    SwapchainExtent.height = Extents.Height;
  }

  Swapchain->Extent.Width = SwapchainExtent.width;
  Swapchain->Extent.Height = SwapchainExtent.height;
  LogInfo("Swapchain extents: { width=%u, height=%u }", Swapchain->Extent.Width, Swapchain->Extent.Height);


  //
  // Select a present mode
  //

  // VK_PRESENT_MODE_FIFO_KHR must always be present, as per the vulkan spec.
  VkPresentModeKHR SwapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

  // If v-sync is not requested, try to find a mailbox mode if present. It's
  // the lowest latency non-tearing present mode available.
  if(VSync == vsync::Off)
  {
    // Fetch available present modes

    uint32 PresentModeCount;
    VulkanVerify(Vulkan.vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan.Gpu.GpuHandle, SurfaceHandle, &PresentModeCount, nullptr));

    temp_allocator TempAllocator;
    scoped_array<VkPresentModeKHR> PresentModes(*TempAllocator);
    ExpandBy(&PresentModes, PresentModeCount);

    VulkanVerify(Vulkan.vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan.Gpu.GpuHandle, SurfaceHandle, &PresentModeCount, PresentModes.Ptr));

    // Find a mailbox or immediate mode (in that order of preference).
    for(auto Candidate : Slice(&PresentModes))
    {
      if(Candidate == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        SwapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }

      Assert(SwapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR);
      if(Candidate == VK_PRESENT_MODE_IMMEDIATE_KHR)
      {
        SwapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      }
    }
  }


  //
  // Determine the number of swapchain images
  //

  uint32 NumDesiredSwapchainImages = SurfaceCapabilities.minImageCount + 1;
  if(SurfaceCapabilities.maxImageCount > 0)
  {
    NumDesiredSwapchainImages = Min(NumDesiredSwapchainImages, SurfaceCapabilities.maxImageCount);
  }


  //
  // Determine surface transform mode
  //

  auto SurfacePreTransform = SurfaceCapabilities.currentTransform;
  if(SurfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
  {
    SurfacePreTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  }


  //
  // Create the new swapchain
  //

  auto SwapchainCreateInfo = InitStruct<VkSwapchainCreateInfoKHR>();
  {
    SwapchainCreateInfo.surface = Swapchain->Surface->SurfaceHandle;
    SwapchainCreateInfo.minImageCount = NumDesiredSwapchainImages;
    SwapchainCreateInfo.imageFormat = Swapchain->Surface->Format;
    SwapchainCreateInfo.imageColorSpace = Swapchain->Surface->ColorSpace;
    SwapchainCreateInfo.imageExtent = SwapchainExtent;
    SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    SwapchainCreateInfo.preTransform = SurfacePreTransform;
    SwapchainCreateInfo.imageArrayLayers = 1;
    SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    SwapchainCreateInfo.presentMode = SwapchainPresentMode;
    SwapchainCreateInfo.clipped = true;
    SwapchainCreateInfo.oldSwapchain = OldSwapchainHandle;
    SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  }

  VulkanVerify(Device.vkCreateSwapchainKHR(DeviceHandle,
                                           &SwapchainCreateInfo,
                                           nullptr,
                                           &Swapchain->SwapchainHandle));
  LogInfo("Created new swapchain.");


  //
  // Cleanup old swapchain (e.g. when resizing)
  //

  if(OldSwapchainHandle != VK_NULL_HANDLE)
  {
    for(auto ViewHandle : Slice(&Swapchain->ImageViews))
    {
      Device.vkDestroyImageView(DeviceHandle, ViewHandle, nullptr);
    }
    Device.vkDestroySwapchainKHR(DeviceHandle, OldSwapchainHandle, nullptr);
    LogInfo("Destroyed old swapchain and image views.");
  }

  //
  // Get swapchain image handles
  //

  VulkanVerify(Device.vkGetSwapchainImagesKHR(DeviceHandle, Swapchain->SwapchainHandle, &Swapchain->ImageCount, nullptr));

  Clear(&Swapchain->Images);
  ExpandBy(&Swapchain->Images, Swapchain->ImageCount);
  VulkanVerify(Device.vkGetSwapchainImagesKHR(DeviceHandle, Swapchain->SwapchainHandle, &Swapchain->ImageCount, Swapchain->Images.Ptr));

  //
  // Create views to the swapchain images
  //

  Clear(&Swapchain->ImageViews);
  for(auto ImageHandle : Slice(&Swapchain->Images))
  {
    VkImageView ViewHandle = {};

    auto ColorAttachmentViewInfo = InitStruct<VkImageViewCreateInfo>();
    {
      ColorAttachmentViewInfo.format = Swapchain->Surface->Format;
      ColorAttachmentViewInfo.components = InitStruct<VkComponentMapping>();
      ColorAttachmentViewInfo.image = ImageHandle;
      ColorAttachmentViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      ColorAttachmentViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      ColorAttachmentViewInfo.subresourceRange.baseMipLevel = 0;
      ColorAttachmentViewInfo.subresourceRange.levelCount = 1;
      ColorAttachmentViewInfo.subresourceRange.baseArrayLayer = 0;
      ColorAttachmentViewInfo.subresourceRange.layerCount = 1;
    }

    VulkanVerify(Device.vkCreateImageView(DeviceHandle,
                                          &ColorAttachmentViewInfo,
                                          nullptr,
                                          &ViewHandle));

    Expand(&Swapchain->ImageViews) = ViewHandle;
  }

  return true;
}

auto
::VulkanCleanupSwapchain(vulkan const& Vulkan, vulkan_swapchain* Swapchain)
  -> void
{
  auto const& Device = *Swapchain->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  for(auto View : Slice(&Swapchain->ImageViews))
  {
    Device.vkDestroyImageView(DeviceHandle, View, nullptr);
  }
  Clear(&Swapchain->ImageViews);

  Device.vkDestroySwapchainKHR(DeviceHandle, Swapchain->SwapchainHandle, nullptr);
  Swapchain->SwapchainHandle = VK_NULL_HANDLE;
}

auto
::VulkanAcquireNextSwapchainImage(vulkan_swapchain const& Swapchain,
                                  VkSemaphore             PresentCompleteSemaphore,
                                  vulkan_swapchain_image* CurrentImage)
  -> VkResult
{
  auto const& Device = *Swapchain.Device;
  auto const DeviceHandle = Device.DeviceHandle;
  auto const Timeout = IntMaxValue<uint64>();
  VkFence const NullFence = {};
  return Device.vkAcquireNextImageKHR(DeviceHandle,
                                      Swapchain.SwapchainHandle,
                                      Timeout,
                                      PresentCompleteSemaphore,
                                      NullFence,
                                      &CurrentImage->Index);
}

auto
::VulkanQueuePresent(vulkan_swapchain*      Swapchain,
                     VkQueue                Queue,
                     vulkan_swapchain_image ImageToPresent,
                     VkSemaphore            WaitSemaphore)
  -> VkResult
{
  auto PresentInfo = InitStruct<VkPresentInfoKHR>();
  {
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains = &Swapchain->SwapchainHandle;
    PresentInfo.pImageIndices = &ImageToPresent.Index;
    if(WaitSemaphore != VK_NULL_HANDLE)
    {
      PresentInfo.waitSemaphoreCount = 1;
      PresentInfo.pWaitSemaphores = &WaitSemaphore;
    }
  }

  return Swapchain->Device->vkQueuePresentKHR(Queue, &PresentInfo);
}

auto
::VulkanPrepareDepth(vulkan* Vulkan, vulkan_depth* Depth, extent2_<uint32> Extents)
  -> bool
{
  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  Depth->Format = VK_FORMAT_D16_UNORM;
  Depth->Extent = Extents;

  auto ImageCreateInfo = InitStruct<VkImageCreateInfo>();
  {
    ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.format = Depth->Format;
    ImageCreateInfo.extent = { Depth->Extent.Width, Depth->Extent.Height, 1 };
    ImageCreateInfo.mipLevels = 1;
    ImageCreateInfo.arrayLayers = 1;
    ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }

  // Create image.
  VulkanVerify(Device.vkCreateImage(DeviceHandle, &ImageCreateInfo, nullptr, &Depth->Image));

  // Get memory requirements for this object.
  VkMemoryRequirements MemoryRequirements;
  Device.vkGetImageMemoryRequirements(DeviceHandle, Depth->Image, &MemoryRequirements);

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
  VulkanVerify(Device.vkAllocateMemory(DeviceHandle, &MemoryAllocateInfo, nullptr, &Depth->Memory));

  // Bind memory.
  VulkanVerify(Device.vkBindImageMemory(DeviceHandle, Depth->Image, Depth->Memory, 0));

  VulkanSetImageLayout(Vulkan->Device, Vulkan->SetupCommandBuffer,
                       Depth->Image,
                       { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
                       VkImageLayout(VK_IMAGE_LAYOUT_UNDEFINED),
                       VkImageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));

  // Create image view.
  auto ImageViewCreateInfo = InitStruct<VkImageViewCreateInfo>();
  {
    ImageViewCreateInfo.image = Depth->Image;
    ImageViewCreateInfo.format = Depth->Format;
    ImageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
  }
  VulkanVerify(Device.vkCreateImageView(DeviceHandle, &ImageViewCreateInfo, nullptr, &Depth->View));

  return true;
}

auto
::VulkanCleanupDepth(vulkan const& Vulkan, vulkan_depth* Depth)
  -> void
{
  // TODO
}

template<typename T>
bool
LoadHelper(vulkan const& Vulkan, char const* FuncName, T** OutPtrToProcPtr)
{
  auto ProcPtr = Vulkan.vkGetInstanceProcAddr(Vulkan.InstanceHandle, FuncName);
  if(ProcPtr)
  {
    *OutPtrToProcPtr = Reinterpret<T*>(ProcPtr);
    return true;
  }
  return false;
}

template<typename T>
bool
LoadHelper(vulkan const& Vulkan, vulkan_device const& Device, char const* FuncName, T** OutPtrToProcPtr)
{
  auto ProcPtr = Vulkan.vkGetDeviceProcAddr(Device.DeviceHandle, FuncName);
  if(ProcPtr)
  {
    *OutPtrToProcPtr = Reinterpret<T*>(ProcPtr);
    return true;
  }
  return false;
}

auto
::VulkanLoadDLL(vulkan* Vulkan)
  -> bool
{
  LogBeginScope("Loading Vulkan DLL.");
  Defer [](){ LogEndScope(""); };


  char const* FileName = "vulkan-1.dll";
  Vulkan->DLL = LoadLibraryA(FileName);
  if(!Vulkan->DLL)
  {
    LogError("Failed to load DLL: %s", FileName);
    return false;
  }

  auto Buffer = Slice(Vulkan->DLLNameBuffer);
  auto CharCount = GetModuleFileNameA(Vulkan->DLL, Buffer.Ptr, Cast<DWORD>(Buffer.Num));
  Vulkan->DLLName = Slice(Buffer, DWORD(0), CharCount);
  LogInfo("Loaded Vulkan DLL: %*s", Coerce<int>(Vulkan->DLLName.Num), Vulkan->DLLName.Ptr);


  Vulkan->vkCreateInstance = Reinterpret<decltype(Vulkan->vkCreateInstance)>(GetProcAddress(Vulkan->DLL, "vkCreateInstance"));
  if(Vulkan->vkCreateInstance == nullptr)
  {
    LogError("Failed to load vkCreateInstance!!");
    return false;
  }

  Vulkan->vkGetInstanceProcAddr = Reinterpret<decltype(Vulkan->vkGetInstanceProcAddr)>(GetProcAddress(Vulkan->DLL, "vkGetInstanceProcAddr"));
  if(Vulkan->vkGetInstanceProcAddr == nullptr)
  {
    LogError("Failed to load vkGetInstanceProcAddr!!");
    return false;
  }

  Vulkan->vkEnumerateInstanceLayerProperties = Reinterpret<decltype(Vulkan->vkEnumerateInstanceLayerProperties)>(GetProcAddress(Vulkan->DLL, "vkEnumerateInstanceLayerProperties"));
  if(Vulkan->vkEnumerateInstanceLayerProperties == nullptr)
  {
    LogError("Failed to load vkEnumerateInstanceLayerProperties!!");
    return false;
  }

  Vulkan->vkEnumerateInstanceExtensionProperties = Reinterpret<decltype(Vulkan->vkEnumerateInstanceExtensionProperties)>(GetProcAddress(Vulkan->DLL, "vkEnumerateInstanceExtensionProperties"));
  if(Vulkan->vkEnumerateInstanceExtensionProperties == nullptr)
  {
    LogError("Failed to load vkEnumerateInstanceExtensionProperties!!");
    return false;
  }

  return true;
}

auto
::VulkanLoadInstanceFunctions(vulkan* Vulkan)
  -> void
{
  Assert(Vulkan->DLL);

  LogBeginScope("Loading Vulkan instance procedures.");
  Defer [](){ LogEndScope(""); };

  #define TRY_LOAD(Name) if(!LoadHelper(*Vulkan, #Name, &Vulkan->##Name)) \
  { \
    LogWarning("Failed to load procedure: %s", #Name); \
  }

  TRY_LOAD(vkAcquireNextImageKHR);
  TRY_LOAD(vkAllocateCommandBuffers);
  TRY_LOAD(vkAllocateDescriptorSets);
  TRY_LOAD(vkAllocateMemory);
  TRY_LOAD(vkBeginCommandBuffer);
  TRY_LOAD(vkBindBufferMemory);
  TRY_LOAD(vkBindImageMemory);
  TRY_LOAD(vkCmdBeginQuery);
  TRY_LOAD(vkCmdBeginRenderPass);
  TRY_LOAD(vkCmdBindDescriptorSets);
  TRY_LOAD(vkCmdBindIndexBuffer);
  TRY_LOAD(vkCmdBindPipeline);
  TRY_LOAD(vkCmdBindVertexBuffers);
  TRY_LOAD(vkCmdBlitImage);
  TRY_LOAD(vkCmdClearAttachments);
  TRY_LOAD(vkCmdClearColorImage);
  TRY_LOAD(vkCmdClearDepthStencilImage);
  TRY_LOAD(vkCmdCopyBuffer);
  TRY_LOAD(vkCmdCopyBufferToImage);
  TRY_LOAD(vkCmdCopyImage);
  TRY_LOAD(vkCmdCopyImageToBuffer);
  TRY_LOAD(vkCmdCopyQueryPoolResults);
  TRY_LOAD(vkCmdDebugMarkerBeginEXT);
  TRY_LOAD(vkCmdDebugMarkerEndEXT);
  TRY_LOAD(vkCmdDebugMarkerInsertEXT);
  TRY_LOAD(vkCmdDispatch);
  TRY_LOAD(vkCmdDispatchIndirect);
  TRY_LOAD(vkCmdDraw);
  TRY_LOAD(vkCmdDrawIndexed);
  TRY_LOAD(vkCmdDrawIndexedIndirect);
  TRY_LOAD(vkCmdDrawIndirect);
  TRY_LOAD(vkCmdEndQuery);
  TRY_LOAD(vkCmdEndRenderPass);
  TRY_LOAD(vkCmdExecuteCommands);
  TRY_LOAD(vkCmdFillBuffer);
  TRY_LOAD(vkCmdNextSubpass);
  TRY_LOAD(vkCmdPipelineBarrier);
  TRY_LOAD(vkCmdPushConstants);
  TRY_LOAD(vkCmdResetEvent);
  TRY_LOAD(vkCmdResetQueryPool);
  TRY_LOAD(vkCmdResolveImage);
  TRY_LOAD(vkCmdSetBlendConstants);
  TRY_LOAD(vkCmdSetDepthBias);
  TRY_LOAD(vkCmdSetDepthBounds);
  TRY_LOAD(vkCmdSetEvent);
  TRY_LOAD(vkCmdSetLineWidth);
  TRY_LOAD(vkCmdSetScissor);
  TRY_LOAD(vkCmdSetStencilCompareMask);
  TRY_LOAD(vkCmdSetStencilReference);
  TRY_LOAD(vkCmdSetStencilWriteMask);
  TRY_LOAD(vkCmdSetViewport);
  TRY_LOAD(vkCmdUpdateBuffer);
  TRY_LOAD(vkCmdWaitEvents);
  TRY_LOAD(vkCmdWriteTimestamp);
  TRY_LOAD(vkCreateBuffer);
  TRY_LOAD(vkCreateBufferView);
  TRY_LOAD(vkCreateCommandPool);
  TRY_LOAD(vkCreateComputePipelines);
  TRY_LOAD(vkCreateDebugReportCallbackEXT);
  TRY_LOAD(vkCreateDescriptorPool);
  TRY_LOAD(vkCreateDescriptorSetLayout);
  TRY_LOAD(vkCreateDevice);
  TRY_LOAD(vkCreateDisplayModeKHR);
  TRY_LOAD(vkCreateDisplayPlaneSurfaceKHR);
  TRY_LOAD(vkCreateEvent);
  TRY_LOAD(vkCreateFence);
  TRY_LOAD(vkCreateFramebuffer);
  TRY_LOAD(vkCreateGraphicsPipelines);
  TRY_LOAD(vkCreateImage);
  TRY_LOAD(vkCreateImageView);
  TRY_LOAD(vkCreatePipelineCache);
  TRY_LOAD(vkCreatePipelineLayout);
  TRY_LOAD(vkCreateQueryPool);
  TRY_LOAD(vkCreateRenderPass);
  TRY_LOAD(vkCreateSampler);
  TRY_LOAD(vkCreateSemaphore);
  TRY_LOAD(vkCreateShaderModule);
  TRY_LOAD(vkCreateSharedSwapchainsKHR);
  TRY_LOAD(vkCreateSwapchainKHR);
  TRY_LOAD(vkCreateWin32SurfaceKHR);
  TRY_LOAD(vkDebugMarkerSetObjectNameEXT);
  TRY_LOAD(vkDebugMarkerSetObjectTagEXT);
  TRY_LOAD(vkDebugReportCallbackEXT);
  TRY_LOAD(vkDebugReportMessageEXT);
  TRY_LOAD(vkDestroyBuffer);
  TRY_LOAD(vkDestroyBufferView);
  TRY_LOAD(vkDestroyCommandPool);
  TRY_LOAD(vkDestroyDebugReportCallbackEXT);
  TRY_LOAD(vkDestroyDescriptorPool);
  TRY_LOAD(vkDestroyDescriptorSetLayout);
  TRY_LOAD(vkDestroyDevice);
  TRY_LOAD(vkDestroyEvent);
  TRY_LOAD(vkDestroyFence);
  TRY_LOAD(vkDestroyFramebuffer);
  TRY_LOAD(vkDestroyImage);
  TRY_LOAD(vkDestroyImageView);
  TRY_LOAD(vkDestroyInstance);
  TRY_LOAD(vkDestroyPipeline);
  TRY_LOAD(vkDestroyPipelineCache);
  TRY_LOAD(vkDestroyPipelineLayout);
  TRY_LOAD(vkDestroyQueryPool);
  TRY_LOAD(vkDestroyRenderPass);
  TRY_LOAD(vkDestroySampler);
  TRY_LOAD(vkDestroySemaphore);
  TRY_LOAD(vkDestroyShaderModule);
  TRY_LOAD(vkDestroySurfaceKHR);
  TRY_LOAD(vkDestroySwapchainKHR);
  TRY_LOAD(vkDeviceWaitIdle);
  TRY_LOAD(vkEndCommandBuffer);
  TRY_LOAD(vkEnumerateDeviceExtensionProperties);
  TRY_LOAD(vkEnumerateDeviceLayerProperties);
  TRY_LOAD(vkEnumeratePhysicalDevices);
  TRY_LOAD(vkFlushMappedMemoryRanges);
  TRY_LOAD(vkFreeCommandBuffers);
  TRY_LOAD(vkFreeDescriptorSets);
  TRY_LOAD(vkFreeMemory);
  TRY_LOAD(vkGetBufferMemoryRequirements);
  TRY_LOAD(vkGetDeviceMemoryCommitment);
  TRY_LOAD(vkGetDeviceProcAddr);
  TRY_LOAD(vkGetDeviceQueue);
  TRY_LOAD(vkGetDisplayModePropertiesKHR);
  TRY_LOAD(vkGetDisplayPlaneCapabilitiesKHR);
  TRY_LOAD(vkGetDisplayPlaneSupportedDisplaysKHR);
  TRY_LOAD(vkGetEventStatus);
  TRY_LOAD(vkGetFenceStatus);
  TRY_LOAD(vkGetImageMemoryRequirements);
  TRY_LOAD(vkGetImageSparseMemoryRequirements);
  TRY_LOAD(vkGetImageSubresourceLayout);
  TRY_LOAD(vkGetPhysicalDeviceDisplayPlanePropertiesKHR);
  TRY_LOAD(vkGetPhysicalDeviceDisplayPropertiesKHR);
  TRY_LOAD(vkGetPhysicalDeviceFeatures);
  TRY_LOAD(vkGetPhysicalDeviceFormatProperties);
  TRY_LOAD(vkGetPhysicalDeviceImageFormatProperties);
  TRY_LOAD(vkGetPhysicalDeviceMemoryProperties);
  TRY_LOAD(vkGetPhysicalDeviceProperties);
  TRY_LOAD(vkGetPhysicalDeviceQueueFamilyProperties);
  TRY_LOAD(vkGetPhysicalDeviceSparseImageFormatProperties);
  TRY_LOAD(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
  TRY_LOAD(vkGetPhysicalDeviceSurfaceFormatsKHR);
  TRY_LOAD(vkGetPhysicalDeviceSurfacePresentModesKHR);
  TRY_LOAD(vkGetPhysicalDeviceSurfaceSupportKHR);
  TRY_LOAD(vkGetPhysicalDeviceWin32PresentationSupportKHR);
  TRY_LOAD(vkGetPipelineCacheData);
  TRY_LOAD(vkGetQueryPoolResults);
  TRY_LOAD(vkGetRenderAreaGranularity);
  TRY_LOAD(vkGetSwapchainImagesKHR);
  TRY_LOAD(vkInvalidateMappedMemoryRanges);
  TRY_LOAD(vkMapMemory);
  TRY_LOAD(vkMergePipelineCaches);
  TRY_LOAD(vkQueueBindSparse);
  TRY_LOAD(vkQueuePresentKHR);
  TRY_LOAD(vkQueueSubmit);
  TRY_LOAD(vkQueueWaitIdle);
  TRY_LOAD(vkResetCommandBuffer);
  TRY_LOAD(vkResetCommandPool);
  TRY_LOAD(vkResetDescriptorPool);
  TRY_LOAD(vkResetEvent);
  TRY_LOAD(vkResetFences);
  TRY_LOAD(vkSetEvent);
  TRY_LOAD(vkUnmapMemory);
  TRY_LOAD(vkUpdateDescriptorSets);
  TRY_LOAD(vkWaitForFences);

  #undef TRY_LOAD
}

auto
::VulkanLoadDeviceFunctions(vulkan const& Vulkan, vulkan_device* Device)
  -> void
{
  LogBeginScope("Loading Vulkan device procedures.");
  Defer [](){ LogEndScope(""); };

  #define TRY_LOAD(Name) if(!LoadHelper(Vulkan, *Device, #Name, &Device->##Name)) \
  { \
    LogWarning("Unable to load device procedure: %s", #Name); \
    Device->##Name = Vulkan.##Name; \
  }

  TRY_LOAD(vkGetDeviceProcAddr);
  TRY_LOAD(vkDestroyDevice);
  TRY_LOAD(vkGetDeviceQueue);
  TRY_LOAD(vkDeviceWaitIdle);
  TRY_LOAD(vkAllocateMemory);
  TRY_LOAD(vkFreeMemory);
  TRY_LOAD(vkMapMemory);
  TRY_LOAD(vkUnmapMemory);
  TRY_LOAD(vkFlushMappedMemoryRanges);
  TRY_LOAD(vkInvalidateMappedMemoryRanges);
  TRY_LOAD(vkGetDeviceMemoryCommitment);
  TRY_LOAD(vkBindBufferMemory);
  TRY_LOAD(vkBindImageMemory);
  TRY_LOAD(vkGetBufferMemoryRequirements);
  TRY_LOAD(vkGetImageMemoryRequirements);
  TRY_LOAD(vkGetImageSparseMemoryRequirements);
  TRY_LOAD(vkCreateFence);
  TRY_LOAD(vkDestroyFence);
  TRY_LOAD(vkResetFences);
  TRY_LOAD(vkGetFenceStatus);
  TRY_LOAD(vkWaitForFences);
  TRY_LOAD(vkCreateSemaphore);
  TRY_LOAD(vkDestroySemaphore);
  TRY_LOAD(vkCreateEvent);
  TRY_LOAD(vkDestroyEvent);
  TRY_LOAD(vkGetEventStatus);
  TRY_LOAD(vkSetEvent);
  TRY_LOAD(vkResetEvent);
  TRY_LOAD(vkCreateQueryPool);
  TRY_LOAD(vkDestroyQueryPool);
  TRY_LOAD(vkGetQueryPoolResults);
  TRY_LOAD(vkCreateBuffer);
  TRY_LOAD(vkDestroyBuffer);
  TRY_LOAD(vkCreateBufferView);
  TRY_LOAD(vkDestroyBufferView);
  TRY_LOAD(vkCreateImage);
  TRY_LOAD(vkDestroyImage);
  TRY_LOAD(vkGetImageSubresourceLayout);
  TRY_LOAD(vkCreateImageView);
  TRY_LOAD(vkDestroyImageView);
  TRY_LOAD(vkCreateShaderModule);
  TRY_LOAD(vkDestroyShaderModule);
  TRY_LOAD(vkCreatePipelineCache);
  TRY_LOAD(vkDestroyPipelineCache);
  TRY_LOAD(vkGetPipelineCacheData);
  TRY_LOAD(vkMergePipelineCaches);
  TRY_LOAD(vkCreateGraphicsPipelines);
  TRY_LOAD(vkCreateComputePipelines);
  TRY_LOAD(vkDestroyPipeline);
  TRY_LOAD(vkCreatePipelineLayout);
  TRY_LOAD(vkDestroyPipelineLayout);
  TRY_LOAD(vkCreateSampler);
  TRY_LOAD(vkDestroySampler);
  TRY_LOAD(vkCreateDescriptorSetLayout);
  TRY_LOAD(vkDestroyDescriptorSetLayout);
  TRY_LOAD(vkCreateDescriptorPool);
  TRY_LOAD(vkDestroyDescriptorPool);
  TRY_LOAD(vkResetDescriptorPool);
  TRY_LOAD(vkAllocateDescriptorSets);
  TRY_LOAD(vkFreeDescriptorSets);
  TRY_LOAD(vkUpdateDescriptorSets);
  TRY_LOAD(vkCreateFramebuffer);
  TRY_LOAD(vkDestroyFramebuffer);
  TRY_LOAD(vkCreateRenderPass);
  TRY_LOAD(vkDestroyRenderPass);
  TRY_LOAD(vkGetRenderAreaGranularity);
  TRY_LOAD(vkCreateCommandPool);
  TRY_LOAD(vkDestroyCommandPool);
  TRY_LOAD(vkResetCommandPool);
  TRY_LOAD(vkAllocateCommandBuffers);
  TRY_LOAD(vkFreeCommandBuffers);
  TRY_LOAD(vkCreateSwapchainKHR);
  TRY_LOAD(vkDestroySwapchainKHR);
  TRY_LOAD(vkGetSwapchainImagesKHR);
  TRY_LOAD(vkAcquireNextImageKHR);
  TRY_LOAD(vkCreateSharedSwapchainsKHR);
  TRY_LOAD(vkDebugMarkerSetObjectTagEXT);
  TRY_LOAD(vkDebugMarkerSetObjectNameEXT);
  TRY_LOAD(vkQueueSubmit);
  TRY_LOAD(vkQueueWaitIdle);
  TRY_LOAD(vkQueueBindSparse);
  TRY_LOAD(vkQueuePresentKHR);
  TRY_LOAD(vkBeginCommandBuffer);
  TRY_LOAD(vkEndCommandBuffer);
  TRY_LOAD(vkResetCommandBuffer);
  TRY_LOAD(vkCmdBindPipeline);
  TRY_LOAD(vkCmdSetViewport);
  TRY_LOAD(vkCmdSetScissor);
  TRY_LOAD(vkCmdSetLineWidth);
  TRY_LOAD(vkCmdSetDepthBias);
  TRY_LOAD(vkCmdSetBlendConstants);
  TRY_LOAD(vkCmdSetDepthBounds);
  TRY_LOAD(vkCmdSetStencilCompareMask);
  TRY_LOAD(vkCmdSetStencilWriteMask);
  TRY_LOAD(vkCmdSetStencilReference);
  TRY_LOAD(vkCmdBindDescriptorSets);
  TRY_LOAD(vkCmdBindIndexBuffer);
  TRY_LOAD(vkCmdBindVertexBuffers);
  TRY_LOAD(vkCmdDraw);
  TRY_LOAD(vkCmdDrawIndexed);
  TRY_LOAD(vkCmdDrawIndirect);
  TRY_LOAD(vkCmdDrawIndexedIndirect);
  TRY_LOAD(vkCmdDispatch);
  TRY_LOAD(vkCmdDispatchIndirect);
  TRY_LOAD(vkCmdCopyBuffer);
  TRY_LOAD(vkCmdCopyImage);
  TRY_LOAD(vkCmdBlitImage);
  TRY_LOAD(vkCmdCopyBufferToImage);
  TRY_LOAD(vkCmdCopyImageToBuffer);
  TRY_LOAD(vkCmdUpdateBuffer);
  TRY_LOAD(vkCmdFillBuffer);
  TRY_LOAD(vkCmdClearColorImage);
  TRY_LOAD(vkCmdClearDepthStencilImage);
  TRY_LOAD(vkCmdClearAttachments);
  TRY_LOAD(vkCmdResolveImage);
  TRY_LOAD(vkCmdSetEvent);
  TRY_LOAD(vkCmdResetEvent);
  TRY_LOAD(vkCmdWaitEvents);
  TRY_LOAD(vkCmdPipelineBarrier);
  TRY_LOAD(vkCmdBeginQuery);
  TRY_LOAD(vkCmdEndQuery);
  TRY_LOAD(vkCmdResetQueryPool);
  TRY_LOAD(vkCmdWriteTimestamp);
  TRY_LOAD(vkCmdCopyQueryPoolResults);
  TRY_LOAD(vkCmdPushConstants);
  TRY_LOAD(vkCmdBeginRenderPass);
  TRY_LOAD(vkCmdNextSubpass);
  TRY_LOAD(vkCmdEndRenderPass);
  TRY_LOAD(vkCmdExecuteCommands);
  TRY_LOAD(vkCmdDebugMarkerBeginEXT);
  TRY_LOAD(vkCmdDebugMarkerEndEXT);
  TRY_LOAD(vkCmdDebugMarkerInsertEXT);

  #undef TRY_LOAD
}

auto
::VulkanEnumName(VkFormat Format)
  -> char const*
{
  switch(Format)
  {
    case VK_FORMAT_UNDEFINED:                  return "VK_FORMAT_UNDEFINED";
    case VK_FORMAT_R4G4_UNORM_PACK8:           return "VK_FORMAT_R4G4_UNORM_PACK8";
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:      return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:      return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
    case VK_FORMAT_R5G6B5_UNORM_PACK16:        return "VK_FORMAT_R5G6B5_UNORM_PACK16";
    case VK_FORMAT_B5G6R5_UNORM_PACK16:        return "VK_FORMAT_B5G6R5_UNORM_PACK16";
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:      return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:      return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:      return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
    case VK_FORMAT_R8_UNORM:                   return "VK_FORMAT_R8_UNORM";
    case VK_FORMAT_R8_SNORM:                   return "VK_FORMAT_R8_SNORM";
    case VK_FORMAT_R8_USCALED:                 return "VK_FORMAT_R8_USCALED";
    case VK_FORMAT_R8_SSCALED:                 return "VK_FORMAT_R8_SSCALED";
    case VK_FORMAT_R8_UINT:                    return "VK_FORMAT_R8_UINT";
    case VK_FORMAT_R8_SINT:                    return "VK_FORMAT_R8_SINT";
    case VK_FORMAT_R8_SRGB:                    return "VK_FORMAT_R8_SRGB";
    case VK_FORMAT_R8G8_UNORM:                 return "VK_FORMAT_R8G8_UNORM";
    case VK_FORMAT_R8G8_SNORM:                 return "VK_FORMAT_R8G8_SNORM";
    case VK_FORMAT_R8G8_USCALED:               return "VK_FORMAT_R8G8_USCALED";
    case VK_FORMAT_R8G8_SSCALED:               return "VK_FORMAT_R8G8_SSCALED";
    case VK_FORMAT_R8G8_UINT:                  return "VK_FORMAT_R8G8_UINT";
    case VK_FORMAT_R8G8_SINT:                  return "VK_FORMAT_R8G8_SINT";
    case VK_FORMAT_R8G8_SRGB:                  return "VK_FORMAT_R8G8_SRGB";
    case VK_FORMAT_R8G8B8_UNORM:               return "VK_FORMAT_R8G8B8_UNORM";
    case VK_FORMAT_R8G8B8_SNORM:               return "VK_FORMAT_R8G8B8_SNORM";
    case VK_FORMAT_R8G8B8_USCALED:             return "VK_FORMAT_R8G8B8_USCALED";
    case VK_FORMAT_R8G8B8_SSCALED:             return "VK_FORMAT_R8G8B8_SSCALED";
    case VK_FORMAT_R8G8B8_UINT:                return "VK_FORMAT_R8G8B8_UINT";
    case VK_FORMAT_R8G8B8_SINT:                return "VK_FORMAT_R8G8B8_SINT";
    case VK_FORMAT_R8G8B8_SRGB:                return "VK_FORMAT_R8G8B8_SRGB";
    case VK_FORMAT_B8G8R8_UNORM:               return "VK_FORMAT_B8G8R8_UNORM";
    case VK_FORMAT_B8G8R8_SNORM:               return "VK_FORMAT_B8G8R8_SNORM";
    case VK_FORMAT_B8G8R8_USCALED:             return "VK_FORMAT_B8G8R8_USCALED";
    case VK_FORMAT_B8G8R8_SSCALED:             return "VK_FORMAT_B8G8R8_SSCALED";
    case VK_FORMAT_B8G8R8_UINT:                return "VK_FORMAT_B8G8R8_UINT";
    case VK_FORMAT_B8G8R8_SINT:                return "VK_FORMAT_B8G8R8_SINT";
    case VK_FORMAT_B8G8R8_SRGB:                return "VK_FORMAT_B8G8R8_SRGB";
    case VK_FORMAT_R8G8B8A8_UNORM:             return "VK_FORMAT_R8G8B8A8_UNORM";
    case VK_FORMAT_R8G8B8A8_SNORM:             return "VK_FORMAT_R8G8B8A8_SNORM";
    case VK_FORMAT_R8G8B8A8_USCALED:           return "VK_FORMAT_R8G8B8A8_USCALED";
    case VK_FORMAT_R8G8B8A8_SSCALED:           return "VK_FORMAT_R8G8B8A8_SSCALED";
    case VK_FORMAT_R8G8B8A8_UINT:              return "VK_FORMAT_R8G8B8A8_UINT";
    case VK_FORMAT_R8G8B8A8_SINT:              return "VK_FORMAT_R8G8B8A8_SINT";
    case VK_FORMAT_R8G8B8A8_SRGB:              return "VK_FORMAT_R8G8B8A8_SRGB";
    case VK_FORMAT_B8G8R8A8_UNORM:             return "VK_FORMAT_B8G8R8A8_UNORM";
    case VK_FORMAT_B8G8R8A8_SNORM:             return "VK_FORMAT_B8G8R8A8_SNORM";
    case VK_FORMAT_B8G8R8A8_USCALED:           return "VK_FORMAT_B8G8R8A8_USCALED";
    case VK_FORMAT_B8G8R8A8_SSCALED:           return "VK_FORMAT_B8G8R8A8_SSCALED";
    case VK_FORMAT_B8G8R8A8_UINT:              return "VK_FORMAT_B8G8R8A8_UINT";
    case VK_FORMAT_B8G8R8A8_SINT:              return "VK_FORMAT_B8G8R8A8_SINT";
    case VK_FORMAT_B8G8R8A8_SRGB:              return "VK_FORMAT_B8G8R8A8_SRGB";
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:      return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:      return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:    return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:    return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:       return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:       return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:       return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:   return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:   return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32: return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:    return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:    return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:   return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:   return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32: return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:    return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:    return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
    case VK_FORMAT_R16_UNORM:                  return "VK_FORMAT_R16_UNORM";
    case VK_FORMAT_R16_SNORM:                  return "VK_FORMAT_R16_SNORM";
    case VK_FORMAT_R16_USCALED:                return "VK_FORMAT_R16_USCALED";
    case VK_FORMAT_R16_SSCALED:                return "VK_FORMAT_R16_SSCALED";
    case VK_FORMAT_R16_UINT:                   return "VK_FORMAT_R16_UINT";
    case VK_FORMAT_R16_SINT:                   return "VK_FORMAT_R16_SINT";
    case VK_FORMAT_R16_SFLOAT:                 return "VK_FORMAT_R16_SFLOAT";
    case VK_FORMAT_R16G16_UNORM:               return "VK_FORMAT_R16G16_UNORM";
    case VK_FORMAT_R16G16_SNORM:               return "VK_FORMAT_R16G16_SNORM";
    case VK_FORMAT_R16G16_USCALED:             return "VK_FORMAT_R16G16_USCALED";
    case VK_FORMAT_R16G16_SSCALED:             return "VK_FORMAT_R16G16_SSCALED";
    case VK_FORMAT_R16G16_UINT:                return "VK_FORMAT_R16G16_UINT";
    case VK_FORMAT_R16G16_SINT:                return "VK_FORMAT_R16G16_SINT";
    case VK_FORMAT_R16G16_SFLOAT:              return "VK_FORMAT_R16G16_SFLOAT";
    case VK_FORMAT_R16G16B16_UNORM:            return "VK_FORMAT_R16G16B16_UNORM";
    case VK_FORMAT_R16G16B16_SNORM:            return "VK_FORMAT_R16G16B16_SNORM";
    case VK_FORMAT_R16G16B16_USCALED:          return "VK_FORMAT_R16G16B16_USCALED";
    case VK_FORMAT_R16G16B16_SSCALED:          return "VK_FORMAT_R16G16B16_SSCALED";
    case VK_FORMAT_R16G16B16_UINT:             return "VK_FORMAT_R16G16B16_UINT";
    case VK_FORMAT_R16G16B16_SINT:             return "VK_FORMAT_R16G16B16_SINT";
    case VK_FORMAT_R16G16B16_SFLOAT:           return "VK_FORMAT_R16G16B16_SFLOAT";
    case VK_FORMAT_R16G16B16A16_UNORM:         return "VK_FORMAT_R16G16B16A16_UNORM";
    case VK_FORMAT_R16G16B16A16_SNORM:         return "VK_FORMAT_R16G16B16A16_SNORM";
    case VK_FORMAT_R16G16B16A16_USCALED:       return "VK_FORMAT_R16G16B16A16_USCALED";
    case VK_FORMAT_R16G16B16A16_SSCALED:       return "VK_FORMAT_R16G16B16A16_SSCALED";
    case VK_FORMAT_R16G16B16A16_UINT:          return "VK_FORMAT_R16G16B16A16_UINT";
    case VK_FORMAT_R16G16B16A16_SINT:          return "VK_FORMAT_R16G16B16A16_SINT";
    case VK_FORMAT_R16G16B16A16_SFLOAT:        return "VK_FORMAT_R16G16B16A16_SFLOAT";
    case VK_FORMAT_R32_UINT:                   return "VK_FORMAT_R32_UINT";
    case VK_FORMAT_R32_SINT:                   return "VK_FORMAT_R32_SINT";
    case VK_FORMAT_R32_SFLOAT:                 return "VK_FORMAT_R32_SFLOAT";
    case VK_FORMAT_R32G32_UINT:                return "VK_FORMAT_R32G32_UINT";
    case VK_FORMAT_R32G32_SINT:                return "VK_FORMAT_R32G32_SINT";
    case VK_FORMAT_R32G32_SFLOAT:              return "VK_FORMAT_R32G32_SFLOAT";
    case VK_FORMAT_R32G32B32_UINT:             return "VK_FORMAT_R32G32B32_UINT";
    case VK_FORMAT_R32G32B32_SINT:             return "VK_FORMAT_R32G32B32_SINT";
    case VK_FORMAT_R32G32B32_SFLOAT:           return "VK_FORMAT_R32G32B32_SFLOAT";
    case VK_FORMAT_R32G32B32A32_UINT:          return "VK_FORMAT_R32G32B32A32_UINT";
    case VK_FORMAT_R32G32B32A32_SINT:          return "VK_FORMAT_R32G32B32A32_SINT";
    case VK_FORMAT_R32G32B32A32_SFLOAT:        return "VK_FORMAT_R32G32B32A32_SFLOAT";
    case VK_FORMAT_R64_UINT:                   return "VK_FORMAT_R64_UINT";
    case VK_FORMAT_R64_SINT:                   return "VK_FORMAT_R64_SINT";
    case VK_FORMAT_R64_SFLOAT:                 return "VK_FORMAT_R64_SFLOAT";
    case VK_FORMAT_R64G64_UINT:                return "VK_FORMAT_R64G64_UINT";
    case VK_FORMAT_R64G64_SINT:                return "VK_FORMAT_R64G64_SINT";
    case VK_FORMAT_R64G64_SFLOAT:              return "VK_FORMAT_R64G64_SFLOAT";
    case VK_FORMAT_R64G64B64_UINT:             return "VK_FORMAT_R64G64B64_UINT";
    case VK_FORMAT_R64G64B64_SINT:             return "VK_FORMAT_R64G64B64_SINT";
    case VK_FORMAT_R64G64B64_SFLOAT:           return "VK_FORMAT_R64G64B64_SFLOAT";
    case VK_FORMAT_R64G64B64A64_UINT:          return "VK_FORMAT_R64G64B64A64_UINT";
    case VK_FORMAT_R64G64B64A64_SINT:          return "VK_FORMAT_R64G64B64A64_SINT";
    case VK_FORMAT_R64G64B64A64_SFLOAT:        return "VK_FORMAT_R64G64B64A64_SFLOAT";
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:    return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:     return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
    case VK_FORMAT_D16_UNORM:                  return "VK_FORMAT_D16_UNORM";
    case VK_FORMAT_X8_D24_UNORM_PACK32:        return "VK_FORMAT_X8_D24_UNORM_PACK32";
    case VK_FORMAT_D32_SFLOAT:                 return "VK_FORMAT_D32_SFLOAT";
    case VK_FORMAT_S8_UINT:                    return "VK_FORMAT_S8_UINT";
    case VK_FORMAT_D16_UNORM_S8_UINT:          return "VK_FORMAT_D16_UNORM_S8_UINT";
    case VK_FORMAT_D24_UNORM_S8_UINT:          return "VK_FORMAT_D24_UNORM_S8_UINT";
    case VK_FORMAT_D32_SFLOAT_S8_UINT:         return "VK_FORMAT_D32_SFLOAT_S8_UINT";
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:        return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:         return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:       return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:        return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
    case VK_FORMAT_BC2_UNORM_BLOCK:            return "VK_FORMAT_BC2_UNORM_BLOCK";
    case VK_FORMAT_BC2_SRGB_BLOCK:             return "VK_FORMAT_BC2_SRGB_BLOCK";
    case VK_FORMAT_BC3_UNORM_BLOCK:            return "VK_FORMAT_BC3_UNORM_BLOCK";
    case VK_FORMAT_BC3_SRGB_BLOCK:             return "VK_FORMAT_BC3_SRGB_BLOCK";
    case VK_FORMAT_BC4_UNORM_BLOCK:            return "VK_FORMAT_BC4_UNORM_BLOCK";
    case VK_FORMAT_BC4_SNORM_BLOCK:            return "VK_FORMAT_BC4_SNORM_BLOCK";
    case VK_FORMAT_BC5_UNORM_BLOCK:            return "VK_FORMAT_BC5_UNORM_BLOCK";
    case VK_FORMAT_BC5_SNORM_BLOCK:            return "VK_FORMAT_BC5_SNORM_BLOCK";
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:          return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:          return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
    case VK_FORMAT_BC7_UNORM_BLOCK:            return "VK_FORMAT_BC7_UNORM_BLOCK";
    case VK_FORMAT_BC7_SRGB_BLOCK:             return "VK_FORMAT_BC7_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:    return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:     return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:  return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:   return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:  return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:   return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
    case VK_FORMAT_EAC_R11_UNORM_BLOCK:        return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
    case VK_FORMAT_EAC_R11_SNORM_BLOCK:        return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:     return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:     return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:       return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:        return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:       return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:        return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:       return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:        return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:       return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:        return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:       return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:        return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:       return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:        return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:       return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:        return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:       return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:        return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:      return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:       return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:      return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:       return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:      return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:       return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:     return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:      return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:     return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:      return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:     return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:      return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";

    default: return "<UNKNOWN>";
  }
}

auto
::VulkanEnumName(VkColorSpaceKHR ColorSpace)
  -> char const*
{
  switch(ColorSpace)
  {
    case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR";
    default: return "<UNKNOWN>";
  }
}

auto
::VulkanSetImageLayout(vulkan_device const&    Device,
                       VkCommandBuffer         CommandBuffer,
                       VkImage                 Image,
                       VkImageSubresourceRange SubresourceRange,
                       VkImageLayout           OldImageLayout,
                       VkImageLayout           NewImageLayout)
  -> void
{
  auto ImageMemoryBarrier = InitStruct<VkImageMemoryBarrier>();
  {
    ImageMemoryBarrier.oldLayout = OldImageLayout;
    ImageMemoryBarrier.newLayout = NewImageLayout;
    ImageMemoryBarrier.image = Image;
    ImageMemoryBarrier.subresourceRange = SubresourceRange;
  }

  // Source access mask controls actions that have to be finished on the old layout
  // before it will be transitioned to the new layout
  switch(OldImageLayout)
  {
    case VK_IMAGE_LAYOUT_UNDEFINED:
    {
      // Image layout is undefined (or does not matter)
      // Only valid as initial layout
      // No flags required, listed only for completeness
      ImageMemoryBarrier.srcAccessMask = 0;
    } break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
    {
      // Image is preinitialized
      // Only valid as initial layout for linear images, preserves memory contents
      // Make sure host writes have been finished
      ImageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    } break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    {
      // Image is a color attachment
      // Make sure any writes to the color buffer have been finished
      ImageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    } break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    {
      // Image is a depth/stencil attachment
      // Make sure any writes to the depth/stencil buffer have been finished
      ImageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    {
      // Image is a transfer source
      // Make sure any reads from the image have been finished
      ImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    } break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    {
      // Image is a transfer destination
      // Make sure any writes to the image have been finished
      ImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    {
      // Image is read by a shader
      // Make sure any shader reads from the image have been finished
      ImageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } break;

    default:
      break;
  }

  // Destination access mask controls the dependency for the new image layout
  switch (NewImageLayout)
  {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      // Image will be used as a transfer destination
      // Make sure any writes to the image have been finished
      ImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      // Image will be used as a transfer source
      // Make sure any reads from and writes to the image have been finished
      ImageMemoryBarrier.srcAccessMask = ImageMemoryBarrier.srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
      ImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      // Image will be used as a color attachment
      // Make sure any writes to the color buffer have been finished
      ImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      ImageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      // Image layout will be used as a depth/stencil attachment
      // Make sure any writes to depth/stencil buffer have been finished
      ImageMemoryBarrier.dstAccessMask = ImageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      // Image will be read in a shader (sampler, input attachment)
      // Make sure any writes to the image have been finished
      if (ImageMemoryBarrier.srcAccessMask == 0)
      {
        ImageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
      }
      ImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
  }

  // Put barrier on top
  VkPipelineStageFlags SourceStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags DestinationStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

  Device.vkCmdPipelineBarrier(CommandBuffer,                   // commandBuffer
                              SourceStages, DestinationStages, // dstStageMask, srcStageMask
                              0,                               // dependencyFlags
                              0, nullptr,                      // memoryBarrierCount, pMemoryBarriers
                              0, nullptr,                      // bufferMemoryBarrierCount, pBufferMemoryBarriers
                              1, &ImageMemoryBarrier);         // imageMemoryBarrierCount, pImageMemoryBarriers
}

auto
::VulkanDetermineMemoryTypeIndex(VkPhysicalDeviceMemoryProperties const& MemoryProperties,
                                 uint32 TypeBits,
                                 VkFlags RequirementsMask)
-> uint32
{
  // Search memtypes to find first index with those properties
  for(uint32 Index = 0; Index < 32; ++Index)
  {
    if(IsBitSet(TypeBits, Index))
    {
      // Type is available, does it match user properties?
      auto const PropertyFlags = MemoryProperties.memoryTypes[Index].propertyFlags;
      auto const FilteredFlags = PropertyFlags & RequirementsMask;
      if(FilteredFlags == RequirementsMask)
      {
        // Perfect match.
        return Index;
      }
    }
  }

  // No memory types matched.
  return IntMaxValue<uint32>();
}

/// Create and begin setup command buffer.
auto
::VulkanPrepareSetupCommandBuffer(vulkan* Vulkan)
  -> void
{
  if(Vulkan->SetupCommandBuffer != VK_NULL_HANDLE)
    return;

  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  auto AllocateInfo = InitStruct<VkCommandBufferAllocateInfo>();
  {
    AllocateInfo.commandPool = Vulkan->CommandPool;
    AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocateInfo.commandBufferCount = 1;
  }

  VulkanVerify(Device.vkAllocateCommandBuffers(DeviceHandle,
                                               &AllocateInfo,
                                               &Vulkan->SetupCommandBuffer));

  auto InheritanceInfo = InitStruct<VkCommandBufferInheritanceInfo>();
  auto BeginInfo = InitStruct<VkCommandBufferBeginInfo>();
  BeginInfo.pInheritanceInfo = &InheritanceInfo;

  VulkanVerify(Device.vkBeginCommandBuffer(Vulkan->SetupCommandBuffer, &BeginInfo));
}

auto
::VulkanCleanupSetupCommandBuffer(vulkan* Vulkan, flush_command_buffer Flush)
  -> void
{
  if(Vulkan->SetupCommandBuffer == VK_NULL_HANDLE)
    return;

  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  if(Flush == flush_command_buffer::Yes)
  {
    VulkanVerify(Device.vkEndCommandBuffer(Vulkan->SetupCommandBuffer));

    auto SubmitInfo = InitStruct<VkSubmitInfo>();
    {
      SubmitInfo.commandBufferCount = 1;
      SubmitInfo.pCommandBuffers = &Vulkan->SetupCommandBuffer;
    }
    VkFence NullFence = {};
    VulkanVerify(Device.vkQueueSubmit(Vulkan->Queue, 1, &SubmitInfo, NullFence));

    VulkanVerify(Device.vkQueueWaitIdle(Vulkan->Queue));
  }

  Device.vkFreeCommandBuffers(DeviceHandle, Vulkan->CommandPool, 1, &Vulkan->SetupCommandBuffer);
  Vulkan->SetupCommandBuffer = VK_NULL_HANDLE;
}

auto
::VulkanVerify(VkResult Result)
  -> void
{
  Assert(Result == VK_SUCCESS);
}


auto
::VulkanIsImageCompatibleWithGpu(vulkan_gpu const& Gpu, image const& Image)
  -> bool
{
  VkFormat VulkanTextureFormat = ImageFormatToVulkan(Image.Format);
  if(VulkanTextureFormat == VK_FORMAT_UNDEFINED)
  {
    LogError("Unable to find corresponding Vulkan format for %s.", ImageFormatName(Image.Format));
    return false;
  }

  VkFormatProperties FormatProperties;
  Gpu.Vulkan->vkGetPhysicalDeviceFormatProperties(Gpu.GpuHandle, VulkanTextureFormat, &FormatProperties);

  bool SupportsImageSampling = FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
  if(!SupportsImageSampling)
  {
    LogError("%s: Cannot sample this format with optimal tiling.", VulkanTextureFormat);
    return false;
  }

  VkImageFormatProperties ImageProperties;
  Gpu.Vulkan->vkGetPhysicalDeviceImageFormatProperties(Gpu.GpuHandle,
                                                        VulkanTextureFormat,
                                                        VK_IMAGE_TYPE_2D,
                                                        VK_IMAGE_TILING_OPTIMAL,
                                                        VK_IMAGE_USAGE_SAMPLED_BIT,
                                                        0,
                                                        &ImageProperties);

  VkExtent3D ImageExtent = { Image.Width, Image.Height, 1 };
  if(ImageProperties.maxExtent.width  < ImageExtent.width ||
     ImageProperties.maxExtent.height < ImageExtent.height ||
     ImageProperties.maxExtent.depth  < ImageExtent.depth)
  {
    LogError("Given image extent (%f, %f, %f) does not fit the devices' maximum extent (%f, %f, %f).",
             ImageExtent.width, ImageExtent.height, ImageExtent.depth,
             ImageProperties.maxExtent.width, ImageProperties.maxExtent.height, ImageProperties.maxExtent.depth);
    return false;
  }

  if(Image.NumMipLevels > ImageProperties.maxMipLevels)
  {
    LogError("Physical device accepts a maximum of %d Mip levels, the given image has %d",
             ImageProperties.maxMipLevels, Image.NumMipLevels);
    return false;
  }

  if(Image.NumArrayIndices > ImageProperties.maxArrayLayers)
  {
    LogError("Physical device accepts a maximum of %d array layers, the given image has %d",
             ImageProperties.maxArrayLayers, Image.NumArrayIndices);
    return false;
  }

  // TODO(Manu): sampleCounts, maxResourceSize?

  return true;
}

auto
::ImageFormatToVulkan(image_format KrepelFormat)
  -> VkFormat
{
  // TODO(Manu): complete switch.
  switch(KrepelFormat)
  {
    default: return VK_FORMAT_UNDEFINED;

    //
    // BGR formats
    //
    case image_format::B8G8R8_UNORM:        return VK_FORMAT_B8G8R8_UNORM;

    //
    // RGBA formats
    //
    case image_format::R8G8B8A8_UNORM:      return VK_FORMAT_R8G8B8A8_UNORM;
    case image_format::R8G8B8A8_SNORM:      return VK_FORMAT_R8G8B8A8_SNORM;
    case image_format::R8G8B8A8_UNORM_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
    //case image_format::R8G8B8A8_TYPELESS:   return VK_FORMAT_R8G8B8A8_SSCALED;
    case image_format::R8G8B8A8_UINT:       return VK_FORMAT_R8G8B8A8_UINT;
    case image_format::R8G8B8A8_SINT:       return VK_FORMAT_R8G8B8A8_SINT;

    case image_format::R32G32B32A32_FLOAT:  return VK_FORMAT_R32G32B32A32_SFLOAT;

    //
    // BGRA formats
    //
    case image_format::B8G8R8A8_UNORM:      return VK_FORMAT_B8G8R8A8_UNORM;
    //case image_format::B8G8R8A8_TYPELESS:   return VK_FORMAT_B8G8R8A8_SSCALED;
    case image_format::B8G8R8A8_UNORM_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;

    //
    // Block compressed formats
    //
    case image_format::BC1_UNORM:           return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case image_format::BC1_UNORM_SRGB:      return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

    case image_format::BC2_UNORM:           return VK_FORMAT_BC2_UNORM_BLOCK;
    case image_format::BC2_UNORM_SRGB:      return VK_FORMAT_BC2_SRGB_BLOCK;

    case image_format::BC3_UNORM:           return VK_FORMAT_BC3_UNORM_BLOCK;
    case image_format::BC3_UNORM_SRGB:      return VK_FORMAT_BC3_SRGB_BLOCK;

    case image_format::BC4_UNORM:           return VK_FORMAT_BC4_UNORM_BLOCK;
    case image_format::BC4_SNORM:           return VK_FORMAT_BC4_SNORM_BLOCK;

    case image_format::BC5_UNORM:           return VK_FORMAT_BC5_UNORM_BLOCK;
    case image_format::BC5_SNORM:           return VK_FORMAT_BC5_SNORM_BLOCK;

    case image_format::BC6H_UF16:           return VK_FORMAT_BC6H_UFLOAT_BLOCK;
    case image_format::BC6H_SF16:           return VK_FORMAT_BC6H_SFLOAT_BLOCK;

    case image_format::BC7_UNORM:           return VK_FORMAT_BC7_UNORM_BLOCK;
    case image_format::BC7_UNORM_SRGB:      return VK_FORMAT_BC7_SRGB_BLOCK;
  }
}

auto
::ImageFormatFromVulkan(VkFormat VulkanFormat)
  -> image_format
{
  // TODO(Manu): Complete this. Check out whether the ones that are commented
  // out are correct.
  switch(VulkanFormat)
  {
    default: return image_format::UNKNOWN;

    //
    // BGR formats
    //
    case VK_FORMAT_B8G8R8_UNORM: return image_format::B8G8R8_UNORM;

    //
    // RGBA formats
    //
    case VK_FORMAT_R8G8B8A8_UNORM:      return image_format::R8G8B8A8_UNORM;
    case VK_FORMAT_R8G8B8A8_SNORM:      return image_format::R8G8B8A8_SNORM;
    case VK_FORMAT_R8G8B8A8_SRGB:       return image_format::R8G8B8A8_UNORM_SRGB;
    //case VK_FORMAT_R8G8B8A8_SSCALED:    return image_format::R8G8B8A8_TYPELESS;
    case VK_FORMAT_R8G8B8A8_UINT:       return image_format::R8G8B8A8_UINT;
    case VK_FORMAT_R8G8B8A8_SINT:       return image_format::R8G8B8A8_SINT;

    case VK_FORMAT_R32G32B32A32_SFLOAT: return image_format::R32G32B32A32_FLOAT;

    //
    // BGRA formats
    //
    case VK_FORMAT_B8G8R8A8_UNORM:      return image_format::B8G8R8A8_UNORM;
    //case VK_FORMAT_B8G8R8A8_SSCALED:    return image_format::B8G8R8A8_TYPELESS;
    case VK_FORMAT_B8G8R8A8_SRGB: return image_format::B8G8R8A8_UNORM_SRGB;

    //
    // Block compressed formats
    //
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return image_format::BC1_UNORM;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:  return image_format::BC1_UNORM_SRGB;

    case VK_FORMAT_BC2_UNORM_BLOCK:      return image_format::BC2_UNORM;
    case VK_FORMAT_BC2_SRGB_BLOCK:       return image_format::BC2_UNORM_SRGB;

    case VK_FORMAT_BC3_UNORM_BLOCK:      return image_format::BC3_UNORM;
    case VK_FORMAT_BC3_SRGB_BLOCK:       return image_format::BC3_UNORM_SRGB;

    case VK_FORMAT_BC4_UNORM_BLOCK:      return image_format::BC4_UNORM;
    case VK_FORMAT_BC4_SNORM_BLOCK:      return image_format::BC4_SNORM;

    case VK_FORMAT_BC5_UNORM_BLOCK:      return image_format::BC5_UNORM;
    case VK_FORMAT_BC5_SNORM_BLOCK:      return image_format::BC5_SNORM;

    case VK_FORMAT_BC6H_UFLOAT_BLOCK:    return image_format::BC6H_UF16;
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:    return image_format::BC6H_SF16;

    case VK_FORMAT_BC7_UNORM_BLOCK:      return image_format::BC7_UNORM;
    case VK_FORMAT_BC7_SRGB_BLOCK:       return image_format::BC7_UNORM_SRGB;
  }
}

#if 0
auto
::VulkanUploadImageToGpu(vulkan_device const& Device,
                         VkCommandBuffer      CommandBuffer,
                         image const&         Image,
                         gpu_image*           GpuImage)
  -> bool
{


  //
  // 1. Allocate a temporary buffer.
  // 2. Blit the image data into that.
  // 3. Allocate an image with optimal tiling
  // 4. Blit the buffer data into the image.
  // 5. Deallocate the temporary buffer.
  //

  GpuImage->ImageFormat = ImageFormatToVulkan(Image.Format);
  LogInfo("Image format (Ours => Vulkan): %s => %s",
          ImageFormatName(Image.Format),
          VulkanEnumName(GpuImage->ImageFormat));

  if(GpuImage->ImageFormat == VK_FORMAT_UNDEFINED)
  {
    LogError("Could not convert our image format to Vulkan image format. "
             "Did you run IsImageCompatibleWithGpu before calling this function?");
    return false;
  }

  //
  // Create the temporary buffer and upload the data to it.
  //
    auto ImageData = Slice(ImageDataSize(&Image), ImageDataPointer<void>(&Image));

    auto Temp_BufferCreateInfo = InitStruct<VkBufferCreateInfo>();
    {
      Temp_BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      Temp_BufferCreateInfo.size = Cast<uint32>(ImageData.Num);
    }

    VkBuffer Temp_Buffer;
    VulkanVerify(Device.vkCreateBuffer(Device.DeviceHandle, &Temp_BufferCreateInfo, nullptr, &Temp_Buffer));
    Defer [&, Temp_Buffer](){ Device.vkDestroyBuffer(Device.DeviceHandle, Temp_Buffer, nullptr); };

    VkMemoryRequirements Temp_MemoryRequirements;
    Device.vkGetBufferMemoryRequirements(Device.DeviceHandle, Temp_Buffer, &Temp_MemoryRequirements);

    auto Temp_MemoryAllocationInfo = InitStruct<VkMemoryAllocateInfo>();
    {
      Temp_MemoryAllocationInfo.allocationSize = Temp_MemoryRequirements.size;
      Temp_MemoryAllocationInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Device.Gpu->MemoryProperties,
                                                                                 Temp_MemoryRequirements.memoryTypeBits,
                                                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      if(Temp_MemoryAllocationInfo.memoryTypeIndex == IntMaxValue<uint32>())
      {
        LogError("Unable to determine memory type for temporary buffer memory.");
        return false;
      }
    }

    VkDeviceMemory Temp_Memory;
    VulkanVerify(Device.vkAllocateMemory(Device.DeviceHandle, &Temp_MemoryAllocationInfo, nullptr, &Temp_Memory));
    Defer [&Device, Temp_Memory](){ Device.vkFreeMemory(Device.DeviceHandle, Temp_Memory, nullptr); };

    // Copy over the image data to the temporary buffer.
    {
      void* RawData;
      VulkanVerify(Device.vkMapMemory(Device.DeviceHandle,
                                      Temp_Memory,
                                      0, // offset
                                      VK_WHOLE_SIZE,
                                      0, // flags
                                      &RawData));

      MemCopy(ImageData.Num, RawData, ImageData.Ptr);

      Device.vkUnmapMemory(Device.DeviceHandle, Temp_Memory);
    }

    VulkanVerify(Device.vkBindBufferMemory(Device.DeviceHandle, Temp_Buffer, Temp_Memory, 0));

  //
  // Create the actual texture image.
  //
    // Now that we have the buffer in place, holding the texture data, we copy
    // it over to an image.
    auto Image_CreateInfo = InitStruct<VkImageCreateInfo>();
    {
      Image_CreateInfo.imageType = VK_IMAGE_TYPE_2D;
      Image_CreateInfo.format = GpuImage->ImageFormat;
      Image_CreateInfo.extent = VkExtent3D{ Image.Width, Image.Height, 1 };
      Image_CreateInfo.mipLevels = Image.NumMipLevels;
      Image_CreateInfo.arrayLayers = Image.NumArrayIndices;
      Image_CreateInfo.samples = VK_SAMPLE_COUNT_1_BIT; // TODO(Manu): Should probably be passed in as a parameter to this function, because it must be consistent with the renderpass, etc.
      Image_CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      Image_CreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
      Image_CreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      Image_CreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    VulkanVerify(Device.vkCreateImage(Device.DeviceHandle, &Image_CreateInfo, nullptr, &GpuImage->ImageHandle));

    VkMemoryRequirements Image_MemoryRequirements;
    Device.vkGetImageMemoryRequirements(Device.DeviceHandle, GpuImage->ImageHandle, &Image_MemoryRequirements);

    LogInfo("Requiring %d bytes on the GPU for the given image data.", Image_MemoryRequirements.size);

    auto Image_MemoryAllocateInfo = InitStruct<VkMemoryAllocateInfo>();
    {
      Image_MemoryAllocateInfo.allocationSize = Image_MemoryRequirements.size;
      Image_MemoryAllocateInfo.memoryTypeIndex =  VulkanDetermineMemoryTypeIndex(Device.Gpu->MemoryProperties,
                                                                                 Image_MemoryRequirements.memoryTypeBits,
                                                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      if(Image_MemoryAllocateInfo.memoryTypeIndex == IntMaxValue<uint32>())
      {
        LogError("Unable to determine memory type for image memory.");
        return false;
      }
    }

    // Allocate image memory
    VulkanVerify(Device.vkAllocateMemory(Device.DeviceHandle, &Image_MemoryAllocateInfo, nullptr, &GpuImage->MemoryHandle));

    // Bind image and image memory
    VulkanVerify(Device.vkBindImageMemory(Device.DeviceHandle, GpuImage->ImageHandle, GpuImage->MemoryHandle, 0));

    // Since the initial layout is VK_IMAGE_LAYOUT_UNDEFINED, and this image
    // is a blit-target, we set its current layout to
    // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
    VulkanSetImageLayout(Device, CommandBuffer,
                         GpuImage->ImageHandle,
                         VkImageAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT),
                         VkImageLayout(VK_IMAGE_LAYOUT_UNDEFINED),
                         VkImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
                         VkAccessFlags(0));

  //
  // Copy the buffered data over to the image memory.
  //
  {
    temp_allocator TempAllocator;
    scoped_array<VkBufferImageCopy> Regions(*TempAllocator);
    // TODO(Manu): Upload all MIP levels.
    for(uint32 MipLevel = 0; MipLevel < 1; ++MipLevel)
    {
      auto Region = &Expand(&Regions);
      //Region.bufferOffset = ;
      Region->imageExtent = Image_CreateInfo.extent;
      Region->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      Region->imageSubresource.mipLevel = MipLevel;
      Region->imageSubresource.baseArrayLayer = 0;
      Region->imageSubresource.layerCount = 1; // TODO(Manu): Should be more than 1.
    }
    Device.vkCmdCopyBufferToImage(CommandBuffer,
                                  Temp_Buffer,
                                  GpuImage->ImageHandle,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  Cast<uint32>(Regions.Num),
                                  Regions.Ptr);
  }
  //
  // Set the image layout.
  //
    VulkanSetImageLayout(Device, CommandBuffer,
                         GpuImage->ImageHandle,
                         VkImageAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT),
                         VkImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
                         VkImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                         VkAccessFlags(0));

  // TODO(Manu): Synchronization?

  return true;
}
#endif

auto
::VulkanCreateSceneObject(vulkan* Vulkan, slice<char const> Name)
  -> vulkan_scene_object*
{
  auto Allocator = Vulkan->RenderableAllocator;

  auto SceneObject = Allocate<vulkan_scene_object>(Allocator);
  MemDefaultConstruct(1, SceneObject);
  SceneObject->IsDirty = true;
  SceneObject->Name = Name;
  Init(&SceneObject->Texture, Allocator);

  SceneObject->Foo = &Vulkan->SceneObjectsFoo;

  Expand(&Vulkan->SceneObjects) = SceneObject;
  Expand(&Vulkan->Renderables) = SceneObject;

  return SceneObject;
}

auto
::VulkanDestroySceneObject(vulkan* Vulkan, vulkan_scene_object* SceneObjectHandle)
  -> void
{
  // TODO:

  // if(SceneObjectHandle == nullptr)
  // {
  //   LogWarning("Ignoring a null handle in VulkanDestroyAndDeallocateSceneObject.");
  //   return;
  // }

  // auto SceneObjectIndex = TranslateHandleToIndex(SceneObjectHandle);
  // auto SceneObject = &Vulkan->SceneObjects[SceneObjectIndex];

  // if(SceneObject->IsAllocated)
  // {
  //   LogError("Trying to destroy a scene object that is already destroyed. (Ignored)");
  //   return;
  // }

  // if(SceneObject->NumExternalReferences > 0)
  // {
  //   LogError("Cannot destroy a scene object that is still externally referenced.");
  //   return;
  // }

  // Finalize(&SceneObject->Texture);

  // auto const Device = &Vulkan->Device;
  // auto const DeviceHandle = Device->DeviceHandle;

  // Device->vkDestroyBuffer(DeviceHandle, SceneObject->Indices.Buffer, nullptr);
  // Device->vkFreeMemory(DeviceHandle, SceneObject->Indices.Memory, nullptr);

  // Device->vkDestroyBuffer(DeviceHandle, SceneObject->Vertices.Buffer, nullptr);
  // Device->vkFreeMemory(DeviceHandle, SceneObject->Vertices.Memory, nullptr);

  // Device->vkDestroySampler(DeviceHandle,   SceneObject->Texture.SamplerHandle, nullptr);
  // Device->vkDestroyImageView(DeviceHandle, SceneObject->Texture.ImageViewHandle, nullptr);
  // Device->vkDestroyImage(DeviceHandle,     SceneObject->Texture.ImageHandle, nullptr);
  // Device->vkFreeMemory(DeviceHandle,       SceneObject->Texture.MemoryHandle, nullptr);

  // // TODO: Free DescriptorSet and friends

  // // Note: This sets IsAllocated to false.
  // *SceneObject = {};
  // Assert(SceneObject->IsAllocated == false);
}

auto
::VulkanCreateDebugGrid(vulkan* Vulkan, slice<char const> Name)
  -> vulkan_debug_grid*
{
  auto Allocator = Vulkan->RenderableAllocator;

  auto DebugGrid = Allocate<vulkan_debug_grid>(Allocator);
  MemDefaultConstruct(1, DebugGrid);
  DebugGrid->IsDirty = true;
  DebugGrid->Name = Name;

  DebugGrid->Foo = &Vulkan->DebugGridsFoo;

  Expand(&Vulkan->DebugGrids) = DebugGrid;
  Expand(&Vulkan->Renderables) = DebugGrid;

  return DebugGrid;
}

auto
::VulkanDestroyDebugGrid(vulkan*            Vulkan,
                         vulkan_debug_grid* SceneObject)
  -> void
{
  // TODO: See VulkanDestroySceneObject
}

auto
::VulkanUploadTexture(vulkan const&              Vulkan,
                      VkCommandBuffer            CommandBuffer,
                      vulkan_texture2d*          Texture,
                      vulkan_force_linear_tiling ForceLinearTiling,
                      VkImageUsageFlags          ImageUsageFlags
)
  -> bool
{
  temp_allocator TempAllocator;
  auto Allocator = *TempAllocator;

  auto const& Device = Vulkan.Device;
  auto const DeviceHandle = Device.DeviceHandle;

  // Check for compatibility.
  Assert(VulkanIsImageCompatibleWithGpu(Vulkan.Gpu, Texture->Image));

  Texture->ImageFormat = ImageFormatToVulkan(Texture->Image.Format);
  LogInfo("Image format (Ours => Vulkan): %s => %s",
          ImageFormatName(Texture->Image.Format),
          VulkanEnumName(Texture->ImageFormat));

  //
  // Determine whether to use linear or optimal tiling
  //
  bool UseOptimalTiling;
  {
    auto FormatProperties = InitStruct<VkFormatProperties>();
    Vulkan.vkGetPhysicalDeviceFormatProperties(Vulkan.Gpu.GpuHandle, Texture->ImageFormat, &FormatProperties);

    bool const SupportsLinearSampling = (FormatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
    bool const SupportsOptimalSampling = (FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;

    if(SupportsLinearSampling == false && SupportsOptimalSampling == false)
    {
      LogError("The given format can neither be sampled linearly nor optimally.");
      return false;
    }

    if(ForceLinearTiling == vulkan_force_linear_tiling::Yes)
    {
      if(SupportsLinearSampling)
      {
        UseOptimalTiling = false;
      }
      else
      {
        LogWarning("Linear tiling for sampled images is requested but not supported for this format. Using optimal tiling instead.");
        UseOptimalTiling = true;
      }
    }
    else
    {
      UseOptimalTiling = SupportsOptimalSampling;
      if(!UseOptimalTiling)
      {
        LogWarning("Optimal tiling for a sampled image is not supported. Falling back to linear tiling.");
      }
    }
  }

  //
  // Image creation
  //
  {
    auto CommandBufferBeginInfo = InitStruct<VkCommandBufferBeginInfo>();
    VulkanVerify(Device.vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

    if(UseOptimalTiling)
    {
      Texture->ImageTiling = VK_IMAGE_TILING_OPTIMAL;

      auto MemoryAllocateInfo = InitStruct<VkMemoryAllocateInfo>();
      auto MemoryRequirements = InitStruct<VkMemoryRequirements>();

      // Create a host-visible staging buffer that contains the raw image data
      VkBuffer StagingBuffer{};
      VkDeviceMemory StagingMemory{};

      {
        auto BufferCreateInfo = InitStruct<VkBufferCreateInfo>();
        BufferCreateInfo.size = ImageDataSize(&Texture->Image);
        // This buffer is used as a transfer source for the buffer copy
        BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VulkanVerify(Device.vkCreateBuffer(DeviceHandle, &BufferCreateInfo, nullptr, &StagingBuffer));
      }

      // Get memory requirements for the staging buffer (alignment, memory type bits)
      Device.vkGetBufferMemoryRequirements(DeviceHandle, StagingBuffer, &MemoryRequirements);

      MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
      // Get memory type index for a host visible buffer
      MemoryAllocateInfo.memoryTypeIndex =
        VulkanDetermineMemoryTypeIndex(Vulkan.Gpu.MemoryProperties,
                                       MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

      VulkanVerify(Device.vkAllocateMemory(DeviceHandle, &MemoryAllocateInfo, nullptr, &StagingMemory));
      VulkanVerify(Device.vkBindBufferMemory(DeviceHandle, StagingBuffer, StagingMemory, 0));

      // Copy texture data into staging buffer
      {
        void* RawData{};
        VulkanVerify(Device.vkMapMemory(DeviceHandle, StagingMemory, 0, MemoryRequirements.size, 0, &RawData));

        MemCopy(ImageDataSize(&Texture->Image),
                Reinterpret<uint8*>(RawData),
                ImageDataPointer<uint8 const>(&Texture->Image));

        Device.vkUnmapMemory(DeviceHandle, StagingMemory);
      }

      // Setup buffer copy regions for each mip level
      scoped_array<VkBufferImageCopy> BufferCopyRegions{ Allocator };

      for (uint32 MipLevel = 0; MipLevel < Texture->Image.NumMipLevels; MipLevel++)
      {
        VkBufferImageCopy BufferCopyRegion = InitStruct<VkBufferImageCopy>();
        BufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        BufferCopyRegion.imageSubresource.mipLevel = MipLevel;
        BufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        BufferCopyRegion.imageSubresource.layerCount = 1;
        BufferCopyRegion.imageExtent.width = ImageWidth(&Texture->Image, MipLevel);
        BufferCopyRegion.imageExtent.height = ImageHeight(&Texture->Image, MipLevel);
        BufferCopyRegion.imageExtent.depth = 1;
        BufferCopyRegion.bufferOffset = ImageDataOffSet(&Texture->Image, MipLevel);

        Expand(&BufferCopyRegions) = BufferCopyRegion;
      }

      // Create optimal tiled target image
      auto ImageCreateInfo = InitStruct<VkImageCreateInfo>();
      {
        ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        ImageCreateInfo.format = Texture->ImageFormat;
        ImageCreateInfo.mipLevels = Texture->Image.NumMipLevels;
        ImageCreateInfo.arrayLayers = 1;
        ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageCreateInfo.extent = { Texture->Image.Width, Texture->Image.Height, 1 };
        ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | ImageUsageFlags;
      }

      VulkanVerify(Device.vkCreateImage(DeviceHandle, &ImageCreateInfo, nullptr, &Texture->ImageHandle));

      Device.vkGetImageMemoryRequirements(DeviceHandle, Texture->ImageHandle, &MemoryRequirements);

      MemoryAllocateInfo.allocationSize = MemoryRequirements.size;

      MemoryAllocateInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan.Gpu.MemoryProperties, MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VulkanVerify(Device.vkAllocateMemory(DeviceHandle, &MemoryAllocateInfo, nullptr, &Texture->MemoryHandle));
      VulkanVerify(Device.vkBindImageMemory(DeviceHandle, Texture->ImageHandle, Texture->MemoryHandle, 0));

      auto SubresourceRange = InitStruct<VkImageSubresourceRange>();
      {
        SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        SubresourceRange.baseMipLevel = 0;
        SubresourceRange.levelCount = Texture->Image.NumMipLevels;
        SubresourceRange.layerCount = 1;
      }

      // Image barrier for optimal image (target)
      // Optimal image will be used as destination for the copy
      VulkanSetImageLayout(Device, CommandBuffer,
                           Texture->ImageHandle,
                           SubresourceRange,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

      // Copy mip levels from staging buffer
      Device.vkCmdCopyBufferToImage(CommandBuffer,
                                    StagingBuffer,
                                    Texture->ImageHandle,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    Convert<uint32>(BufferCopyRegions.Num),
                                    BufferCopyRegions.Ptr);

      // Change texture image layout to shader read after all mip levels have been copied
      Texture->ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      VulkanSetImageLayout(Device, CommandBuffer,
                           Texture->ImageHandle,
                           SubresourceRange,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           Texture->ImageLayout);


      // Submit command buffer containing copy and image layout commands
      VulkanVerify(Device.vkEndCommandBuffer(CommandBuffer));

      // Create a fence to make sure that the copies have finished before continuing
      VkFence CopyFence;
      auto FenceCreateInfo = InitStruct<VkFenceCreateInfo>();
      VulkanVerify(Device.vkCreateFence(DeviceHandle, &FenceCreateInfo, nullptr, &CopyFence));

      auto SubmitInfo = InitStruct<VkSubmitInfo>();
      SubmitInfo.commandBufferCount = 1;
      SubmitInfo.pCommandBuffers = &CommandBuffer;

      VulkanVerify(Device.vkQueueSubmit(Vulkan.Queue, 1, &SubmitInfo, CopyFence));

      auto FenceTimeOut = Seconds(100);
      VulkanVerify(Device.vkWaitForFences(DeviceHandle, 1, &CopyFence, VK_TRUE, Convert<uint64>(TimeAsNanoseconds(FenceTimeOut))));

      Device.vkDestroyFence(DeviceHandle, CopyFence, nullptr);

      // Clean up staging resources
      Device.vkFreeMemory(DeviceHandle, StagingMemory, nullptr);
      Device.vkDestroyBuffer(DeviceHandle, StagingBuffer, nullptr);
    }
    else
    {
      Texture->ImageTiling = VK_IMAGE_TILING_LINEAR;

      // Only load mip level 0
      auto ImageCreateInfo = InitStruct<VkImageCreateInfo>();
      {
        ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        ImageCreateInfo.format = Texture->ImageFormat;
        ImageCreateInfo.extent = { Texture->Image.Width, Texture->Image.Height, 1 };
        ImageCreateInfo.mipLevels = 1;
        ImageCreateInfo.arrayLayers = 1;
        ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        ImageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
        ImageCreateInfo.usage = ImageUsageFlags;
        ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
      }

      VulkanVerify(Device.vkCreateImage(DeviceHandle,
                                        &ImageCreateInfo,
                                        nullptr,
                                        &Texture->ImageHandle));

      VkMemoryRequirements MemoryRequirements;
      Device.vkGetImageMemoryRequirements(DeviceHandle, Texture->ImageHandle, &MemoryRequirements);

      auto MemoryAllocateInfo = InitStruct<VkMemoryAllocateInfo>();
      MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
      MemoryAllocateInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan.Gpu.MemoryProperties,
                                                                          MemoryRequirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

      VulkanVerify(Device.vkAllocateMemory(DeviceHandle,
                                           &MemoryAllocateInfo,
                                           nullptr,
                                           &Texture->MemoryHandle));

      VulkanVerify(Device.vkBindImageMemory(DeviceHandle,
                                            Texture->ImageHandle,
                                            Texture->MemoryHandle,
                                            0)); // Offset

      {
        void* MappedData;
        VulkanVerify(Device.vkMapMemory(DeviceHandle,
                                        Texture->MemoryHandle,
                                        0,
                                        VK_WHOLE_SIZE,
                                        0,
                                        &MappedData));

        MemCopy(ImageDataSize(&Texture->Image),
                Reinterpret<uint8*>(MappedData),
                ImageDataPointer<uint8 const>(&Texture->Image));

        Device.vkUnmapMemory(DeviceHandle, Texture->MemoryHandle);
      }

      VulkanSetImageLayout(Device, CommandBuffer,
                           Texture->ImageHandle,
                           { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
                           VK_IMAGE_LAYOUT_PREINITIALIZED,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

      Texture->ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      VulkanVerify(Device.vkEndCommandBuffer(CommandBuffer));

      VkFence NullFence = {};
      auto SubmitInfo = InitStruct<VkSubmitInfo>();
      SubmitInfo.commandBufferCount = 1;
      SubmitInfo.pCommandBuffers = &CommandBuffer;

      VulkanVerify(Device.vkQueueSubmit(Vulkan.Queue, 1, &SubmitInfo, NullFence));
      VulkanVerify(Device.vkQueueWaitIdle(Vulkan.Queue));
    }
  }

  //
  // Create sampler.
  //
  {
    auto SamplerCreateInfo = InitStruct<VkSamplerCreateInfo>();
    {
      SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
      SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
      SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      SamplerCreateInfo.anisotropyEnable = VK_TRUE;
      if(UseOptimalTiling)
      {
        SamplerCreateInfo.maxLod = Cast<float>(Texture->Image.NumMipLevels);
      }
      SamplerCreateInfo.maxAnisotropy = 8;
      SamplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
      SamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
      SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    }
    VulkanVerify(Device.vkCreateSampler(DeviceHandle,
                                        &SamplerCreateInfo,
                                        nullptr,
                                        &Texture->SamplerHandle));
  }

  //
  // Create image view
  //
  {
    auto ImageViewCreateInfo = InitStruct<VkImageViewCreateInfo>();
    {
      ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      ImageViewCreateInfo.format = Texture->ImageFormat;
      ImageViewCreateInfo.components = VkComponentMapping{ VK_COMPONENT_SWIZZLE_R,
                                                           VK_COMPONENT_SWIZZLE_G,
                                                           VK_COMPONENT_SWIZZLE_B,
                                                           VK_COMPONENT_SWIZZLE_A };
      ImageViewCreateInfo.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
      if(UseOptimalTiling)
      {
        ImageViewCreateInfo.subresourceRange.levelCount = Texture->Image.NumMipLevels;
      }
      ImageViewCreateInfo.image = Texture->ImageHandle;
    }
    VulkanVerify(Device.vkCreateImageView(DeviceHandle,
                                          &ImageViewCreateInfo,
                                          nullptr,
                                          &Texture->ImageViewHandle));
  }

  return true;
}

auto
::VulkanSetQuadGeometry(vulkan* Vulkan,
                        vertex_buffer* VertexBuffer,
                        index_buffer* IndexBuffer)
  -> void
{
  auto const Device = &Vulkan->Device;
  auto const DeviceHandle = Device->DeviceHandle;

  // TODO: Make this function more generic?
  using vertex = vulkan_scene_object::vertex;

  vertex const TopLeft     { Vec3( 0.0f, -0.5f, +0.5f), Vec2(0.0f, 0.0f) };
  vertex const TopRight    { Vec3( 0.0f, +0.5f, +0.5f), Vec2(1.0f, 0.0f) };
  vertex const BottomLeft  { Vec3( 0.0f, -0.5f, -0.5f), Vec2(0.0f, 1.0f) };
  vertex const BottomRight { Vec3( 0.0f, +0.5f, -0.5f), Vec2(1.0f, 1.0f) };

  vertex const VertexArray[] =
  {
    /*0*/TopLeft,    /*1*/TopRight,
    /*2*/BottomLeft, /*3*/BottomRight,
  };
  auto Vertices = Slice(VertexArray);
  VertexBuffer->NumVertices = Cast<uint32>(Vertices.Num);

  uint32 IndexArray[] =
  {
    0, 2, 3,
    0, 3, 1,
  };
  auto Indices = Slice(IndexArray);
  IndexBuffer->NumIndices = Cast<uint32>(Indices.Num);

  // Vertex Buffer Setup
  {
    auto BufferCreateInfo = InitStruct<VkBufferCreateInfo>();
    {
      BufferCreateInfo.size = SliceByteSize(Vertices);
      BufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    VulkanVerify(Device->vkCreateBuffer(DeviceHandle, &BufferCreateInfo, nullptr, &VertexBuffer->BufferHandle));

    VkMemoryRequirements MemoryRequirements;
    Device->vkGetBufferMemoryRequirements(DeviceHandle, VertexBuffer->BufferHandle, &MemoryRequirements);

    auto MemoryAllocateInfo = InitStruct<VkMemoryAllocateInfo>();
    {
      MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
      MemoryAllocateInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan->Gpu.MemoryProperties,
                                                                          MemoryRequirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      Assert(MemoryAllocateInfo.memoryTypeIndex != IntMaxValue<uint32>());
    }

    VulkanVerify(Device->vkAllocateMemory(DeviceHandle, &MemoryAllocateInfo, nullptr, &VertexBuffer->MemoryHandle));

    // Copy data from host to the device.
    {
      void* RawData;
      VulkanVerify(Device->vkMapMemory(DeviceHandle, VertexBuffer->MemoryHandle, 0, MemoryAllocateInfo.allocationSize, 0, &RawData));

      MemCopy(Vertices.Num, Reinterpret<vertex*>(RawData), Vertices.Ptr);

      Device->vkUnmapMemory(DeviceHandle, VertexBuffer->MemoryHandle);
    }

    VulkanVerify(Device->vkBindBufferMemory(DeviceHandle, VertexBuffer->BufferHandle, VertexBuffer->MemoryHandle, 0));
  }

  // Index Buffer Setup
  {
    auto BufferCreateInfo = InitStruct<VkBufferCreateInfo>();
    {
      BufferCreateInfo.size = SliceByteSize(Indices);
      BufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    VulkanVerify(Device->vkCreateBuffer(DeviceHandle, &BufferCreateInfo, nullptr, &IndexBuffer->BufferHandle));

    VkMemoryRequirements MemoryRequirements;
    Device->vkGetBufferMemoryRequirements(DeviceHandle, IndexBuffer->BufferHandle, &MemoryRequirements);

    auto MemoryAllocateInfo = InitStruct<VkMemoryAllocateInfo>();
    {
      MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
      MemoryAllocateInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan->Gpu.MemoryProperties,
                                                                          MemoryRequirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      Assert(MemoryAllocateInfo.memoryTypeIndex != IntMaxValue<uint32>());
    }

    VulkanVerify(Device->vkAllocateMemory(DeviceHandle, &MemoryAllocateInfo, nullptr, &IndexBuffer->MemoryHandle));

    // Copy data from host to the device.
    {
      void* RawData;
      VulkanVerify(Device->vkMapMemory(DeviceHandle, IndexBuffer->MemoryHandle, 0, MemoryAllocateInfo.allocationSize, 0, &RawData));

      MemCopy(Indices.Num, Reinterpret<uint32*>(RawData), Indices.Ptr);

      Device->vkUnmapMemory(DeviceHandle, IndexBuffer->MemoryHandle);
    }

    VulkanVerify(Device->vkBindBufferMemory(DeviceHandle, IndexBuffer->BufferHandle, IndexBuffer->MemoryHandle, 0));
  }
}

auto
::VulkanSetBoxGeometry(vulkan*        Vulkan,
                       vertex_buffer* VertexBuffer,
                       index_buffer*  IndexBuffer)
  -> void
{
  auto const Device = &Vulkan->Device;
  auto const DeviceHandle = Device->DeviceHandle;

  temp_allocator TempAllocator{};
  allocator_interface* Allocator = *TempAllocator;

  using vertex = vulkan_scene_object::vertex;

  scoped_array<vertex> Vertices{ Allocator };
  SetNum(&Vertices, 8 * 6);

  scoped_array<uint32> Indices{ Allocator };

  float const P = 0.5f;  // Positive
  float const N = -0.5f; // Negative

  // Top
  {
    Vertices[ 0] = { Vec3(P, N, P), Vec2(0.0f, 0.0f) };
    Vertices[ 1] = { Vec3(P, P, P), Vec2(1.0f, 0.0f) };
    Vertices[ 2] = { Vec3(N, N, P), Vec2(0.0f, 1.0f) };
    Vertices[ 3] = { Vec3(N, P, P), Vec2(1.0f, 1.0f) };

    uint32 const FaceIndices[]
    {
      0, 2, 3,
      0, 3, 1,
    };
    Append(&Indices, Slice(FaceIndices));
  }

  // Bottom
  {
    Vertices[ 4] = { Vec3(N, N, N), Vec2(0.0f, 0.0f) };
    Vertices[ 5] = { Vec3(N, P, N), Vec2(1.0f, 0.0f) };
    Vertices[ 6] = { Vec3(P, N, N), Vec2(0.0f, 1.0f) };
    Vertices[ 7] = { Vec3(P, P, N), Vec2(1.0f, 1.0f) };

    uint32 const FaceIndices[]
    {
      4, 6, 7,
      4, 7, 5,
    };
    Append(&Indices, Slice(FaceIndices));
  }

  // Left
  {
    Vertices[ 8] = { Vec3(P, N, P), Vec2(0.0f, 0.0f) };
    Vertices[ 9] = { Vec3(N, N, P), Vec2(1.0f, 0.0f) };
    Vertices[10] = { Vec3(P, N, N), Vec2(0.0f, 1.0f) };
    Vertices[11] = { Vec3(N, N, N), Vec2(1.0f, 1.0f) };

    uint32 const FaceIndices[]
    {
      8, 10, 11,
      8, 11, 9,
    };
    Append(&Indices, Slice(FaceIndices));
  }

  // Right
  {
    Vertices[12] = { Vec3(N, P, P), Vec2(0.0f, 0.0f) };
    Vertices[13] = { Vec3(P, P, P), Vec2(1.0f, 0.0f) };
    Vertices[14] = { Vec3(N, P, N), Vec2(0.0f, 1.0f) };
    Vertices[15] = { Vec3(P, P, N), Vec2(1.0f, 1.0f) };

    uint32 const FaceIndices[]
    {
      12, 14, 15,
      12, 15, 13,
    };
    Append(&Indices, Slice(FaceIndices));
  }

  // Front
  {
    Vertices[16] = { Vec3(N, N, P), Vec2(0.0f, 0.0f) };
    Vertices[17] = { Vec3(N, P, P), Vec2(1.0f, 0.0f) };
    Vertices[18] = { Vec3(N, N, N), Vec2(0.0f, 1.0f) };
    Vertices[19] = { Vec3(N, P, N), Vec2(1.0f, 1.0f) };

    uint32 const FaceIndices[]
    {
      16, 18, 19,
      16, 19, 17,
    };
    Append(&Indices, Slice(FaceIndices));
  }

  // Back
  {
    Vertices[20] = { Vec3(P, P, P), Vec2(0.0f, 0.0f) };
    Vertices[21] = { Vec3(P, N, P), Vec2(1.0f, 0.0f) };
    Vertices[22] = { Vec3(P, P, N), Vec2(0.0f, 1.0f) };
    Vertices[23] = { Vec3(P, N, N), Vec2(1.0f, 1.0f) };

    uint32 const FaceIndices[]
    {
      20, 22, 23,
      20, 23, 21,
    };
    Append(&Indices, Slice(FaceIndices));
  }

  VertexBuffer->NumVertices = Cast<uint32>(Vertices.Num);
  IndexBuffer->NumIndices = Cast<uint32>(Indices.Num);

  // Vertex Buffer Setup
  {
    auto BufferCreateInfo = InitStruct<VkBufferCreateInfo>();
    {
      BufferCreateInfo.size = SliceByteSize(Slice(&Vertices));
      BufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    VulkanVerify(Device->vkCreateBuffer(DeviceHandle, &BufferCreateInfo, nullptr, &VertexBuffer->BufferHandle));

    VkMemoryRequirements MemoryRequirements;
    Device->vkGetBufferMemoryRequirements(DeviceHandle, VertexBuffer->BufferHandle, &MemoryRequirements);

    auto MemoryAllocateInfo = InitStruct<VkMemoryAllocateInfo>();
    {
      MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
      MemoryAllocateInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan->Gpu.MemoryProperties,
                                                                          MemoryRequirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      Assert(MemoryAllocateInfo.memoryTypeIndex != IntMaxValue<uint32>());
    }

    VulkanVerify(Device->vkAllocateMemory(DeviceHandle, &MemoryAllocateInfo, nullptr, &VertexBuffer->MemoryHandle));

    // Copy data from host to the device.
    {
      void* RawData;
      VulkanVerify(Device->vkMapMemory(DeviceHandle, VertexBuffer->MemoryHandle, 0, MemoryAllocateInfo.allocationSize, 0, &RawData));

      MemCopy(Vertices.Num, Reinterpret<vertex*>(RawData), Vertices.Ptr);

      Device->vkUnmapMemory(DeviceHandle, VertexBuffer->MemoryHandle);
    }

    VulkanVerify(Device->vkBindBufferMemory(DeviceHandle, VertexBuffer->BufferHandle, VertexBuffer->MemoryHandle, 0));
  }

  // Index Buffer Setup
  {
    auto BufferCreateInfo = InitStruct<VkBufferCreateInfo>();
    {
      BufferCreateInfo.size = SliceByteSize(Slice(&Indices));
      BufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    VulkanVerify(Device->vkCreateBuffer(DeviceHandle, &BufferCreateInfo, nullptr, &IndexBuffer->BufferHandle));

    VkMemoryRequirements MemoryRequirements;
    Device->vkGetBufferMemoryRequirements(DeviceHandle, IndexBuffer->BufferHandle, &MemoryRequirements);

    auto MemoryAllocateInfo = InitStruct<VkMemoryAllocateInfo>();
    {
      MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
      MemoryAllocateInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan->Gpu.MemoryProperties,
                                                                          MemoryRequirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      Assert(MemoryAllocateInfo.memoryTypeIndex != IntMaxValue<uint32>());
    }

    VulkanVerify(Device->vkAllocateMemory(DeviceHandle, &MemoryAllocateInfo, nullptr, &IndexBuffer->MemoryHandle));

    // Copy data from host to the device.
    {
      void* RawData;
      VulkanVerify(Device->vkMapMemory(DeviceHandle, IndexBuffer->MemoryHandle, 0, MemoryAllocateInfo.allocationSize, 0, &RawData));

      MemCopy(Indices.Num, Reinterpret<uint32*>(RawData), Indices.Ptr);

      Device->vkUnmapMemory(DeviceHandle, IndexBuffer->MemoryHandle);
    }

    VulkanVerify(Device->vkBindBufferMemory(DeviceHandle, IndexBuffer->BufferHandle, IndexBuffer->MemoryHandle, 0));
  }

}

auto
::VulkanSetDebugGridGeometry(vulkan*        Vulkan,
                             vec3           HalfExtents,
                             vec3_<uint>    NumSamples,
                             vertex_buffer* VertexBuffer,
                             index_buffer*  IndexBuffer)
  -> void
{
  temp_allocator TempAllocator;
  allocator_interface* Allocator = *TempAllocator;

  auto const Device = &Vulkan->Device;
  auto const DeviceHandle = Device->DeviceHandle;

  using vertex = vulkan_debug_grid::vertex;

  scoped_array<vertex> Vertices{ Allocator };
  size_t const TotalNumVertices = 2 * (NumSamples.X + NumSamples.Y + NumSamples.Z);
  Reserve(&Vertices, TotalNumVertices);

  // X Axis
  for(uint Index = 0; Index < NumSamples.X; ++Index)
  {
    auto Progress = 2 * HalfExtents.X * NormalizeValue<float>(Index, uint(0), NumSamples.X - 1) - HalfExtents.X;

    {
      auto& Start = Expand(&Vertices);
      Start.VertexPosition.X = Progress;
      Start.VertexPosition.Y = -HalfExtents.Y;
      Start.VertexPosition.Z = 0;
      Start.VertexColor = color::Red;
    }

    {
      auto& End = Expand(&Vertices);
      End.VertexPosition.X = Progress;
      End.VertexPosition.Y = HalfExtents.Y;
      End.VertexPosition.Z = 0;
      End.VertexColor = color::Red;
    }
  }

  // Y Axis
  for(uint Index = 0; Index < NumSamples.Y; ++Index)
  {
    auto Progress = 2 * HalfExtents.Y * NormalizeValue<float>(Index, uint(0), NumSamples.Y - 1) - HalfExtents.Y;

    {
      auto& Start = Expand(&Vertices);
      Start.VertexPosition.X = -HalfExtents.X;
      Start.VertexPosition.Y = Progress;
      Start.VertexPosition.Z = 0;
      Start.VertexColor = color::Lime;
    }

    {
      auto& End = Expand(&Vertices);
      End.VertexPosition.X = HalfExtents.X;
      End.VertexPosition.Y = Progress;
      End.VertexPosition.Z = 0;
      End.VertexColor = color::Lime;
    }
  }

  // Z Axis
  for(uint Index = 0; Index < NumSamples.Z; ++Index)
  {
    auto Progress = 2 * HalfExtents.X * NormalizeValue<float>(Index, uint(0), NumSamples.X - 1) - HalfExtents.X;

    {
      auto& Start = Expand(&Vertices);
      Start.VertexPosition.X = Progress;
      Start.VertexPosition.Y = -HalfExtents.Y;
      Start.VertexPosition.Z = 0;
      Start.VertexColor = color::Blue;
    }

    {
      auto& End = Expand(&Vertices);
      End.VertexPosition.X = Progress;
      End.VertexPosition.Y = -HalfExtents.Y;
      End.VertexPosition.Z = 2 * HalfExtents.Z;
      End.VertexColor = color::Blue;
    }
  }

  VertexBuffer->NumVertices = Cast<uint32>(Vertices.Num);

  scoped_array<uint32> Indices{ Allocator };
  SetNum(&Indices, Vertices.Num);

  for(uint32 Index = 0; Index < Indices.Num; ++Index)
  {
    Indices[Index] = Index;
  }

  IndexBuffer->NumIndices = Cast<uint32>(Indices.Num);

  // Vertex Buffer Setup
  {
    auto BufferCreateInfo = InitStruct<VkBufferCreateInfo>();
    {
      BufferCreateInfo.size = SliceByteSize(Slice(&Vertices));
      BufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    VulkanVerify(Device->vkCreateBuffer(DeviceHandle, &BufferCreateInfo, nullptr, &VertexBuffer->BufferHandle));

    VkMemoryRequirements MemoryRequirements;
    Device->vkGetBufferMemoryRequirements(DeviceHandle, VertexBuffer->BufferHandle, &MemoryRequirements);

    auto MemoryAllocateInfo = InitStruct<VkMemoryAllocateInfo>();
    {
      MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
      MemoryAllocateInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan->Gpu.MemoryProperties,
                                                                          MemoryRequirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      Assert(MemoryAllocateInfo.memoryTypeIndex != IntMaxValue<uint32>());
    }

    VulkanVerify(Device->vkAllocateMemory(DeviceHandle, &MemoryAllocateInfo, nullptr, &VertexBuffer->MemoryHandle));

    // Copy data from host to the device.
    {
      void* RawData;
      VulkanVerify(Device->vkMapMemory(DeviceHandle, VertexBuffer->MemoryHandle, 0, MemoryAllocateInfo.allocationSize, 0, &RawData));

      MemCopy(Vertices.Num, Reinterpret<vertex*>(RawData), Vertices.Ptr);

      Device->vkUnmapMemory(DeviceHandle, VertexBuffer->MemoryHandle);
    }

    VulkanVerify(Device->vkBindBufferMemory(DeviceHandle, VertexBuffer->BufferHandle, VertexBuffer->MemoryHandle, 0));
  }

  // Index Buffer Setup
  {
    auto BufferCreateInfo = InitStruct<VkBufferCreateInfo>();
    {
      BufferCreateInfo.size = SliceByteSize(Slice(&Indices));
      BufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    VulkanVerify(Device->vkCreateBuffer(DeviceHandle, &BufferCreateInfo, nullptr, &IndexBuffer->BufferHandle));

    VkMemoryRequirements MemoryRequirements;
    Device->vkGetBufferMemoryRequirements(DeviceHandle, IndexBuffer->BufferHandle, &MemoryRequirements);

    auto MemoryAllocateInfo = InitStruct<VkMemoryAllocateInfo>();
    {
      MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
      MemoryAllocateInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan->Gpu.MemoryProperties,
                                                                          MemoryRequirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      Assert(MemoryAllocateInfo.memoryTypeIndex != IntMaxValue<uint32>());
    }

    VulkanVerify(Device->vkAllocateMemory(DeviceHandle, &MemoryAllocateInfo, nullptr, &IndexBuffer->MemoryHandle));

    // Copy data from host to the device.
    {
      void* RawData;
      VulkanVerify(Device->vkMapMemory(DeviceHandle, IndexBuffer->MemoryHandle, 0, MemoryAllocateInfo.allocationSize, 0, &RawData));

      MemCopy(Indices.Num, Reinterpret<uint32*>(RawData), Indices.Ptr);

      Device->vkUnmapMemory(DeviceHandle, IndexBuffer->MemoryHandle);
    }

    VulkanVerify(Device->vkBindBufferMemory(DeviceHandle, IndexBuffer->BufferHandle, IndexBuffer->MemoryHandle, 0));
  }
}

auto
::Init(vulkan_texture2d* Texture, allocator_interface* Allocator)
  -> void
{
  Init(&Texture->Image, Allocator);
}

auto
::Finalize(vulkan_texture2d* Texture)
  -> void
{
  Finalize(&Texture->Image);
}

auto
::ImplVulkanCreateShaderBuffer(vulkan* Vulkan, vulkan_shader_buffer_header* ShaderBuffer, size_t NumBytesForBuffer, bool IsReadOnlyForShader)
  -> void
{
  auto const& Device = Vulkan->Device;
  auto const DeviceHandle = Device.DeviceHandle;

  auto BufferCreateInfo = InitStruct<VkBufferCreateInfo>();
  BufferCreateInfo.size = NumBytesForBuffer;

  if(IsReadOnlyForShader)
  {
    ShaderBuffer->DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    BufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }
  else
  {
    ShaderBuffer->DescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    BufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  }

  VulkanVerify(Device.vkCreateBuffer(DeviceHandle, &BufferCreateInfo, nullptr,
                                     &ShaderBuffer->BufferHandle));

  VkMemoryRequirements MemoryRequirements;
  Device.vkGetBufferMemoryRequirements(DeviceHandle, ShaderBuffer->BufferHandle,
                                       &MemoryRequirements);

  auto MemoryAllocationInfo = InitStruct<VkMemoryAllocateInfo>();
  {
    MemoryAllocationInfo.allocationSize = MemoryRequirements.size;
    MemoryAllocationInfo.memoryTypeIndex = VulkanDetermineMemoryTypeIndex(Vulkan->Device.Gpu->MemoryProperties,
                                                                          MemoryRequirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    Assert(MemoryAllocationInfo.memoryTypeIndex != IntMaxValue<uint32>());
  }

  VulkanVerify(Device.vkAllocateMemory(DeviceHandle, &MemoryAllocationInfo, nullptr, &ShaderBuffer->MemoryHandle));

  VulkanVerify(Device.vkBindBufferMemory(DeviceHandle,
                                         ShaderBuffer->BufferHandle,
                                         ShaderBuffer->MemoryHandle,
                                         0));
}

auto
::VulkanEnsureIsReadyForDrawing(vulkan* Vulkan, vulkan_renderable* Renderable)
  -> void
{
  if(!Renderable->IsDirty)
    return;

  Renderable->PrepareForDrawing(Vulkan);
  Renderable->IsDirty = false;
}

auto
::ImplVulkanUploadBufferData(vulkan_device const& Device, vulkan_shader_buffer_header const& ShaderBuffer, slice<void const> Data)
  -> void
{
  void* RawTarget;
  VulkanVerify(Device.vkMapMemory(Device.DeviceHandle,
                                  ShaderBuffer.MemoryHandle,
                                  0, // offset
                                  VK_WHOLE_SIZE,
                                  0, // flags
                                  &RawTarget));

  MemCopy(Data.Num, RawTarget, Data.Ptr);

  Device.vkUnmapMemory(Device.DeviceHandle, ShaderBuffer.MemoryHandle);
}
