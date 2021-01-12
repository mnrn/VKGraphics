#pragma once

#include <vulkan/vulkan.h>

struct Instance;

namespace Image {
void Create(const Instance &instance, uint32_t w, uint32_t h,
            VkImageCreateFlags, VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags usage, VkMemoryPropertyFlags props,
            VkImage &image, VkDeviceMemory &memory);
}
