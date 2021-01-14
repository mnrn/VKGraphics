/**
 * @brief Swapchain
 */

#pragma once

#include <vulkan/vulkan.h>

#include <vector>

struct Instance;

struct Swapchain {
  VkSwapchainKHR handle = VK_NULL_HANDLE;
  std::vector<VkImage> images;
  std::vector<VkImageView> views;
  VkFormat format;
  VkExtent2D extent;

  void Create(const Instance &instance, int width, int height, bool forceFifo = false);
  [[nodiscard]]const VkSwapchainKHR &Get() const;
  void Destroy(const Instance &instance);
};
