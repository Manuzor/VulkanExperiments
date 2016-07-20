
#include <Backbone.hpp>

#include <Core/DynamicArray.hpp>
#include <Core/Math.hpp>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <Windows.h>

#include "VulkanHelper.inl"

struct image;
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
  VkDevice DeviceHandle;
  vulkan_gpu* Gpu;
};

struct vulkan_depth
{
  VkFormat Format;

  VkImage Image;
  VkDeviceMemory Memory;
  VkImageView View;
};

struct vulkan_uniform_buffer
{
  VkBuffer Buffer;
  VkDeviceMemory Memory;
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
};

struct index_buffer
{
  VkBuffer Buffer;
  VkDeviceMemory Memory;

  uint32 NumIndices;
};

struct vertex
{
  vec3 Position;
  vec2 TexCoord;
};

struct scene_object
{
  bool IsCreated;

  texture Texture;
  vertex_buffer Vertices;
  index_buffer Indices;
  VkPipeline Pipeline;
};

struct vulkan_queue_node
{
  uint32 Index = IntMaxValue<uint32>();
};

struct vulkan_surface
{
  VkSurfaceKHR SurfaceHandle;
  VkFormat Format;
  VkColorSpaceKHR ColorSpace;
  vulkan_queue_node PresentNode;
};

struct vulkan_swapchain
{
  VkSwapchainKHR SwapchainHandle;
  vulkan_device* Device;
  vulkan_surface* Surface;
  extent2_<uint32> Extent;

  uint32 ImageCount;
  dynamic_array<VkImage> Images;
  dynamic_array<VkImageView> ImageViews;
};

enum class vsync : bool { Off = false, On = true };

struct vulkan_swapchain_image
{
  uint32 Index;
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

  vulkan_device Device;
  vulkan_surface Surface;
  vulkan_swapchain Swapchain;
  vulkan_depth Depth;

  VkCommandPool CommandPool;

  VkQueue Queue;

  //
  // Swapchain Data
  //

  VkCommandBuffer SetupCommand;
  VkCommandBuffer DrawCommand;

  //
  // Misc
  //
  VkPipelineLayout PipelineLayout;
  VkDescriptorSetLayout DescriptorSetLayout;

  VkRenderPass RenderPass;
  VkDescriptorPool DescriptorPool;
  VkDescriptorSet DescriptorSet;
  VkPipeline Pipeline;

  dynamic_array<VkFramebuffer> Framebuffers;
  vulkan_swapchain_image CurrentSwapchainImage;

  vulkan_uniform_buffer GlobalsUBO;

  float DepthStencilValue = 1.0f;

  dynamic_array<scene_object> SceneObjects;
};


//
// `vulkan`
//

void
Init(vulkan*              Vulkan,
     allocator_interface* Allocator);

void
Finalize(vulkan* Vulkan);


/// Loads the DLL and some crucial functions needed to create an instance.
bool
VulkanLoadDLL(vulkan* Vulkan);

void
VulkanLoadInstanceFunctions(vulkan* Vulkan);

scene_object*
VulkanCreateSceneObject(vulkan* Vulkan);

void
VulkanDestroySceneObject(vulkan*       Vulkan,
                         scene_object* SceneObject);

bool
VulkanSetTextureFromFile(vulkan*     Vulkan,
                         char const* FileName,
                         texture*    Texture);

void
VulkanSetQuadGeometry(vulkan*        Vulkan,
                      extent2 const& Extents,
                      vertex_buffer* Vertices,
                      index_buffer*  Indices);


//
// `vulkan_device`
//

void
VulkanLoadDeviceFunctions(vulkan const& Vulkan, vulkan_device* Device);

void
VulkanSetImageLayout(vulkan_device const& Device,
                     VkCommandPool        CommandPool,
                     VkCommandBuffer*     CommandBuffer,
                     VkImage              Image,
                     VkImageAspectFlags   AspectMask,
                     VkImageLayout        OldImageLayout,
                     VkImageLayout        NewImageLayout,
                     VkAccessFlags        SourceAccessMask);

void
VulkanEnsureSetupCommandIsReady(vulkan_device const& Device,
                                VkCommandPool        CommandPool,
                                VkCommandBuffer*     CommandBuffer);

void
VulkanFlushSetupCommand(vulkan_device const& Device,
                        VkQueue              Queue,
                        VkCommandPool        CommandPool,
                        VkCommandBuffer*     CommandBuffer);

bool
VulkanUploadImageToGpu(vulkan_device const& Device,
                       VkCommandPool        CommandPool,
                       VkCommandBuffer*     CommandBuffer,
                       image const&         Image,
                       gpu_image*           GpuImage);


//
// `vulkan_swapchain`
//

void
Init(vulkan_swapchain*    Swapchain,
     vulkan_device*       Device,
     allocator_interface* Allocator);

void
Finalize(vulkan_swapchain* Swapchain);

void
VulkanSwapchainConnect(vulkan_swapchain* Swapchain,
                       vulkan_device* Device,
                       vulkan_surface* Surface);

bool
VulkanPrepareSurface(vulkan const&   Vulkan,
                     vulkan_surface* Surface,
                     HINSTANCE       ProcessHandle,
                     HWND            WindowHandle);

void
VulkanCleanupSurface(vulkan const&   Vulkan,
                     vulkan_surface* Surface);

bool
VulkanPrepareSwapchain(vulkan const&     Vulkan,
                       vulkan_swapchain* Swapchain,
                       extent2_<uint32>  Extents,
                       vsync             VSync);

void
VulkanCleanupSwapchain(vulkan const& Vulkan, vulkan_swapchain* Swapchain);

VkResult
VulkanAcquireNextSwapchainImage(vulkan_swapchain const& Swapchain,
                                VkSemaphore             PresentCompleteSemaphore,
                                vulkan_swapchain_image* CurrentImage);

VkResult
VulkanQueuePresent(vulkan_swapchain*      Swapchain,
                   VkQueue                Queue,
                   vulkan_swapchain_image ImageToPresent,
                   VkSemaphore            WaitSemaphore);

bool
VulkanPrepareDepth(vulkan*          Vulkan,
                   vulkan_depth*    Depth,
                   extent2_<uint32> Extents);

void
VulkanCleanupDepth(vulkan const& Vulkan,
                   vulkan_depth* Depth);


//
// Misc
//

char const*
VulkanEnumName(VkFormat Format);

char const*
VulkanEnumName(VkColorSpaceKHR ColorSpace);

uint32
VulkanDetermineMemoryTypeIndex(VkPhysicalDeviceMemoryProperties const& MemoryProperties,
                               uint32                                  TypeBits,
                               VkFlags                                 RequirementsMask);

void
VulkanVerify(VkResult Result);

bool
VulkanIsImageCompatibleWithGpu(vulkan_gpu const& Gpu,
                               image const&      Image);

VkFormat
ImageFormatToVulkan(enum class image_format KrepelFormat);

enum class image_format
ImageFormatFromVulkan(VkFormat VulkanFormat);
