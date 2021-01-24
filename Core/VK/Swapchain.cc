/**
 * @brief Swapchain
 */

#include "VK/Swapchain.h"

#include <algorithm>
#include <boost/assert.hpp>

#include "VK/Common.h"
#include "VK/Device.h"
#include "VK/Initializer.h"

static VkSurfaceFormatKHR
FindSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available) {
  if (available.size() == 1 && available[0].format == VK_FORMAT_UNDEFINED) {
    return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  }
  for (const auto &format : available) {
    if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }
  return available[0];
}

static VkPresentModeKHR
FindSwapPresentMode(const std::vector<VkPresentModeKHR> &available) {
  auto best = VK_PRESENT_MODE_FIFO_KHR;
  for (const auto &mode : available) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return mode;
    } else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      best = mode;
    }
  }
  return best;
}

static VkExtent2D
FindSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities, int width,
               int height) {
  if (surfaceCapabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return surfaceCapabilities.currentExtent;
  }

  VkExtent2D extent{};
  extent.width = std::max(surfaceCapabilities.minImageExtent.width,
                          std::min(surfaceCapabilities.maxImageExtent.width,
                                   static_cast<uint32_t>(width)));
  extent.height = std::max(surfaceCapabilities.minImageExtent.height,
                           std::min(surfaceCapabilities.maxImageExtent.height,
                                    static_cast<uint32_t>(height)));
  return extent;
}

[[maybe_unused]] static VkCompositeAlphaFlagBitsKHR FindCompositeAlpha(
    VkSurfaceCapabilitiesKHR surfaceCapabilities,
    const std::vector<VkCompositeAlphaFlagBitsKHR> &compositeAlphaFlags) {
  for (const auto &compositeAlphaFlag : compositeAlphaFlags) {
    if (surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlag) {
      return compositeAlphaFlag;
    }
  }
  return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

/**
 * @brief
 * プレゼンテーションに用いられるウィンドウサーフェイスを生成し、キューのインデックスを取得します。
 * @param instance Vulkanインスタンス
 * @param window ウィンドウハンドル
 * @param physicalDevice 物理デバイス
 */
void Swapchain::Init(VkInstance instance, GLFWwindow *window,
                     VkPhysicalDevice physicalDevice) {
  // ウィンドウサーフェイスを生成します。
  VK_CHECK_RESULT(glfwCreateWindowSurface(instance, window, nullptr, &surface));

  // 利用可能なキュープロパティを取得します。
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);
  BOOST_ASSERT(queueFamilyCount > 0);
  std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilyProperties.data());

  // 各キューを繰り返し処理して、presentationをサポートしているかどうかを確認します。
  // 現在サポートされているキューを検索します。
  // スワップチェーンイメージをウィンドウシステムに表示(present)するために使用されます。
  std::vector<VkBool32> supportedPresent(queueFamilyCount);
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                         &supportedPresent[i]);
  }

  // キューファミリーの配列からグラフィックスキューとプレゼンテーションキューを探します。
  std::optional<uint32_t> graphicsQueueNodeIndex = std::nullopt;
  std::optional<uint32_t> presentQueueNodeIndex = std::nullopt;
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
      if (graphicsQueueNodeIndex == std::nullopt) {
        graphicsQueueNodeIndex = i;
      }
      if (supportedPresent[i] == VK_TRUE) {
        graphicsQueueNodeIndex = i;
        presentQueueNodeIndex = i;
        break;
      }
    }
  }
  // グラフィックスとプレゼンテーションの両方をサポートするキューがない場合、別のキューを探します。
  if (presentQueueNodeIndex == std::nullopt) {
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
      if (supportedPresent[i] == VK_TRUE) {
        presentQueueNodeIndex = i;
        break;
      }
    }
  }

  BOOST_ASSERT_MSG(graphicsQueueNodeIndex != std::nullopt &&
                       graphicsQueueNodeIndex != std::nullopt,
                   "Failed to find a graphics and/or presenting queue!");

  BOOST_ASSERT_MSG(
      graphicsQueueNodeIndex == presentQueueNodeIndex,
      "Separate graphics and presenting queues are not supported yet!");

  queueFamilyIndex = graphicsQueueNodeIndex.value();
}

/**
 * @brief スワップチェーンを生成します。
 * @param device デバイスオブジェクト
 * @param width スワップチェーンイメージの幅
 * @param height スワップチェーンイメージの高さ
 * @param vsync 垂直同期を有効にするか？
 */
