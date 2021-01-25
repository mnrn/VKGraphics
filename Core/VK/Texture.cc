/**
 * @brief Texture
 */

#include "Texture.h"

#include <boost/assert.hpp>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "VK/Common.h"
#include "VK/Device.h"
#include "VK/Initializer.h"
#include "VK/Utils.h"

namespace Pixels {
static stbi_uc *Load(const std::string &path, int &w, int &h,
                     bool flip = true) {
  int bytesPerPix;
  stbi_set_flip_vertically_on_load(flip);
  return stbi_load(path.c_str(), &w, &h, &bytesPerPix, 4);
}

static void Free(unsigned char *data) { stbi_image_free(data); }
} // namespace Pixels

namespace Mipmaps {
static void Generate(VkImage image, int32_t width, int32_t height,
                     int32_t depth, uint32_t layerCount, uint32_t mipLevels,
                     VkCommandBuffer commandBuffer, VkFilter blitFilter,
                     VkImageLayout initialLayout, VkImageLayout finalLayout) {
  VkImageSubresourceRange imageSubresourceRange{};
  imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageSubresourceRange.baseMipLevel = 0;
  imageSubresourceRange.levelCount = 1;
  imageSubresourceRange.baseArrayLayer = 0;
  imageSubresourceRange.layerCount = layerCount;

  TransitionImageLayout(commandBuffer, image, imageSubresourceRange,
                        initialLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  for (uint32_t i = 1; i < mipLevels; i++) {
    VkImageBlit imageBlit{};

    // Source
    imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlit.srcSubresource.layerCount = layerCount;
    imageBlit.srcSubresource.mipLevel = i - 1;
    imageBlit.srcOffsets[1].x = std::max(width >> i, 1);
    imageBlit.srcOffsets[1].y = std::max(height >> i, 1);
    imageBlit.srcOffsets[1].z = std::max(depth >> i, 1);

    // Destination
    imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlit.dstSubresource.layerCount = 1;
    imageBlit.dstSubresource.mipLevel = i;
    imageBlit.dstOffsets[1].x = std::max(width >> i, 1);
    imageBlit.dstOffsets[1].y = std::max(height >> i, 1);
    imageBlit.dstOffsets[1].z = std::max(depth >> i, 1);

    VkImageSubresourceRange mipSubresourceRange{};
    mipSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    mipSubresourceRange.baseMipLevel = i;
    mipSubresourceRange.levelCount = 1;
    mipSubresourceRange.layerCount = layerCount;

    // 現在のミップレベルを転送先に送ります。
    TransitionImageLayout(commandBuffer, image, mipSubresourceRange,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // 前のミップレベルからBlitします。
    vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit,
                   blitFilter);

    // 現在のミップレベルを転送元に遷移させます。
    TransitionImageLayout(commandBuffer, image, mipSubresourceRange,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  }

  imageSubresourceRange.levelCount = mipLevels;
  TransitionImageLayout(commandBuffer, image, imageSubresourceRange,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout);
}
} // namespace Mipmaps

void Texture::Destroy(const Device &device) const {
  if (sampler != nullptr) {
    vkDestroySampler(device, sampler, nullptr);
  }
  vkDestroyImageView(device, view, nullptr);
  vkDestroyImage(device, image, nullptr);
  vkFreeMemory(device, memory, nullptr);
}

void Texture::Load(const Device &device, const std::string &filepath,
                   VkFormat format, VkQueue copyQueue,
                   VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout,
                   bool useStaging, bool generateMipmaps) {
  std::error_code ec;
  if (!std::filesystem::exists(filepath, ec)) {
    std::cerr << "Failed to load texture from " << filepath << std::endl;
    std::cerr << ec.value() << ": " << ec.message() << std::endl;
    BOOST_ASSERT_MSG(!ec, "Failed to load texture!");
    return;
  }

  int w, h;
  stbi_uc *pixels = Pixels::Load(filepath, w, h);
  if (pixels == nullptr) {
    std::cerr << "Failed to load " << filepath << std::endl;
    BOOST_ASSERT_MSG(pixels != nullptr, "Failed to load texture!");
  }
  const auto width = static_cast<uint32_t>(w);
  const auto height = static_cast<uint32_t>(h);
  const auto mipLevels =
      generateMipmaps ? static_cast<uint32_t>(
                            std::floor(std::log2(std::max(width, height)))) +
                            1
                      : 1;
  const auto size = static_cast<VkDeviceSize>(w * h * 4);

  // 要求されたテクスチャ形式のデバイスプロパティを取得します。
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format,
                                      &formatProperties);

  // テクスチャの読み込みに別のコマンドバッファを使用します。
  VkCommandBuffer copyCommand =
      device.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  if (useStaging) {
    // 生の画像データを含むホストに表示されるステージングバッファを生成します。
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    VK_CHECK_RESULT(
        device.CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            size, &stagingBuffer, &stagingMemory, pixels));

    // バッファコピー領域を設定します。
    VkBufferImageCopy bufferImageCopyRegion{};
    bufferImageCopyRegion.imageSubresource.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;
    bufferImageCopyRegion.imageSubresource.mipLevel = 0;
    bufferImageCopyRegion.imageSubresource.baseArrayLayer = 0;
    bufferImageCopyRegion.imageSubresource.layerCount = 1;
    bufferImageCopyRegion.imageExtent.width = width;
    bufferImageCopyRegion.imageExtent.height = height;
    bufferImageCopyRegion.imageExtent.depth = 1;

    // 最適なタイルターゲット画像を生成します。
    VkImageCreateInfo imageCreateInfo = Initializer::ImageCreateInfo();
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = {width, height, 1};
    imageCreateInfo.usage = imageUsageFlags;
    if ((imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0) {
      imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &image));

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(device, image, &memoryRequirements);
    VkMemoryAllocateInfo memoryAllocateInfo = Initializer::MemoryAllocateInfo();
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = device.FindMemoryType(
        memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(
        vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory));
    VK_CHECK_RESULT(vkBindImageMemory(device, image, memory, 0));

    VkImageSubresourceRange imageSubresourceRange{};
    imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresourceRange.baseMipLevel = 0;
    imageSubresourceRange.levelCount = mipLevels;
    imageSubresourceRange.layerCount = 1;

    // イメージレイアウト遷移を実行するパイプラインステージにメモリ依存を挿入します。
    TransitionImageLayout(copyCommand, image, imageSubresourceRange,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // ステージングバッファからコピーします。
    vkCmdCopyBufferToImage(copyCommand, stagingBuffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &bufferImageCopyRegion);

    if (generateMipmaps) {
      Mipmaps::Generate(image, width, height, 1, 1, mipLevels, copyCommand,
                        VK_FILTER_LINEAR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        imageLayout);
    } else {
      // すべてコピーされた後、テクスチャのイメージレイアウトを変更します。
      TransitionImageLayout(copyCommand, image, imageSubresourceRange,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout);
    }
    device.FlushCommandBuffer(copyCommand, copyQueue);

    // ステージングリソースを破棄します。
    vkFreeMemory(device, stagingMemory, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
  } else {
    BOOST_ASSERT_MSG(formatProperties.linearTilingFeatures &
                         VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT,
                     "Linear tiling is not supported!");

    VkImageCreateInfo imageCreateInfo = Initializer::ImageCreateInfo();
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent = {width, height, 1};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imageCreateInfo.usage = imageUsageFlags;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &image));

    // このイメージのメモリ要件を取得し、ホストメモリを割り当てます。
    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(device, image, &memoryRequirements);
    VkMemoryAllocateInfo memoryAllocateInfo = Initializer::MemoryAllocateInfo();
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex =
        device.FindMemoryType(memoryRequirements.memoryTypeBits,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK_RESULT(
        vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory));
    VK_CHECK_RESULT(vkBindImageMemory(device, image, memory, 0));

    // サブリソースのレイアウトを取得します。
    VkImageSubresource imageSubresource{};
    imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresource.mipLevel = 0;
    VkSubresourceLayout subresourceLayout{};
    vkGetImageSubresourceLayout(device, image, &imageSubresource,
                                &subresourceLayout);

    // イメージメモリをマップし、イメージデータをメモリにコピーします。
    void *data;
    VK_CHECK_RESULT(
        vkMapMemory(device, memory, 0, memoryRequirements.size, 0, &data));
    std::memcpy(data, pixels, size);
    vkUnmapMemory(device, memory);

    // 画像のメモリバリアを設定します。
    TransitionImageLayout(copyCommand, image, VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);
    device.FlushCommandBuffer(copyCommand, copyQueue);
  }
  Pixels::Free(pixels);

