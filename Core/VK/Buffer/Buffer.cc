#include "VK/Buffer/Buffer.h"

#include <boost/assert.hpp>

#include "VK/Buffer/Command.h"
#include "VK/Buffer/Memory.h"
#include "VK/Instance.h"

void Buffer::Create(const Instance &instance, VkDeviceSize size,
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
  if (const auto type = Memory::FindType(req.memoryTypeBits, memProp,
                                         instance.physicalDevice)) {
    alloc.memoryTypeIndex = type.value();
  } else {
    BOOST_ASSERT_MSG(false, "Failed to find memory type!");
  }

  if (vkAllocateMemory(instance.device, &alloc, nullptr, &memory)) {
    BOOST_ASSERT_MSG(false, "Failed to allocate buffer memory!");
  }
  vkBindBufferMemory(instance.device, buffer, memory, 0);
}

void Buffer::Copy(const Instance &instance, VkBuffer src, VkBuffer dst,
                  VkDeviceSize size) {
  VkCommandBuffer command = Command::BeginSingleTime(instance);

  VkBufferCopy copy{};
  copy.size = size;
  vkCmdCopyBuffer(command, src, dst, 1, &copy);
  Command::EndSingleTime(instance, command);
}

void Buffer::CopyToImage(const Instance &instance, VkBuffer buffer,
                         VkImage image, uint32_t width, uint32_t height) {
  VkCommandBuffer command = Command::BeginSingleTime(instance);

  VkBufferImageCopy copy{};
  copy.bufferOffset = 0;
  copy.bufferRowLength = 0;
  copy.bufferImageHeight = 0;
  copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy.imageSubresource.mipLevel = 0;
  copy.imageSubresource.baseArrayLayer = 0;
  copy.imageSubresource.layerCount = 1;
  copy.imageOffset = {0, 0, 0};
  copy.imageExtent = { width, height, 1};

  vkCmdCopyBufferToImage(command, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

  Command::EndSingleTime(instance, command);
}

void Buffer::Destroy(const Instance &instance) const {
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(instance.device, buffer, nullptr);
  }
  if (memory != VK_NULL_HANDLE) {
    vkFreeMemory(instance.device, memory, nullptr);
  }
}
