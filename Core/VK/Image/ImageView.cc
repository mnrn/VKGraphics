/**
 * @brief Vulkan Image View
 */

//*-----------------------------------------------------------------------------
// Including files
//*-----------------------------------------------------------------------------

#include "ImageView.h"

#include <boost/assert.hpp>

#include "VK/Common.h"
#include "VK/Device.h"
#include "VK/Initializer.h"

//*-----------------------------------------------------------------------------
// Image View
//*-----------------------------------------------------------------------------

namespace ImageView {
VkImageView Create(const Device &device, VkImage image, VkImageViewType type,
                   VkFormat format, VkImageAspectFlags flags) {
  VkImageViewCreateInfo create = Initializer::ImageViewCreateInfo();
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
  VK_CHECK_RESULT(vkCreateImageView(device, &create, nullptr, &imageView));
  return imageView;
}
} // namespace ImageView
