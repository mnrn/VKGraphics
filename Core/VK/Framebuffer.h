/**
 * @brief Framebuffer for Vulkan
 */

#pragma once

#include <vulkan/vulkan.h>

#include <algorithm>
#include <vector>

struct Device;

/**
 * @brief 単一のフレームバッファアタッチメントをカプセル化します。
 */
struct FramebufferAttachment {
  /**
   * @brief アタッチメントに深度コンポーネントがある場合はtrueを返します。
   */
  [[nodiscard]] bool HasDepth() const {
    static const std::vector<VkFormat> formats{
        VK_FORMAT_D16_UNORM,         VK_FORMAT_X8_D24_UNORM_PACK32,
        VK_FORMAT_D32_SFLOAT,        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT,
    };
    return std::find(std::begin(formats), std::end(formats), format) !=
           std::end(formats);
  }
  /**
   * @brief アタッチメントにステンシルコンポーネントがある場合はtrueを返します。
   */
  [[nodiscard]] bool HasStencil() const {
    static const std::vector<VkFormat> formats{
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
    };
    return std::find(std::begin(formats), std::end(formats), format) !=
           std::end(formats);
  }

  /**
   * @brief
   * アタッチメントに深度またはステンシルコンポーネントがある場合はtrueを返します。
   */
  [[nodiscard]] bool IsDepthStencil() const { return HasDepth() || HasStencil(); }

  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkFormat format = VK_FORMAT_UNDEFINED;
  VkImageSubresourceRange subresourceRange{};
  VkAttachmentDescription description{};
};

/**
 * @brief 生成するアタッチメントのAttributesについて
 */
struct AttachmentCreateInfo {
  uint32_t width;
  uint32_t height;
  uint32_t layerCount;
  VkFormat format;
  VkImageUsageFlags usage;
  VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
};

struct Framebuffer {
  uint32_t AddAttachment(const Device& device, const AttachmentCreateInfo &attachmentCreateInfo);
  VkResult CreateSampler(const Device& device, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode);
  VkResult CreateRenderPass(const Device& device);
  void Destroy(const Device &device) const;

  uint32_t width;
  uint32_t height;
  VkFramebuffer framebuffer = VK_NULL_HANDLE;
  VkRenderPass renderPass = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  std::vector<FramebufferAttachment> attachments{};
};
