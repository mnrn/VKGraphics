#include "VK/Image/Image.h"

#include <boost/assert.hpp>

#include "VK/Buffer/Memory.h"
#include "VK/Instance.h"

namespace Image {
void Create(const Instance &instance, uint32_t w, uint32_t h,
            VkImageCreateFlags flags, VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags usage, VkMemoryPropertyFlags props,
            VkImage &image, VkDeviceMemory &memory) {
  VkImageCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  create.flags = flags;
  create.imageType = VK_IMAGE_TYPE_2D;
  create.extent.width = w;
  create.extent.height = h;
  create.extent.depth = 1;
  create.mipLevels = 1;
  create.arrayLayers = (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? 6 : 1;
  create.format = format;
  create.tiling = tiling;
  create.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  create.usage = usage;
  create.samples = VK_SAMPLE_COUNT_1_BIT;
  create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateImage(instance.device, &create, nullptr, &image)) {
    BOOST_ASSERT_MSG(false, "Failed to create image!");
  }

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(instance.device, image, &req);

  VkMemoryAllocateInfo alloc{};
  alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc.allocationSize = req.size;
  if (const auto type = Memory::FindType(req.memoryTypeBits, props,
                                         instance.physicalDevice)) {
    alloc.memoryTypeIndex = type.value();
  } else {
    BOOST_ASSERT_MSG(false, "Failed to find memory type!");
  }
  if (vkAllocateMemory(instance.device, &alloc, nullptr, &memory)) {
    BOOST_ASSERT_MSG(false, "Failed to allocate buffer memory!");
  }
  vkBindImageMemory(instance.device, image, memory, 0);
}
} // namespace Image
