
struct vulkan_instance_functions
{
  PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
  PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
  PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
  PFN_vkAllocateMemory vkAllocateMemory;
  PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
  PFN_vkBindBufferMemory vkBindBufferMemory;
  PFN_vkBindImageMemory vkBindImageMemory;
  PFN_vkCmdBeginQuery vkCmdBeginQuery;
  PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
  PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
  PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
  PFN_vkCmdBindPipeline vkCmdBindPipeline;
  PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
  PFN_vkCmdBlitImage vkCmdBlitImage;
  PFN_vkCmdClearAttachments vkCmdClearAttachments;
  PFN_vkCmdClearColorImage vkCmdClearColorImage;
  PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage;
  PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
  PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
  PFN_vkCmdCopyImage vkCmdCopyImage;
  PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
  PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults;
  PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT;
  PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT;
  PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXT;
  PFN_vkCmdDispatch vkCmdDispatch;
  PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect;
  PFN_vkCmdDraw vkCmdDraw;
  PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
  PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
  PFN_vkCmdDrawIndirect vkCmdDrawIndirect;
  PFN_vkCmdEndQuery vkCmdEndQuery;
  PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
  PFN_vkCmdExecuteCommands vkCmdExecuteCommands;
  PFN_vkCmdFillBuffer vkCmdFillBuffer;
  PFN_vkCmdNextSubpass vkCmdNextSubpass;
  PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
  PFN_vkCmdPushConstants vkCmdPushConstants;
  PFN_vkCmdResetEvent vkCmdResetEvent;
  PFN_vkCmdResetQueryPool vkCmdResetQueryPool;
  PFN_vkCmdResolveImage vkCmdResolveImage;
  PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants;
  PFN_vkCmdSetDepthBias vkCmdSetDepthBias;
  PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds;
  PFN_vkCmdSetEvent vkCmdSetEvent;
  PFN_vkCmdSetLineWidth vkCmdSetLineWidth;
  PFN_vkCmdSetScissor vkCmdSetScissor;
  PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask;
  PFN_vkCmdSetStencilReference vkCmdSetStencilReference;
  PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask;
  PFN_vkCmdSetViewport vkCmdSetViewport;
  PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer;
  PFN_vkCmdWaitEvents vkCmdWaitEvents;
  PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp;
  PFN_vkCreateBuffer vkCreateBuffer;
  PFN_vkCreateBufferView vkCreateBufferView;
  PFN_vkCreateCommandPool vkCreateCommandPool;
  PFN_vkCreateComputePipelines vkCreateComputePipelines;
  PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
  PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
  PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
  PFN_vkCreateDevice vkCreateDevice;
  PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR;
  PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR;
  PFN_vkCreateEvent vkCreateEvent;
  PFN_vkCreateFence vkCreateFence;
  PFN_vkCreateFramebuffer vkCreateFramebuffer;
  PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
  PFN_vkCreateImage vkCreateImage;
  PFN_vkCreateImageView vkCreateImageView;
  PFN_vkCreateInstance vkCreateInstance;
  PFN_vkCreatePipelineCache vkCreatePipelineCache;
  PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
  PFN_vkCreateQueryPool vkCreateQueryPool;
  PFN_vkCreateRenderPass vkCreateRenderPass;
  PFN_vkCreateSampler vkCreateSampler;
  PFN_vkCreateSemaphore vkCreateSemaphore;
  PFN_vkCreateShaderModule vkCreateShaderModule;
  PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR;
  PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
  PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
  PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT;
  PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTagEXT;
  PFN_vkDebugReportCallbackEXT vkDebugReportCallbackEXT;
  PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;
  PFN_vkDestroyBuffer vkDestroyBuffer;
  PFN_vkDestroyBufferView vkDestroyBufferView;
  PFN_vkDestroyCommandPool vkDestroyCommandPool;
  PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
  PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
  PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
  PFN_vkDestroyDevice vkDestroyDevice;
  PFN_vkDestroyEvent vkDestroyEvent;
  PFN_vkDestroyFence vkDestroyFence;
  PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
  PFN_vkDestroyImage vkDestroyImage;
  PFN_vkDestroyImageView vkDestroyImageView;
  PFN_vkDestroyInstance vkDestroyInstance;
  PFN_vkDestroyPipeline vkDestroyPipeline;
  PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
  PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
  PFN_vkDestroyQueryPool vkDestroyQueryPool;
  PFN_vkDestroyRenderPass vkDestroyRenderPass;
  PFN_vkDestroySampler vkDestroySampler;
  PFN_vkDestroySemaphore vkDestroySemaphore;
  PFN_vkDestroyShaderModule vkDestroyShaderModule;
  PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
  PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
  PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
  PFN_vkEndCommandBuffer vkEndCommandBuffer;
  PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
  PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties;
  PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
  PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
  PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
  PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
  PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
  PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
  PFN_vkFreeMemory vkFreeMemory;
  PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
  PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment;
  PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
  PFN_vkGetDeviceQueue vkGetDeviceQueue;
  PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR;
  PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR;
  PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR;
  PFN_vkGetEventStatus vkGetEventStatus;
  PFN_vkGetFenceStatus vkGetFenceStatus;
  PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
  PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements;
  PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
  PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR;
  PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR;
  PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
  PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
  PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties;
  PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
  PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
  PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
  PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties;
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
  PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
  PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
  PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;
  PFN_vkGetPipelineCacheData vkGetPipelineCacheData;
  PFN_vkGetQueryPoolResults vkGetQueryPoolResults;
  PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity;
  PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
  PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
  PFN_vkMapMemory vkMapMemory;
  PFN_vkMergePipelineCaches vkMergePipelineCaches;
  PFN_vkQueueBindSparse vkQueueBindSparse;
  PFN_vkQueuePresentKHR vkQueuePresentKHR;
  PFN_vkQueueSubmit vkQueueSubmit;
  PFN_vkQueueWaitIdle vkQueueWaitIdle;
  PFN_vkResetCommandBuffer vkResetCommandBuffer;
  PFN_vkResetCommandPool vkResetCommandPool;
  PFN_vkResetDescriptorPool vkResetDescriptorPool;
  PFN_vkResetEvent vkResetEvent;
  PFN_vkResetFences vkResetFences;
  PFN_vkSetEvent vkSetEvent;
  PFN_vkUnmapMemory vkUnmapMemory;
  PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
  PFN_vkWaitForFences vkWaitForFences;
};

