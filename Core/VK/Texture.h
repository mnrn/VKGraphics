/**
 * @brief Pipeline
 */
#pragma once

#include <vulkan/vulkan.h>

#include <string>

struct Device;

struct Texture {
  void Destroy(const Device &device) const;

  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo descriptor{};
};

struct Texture2D : public Texture {
  void
  Load(const Device &device, const std::string &filepath, VkFormat format,
       VkQueue copyQueue,
       VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
       VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
       bool forceLinear = false);
};