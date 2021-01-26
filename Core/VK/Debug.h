/**
 * @brief Debug for Vulkan
 */

#pragma once

#include <vulkan/vulkan.h>

struct DebugMessenger {
public:
  static VkDebugUtilsMessengerCreateInfoEXT ExtractCreateInfo();
  void Setup(VkInstance);
  void Cleanup(VkInstance) const;

  VkDebugUtilsMessengerEXT msg;

private:
  static VkResult Create(VkInstance instance,
                         const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                         const VkAllocationCallbacks *pAllocator,
                         VkDebugUtilsMessengerEXT *pDebugMessenger);
  static void Destroy(VkInstance instance,
                      VkDebugUtilsMessengerEXT debugMessenger,
                      const VkAllocationCallbacks *pAllocator);
};
