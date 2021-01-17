#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "VK/Instance.h"

struct Buffer {
  static void Create(const Instance &instance, VkDeviceSize size,
                     VkBufferUsageFlags usage, VkMemoryPropertyFlags memProp,
                     VkBuffer &buffer, VkDeviceMemory &memory);

  static void Copy(const Instance &instance, VkBuffer src, VkBuffer dst,
                   VkDeviceSize size);

  template <typename T>
  void Create(const Instance &instance, const std::vector<T> &src,
              VkBufferUsageFlags usageFlag) {
    VkDeviceSize size = sizeof(T) * src.size();
    VkBuffer staging;
    VkDeviceMemory stagingMemory;
    Buffer::Create(instance, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   staging, stagingMemory);
    void *data;
    vkMapMemory(instance.device, stagingMemory, 0, size, 0, &data);
    std::memcpy(data, src.data(), size);
    vkUnmapMemory(instance.device, stagingMemory);

    Buffer::Create(instance, size, usageFlag | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);
    Buffer::Copy(instance, staging, buffer, size);
    vkDestroyBuffer(instance.device, staging, nullptr);
    vkFreeMemory(instance.device, stagingMemory, nullptr);
  }

  void Destroy(const Instance &instance) const;

  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
};
