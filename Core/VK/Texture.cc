/**
 * @brief Texture
 */

#include "Texture.h"

#include <boost/assert.hpp>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <gli/gli.hpp>
#include <iostream>

#include "VK/Common.h"
#include "VK/Device.h"
#include "VK/Initializer.h"
#include "VK/Utils.h"

void Texture::Destroy(const Device &device) const {
  if (sampler != VK_NULL_HANDLE) {
    vkDestroySampler(device, sampler, nullptr);
  }
  vkDestroyImageView(device, view, nullptr);
  vkDestroyImage(device, image, nullptr);
  vkFreeMemory(device, memory, nullptr);
}

void Texture2D::Load(const Device &device, const std::string &filepath,
                     VkQueue copyQueue, VkFormat format,
                     VkImageUsageFlags imageUsageFlags,
                     VkImageLayout imageLayout, bool useStaging) {
  std::error_code ec;
  if (!std::filesystem::exists(filepath, ec)) {
    std::cerr << "Failed to load texture from " << filepath << std::endl;
    std::cerr << ec.value() << ": " << ec.message() << std::endl;
    BOOST_ASSERT_MSG(ec, "Failed to load texture!");
    return;
  }

  gli::texture2d tex2d(gli::load(filepath.c_str()));
  BOOST_ASSERT_MSG(!tex2d.empty(), "Failed to load texture!");
  width = static_cast<uint32_t>(tex2d[0].extent().x);
  height = static_cast<uint32_t>(tex2d[0].extent().y);
  mipLevels = static_cast<uint32_t>(tex2d.levels());

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

    VK_CHECK_RESULT(device.CreateBuffer(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        tex2d.data(), tex2d.size(), stagingBuffer, stagingMemory));

    // バッファコピー領域を設定します。
    std::vector<VkBufferImageCopy> bufferImageCopyRegions{};
    uint32_t offset = 0;
    for (uint32_t i = 0; i < mipLevels; i++) {
      VkBufferImageCopy bufferImageCopyRegion{};
      bufferImageCopyRegion.imageSubresource.aspectMask =
          VK_IMAGE_ASPECT_COLOR_BIT;
      bufferImageCopyRegion.imageSubresource.mipLevel = i;
      bufferImageCopyRegion.imageSubresource.baseArrayLayer = 0;
      bufferImageCopyRegion.imageSubresource.layerCount = 1;
      bufferImageCopyRegion.imageExtent.width =
          static_cast<uint32_t>(tex2d[i].extent().x);
      bufferImageCopyRegion.imageExtent.height =
          static_cast<uint32_t>(tex2d[i].extent().y);
      bufferImageCopyRegion.imageExtent.depth = 1;
      bufferImageCopyRegion.bufferOffset = offset;
      bufferImageCopyRegions.emplace_back(bufferImageCopyRegion);
      offset += static_cast<uint32_t>(tex2d[i].size());
    }

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
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(bufferImageCopyRegions.size()),
                           bufferImageCopyRegions.data());

    // すべてコピーされた後、テクスチャのイメージレイアウトを変更します。
    TransitionImageLayout(copyCommand, image, imageSubresourceRange,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout);
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
    std::memcpy(data, tex2d[0].data(), tex2d[0].size());
    vkUnmapMemory(device, memory);

    // 画像のメモリバリアを設定します。
    TransitionImageLayout(copyCommand, image, VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);
    device.FlushCommandBuffer(copyCommand, copyQueue);
  }

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
  imageViewCreateInfo.subresourceRange = {
      VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1,
  };
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

/**
 * @brief バッファから2Dテクスチャを生成します。
 * @param device
 * @param buffer
 * @param bufferSize
 * @param format
 * @param texWidth
 * @param texHeight
 * @param copyQueue
 * @param filter
 * @param imageUsageFlags
 * @param imageLayout
 */
void Texture2D::FromBuffer(const Device &device, void *buffer,
                           VkDeviceSize bufferSize, VkFormat format,
                           uint32_t texWidth, uint32_t texHeight,
                           VkQueue copyQueue, VkFilter filter,
                           VkImageUsageFlags imageUsageFlags,
                           VkImageLayout imageLayout) {
  BOOST_ASSERT(buffer);

  width = texWidth;
  height = texHeight;
  mipLevels = 1;

  VkCommandBuffer copyCmd = device.CreateCommandBuffer();

  // 生の画像データを含むホストに表示されるステージングバッファを生成します。
  VkBufferCreateInfo bufferCreateInfo = Initializer::BufferCreateInfo(
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bufferSize);
  VkBuffer stagingBuffer;
  VK_CHECK_RESULT(
      vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));

  // ステージングバッファのメモリ要件を取得します。
  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(device, stagingBuffer, &memoryRequirements);

  VkMemoryAllocateInfo memoryAllocateInfo = Initializer::MemoryAllocateInfo();
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex =
      device.FindMemoryType(memoryRequirements.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VkDeviceMemory stagingMemory;
  VK_CHECK_RESULT(
      vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &stagingMemory));
  VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0));

  // ステージングバッファへテクスチャのデータをコピーします。
  std::byte *data;
  VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, memoryRequirements.size,
                              0, reinterpret_cast<void **>(&data)));
  std::memcpy(data, buffer, bufferSize);
  vkUnmapMemory(device, stagingMemory);

  VkBufferImageCopy bufferCopyRegion{};
  bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferCopyRegion.imageSubresource.mipLevel = 0;
  bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
  bufferCopyRegion.imageSubresource.layerCount = 1;
  bufferCopyRegion.imageExtent.width = width;
  bufferCopyRegion.imageExtent.height = height;
  bufferCopyRegion.imageExtent.depth = 1;
  bufferCopyRegion.bufferOffset = 0;

  // 最適なタイルターゲット画像を生成します。
  CreateImage(device, image, memory, format, VK_IMAGE_TYPE_2D, width, height, 1,
              mipLevels, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              imageUsageFlags | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
              VK_IMAGE_TILING_OPTIMAL);

  // イメージバリア
  VkImageSubresourceRange imageSubresourceRange{};
  imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageSubresourceRange.baseMipLevel = 0;
  imageSubresourceRange.levelCount = mipLevels;
  imageSubresourceRange.layerCount = 1;
  TransitionImageLayout(copyCmd, image, imageSubresourceRange,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // ステージングバッファからミップレベルをコピーします。
  vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &bufferCopyRegion);

  // すべてのミップレベルがコピーされた後、テクスチャのイメージレイアウトをシェーダー読み取りに変更します。
  TransitionImageLayout(copyCmd, image, imageSubresourceRange,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout);
  device.FlushCommandBuffer(copyCmd, copyQueue);

  // ステージングリソースを破棄します。
  vkFreeMemory(device, stagingMemory, nullptr);
  vkDestroyBuffer(device, stagingBuffer, nullptr);

  // サンプラーの生成を行います。
  CreateSampler(device, sampler, filter, filter, VK_FALSE, VK_COMPARE_OP_NEVER);

  // イメージビューの生成を行います。
  CreateImageView(device, view, image, VK_IMAGE_VIEW_TYPE_2D, format);

  // 記述子セットの設定に使用する情報の更新を行います。
  descriptor.sampler = sampler;
  descriptor.imageView = view;
  descriptor.imageLayout = imageLayout;
}