struct vulkan_device_functions
{
  PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
  PFN_vkDestroyDevice vkDestroyDevice;
  PFN_vkGetDeviceQueue vkGetDeviceQueue;
  PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
  PFN_vkAllocateMemory vkAllocateMemory;
  PFN_vkFreeMemory vkFreeMemory;
  PFN_vkMapMemory vkMapMemory;
  PFN_vkUnmapMemory vkUnmapMemory;
  PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
  PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
  PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment;
  PFN_vkBindBufferMemory vkBindBufferMemory;
  PFN_vkBindImageMemory vkBindImageMemory;
  PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
  PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
  PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements;
  PFN_vkCreateFence vkCreateFence;
  PFN_vkDestroyFence vkDestroyFence;
  PFN_vkResetFences vkResetFences;
  PFN_vkGetFenceStatus vkGetFenceStatus;
  PFN_vkWaitForFences vkWaitForFences;
  PFN_vkCreateSemaphore vkCreateSemaphore;
  PFN_vkDestroySemaphore vkDestroySemaphore;
  PFN_vkCreateEvent vkCreateEvent;
  PFN_vkDestroyEvent vkDestroyEvent;
  PFN_vkGetEventStatus vkGetEventStatus;
  PFN_vkSetEvent vkSetEvent;
  PFN_vkResetEvent vkResetEvent;
  PFN_vkCreateQueryPool vkCreateQueryPool;
  PFN_vkDestroyQueryPool vkDestroyQueryPool;
  PFN_vkGetQueryPoolResults vkGetQueryPoolResults;
  PFN_vkCreateBuffer vkCreateBuffer;
  PFN_vkDestroyBuffer vkDestroyBuffer;
  PFN_vkCreateBufferView vkCreateBufferView;
  PFN_vkDestroyBufferView vkDestroyBufferView;
  PFN_vkCreateImage vkCreateImage;
  PFN_vkDestroyImage vkDestroyImage;
  PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout;
  PFN_vkCreateImageView vkCreateImageView;
  PFN_vkDestroyImageView vkDestroyImageView;
  PFN_vkCreateShaderModule vkCreateShaderModule;
  PFN_vkDestroyShaderModule vkDestroyShaderModule;
  PFN_vkCreatePipelineCache vkCreatePipelineCache;
  PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
  PFN_vkGetPipelineCacheData vkGetPipelineCacheData;
  PFN_vkMergePipelineCaches vkMergePipelineCaches;
  PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
  PFN_vkCreateComputePipelines vkCreateComputePipelines;
  PFN_vkDestroyPipeline vkDestroyPipeline;
  PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
  PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
  PFN_vkCreateSampler vkCreateSampler;
  PFN_vkDestroySampler vkDestroySampler;
  PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
  PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
  PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
  PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
  PFN_vkResetDescriptorPool vkResetDescriptorPool;
  PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
  PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
  PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
  PFN_vkCreateFramebuffer vkCreateFramebuffer;
  PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
  PFN_vkCreateRenderPass vkCreateRenderPass;
  PFN_vkDestroyRenderPass vkDestroyRenderPass;
  PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity;
  PFN_vkCreateCommandPool vkCreateCommandPool;
  PFN_vkDestroyCommandPool vkDestroyCommandPool;
  PFN_vkResetCommandPool vkResetCommandPool;
  PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
  PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
  PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
  PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
  PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
  PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
  PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR;
  PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTagEXT;
  PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT;
  PFN_vkQueueSubmit vkQueueSubmit;
  PFN_vkQueueWaitIdle vkQueueWaitIdle;
  PFN_vkQueueBindSparse vkQueueBindSparse;
  PFN_vkQueuePresentKHR vkQueuePresentKHR;
  PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
  PFN_vkEndCommandBuffer vkEndCommandBuffer;
  PFN_vkResetCommandBuffer vkResetCommandBuffer;
  PFN_vkCmdBindPipeline vkCmdBindPipeline;
  PFN_vkCmdSetViewport vkCmdSetViewport;
  PFN_vkCmdSetScissor vkCmdSetScissor;
  PFN_vkCmdSetLineWidth vkCmdSetLineWidth;
  PFN_vkCmdSetDepthBias vkCmdSetDepthBias;
  PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants;
  PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds;
  PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask;
  PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask;
  PFN_vkCmdSetStencilReference vkCmdSetStencilReference;
  PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
  PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
  PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
  PFN_vkCmdDraw vkCmdDraw;
  PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
  PFN_vkCmdDrawIndirect vkCmdDrawIndirect;
  PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
  PFN_vkCmdDispatch vkCmdDispatch;
  PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect;
  PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
  PFN_vkCmdCopyImage vkCmdCopyImage;
  PFN_vkCmdBlitImage vkCmdBlitImage;
  PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
  PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
  PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer;
  PFN_vkCmdFillBuffer vkCmdFillBuffer;
  PFN_vkCmdClearColorImage vkCmdClearColorImage;
  PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage;
  PFN_vkCmdClearAttachments vkCmdClearAttachments;
  PFN_vkCmdResolveImage vkCmdResolveImage;
  PFN_vkCmdSetEvent vkCmdSetEvent;
  PFN_vkCmdResetEvent vkCmdResetEvent;
  PFN_vkCmdWaitEvents vkCmdWaitEvents;
  PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
  PFN_vkCmdBeginQuery vkCmdBeginQuery;
  PFN_vkCmdEndQuery vkCmdEndQuery;
  PFN_vkCmdResetQueryPool vkCmdResetQueryPool;
  PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp;
  PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults;
  PFN_vkCmdPushConstants vkCmdPushConstants;
  PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
  PFN_vkCmdNextSubpass vkCmdNextSubpass;
  PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
  PFN_vkCmdExecuteCommands vkCmdExecuteCommands;
  PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT;
  PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT;
  PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXT;
};