void Swapchain::Create(const Device &device, int width, int height,
                       bool vsync) {
  VkSwapchainKHR oldSwapchain = handle;

  VkSurfaceCapabilitiesKHR surfaceCapabilities{};
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      device.physicalDevice, surface, &surfaceCapabilities));

  uint32_t formatCount = 0;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
      device.physicalDevice, surface, &formatCount, nullptr));
  std::vector<VkSurfaceFormatKHR> formats(formatCount);
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
      device.physicalDevice, surface, &formatCount, formats.data()));

  uint32_t presentCount = 0;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
      device.physicalDevice, surface, &presentCount, nullptr));
  std::vector<VkPresentModeKHR> presents(presentCount);
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
      device.physicalDevice, surface, &presentCount, presents.data()));

  const auto surfaceFormat = FindSwapSurfaceFormat(formats);
  const auto presentMode =
      vsync ? VK_PRESENT_MODE_FIFO_KHR : FindSwapPresentMode(presents);
  extent = FindSwapExtent(surfaceCapabilities, width, height);

  uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
  if (surfaceCapabilities.maxImageCount > 0 &&
      imageCount > surfaceCapabilities.maxImageCount) {
    imageCount = surfaceCapabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create{};
  create.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create.pNext = nullptr;
  create.surface = surface;
  create.minImageCount = imageCount;
  create.imageFormat = surfaceFormat.format;
  create.imageColorSpace = surfaceFormat.colorSpace;
  create.imageExtent = extent;
  create.imageArrayLayers = 1;
  create.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  create.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create.queueFamilyIndexCount = 0;
  create.pQueueFamilyIndices = nullptr;
  // サーフェイストランスフォームが無回転をサポートしているならそれを優先します。
  if (surfaceCapabilities.supportedTransforms &
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    create.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    create.preTransform = surfaceCapabilities.currentTransform;
  }
  create.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create.presentMode = presentMode;
  // clippedをVK_TRUEに設定した場合、サーフェイス領域外のレンダリングを破棄できます。
  create.clipped = VK_TRUE;
  create.oldSwapchain = oldSwapchain;

  // サポートされている場合、スワップチェーンイメージで転送元を有効にします。
  if (surfaceCapabilities.supportedUsageFlags &
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
    create.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }
  // サポートされている場合、スワップチェーンイメージで転送先を有効にします。
  if (surfaceCapabilities.supportedUsageFlags &
      VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    create.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &create, nullptr, &handle));

  // swapchainを再生成する場合、presentable imagesをすべて破棄します。
  if (oldSwapchain != VK_NULL_HANDLE) {
    for (auto &view : views) {
      vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
  }

  // swapchain images を取得します。
  vkGetSwapchainImagesKHR(device, handle, &imageCount, nullptr);
  images.resize(imageCount);
  vkGetSwapchainImagesKHR(device, handle, &imageCount, images.data());

  format = surfaceFormat.format;

  // swapchain buffers に含まれるimage viewを取得します。
  views.resize(imageCount);
  for (size_t i = 0; i < imageCount; i++) {
    VkImageViewCreateInfo imageViewCreateInfo =
        Initializer::ImageViewCreateInfo();
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.components = {
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A,
    };
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = images[i];
    VK_CHECK_RESULT(
        vkCreateImageView(device, &imageViewCreateInfo, nullptr, &views[i]));
  }
}

/**
 * @brief スワップチェーンの破棄
 * @param instance Vulkanインスタンス
 * @param device 論理デバイス
 */
void Swapchain::Destroy(VkInstance instance, VkDevice device) {
  for (auto &view : views) {
    vkDestroyImageView(device, view, nullptr);
  }
  vkDestroySwapchainKHR(device, handle, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
}

/**
 * @brief スワップチェーンの次のイメージ(画像)を取得します。
 * @param device 論理デバイス
 * @param presentCompleteSemaphore
 * 画像を使用する準備ができたときに通知されるセマフォ
 * @param pImageIndex 次の画像を取得できた場合に増加するインデックスへのポインタ
 * @return 画像が取得できたかどうかのVkResult
 */
VkResult Swapchain::AcquiredNextImage(VkDevice device,
                                      VkSemaphore presentCompleteSemaphore,
                                      uint32_t *pImageIndex) const {
  return vkAcquireNextImageKHR(
      device, handle, std::numeric_limits<uint64_t>::max(),
      presentCompleteSemaphore, VK_NULL_HANDLE, pImageIndex);
}

/**
 * @brief プレゼンテーション用にスワップチェーンイメージをキューに入れます。
 * @param queue 画像を表示するためのPresenting Queue
 * @param imageIndex
 * プレゼンテーション用にキューに入れるスワップチェーンイメージのインデックス
 * @param waitSemaphore(オプションです。) 画像が表示される前に待機するセマフォ
 * @return
 */
VkResult Swapchain::QueuePresent(VkQueue queue, uint32_t imageIndex,
                                 VkSemaphore waitSemaphore) const {
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &handle;
  presentInfo.pImageIndices = &imageIndex;
  if (waitSemaphore != VK_NULL_HANDLE) {
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
  }
  return vkQueuePresentKHR(queue, &presentInfo);
}
