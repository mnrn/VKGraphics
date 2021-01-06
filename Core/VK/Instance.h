/**
 * @brief Vulkan Instance
 */

#pragma once

#include <set>
#include <vulkan/vulkan.h>

struct Instance {
  Instance() = default;
  Instance(const Instance &) = delete;
  Instance(Instance &&) = delete;
  Instance &operator=(const Instance &) = delete;
  Instance &operator=(Instance &&) = delete;

  const VkInstance &Get() const;
  VkInstance *Set();
  void Cleanup();

  VkInstance instance = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkCommandPool pool = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;

  struct {
    VkQueue graphics = VK_NULL_HANDLE;
    VkQueue presentation = VK_NULL_HANDLE;
  } queues;

  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties properties;
};

struct QueueFamilies {
  int graphics = -1;
  int presentation = -1;

  bool IsComplete() const;
  std::set<int> UniqueFamilies() const;

  static QueueFamilies Find(VkPhysicalDevice physicakDevice,
                            VkSurfaceKHR surface);
  static QueueFamilies Find(const Instance &instance);
};
