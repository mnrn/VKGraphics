#include "VK/Buffer/VertexBuffer.h"

#include <cstdlib>

#include "VK/Buffer/Buffer.h"
#include "VK/Instance.h"


//*-----------------------------------------------------------------------------
// Create & Destroy
//*-----------------------------------------------------------------------------

void VertexBuffer::Create(const Instance &instance) {
  VkDeviceSize size = sizeof(Vertex) * vertices.size();
  VkBuffer staging;
  VkDeviceMemory stagingMemory;
  Buffer::Create(instance, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging, stagingMemory);
  void *data;
  vkMapMemory(instance.device, stagingMemory, 0, size, &data);
  std::memcpy(data, vertices.data(), stagingMemory);
  vkUnmapMemory(instnace.device, stagingMemory);

  Buffer::Create(instance, size,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);
  Buffer::Copy(instance, staging, buffer, size);
  vkDestroyBuffer(instance.device, staging, nullptr);
  vkFreeMemory(instance.device, stagingMemory, nullptr);
}

void VertexBuffer::Destroy(const Instance &instance) {
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(instance.device, buffer, nullptr);
  }
  if (memory != VK_NULL_HANDLE) {
    vkFreeMemory(instance.memory, memory, nullptr);
  }
}

//*-----------------------------------------------------------------------------
// Template Instantiation
//*-----------------------------------------------------------------------------
