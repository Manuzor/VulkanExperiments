
#include <Backbone.hpp>

#include <Core/DynamicArray.hpp>
#include <Core/Math.hpp>
#include <Core/Color.hpp>
#include <Core/Image.hpp>

#include "ShaderManager.hpp"

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
  VkDevice DeviceHandle;
  vulkan_gpu* Gpu;
};

struct vulkan_depth
{
  VkFormat Format;

  VkImage Image;
  extent2_<uint32> Extent;
  VkDeviceMemory Memory;
  VkImageView View;
};

struct vulkan_shader_buffer_header
{
  VkDescriptorType DescriptorType;
  VkBuffer BufferHandle;
  VkDeviceMemory MemoryHandle;
};

template<typename DataType>
struct vulkan_shader_buffer : public vulkan_shader_buffer_header
{
  DataType Data;
};

struct vulkan_texture2d
{
  VkSampler SamplerHandle;
  VkImageView ImageViewHandle;
  VkImageTiling ImageTiling;
  VkImageLayout ImageLayout;
  VkFormat ImageFormat;
  VkImage ImageHandle;

  VkDeviceMemory MemoryHandle;

  image Image;
};

struct vertex_buffer
{
  VkBuffer Buffer;
  VkDeviceMemory Memory;

  uint32 NumVertices;
};

struct index_buffer
{
  VkBuffer Buffer;
  VkDeviceMemory Memory;

  uint32 NumIndices;
};

struct vulkan_scene_object_vertex
{
  vec3 VertexPosition;
  vec2 VertexTextureCoordinates;
};

struct vulkan_debug_grid_vertex
{
  vec3 VertexPosition;
  color_linear VertexColor;
};

struct vulkan_scene_object_shader_globals
{
  mat4x4 ViewProjectionMatrix;
};

struct vulkan_scene_object_gfx_state
{
  VkPipeline Pipeline;
  VkPipelineLayout PipelineLayout;
  VkDescriptorSetLayout DescriptorSetLayout;
  vulkan_shader_buffer<vulkan_scene_object_shader_globals> GlobalsUBO;

  dynamic_array<VkVertexInputBindingDescription> VertexInputBindingDescs;
  dynamic_array<VkVertexInputAttributeDescription> VertexInputAttributeDescs;
};

struct vulkan_scene_object
{
  bool IsAllocated;
  size_t NumExternalReferences;

  vulkan_texture2d Texture;
  vertex_buffer Vertices;
  index_buffer Indices;

  VkPipeline Pipeline;
  VkDescriptorSet DescriptorSet;
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

  uint32 ImageCount; // TODO: Rename => NumImages
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

  shader_manager* ShaderManager;
  allocator_interface* ShaderManagerAllocator;

  VkInstance InstanceHandle;

  VkDebugReportCallbackEXT DebugCallbackHandle;

  vulkan_gpu Gpu;

  vulkan_device Device;
  vulkan_surface Surface;
  vulkan_swapchain Swapchain;
  vulkan_depth Depth;

  VkDescriptorPool DescriptorPool;
  VkCommandPool CommandPool;

  VkQueue Queue;

  vulkan_swapchain_image CurrentSwapchainImage;

  VkPipelineCache PipelineCache;

  VkRenderPass RenderPass;

  dynamic_array<VkFramebuffer> Framebuffers;


  //
  // Command Buffers
  //
  VkCommandBuffer SetupCommandBuffer;
  // TODO Combine arrays into one and assign slices instead.
  dynamic_array<VkCommandBuffer> DrawCommandBuffers;
  dynamic_array<VkCommandBuffer> PrePresentCommandBuffers;
  dynamic_array<VkCommandBuffer> PostPresentCommandBuffers;


  //
  // Semaphores
  //
  VkSemaphore PresentCompleteSemaphore;
  VkSemaphore RenderCompleteSemaphore;


  //
  // Scene Objects
  //
  vulkan_scene_object_gfx_state SceneObjectGraphicsState;

  dynamic_array<vulkan_scene_object> SceneObjects;
  size_t NumExternalSceneObjectPtrs;


  //
  // Misc
  //

  float DepthStencilValue = 1.0f;
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

DefineOpaqueHandle(vulkan_scene_object_handle);

vulkan_scene_object_handle
VulkanCreateSceneObject(vulkan* Vulkan, allocator_interface* Allocator);

void
VulkanDestroyAndDeallocateSceneObject(vulkan*                    Vulkan,
                                      vulkan_scene_object_handle SceneObject);

vulkan_scene_object*
VulkanBeginAccess(vulkan* Vulkan, vulkan_scene_object_handle SceneObjectHandle);

void
VulkanEndAccess(vulkan* Vulkan, vulkan_scene_object* SceneObject);

enum class vulkan_force_linear_tiling : bool { No = false, Yes = true };

bool
VulkanUploadTexture(
  vulkan const&              Vulkan,
  VkCommandBuffer            CommandBuffer,
  vulkan_texture2d*          Texture,
  vulkan_force_linear_tiling ForceLinearTiling = vulkan_force_linear_tiling::No,
  VkImageUsageFlags          ImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT
);

void
VulkanSetQuadGeometry(vulkan*        Vulkan,
                      extent2 const& Extents,
                      vertex_buffer* Vertices,
                      index_buffer*  Indices);

void
VulkanSetDebugGridGeometry(vulkan*        Vulkan,
                           extent2 const& Extents,
                           vertex_buffer* Vertices,
                           index_buffer*  Indices);

void
VulkanPrepareSceneObjectForRendering(vulkan const*        Vulkan,
                                     vulkan_scene_object* SceneObject);


//
// `vulkan_device`
//

void
VulkanLoadDeviceFunctions(vulkan const& Vulkan, vulkan_device* Device);

void
VulkanSetImageLayout(vulkan_device const&    Device,
                     VkCommandBuffer         CommandBuffer,
                     VkImage                 Image,
                     VkImageSubresourceRange SubresourceRange,
                     VkImageLayout           OldImageLayout,
                     VkImageLayout           NewImageLayout);

void
VulkanPrepareSetupCommandBuffer(vulkan* Vulkan);

enum class flush_command_buffer { No, Yes };

void
VulkanCleanupSetupCommandBuffer(vulkan* Vulkan, flush_command_buffer Flush);

#if 0
bool
VulkanUploadImageToGpu(vulkan_device const& Device,
                       VkCommandBuffer      CommandBuffer,
                       image const&         Image,
                       gpu_image*           GpuImage);
#endif


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

void
Init(vulkan_texture2d* Texture, allocator_interface* Allocator);

void
Finalize(vulkan_texture2d* Texture);


void
ImplCreateShaderBuffer(vulkan* Vulkan, vulkan_shader_buffer_header* ShaderBuffer, size_t NumBytesForBuffer, bool IsReadOnlyForShader);

enum class is_read_only_for_shader : bool { No = false, Yes = true };

template<typename T>
void
CreateShaderBuffer(vulkan* Vulkan, vulkan_shader_buffer<T>* ShaderBuffer, is_read_only_for_shader IsReadOnlyForShader)
{
  ImplCreateShaderBuffer(Vulkan, ShaderBuffer, SizeOf<T>(), IsReadOnlyForShader == is_read_only_for_shader::Yes);
}
