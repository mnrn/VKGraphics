/**
 * @brief Swapchain
 */

#pragma once

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <limits>
#include <vector>

struct Device;

struct Swapchain {
  VkSwapchainKHR handle = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  std::vector<VkImage> images;
  std::vector<VkImageView> views;
  VkFormat format;
  VkExtent2D extent;
  uint32_t queueFamilyIndex = std::numeric_limits<uint32_t>::max();

  void Init(VkInstance instance, GLFWwindow *window,
            VkPhysicalDevice physicalDevice);
  void Destroy(VkInstance instance, VkDevice device);
  void Create(const Device &device, int width, int height, bool vsync = false);

  VkResult AcquiredNextImage(VkDevice device,
                             VkSemaphore presentCompleteSemaphore,
                             uint32_t *pImageIndex) const;
  VkResult QueuePresent(VkQueue queue, uint32_t imageIndex,
                        VkSemaphore waitSemaphore = VK_NULL_HANDLE) const;

  operator VkSwapchainKHR() const noexcept { return handle; }
};
