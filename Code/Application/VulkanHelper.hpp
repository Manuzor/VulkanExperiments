
#include <Backbone.hpp>

#include <Core/DynamicArray.hpp>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <Windows.h>

#include "VulkanHelper.inl"

struct vulkan;

struct vulkan_gpu
{
  vulkan* Vulkan;
  VkPhysicalDevice GpuHandle;

  VkPhysicalDeviceProperties Properties;
  VkPhysicalDeviceMemoryProperties MemoryProperties;
  VkPhysicalDeviceFeatures Features;

  dynamic_array<VkQueueFamilyProperties> QueueProperties;
};

struct vulkan_device : public vulkan_device_functions
{
  vulkan* Vulkan;
  vulkan_gpu* Gpu;
  VkDevice DeviceHandle;
};

struct swapchain_buffer
{
  VkImage Image;
  VkCommandBuffer Command;
  VkImageView View;
};

struct depth
{
  VkFormat Format;

  VkImage Image;
  VkDeviceMemory Memory;
  VkImageView View;
};

struct gpu_image
{
  VkImage ImageHandle;
  VkImageLayout ImageLayout;
  VkFormat ImageFormat;

  VkDeviceMemory MemoryHandle;
};

struct texture
{
  VkSampler SamplerHandle;
  gpu_image GpuImage;
  VkImageView ImageViewHandle;
};

struct vertex_buffer
{
  VkBuffer Buffer;
  VkDeviceMemory Memory;

  uint32 BindID;
  uint32 NumVertices;
  fixed_block<1, VkVertexInputBindingDescription> VertexInputBindingDescs;
  fixed_block<2, VkVertexInputAttributeDescription> VertexInputAttributeDescs;
};

struct index_buffer
{
  VkBuffer Buffer;
  VkDeviceMemory Memory;

  uint32 NumIndices;
};

struct vulkan : public vulkan_instance_functions
{
  bool IsPrepared;

  HMODULE DLL;
  fixed_block<KiB(1), char> DLLNameBuffer;
  slice<char> DLLName;

  VkInstance InstanceHandle;

  VkDebugReportCallbackEXT DebugCallbackHandle;

  vulkan_gpu Gpu;
  uint32 QueueNodeIndex = IntMaxValue<uint32>();

  vulkan_device Device;

  VkSurfaceKHR Surface;

  VkQueue Queue;

  VkFormat Format;
  VkColorSpaceKHR ColorSpace;

  //
  // Swapchain Data
  //
  uint32 Width;
  uint32 Height;

  VkCommandPool CommandPool;
  VkCommandBuffer SetupCommand;
  VkCommandBuffer DrawCommand;

  VkSwapchainKHR Swapchain;
  uint32 SwapchainImageCount;
  dynamic_array<swapchain_buffer> SwapchainBuffers;
  uint32 CurrentBufferIndex;

  depth Depth;

  //
  // Misc
  //
  texture Texture;

  vertex_buffer Vertices;
  index_buffer Indices;

  VkPipelineLayout PipelineLayout;
  VkDescriptorSetLayout DescriptorSetLayout;

  VkRenderPass RenderPass;
  VkPipeline Pipeline;

  VkDescriptorPool DescriptorPool;
  VkDescriptorSet DescriptorSet;

  dynamic_array<VkFramebuffer> Framebuffers;

  VkBuffer GlobalUBO_BufferHandle;
  VkDeviceMemory GlobalUBO_MemoryHandle;

  float DepthStencilValue = 1.0f;
};

void
Init(vulkan* Vulkan, allocator_interface* Allocator);

void Finalize(vulkan* Vulkan);


/// Loads the DLL and some crucial functions needed to create an instance.
bool
VulkanLoadDLL(vulkan* Vulkan);

void
VulkanLoadInstanceFunctions(vulkan* Vulkan);

void
VulkanLoadDeviceFunctions(vulkan_device* Device);

char const*
VulkanEnumName(VkFormat Format);

char const*
VulkanEnumName(VkColorSpaceKHR ColorSpace);

template<typename T>
constexpr auto
VulkanStruct() -> decltype(impl_vulkan_struct<rm_ref<T>>::Do())
{
  // Note: impl_vulkan_struct is found in VulkanHelper.inl
  return impl_vulkan_struct<rm_ref<T>>::Do();
}

void
VulkanSetImageLayout(vulkan_device const& Device,
                     VkCommandPool CommandPool,
                     VkCommandBuffer* CommandBuffer,
                     VkImage Image,
                     VkImageAspectFlags AspectMask,
                     VkImageLayout OldImageLayout,
                     VkImageLayout NewImageLayout,
                     VkAccessFlags SourceAccessMask);

uint32
VulkanDetermineMemoryTypeIndex(VkPhysicalDeviceMemoryProperties const& MemoryProperties,
                               uint32 TypeBits,
                               VkFlags RequirementsMask);

void
VulkanEnsureSetupCommandIsReady(vulkan_device const& Device,
                                VkCommandPool CommandPool,
                                VkCommandBuffer* CommandBuffer);

void
VulkanFlushSetupCommand(vulkan_device const& Device,
                        VkQueue Queue,
                        VkCommandPool CommandPool,
                        VkCommandBuffer* CommandBuffer);

void
VulkanVerify(VkResult Result);

bool
VulkanIsImageCompatibleWithGpu(vulkan_gpu const& Gpu, struct image const& Image);

VkFormat
ImageFormatToVulkan(enum class image_format KrepelFormat);

enum class image_format
ImageFormatFromVulkan(VkFormat VulkanFormat);

bool
VulkanUploadImageToGpu(allocator_interface* TempAllocator,
                       vulkan_device const& Device,
                       VkCommandPool CommandPool,
                       VkCommandBuffer* CommandBuffer,
                       image const& Image,
                       gpu_image* GpuImage);