  // デフォルトのサンプラーを生成します。
  VkSamplerCreateInfo samplerCreateInfo = Initializer::SamplerCreateInfo();
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.mipLodBias = 0.0f;
  samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
  samplerCreateInfo.minLod = 0.0f;
  // 最大LODレベルはmipレベルを一致する必要があります。
  samplerCreateInfo.maxLod =
      (useStaging) ? static_cast<float>(mipLevels) : 0.0f;
  // デバイスで有効になっている場合にのみ異方性フィルタリングを有効します。
  samplerCreateInfo.maxAnisotropy =
      device.enabledFeatures.samplerAnisotropy
          ? device.properties.limits.maxSamplerAnisotropy
          : 1.0f;
  samplerCreateInfo.anisotropyEnable = device.enabledFeatures.samplerAnisotropy;
  samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK_RESULT(
      vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler));

  // イメージビューを生成します。
  // テクスチャはシェーダーから直接アクセスされず、追加した情報とサブリソース範囲を含むイメージビューによって抽象化されます。
  VkImageViewCreateInfo imageViewCreateInfo =
      Initializer::ImageViewCreateInfo();
  imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCreateInfo.format = format;
  imageViewCreateInfo.components = {
      VK_COMPONENT_SWIZZLE_R,
      VK_COMPONENT_SWIZZLE_G,
      VK_COMPONENT_SWIZZLE_B,
      VK_COMPONENT_SWIZZLE_A,
  };
  imageViewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
                                          1};
  // 線形タイリングは通常ミップマップをサポートしません。
  // 適切なタイリングが使用されている場合にのみミップマップカウントを設定します。
  imageViewCreateInfo.subresourceRange.levelCount = useStaging ? mipLevels : 1;
  imageViewCreateInfo.image = image;
  VK_CHECK_RESULT(
      vkCreateImageView(device, &imageViewCreateInfo, nullptr, &view));

  descriptor.sampler = sampler;
  descriptor.imageView = view;
  descriptor.imageLayout = imageLayout;
}
