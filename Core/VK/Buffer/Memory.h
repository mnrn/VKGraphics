#pragma once

#include <vulkan/vulkan.h>

#include <optional>

namespace Memory {
std::optional<uint32_t> FindType(uint32_t filter, VkMemoryPropertyFlags flags,
                                 VkPhysicalDevice physicalDevice);
}
