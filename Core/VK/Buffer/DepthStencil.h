#pragma once

#include <vulkan/vulkan.h>

#include <optional>

struct Instance;
struct Swapchain;

struct DepthStencil {
  static std::optional<VkFormat>
  FindDepthFormat(VkPhysicalDevice physicalDevice);
  static bool HasStencilComponent(VkFormat format);

  void Create(const Instance &instance, const Swapchain &swapchain);
  void Destroy(const Instance& instance);

  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
};
