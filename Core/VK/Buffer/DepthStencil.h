#pragma once

#include <vulkan/vulkan.h>

#include <optional>

struct Instance;
struct Swapchain;

struct DepthStencil {
  static std::optional<VkFormat> FindDepthFormat(VkPhysicalDevice physicalDevice);

  void Create(const Instance &, const Swapchain &);

  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
};
