
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

struct vulkan
{
  bool IsPrepared;

  HMODULE DLL;
  fixed_block<KiB(1), char> DLLNameBuffer;
  slice<char> DLLName;

  VkInstance InstanceHandle;

  VkDebugReportCallbackEXT DebugCallbackHandle;

  vulkan_gpu Gpu;

  vulkan_instance_functions F;
};

struct vulkan_device
{
  vulkan* Vulkan;
  vulkan_gpu* Gpu;
  VkDevice DeviceHandle;

  vulkan_device_functions F;
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
