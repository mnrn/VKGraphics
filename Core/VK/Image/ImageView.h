#pragma once

#include <vulkan/vulkan.h>

struct Device;

namespace ImageView {
VkImageView Create(const Device& device, VkImage image,
                   VkImageViewType type, VkFormat format,
                   VkImageAspectFlags flags);
}
