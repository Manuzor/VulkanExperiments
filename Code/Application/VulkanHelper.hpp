
#include <Backbone.hpp>

#include <Core/DynamicArray.hpp>
#include <Core/Math.hpp>
#include <Core/Color.hpp>
#include <Core/Image.hpp>
#include <Core/String.hpp>

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

enum class vulkan_buffer_type
{
  Vertex,
  Index,
};

struct vertex_buffer
{
  VkBuffer BufferHandle;
  VkDeviceMemory MemoryHandle;

  uint32 NumVertices;
};

struct index_buffer
{
  VkBuffer BufferHandle;
  VkDeviceMemory MemoryHandle;

  uint32 NumIndices;
};

struct vulkan_renderable_foo
{
  struct ubo_globals_data
  {
    mat4x4 ViewProjectionMatrix;
  };

  using ubo_globals = vulkan_shader_buffer<ubo_globals_data>;

  compiled_shader* Shader;

  VkDescriptorSetLayout DescriptorSetLayout;
  VkPipelineLayout PipelineLayout;
  VkPipeline Pipeline;
  ubo_globals UboGlobals;
};

struct vulkan_renderable
{
  arc_string Name; // Some name to identify this renderable.
  bool IsDirty = true; // Flag indicating whether the data needs to be (re-) uploaded to the GPU.

  vulkan_renderable_foo* Foo{};

  VkDescriptorSet DescriptorSet{};

  virtual void PrepareForDrawing(struct vulkan* Vulkan);

  virtual void Draw(struct vulkan* Vulkan,
               VkCommandBuffer CommandBuffer) = 0;
};

struct vulkan_scene_object : public vulkan_renderable
{
  struct ubo_model_data
  {
    mat4x4 ViewProjectionMatrix;
  };

  using ubo_model = vulkan_shader_buffer<ubo_model_data>;

  struct vertex
  {
    vec3 VertexPosition;
    vec2 VertexTextureCoordinates;
  };

  vertex_buffer VertexBuffer{};
  index_buffer IndexBuffer{};
  vulkan_texture2d Texture{};

  ubo_model UboModel{};

  transform Transform{ IdentityTransform };

  virtual void PrepareForDrawing(struct vulkan* Vulkan) override;
  virtual void Draw(struct vulkan* Vulkan, VkCommandBuffer CommandBuffer) override;
};

struct vulkan_debug_grid : public vulkan_renderable
{
  struct vertex
  {
    vec3 VertexPosition;
    color_linear VertexColor;
  };

  vertex_buffer VertexBuffer{};
  index_buffer IndexBuffer{};

  virtual void PrepareForDrawing(struct vulkan* Vulkan) override;
  virtual void Draw(struct vulkan* Vulkan, VkCommandBuffer CommandBuffer) override;
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

enum class vsync : bool { Off = false, On = true };

struct vulkan_swapchain
{
  VkSwapchainKHR SwapchainHandle;
  vulkan_device* Device;
  vulkan_surface* Surface;
  extent2_<uint32> Extent;
  vsync VSync;

  uint32 ImageCount; // TODO: Rename => NumImages
  dynamic_array<VkImage> Images;
  dynamic_array<VkImageView> ImageViews;
};

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
  // Renderable objects
  //
  allocator_interface* RenderableAllocator;

  vulkan_renderable_foo SceneObjectsFoo;
  vulkan_renderable_foo DebugGridsFoo;

  dynamic_array<vulkan_scene_object*> SceneObjects;
  dynamic_array<vulkan_debug_grid*> DebugGrids;
  dynamic_array<vulkan_renderable*> Renderables;


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

vulkan_scene_object*
VulkanCreateSceneObject(vulkan* Vulkan, slice<char const> Name);

void
VulkanDestroySceneObject(vulkan*              Vulkan,
                         vulkan_scene_object* SceneObject);

vulkan_debug_grid*
VulkanCreateDebugGrid(vulkan* Vulkan, slice<char const> Name);

void
VulkanDestroyDebugGrid(vulkan*            Vulkan,
                       vulkan_debug_grid* SceneObject);

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
                      vertex_buffer* VertexBuffer,
                      index_buffer*  IndexBuffer);

void
VulkanSetBoxGeometry(vulkan*        Vulkan,
                     vertex_buffer* VertexBuffer,
                     index_buffer*  IndexBuffer);

void
VulkanSetDebugGridGeometry(vulkan*        Vulkan,
                           vec3           HalfExtents,
                           vec3_<uint>    NumSamples,
                           vertex_buffer* VertexBuffer,
                           index_buffer*  IndexBuffer);

#if 0
void
VulkanPrepareSceneObjectForRendering(vulkan const*        Vulkan,
                                     vulkan_scene_object* SceneObject);
#endif


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
                       extent2_<uint>    Extents,
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
ImplVulkanCreateShaderBuffer(vulkan* Vulkan, vulkan_shader_buffer_header* ShaderBuffer, size_t NumBytesForBuffer, bool IsReadOnlyForShader);

enum class is_read_only_for_shader : bool { No = false, Yes = true };

template<typename T>
void
VulkanCreateShaderBuffer(vulkan* Vulkan, vulkan_shader_buffer<T>* ShaderBuffer, is_read_only_for_shader IsReadOnlyForShader)
{
  ImplVulkanCreateShaderBuffer(Vulkan, ShaderBuffer, SizeOf<T>(), IsReadOnlyForShader == is_read_only_for_shader::Yes);
}

void
VulkanEnsureIsReadyForDrawing(vulkan* Vulkan, vulkan_renderable* Renderable);

void
ImplVulkanUploadBufferData(vulkan_device const& Device, vulkan_shader_buffer_header const& ShaderBuffer, slice<void const> Data);

template<typename Type>
void
VulkanUploadShaderBufferData(vulkan* Vulkan, vulkan_shader_buffer<Type> const& ShaderBuffer)
{
  auto Data = Slice(1, &ShaderBuffer.Data);
  auto RawData = SliceReinterpret<void const>(Data);
  ImplVulkanUploadBufferData(Vulkan->Device,
                             ShaderBuffer,
                             RawData);
}
