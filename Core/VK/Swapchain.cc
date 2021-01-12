/**
 * @brief Swapchain
 */

#include "VK/Swapchain.h"

#include <algorithm>
#include <boost/assert.hpp>

#include "VK/Image/ImageView.h"
#include "VK/Instance.h"

static VkSurfaceFormatKHR
SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available) {
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
SelectSwapPresentMode(const std::vector<VkPresentModeKHR> &available) {
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

static VkExtent2D SelectSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                                   int width, int height) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  VkExtent2D extent{};
  extent.width = std::max(capabilities.minImageExtent.width,
                          std::min(capabilities.maxImageExtent.width,
                                   static_cast<uint32_t>(width)));
  extent.height = std::max(capabilities.minImageExtent.height,
                           std::min(capabilities.maxImageExtent.height,
                                    static_cast<uint32_t>(height)));
  return extent;
}

void Swapchain::Create(const Instance &instance, int width, int height,
                       bool forceFifo) {
  VkSurfaceCapabilitiesKHR capabilities{};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(instance.physicalDevice,
                                            instance.surface, &capabilities);

  uint32_t size = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(instance.physicalDevice,
                                       instance.surface, &size, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(size);
  vkGetPhysicalDeviceSurfaceFormatsKHR(instance.physicalDevice,
                                       instance.surface, &size, formats.data());

  size = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(instance.physicalDevice,
                                            instance.surface, &size, nullptr);
  std::vector<VkPresentModeKHR> modes(size);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      instance.physicalDevice, instance.surface, &size, modes.data());

  const auto surfaceFormat = SelectSwapSurfaceFormat(formats);
  const auto presentMode =
      forceFifo ? VK_PRESENT_MODE_FIFO_KHR : SelectSwapPresentMode(modes);
  extent = SelectSwapExtent(capabilities, width, height);

  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create{};
  create.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create.surface = instance.surface;
  create.minImageCount = imageCount;
  create.imageFormat = surfaceFormat.format;
  create.imageColorSpace = surfaceFormat.colorSpace;
  create.imageExtent = extent;
  create.imageArrayLayers = 1;
  create.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilies families = QueueFamilies::Find(instance);
  if (families.presentation != families.graphics) {
    create.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create.queueFamilyIndexCount = 2;
    uint32_t indices[] = {static_cast<uint32_t>(families.presentation),
                          static_cast<uint32_t>(families.graphics)};
    create.pQueueFamilyIndices = indices;
  } else {
    create.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create.queueFamilyIndexCount = 0;
    create.pQueueFamilyIndices = nullptr;
  }

  create.preTransform = capabilities.currentTransform;
  create.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create.presentMode = presentMode;
  create.clipped = VK_TRUE;
  create.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(instance.device, &create, nullptr, &handle)) {
    BOOST_ASSERT_MSG(false, "Failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(instance.device, handle, &imageCount, nullptr);
  images.resize(imageCount);
  vkGetSwapchainImagesKHR(instance.device, handle, &imageCount, images.data());

  format = surfaceFormat.format;

  views.resize(imageCount);
  for (size_t i = 0; i < imageCount; i++) {
    views[i] = ImageView::Create(instance, images[i], VK_IMAGE_VIEW_TYPE_2D,
                                 format, VK_IMAGE_ASPECT_COLOR_BIT);
  }
}

const VkSwapchainKHR &Swapchain::Get() const { return handle; }

void Swapchain::Cleanup(const Instance &instance) {
  for (auto &view : views) {
    vkDestroyImageView(instance.device, view, nullptr);
  }
  vkDestroySwapchainKHR(instance.device, handle, nullptr);
}
