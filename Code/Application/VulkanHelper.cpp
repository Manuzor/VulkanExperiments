#include "VulkanHelper.hpp"

#include <Core/Log.hpp>


void
Init(vulkan* Vulkan, allocator_interface* Allocator)
{
  Init(&Vulkan->Gpu.QueueProperties, Allocator);
  Init(&Vulkan->SwapchainBuffers, Allocator);
  Init(&Vulkan->Framebuffers, Allocator);
  Vulkan->Gpu.Vulkan = Vulkan;
}

void Finalize(vulkan* Vulkan)
{
  Finalize(&Vulkan->Framebuffers);
  Finalize(&Vulkan->SwapchainBuffers);
  Finalize(&Vulkan->Gpu.QueueProperties);
}

template<typename T>
bool
LoadHelper(vulkan* Vulkan, char const* FuncName, T** OutPtrToProcPtr)
{
  auto ProcPtr = Vulkan->vkGetInstanceProcAddr(Vulkan->InstanceHandle, FuncName);
  if(ProcPtr)
  {
    *OutPtrToProcPtr = Reinterpret<T*>(ProcPtr);
    return true;
  }
  return false;
}

template<typename T>
bool
LoadHelper(vulkan_device* Device, char const* FuncName, T** OutPtrToProcPtr)
{
  auto ProcPtr = Device->Vulkan->vkGetDeviceProcAddr(Device->DeviceHandle, FuncName);
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
  Defer(=, LogEndScope(""));

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
  Defer(=, LogEndScope(""));

  #define TRY_LOAD(Name) if(!LoadHelper(Vulkan, #Name, &Vulkan->##Name)) \
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
::VulkanLoadDeviceFunctions(vulkan_device* Device)
  -> void
{
  LogBeginScope("Loading Vulkan device procedures.");
  Defer(=, LogEndScope(""));

  #define TRY_LOAD(Name) if(!LoadHelper(Device, #Name, &Device->##Name)) \
  { \
    LogWarning("Unable to load device function: %s", #Name); \
    Device->##Name = Device->Vulkan->##Name; \
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
::VulkanEnumToString(VkFormat Format)
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
::VulkanEnumToString(VkColorSpaceKHR ColorSpace)
  -> char const*
{
  switch(ColorSpace)
  {
    case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR";
    default: return "<UNKNOWN>";
  }
}
