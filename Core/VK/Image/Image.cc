#include "VK/Image/Image.h"

#include <boost/assert.hpp>

#include "VK/Buffer/Command.h"
#include "VK/Buffer/DepthStencil.h"
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

void TransitionImageLayout(const Instance &instance, VkImage image,
                           VkFormat format, VkImageLayout old,
                           VkImageLayout flesh, bool isCubeMap) {
  VkCommandBuffer command = Command::BeginSingleTime(instance);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old;
  barrier.newLayout = flesh;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;

  if (flesh == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (DepthStencil::HasStencilComponent(format)) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = isCubeMap ? 6 : 1;

  VkPipelineStageFlags src;
  VkPipelineStageFlags dst;

  if (old == VK_IMAGE_LAYOUT_UNDEFINED &&
      flesh == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dst = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             flesh == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    src = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dst = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             flesh == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dst = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    BOOST_ASSERT_MSG(false, "Unsupported layout transition!");
  }

  vkCmdPipelineBarrier(command, src, dst, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);
  Command::EndSingleTime(instance, command);
}
} // namespace Image
