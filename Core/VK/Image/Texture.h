/**
 * @brief Pipeline
 */
#pragma once

#include <vulkan/vulkan.h>

#include <string>

struct Device;

struct Texture {
  void Create(const Device& device, const std::string &filepath);
  void Destroy(const Device& instance) const;

  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
};
