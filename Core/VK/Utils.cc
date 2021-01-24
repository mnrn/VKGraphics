#include "VK/Utils.h"

#include <map>
#include <spdlog/spdlog.h>

#include <boost/assert.hpp>

#include "VK/Common.h"
#include "VK/Device.h"
#include "VK/Initializer.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
                           VkImageSubresourceRange imageSubresourceRange,
                           VkImageLayout oldImageLayout,
                           VkImageLayout newImageLayout,
                           VkPipelineStageFlags srcStageMask,
                           VkPipelineStageFlags dstStageMask) {
  // イメージバリアオブジェクトを生成します。
  VkImageMemoryBarrier imageMemoryBarrier = Initializer::ImageMemoryBarrier();
  imageMemoryBarrier.oldLayout = oldImageLayout;
  imageMemoryBarrier.newLayout = newImageLayout;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange = imageSubresourceRange;

  // Source Layouts (old)
  // srcAccessMaskは新しいレイアウトに移行する前に終了する必要がある古いレイアウトの依存関係を制御します。
  switch (oldImageLayout) {
  case VK_IMAGE_LAYOUT_UNDEFINED:
    // 画像のレイアウトは未定義です。(または重要ではありません。)
    // 初期レイアウトとしてのみ有効です。
    // フラグは不要で、完全を期すためにのみリストされています。
    imageMemoryBarrier.srcAccessMask = 0;
    break;
  case VK_IMAGE_LAYOUT_PREINITIALIZED:
    // 画像は事前に初期化されています。
    // 線形画像の初期レイアウトとしてのみ有効で、メモリの内容を保持します。
    // ホストの書き込みが完了していることを確認します。
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // 画像はカラーアタッチメントです。
    // カラーバッファへの書き込みがすべて終了していることを確認します。
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // 画像は深度ステンシルアタッチメントです。
    // 深度ステンシルバッファへの書き込みがすべて終了していることを確認します。
    imageMemoryBarrier.srcAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // 画像は転送元です。
    // 画像からの読み取りがすべて終了していることを確認します。
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // 画像は転送先です。
    // 画像への書き込みがすべて終了していることを確認します。
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // 画像はシェーダーによって読み取られます。
    // 画像からのシェーダー読み取りが終了していることを確認します。
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    break;
  }

  // Target layouts (new)
  // dstAccessMaskは、新しいイメージレイアウトの依存関係を制御します。
  switch (newImageLayout) {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // 画像は転送先として使用されます。
    // 画像への書き込みがすべて終了していることを確認します。
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // 画像は転送元として使用されます。
    // 画像からの読み取りがすべて終了していることを確認します。
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // 画像はカラーアタッチメントとして使用されます。
    // カラーバッファへの書き込みがすべて終了していることを確認します。
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // 画像は深度ステンシルアタッチメントとして使用されます。
    // 深度ステンシルバッファへの書き込みがすべて終了していることを確認します。
    imageMemoryBarrier.dstAccessMask =
        imageMemoryBarrier.dstAccessMask |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // 画像はシェーダー(サンプラー, 入力アタッチメント)で読み取られます。
    // イメージへの書き込みがすべて終了していることを確認します。
    if (imageMemoryBarrier.srcAccessMask == 0) {
      imageMemoryBarrier.srcAccessMask =
          VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
    }
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    break;
  }

  // バリアをセットアップコマンドバッファ内に配置します。
  vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr,
                       0, nullptr, 1, &imageMemoryBarrier);
}

void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
                           VkImageAspectFlags imageAspectFlags,
                           VkImageLayout oldImageLayout,
                           VkImageLayout newImageLayout,
                           VkPipelineStageFlags srcStageMask,
                           VkPipelineStageFlags dstStageMask) {
  VkImageSubresourceRange imageSubresourceRange{};
  imageSubresourceRange.aspectMask = imageAspectFlags;
  imageSubresourceRange.baseMipLevel = 0;
  imageSubresourceRange.levelCount = 1;
  imageSubresourceRange.layerCount = 1;
  TransitionImageLayout(commandBuffer, image, imageSubresourceRange,
                        oldImageLayout, newImageLayout, srcStageMask,
                        dstStageMask);
}

float CalcDeviceScore(VkPhysicalDevice device,
                      const std::vector<const char *> &deviceExtensions) {
  VkPhysicalDeviceProperties prop{};
  vkGetPhysicalDeviceProperties(device, &prop);
  VkPhysicalDeviceFeatures feat{};
  vkGetPhysicalDeviceFeatures(device, &feat);

  std::map<VkPhysicalDeviceType, float> scores = {
      {VK_PHYSICAL_DEVICE_TYPE_CPU, 0.0f},
      {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 1000.0f}};
  float score = 0.0f;
  if (scores.count(prop.deviceType) > 0) {
    score = scores[prop.deviceType];
  } else {
    score = 1.0f;
  }

  uint32_t size = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &size, nullptr);
  std::vector<VkExtensionProperties> extensions(size);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &size,
                                       extensions.data());

  std::set<std::string> missing(deviceExtensions.begin(),
                                deviceExtensions.end());
  for (const auto &extension : extensions) {
    missing.erase(extension.extensionName);
  }
  if (!missing.empty()) {
    score = 0;
#if !defined(NDEBUG)
    spdlog::warn("Missing device extensions");
#endif
  }

  if (!feat.samplerAnisotropy) {
    score = 0;
  }

  uint32_t major = VK_VERSION_MAJOR(VK_API_VERSION_1_0);
  uint32_t minor = VK_VERSION_MINOR(VK_API_VERSION_1_0);
  uint32_t version = VK_MAKE_VERSION(major, minor, VK_HEADER_VERSION);

  if (prop.apiVersion >= version) {
    score *= 1.1f;
  }
#if !defined(NDEBUG)
  spdlog::info(" - score {}", score);
#endif

  return score;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
