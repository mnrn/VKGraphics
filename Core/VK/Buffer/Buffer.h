#pragma once

#include <vulkan/vulkan.h>

struct Instance;

namespace Buffer {
void Create(const Instance &instance, VkDeviceSize size,
            VkBufferUsageFlags usage, VkMemoryPropertyFlags memProp,
            VkBuffer &buffer, VkDeviceMemory &memory);

void Copy(const Instance &instance, VkBuffer src, VkBuffer dst,
          VkDeviceSize size);
} // namespace Buffer
