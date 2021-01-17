#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "VK/Buffer/Buffer.h"
#include "VK/Instance.h"

struct BufferObject {
  template <typename T>
  void Create(const Instance &instance, const std::vector<T> &vertices) {
    VkDeviceSize size = sizeof(T) * vertices.size();
    VkBuffer staging;
    VkDeviceMemory stagingMemory;
    Buffer::Create(instance, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   staging, stagingMemory);
    void *data;
    vkMapMemory(instance.device, stagingMemory, 0, size, 0, &data);
    std::memcpy(data, vertices.data(), size);
    vkUnmapMemory(instance.device, stagingMemory);

    Buffer::Create(instance, size,
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);
    Buffer::Copy(instance, staging, buffer, size);
    vkDestroyBuffer(instance.device, staging, nullptr);
    vkFreeMemory(instance.device, stagingMemory, nullptr);
  }

  void Destroy(const Instance &instance) const;

  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
};
