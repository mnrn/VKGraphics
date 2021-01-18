#include "VK/Buffer/DepthStencil.h"

#include <boost/assert.hpp>

#include "VK/Image/Image.h"
#include "VK/Image/ImageView.h"
#include "VK/Instance.h"
#include "VK/Swapchain.h"

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

std::optional<VkFormat>
DepthStencil::FindDepthFormat(VkPhysicalDevice physicalDevice) {
  return FindSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
      physicalDevice);
}

bool DepthStencil::HasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void DepthStencil::Create(const Instance &instance,
                          const Swapchain &swapchain) {
  const auto format = FindDepthFormat(instance.physicalDevice);
  BOOST_ASSERT_MSG(format, "Failed to find suitable depth format!");
  Image::Create(instance, swapchain.extent.width, swapchain.extent.height, 0,
                format.value(), VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);
  view = ImageView::Create(instance, image, VK_IMAGE_VIEW_TYPE_2D,
                           format.value(), VK_IMAGE_ASPECT_DEPTH_BIT);
  Image::TransitionImageLayout(
      instance, image, format.value(), VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void DepthStencil::Destroy(const Instance &instance) {
  vkDestroyImageView(instance.device, view, nullptr);
  vkDestroyImage(instance.device, image, nullptr);
  vkFreeMemory(instance.device, memory, nullptr);
}
