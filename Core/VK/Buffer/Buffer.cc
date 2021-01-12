#include "VK/Buffer/Buffer.h"

#include <boost/assert.hpp>

#include "VK/Buffer/Memory.h"
#include "VK/Instance.h"

namespace Buffer {
void Create(const Instance &instance, VkDeviceSize size,
            VkBufferUsageFlags usage, VkMemoryPropertyFlags memProp,
            VkBuffer &buffer, VkDeviceMemory &memory) {
  VkBufferCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create.size = size;
  create.usage = usage;
  create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(instance.device, &create, nullptr, &buffer)) {
    BOOST_ASSERT_MSG(false, "Failed to create buffer!");
  }

  VkMemoryRequirements req{};
  vkGetBufferMemoryRequirements(instance.device, buffer, &req);

  VkMemoryAllocateInfo alloc{};
  alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc.allocationSize = req.size;
  if (const auto type =  Memory::FindType(req.memoryTypeBits, memProp, instance.physicalDevice)) {
    alloc.memoryTypeIndex = type.value();
  } else {
    BOOST_ASSERT_MSG(false, "Failed to find memory type!");
  }

  if (vkAllocateMemory(instance.device, &alloc, nullptr, &memory)) {
    BOOST_ASSERT_MSG(false, "Failed to allocate buffer memory!");
  }
  vkBindBufferMemory(instance.device, buffer, memory, 0);
}
} // namespace Buffer
