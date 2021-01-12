#pragma once

#include <vulkan/vulkan.h>

#include <optional>

struct Instance;
struct Swapchain;

struct DepthStencil {
  static std::optional<VkFormat>
  FindSupportedFormat(const std::vector<VkFormat> &candicates,
                      VkImageTiling tiling, VkFormatFeatureFlags features,
                      VkPhysicalDevice physicalDevice) {
    for (const auto &format : candicates) {
      VkFormatProperties props{};
      vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
      if ((tiling == VK_IMAGE_TILING_LINEAR &&
           (props.linearTilingFeatures & features) == features) ||
          (tiling == VK_IMAGE_TILING_OPTIMAL &&
           (props.optimalTilingFeatures & features) == features)) {
        return format;
      }
    }
    return std::nullopt;
  }

  static std::optional<VkFormat> FindDepthFormat(VkPhysicalDevice physicalDevice) {
    return FindSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        physicalDevice);
  }

  void Create(const Instance &, const Swapchain &);

  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
};
