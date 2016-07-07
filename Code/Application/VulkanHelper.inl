
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

template<typename T>
struct impl_vulkan_struct
{
  // Return an instance of T, filled with zeroes.
  static constexpr T Do() { return {}; }
};

#define IMPL_HELPER(Type, Enum) template<>\
struct impl_vulkan_struct<Type>\
{\
  static constexpr Type Do() { return { Enum }; }\
}


IMPL_HELPER(VkApplicationInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO);
IMPL_HELPER(VkInstanceCreateInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
IMPL_HELPER(VkDeviceQueueCreateInfo, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
IMPL_HELPER(VkDeviceCreateInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
IMPL_HELPER(VkSubmitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO);
IMPL_HELPER(VkMemoryAllocateInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
IMPL_HELPER(VkMappedMemoryRange, VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE);
IMPL_HELPER(VkBindSparseInfo, VK_STRUCTURE_TYPE_BIND_SPARSE_INFO);
IMPL_HELPER(VkFenceCreateInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
IMPL_HELPER(VkSemaphoreCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
IMPL_HELPER(VkEventCreateInfo, VK_STRUCTURE_TYPE_EVENT_CREATE_INFO);
IMPL_HELPER(VkQueryPoolCreateInfo, VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO);
IMPL_HELPER(VkBufferCreateInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
IMPL_HELPER(VkBufferViewCreateInfo, VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO);
IMPL_HELPER(VkImageCreateInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
IMPL_HELPER(VkImageViewCreateInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
IMPL_HELPER(VkShaderModuleCreateInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
IMPL_HELPER(VkPipelineCacheCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO);
IMPL_HELPER(VkPipelineShaderStageCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
IMPL_HELPER(VkPipelineVertexInputStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
IMPL_HELPER(VkPipelineInputAssemblyStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
IMPL_HELPER(VkPipelineTessellationStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO);
IMPL_HELPER(VkPipelineViewportStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
IMPL_HELPER(VkPipelineRasterizationStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
IMPL_HELPER(VkPipelineMultisampleStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
IMPL_HELPER(VkPipelineDepthStencilStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
IMPL_HELPER(VkPipelineColorBlendStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
IMPL_HELPER(VkPipelineDynamicStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
IMPL_HELPER(VkGraphicsPipelineCreateInfo, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
IMPL_HELPER(VkComputePipelineCreateInfo, VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);
IMPL_HELPER(VkPipelineLayoutCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
IMPL_HELPER(VkSamplerCreateInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
IMPL_HELPER(VkDescriptorSetLayoutCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
IMPL_HELPER(VkDescriptorPoolCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
IMPL_HELPER(VkDescriptorSetAllocateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
IMPL_HELPER(VkWriteDescriptorSet, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
IMPL_HELPER(VkCopyDescriptorSet, VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET);
IMPL_HELPER(VkFramebufferCreateInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
IMPL_HELPER(VkRenderPassCreateInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
IMPL_HELPER(VkCommandPoolCreateInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
IMPL_HELPER(VkCommandBufferAllocateInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
IMPL_HELPER(VkCommandBufferInheritanceInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
IMPL_HELPER(VkCommandBufferBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
IMPL_HELPER(VkRenderPassBeginInfo, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
IMPL_HELPER(VkBufferMemoryBarrier, VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER);
IMPL_HELPER(VkImageMemoryBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
IMPL_HELPER(VkMemoryBarrier, VK_STRUCTURE_TYPE_MEMORY_BARRIER);
// VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO?
// VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO?
IMPL_HELPER(VkSwapchainCreateInfoKHR, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
IMPL_HELPER(VkPresentInfoKHR, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
IMPL_HELPER(VkDisplayModeCreateInfoKHR, VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR);
IMPL_HELPER(VkDisplaySurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR);
IMPL_HELPER(VkDisplayPresentInfoKHR, VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR);
//IMPL_HELPER(VkXlibSurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR);
//IMPL_HELPER(VkXcbSurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR);
//IMPL_HELPER(VkWaylandSurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR);
//IMPL_HELPER(VkMirSurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR);
//IMPL_HELPER(VkAndroidSurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR);
IMPL_HELPER(VkWin32SurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
IMPL_HELPER(VkDebugReportCallbackCreateInfoEXT, VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT);
IMPL_HELPER(VkPipelineRasterizationStateRasterizationOrderAMD, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD);
IMPL_HELPER(VkDebugMarkerObjectNameInfoEXT, VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT);
IMPL_HELPER(VkDebugMarkerObjectTagInfoEXT, VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT);
IMPL_HELPER(VkDebugMarkerMarkerInfoEXT, VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT);


#undef IMPL_HELPER
