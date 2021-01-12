#pragma once

#include <vulkan/vulkan.h>

struct Instance;

namespace ImageView {
VkImageView Create(const Instance &instance, VkImage image,
                   VkImageViewType type, VkFormat format,
                   VkImageAspectFlags flags);
}
