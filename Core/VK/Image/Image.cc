#include "VK/Image/Image.h"

#include <boost/assert.hpp>

#include "VK/Device.h"
#include "VK/Common.h"
#include "VK/Initializer.h"

namespace Image {
void Create(const Device &device, uint32_t w, uint32_t h,
            VkImageCreateFlags flags, VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags usage, VkMemoryPropertyFlags props,
            VkImage &image, VkDeviceMemory &memory) {
  VkImageCreateInfo create = Initializer::ImageCreateInfo();
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
  VK_CHECK_RESULT(vkCreateImage(device, &create, nullptr, &image));

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(device, image, &req);

  VkMemoryAllocateInfo alloc{};
  alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc.allocationSize = req.size;
  alloc.memoryTypeIndex = device.FindMemoryType(req.memoryTypeBits, props);

  VK_CHECK_RESULT(vkAllocateMemory(device, &alloc, nullptr, &memory));
  VK_CHECK_RESULT(vkBindImageMemory(device, image, memory, 0));
}

void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
                           VkFormat format, VkImageLayout old,
                           VkImageLayout flesh, bool isCubeMap) {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old;
  barrier.newLayout = flesh;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;

  if (flesh == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT) {
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
  } else if (old == VK_IMAGE_LAYOUT_UNDEFINED &&
             flesh == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dst = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    BOOST_ASSERT_MSG(false, "Unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, src, dst, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);
}
} // namespace Image
