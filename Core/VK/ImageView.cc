/**
 * @brief Vulkan Image View
 */

//*--------------------------------------------------------------------------------
// Including files
//*--------------------------------------------------------------------------------

#include "VK/ImageView.h"

#include <boost/assert.hpp>

#include "VK/Instance.h"

//*--------------------------------------------------------------------------------
// Image View
//*--------------------------------------------------------------------------------

namespace ImageView {
VkImageView Create(const Instance &instance, VkImage image,
                   VkImageViewType type, VkFormat format,
                   VkImageAspectFlags flags) {
  VkImageViewCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  create.image = image;
  create.viewType = type;
  create.format = format;
  create.subresourceRange.aspectMask = flags;
  create.subresourceRange.baseMipLevel = 0;
  create.subresourceRange.levelCount = 1;
  create.subresourceRange.baseArrayLayer = 0;
  create.subresourceRange.layerCount =
      (type == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1;

  VkImageView imageView;
  if (vkCreateImageView(instance.device, &create, nullptr, &imageView)) {
    BOOST_ASSERT_MSG(false, "Failed to create texture image view!");
  }
  return imageView;
}
} // namespace ImageView
