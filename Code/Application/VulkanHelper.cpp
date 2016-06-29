#include "VulkanHelper.hpp"

#include <Core/Log.hpp>


void
Init(vulkan* Vulkan, allocator_interface* Allocator)
{
  Init(&Vulkan->Gpu.QueueProperties, Allocator);
}

void Finalize(vulkan* Vulkan)
{
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

  fixed_block<KiB(1), char> BufferMemory;
  auto Buffer = Slice(BufferMemory);
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
  TRY_LOAD(vkFreeFunction);
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
