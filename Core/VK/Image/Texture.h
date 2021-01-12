/**
 * @brief Pipeline
 */
#pragma once

#include <vulkan/vulkan.h>

#include <string>

struct Instance;
struct Pipelines;

struct Texture {
  void Load(const Instance &instance, const std::string &filepath);
  void CreateDescriptorSet(const Instance &instance,
                           const Pipelines &pipelines);
  void Cleanup(const Instance &instance) const;

  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};
