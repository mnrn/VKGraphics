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

  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t mipLevels = 1;
  uint32_t layerCount = 1;
};

struct Texture2D : public Texture {
  void
  Load(const Device &device, const std::string &filepath, VkQueue copyQueue,
       VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
       VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
       VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
       bool useStaging = true);

  void FromBuffer(
      const Device &device, void *buffer, VkDeviceSize bufferSize,
      VkFormat format, uint32_t texWidth, uint32_t texHeight, VkQueue copyQueue,
      VkFilter filter = VK_FILTER_LINEAR,
      VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
      VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
};