template<>
struct impl_init_struct<VkApplicationInfo>
{
  static constexpr VkApplicationInfo
  Create() { return { VK_STRUCTURE_TYPE_APPLICATION_INFO }; }
};

template<>
struct impl_init_struct<VkInstanceCreateInfo>
{
  static constexpr VkInstanceCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkDeviceQueueCreateInfo>
{
  static constexpr VkDeviceQueueCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkDeviceCreateInfo>
{
  static constexpr VkDeviceCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkSubmitInfo>
{
  static constexpr VkSubmitInfo
  Create() { return { VK_STRUCTURE_TYPE_SUBMIT_INFO }; }
};

template<>
struct impl_init_struct<VkMemoryAllocateInfo>
{
  static constexpr VkMemoryAllocateInfo
  Create() { return { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; }
};

template<>
struct impl_init_struct<VkMappedMemoryRange>
{
  static constexpr VkMappedMemoryRange
  Create() { return { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE }; }
};

template<>
struct impl_init_struct<VkBindSparseInfo>
{
  static constexpr VkBindSparseInfo
  Create() { return { VK_STRUCTURE_TYPE_BIND_SPARSE_INFO }; }
};

template<>
struct impl_init_struct<VkFenceCreateInfo>
{
  static constexpr VkFenceCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkSemaphoreCreateInfo>
{
  static constexpr VkSemaphoreCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkEventCreateInfo>
{
  static constexpr VkEventCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_EVENT_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkQueryPoolCreateInfo>
{
  static constexpr VkQueryPoolCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkBufferCreateInfo>
{
  static constexpr VkBufferCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkBufferViewCreateInfo>
{
  static constexpr VkBufferViewCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkImageCreateInfo>
{
  static constexpr VkImageCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkImageViewCreateInfo>
{
  static constexpr VkImageViewCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkShaderModuleCreateInfo>
{
  static constexpr VkShaderModuleCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineCacheCreateInfo>
{
  static constexpr VkPipelineCacheCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineShaderStageCreateInfo>
{
  static constexpr VkPipelineShaderStageCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineVertexInputStateCreateInfo>
{
  static constexpr VkPipelineVertexInputStateCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineInputAssemblyStateCreateInfo>
{
  static constexpr VkPipelineInputAssemblyStateCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineTessellationStateCreateInfo>
{
  static constexpr VkPipelineTessellationStateCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineViewportStateCreateInfo>
{
  static constexpr VkPipelineViewportStateCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineRasterizationStateCreateInfo>
{
  static constexpr VkPipelineRasterizationStateCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineMultisampleStateCreateInfo>
{
  static constexpr VkPipelineMultisampleStateCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineDepthStencilStateCreateInfo>
{
  static constexpr VkPipelineDepthStencilStateCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineColorBlendStateCreateInfo>
{
  static constexpr VkPipelineColorBlendStateCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineDynamicStateCreateInfo>
{
  static constexpr VkPipelineDynamicStateCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkGraphicsPipelineCreateInfo>
{
  static constexpr VkGraphicsPipelineCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkComputePipelineCreateInfo>
{
  static constexpr VkComputePipelineCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkPipelineLayoutCreateInfo>
{
  static constexpr VkPipelineLayoutCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkSamplerCreateInfo>
{
  static constexpr VkSamplerCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkDescriptorSetLayoutCreateInfo>
{
  static constexpr VkDescriptorSetLayoutCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkDescriptorPoolCreateInfo>
{
  static constexpr VkDescriptorPoolCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkDescriptorSetAllocateInfo>
{
  static constexpr VkDescriptorSetAllocateInfo
  Create() { return { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO }; }
};

template<>
struct impl_init_struct<VkWriteDescriptorSet>
{
  static constexpr VkWriteDescriptorSet
  Create() { return { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET }; }
};

template<>
struct impl_init_struct<VkCopyDescriptorSet>
{
  static constexpr VkCopyDescriptorSet
  Create() { return { VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET }; }
};

template<>
struct impl_init_struct<VkFramebufferCreateInfo>
{
  static constexpr VkFramebufferCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkRenderPassCreateInfo>
{
  static constexpr VkRenderPassCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkCommandPoolCreateInfo>
{
  static constexpr VkCommandPoolCreateInfo
  Create() { return { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO }; }
};

template<>
struct impl_init_struct<VkCommandBufferAllocateInfo>
{
  static constexpr VkCommandBufferAllocateInfo
  Create() { return { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; }
};

template<>
struct impl_init_struct<VkCommandBufferInheritanceInfo>
{
  static constexpr VkCommandBufferInheritanceInfo
  Create() { return { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO }; }
};

template<>
struct impl_init_struct<VkCommandBufferBeginInfo>
{
  static constexpr VkCommandBufferBeginInfo
  Create() { return { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; }
};

template<>
struct impl_init_struct<VkRenderPassBeginInfo>
{
  static constexpr VkRenderPassBeginInfo
  Create() { return { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO }; }
};

template<>
struct impl_init_struct<VkBufferMemoryBarrier>
{
  static constexpr VkBufferMemoryBarrier
  Create() { return { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER }; }
};

template<>
struct impl_init_struct<VkImageMemoryBarrier>
{
  static VkImageMemoryBarrier
  Create()
  {
    VkImageMemoryBarrier Result{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    Result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    return Result;
  }
};

template<>
struct impl_init_struct<VkMemoryBarrier>
{
  static constexpr VkMemoryBarrier
  Create() { return { VK_STRUCTURE_TYPE_MEMORY_BARRIER }; }
};

// VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO?
// VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO?
template<>
struct impl_init_struct<VkSwapchainCreateInfoKHR>
{
  static constexpr VkSwapchainCreateInfoKHR
  Create() { return { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR }; }
};

template<>
struct impl_init_struct<VkPresentInfoKHR>
{
  static constexpr VkPresentInfoKHR
  Create() { return { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR }; }
};

template<>
struct impl_init_struct<VkDisplayModeCreateInfoKHR>
{
  static constexpr VkDisplayModeCreateInfoKHR
  Create() { return { VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR }; }
};

template<>
struct impl_init_struct<VkDisplaySurfaceCreateInfoKHR>
{
  static constexpr VkDisplaySurfaceCreateInfoKHR
  Create() { return { VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR }; }
};

template<>
struct impl_init_struct<VkDisplayPresentInfoKHR>
{
  static constexpr VkDisplayPresentInfoKHR
  Create() { return { VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR }; }
};

// template<>
// struct impl_init_struct<VkXlibSurfaceCreateInfoKHR>
// {
//   static constexpr VkXlibSurfaceCreateInfoKHR
//   Create() { return { VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR }; }
// };

// template<>
// struct impl_init_struct<VkXcbSurfaceCreateInfoKHR>
// {
//   static constexpr VkXcbSurfaceCreateInfoKHR
//   Create() { return { VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR }; }
// };

// template<>
// struct impl_init_struct<VkWaylandSurfaceCreateInfoKHR>
// {
//   static constexpr VkWaylandSurfaceCreateInfoKHR
//   Create() { return { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR }; }
// };

// template<>
// struct impl_init_struct<VkMirSurfaceCreateInfoKHR>
// {
//   static constexpr VkMirSurfaceCreateInfoKHR
//   Create() { return { VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR }; }
// };

// template<>
// struct impl_init_struct<VkAndroidSurfaceCreateInfoKHR>
// {
//   static constexpr VkAndroidSurfaceCreateInfoKHR
//   Create() { return { VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR }; }
// };

template<>
struct impl_init_struct<VkWin32SurfaceCreateInfoKHR>
{
  static constexpr VkWin32SurfaceCreateInfoKHR
  Create() { return { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR }; }
};

template<>
struct impl_init_struct<VkDebugReportCallbackCreateInfoEXT>
{
  static constexpr VkDebugReportCallbackCreateInfoEXT
  Create() { return { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT }; }
};

template<>
struct impl_init_struct<VkPipelineRasterizationStateRasterizationOrderAMD>
{
  static constexpr VkPipelineRasterizationStateRasterizationOrderAMD
  Create() { return { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD }; }
};

template<>
struct impl_init_struct<VkDebugMarkerObjectNameInfoEXT>
{
  static constexpr VkDebugMarkerObjectNameInfoEXT
  Create() { return { VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT }; }
};

template<>
struct impl_init_struct<VkDebugMarkerObjectTagInfoEXT>
{
  static constexpr VkDebugMarkerObjectTagInfoEXT
  Create() { return { VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT }; }
};

template<>
struct impl_init_struct<VkDebugMarkerMarkerInfoEXT>
{
  static constexpr VkDebugMarkerMarkerInfoEXT
  Create() { return { VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT }; }
};

template<>
struct impl_init_struct<VkComponentMapping>
{
  static constexpr VkComponentMapping
  Create()
  {
    return { VK_COMPONENT_SWIZZLE_R,
             VK_COMPONENT_SWIZZLE_G,
             VK_COMPONENT_SWIZZLE_B,
             VK_COMPONENT_SWIZZLE_A };
  }
};
