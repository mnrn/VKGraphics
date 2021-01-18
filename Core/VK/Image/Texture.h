/**
 * @brief Pipeline
 */
#pragma once

#include <vulkan/vulkan.h>

#include <string>

struct Instance;
struct Pipelines;

struct Texture {
  void Create(const Instance &instance, const std::string &filepath);
  void Destroy(const Instance &instance) const;

  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
};
