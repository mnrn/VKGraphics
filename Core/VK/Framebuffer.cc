/**
 * @brief Framebuffer for Vulkan
 */

#include "VK/Framebuffer.h"

#include <array>
#include <boost/assert.hpp>

#include "VK/Common.h"
#include "VK/Device.h"
#include "VK/Initializer.h"
#include "VK/Utils.h"

void Framebuffer::Destroy(const Device &device) const {
  vkDestroyFramebuffer(device, framebuffer, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);
  vkDestroySampler(device, sampler, nullptr);
  for (const auto &attachment : attachments) {
    vkDestroyImageView(device, attachment.view, nullptr);
    vkFreeMemory(device, attachment.memory, nullptr);
    vkDestroyImage(device, attachment.image, nullptr);
  }
}

uint32_t
Framebuffer::AddAttachment(const Device &device,
                           const AttachmentCreateInfo &attachmentCreateInfo) {
  FramebufferAttachment framebufferAttachment{};
  framebufferAttachment.format = attachmentCreateInfo.format;

  VkImageAspectFlags aspectMask = 0;
  // カラーアタッチメント
  if (attachmentCreateInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
    aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }
  // デプスステンシルアタッチメント
  if (attachmentCreateInfo.usage &
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
    if (framebufferAttachment.HasDepth()) {
      aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (framebufferAttachment.HasStencil()) {
      aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }
  BOOST_ASSERT(aspectMask > 0);

  VK_CHECK_RESULT(CreateImage(
      device, framebufferAttachment.image, framebufferAttachment.memory,
      attachmentCreateInfo.format, VK_IMAGE_TYPE_2D, attachmentCreateInfo.width,
      attachmentCreateInfo.height, 1, 1, attachmentCreateInfo.layerCount,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachmentCreateInfo.usage,
      VK_IMAGE_TILING_OPTIMAL, attachmentCreateInfo.imageSampleCount));

  framebufferAttachment.subresourceRange = {};
  framebufferAttachment.subresourceRange.aspectMask = aspectMask;
  framebufferAttachment.subresourceRange.levelCount = 1;
  framebufferAttachment.subresourceRange.layerCount =
      attachmentCreateInfo.layerCount;
  const VkImageViewType imageViewType = attachmentCreateInfo.layerCount == 1
                                            ? VK_IMAGE_VIEW_TYPE_2D
                                            : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  VK_CHECK_RESULT(CreateImageView(
      device, framebufferAttachment.view, framebufferAttachment.image,
      imageViewType, attachmentCreateInfo.format,
      framebufferAttachment.subresourceRange.aspectMask,
      framebufferAttachment.subresourceRange.baseMipLevel,
      framebufferAttachment.subresourceRange.levelCount,
      framebufferAttachment.subresourceRange.baseArrayLayer,
      framebufferAttachment.subresourceRange.layerCount));

  framebufferAttachment.description = {};
  framebufferAttachment.description.samples =
      attachmentCreateInfo.imageSampleCount;
  framebufferAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  framebufferAttachment.description.stencilLoadOp =
      VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  framebufferAttachment.description.stencilStoreOp =
      VK_ATTACHMENT_STORE_OP_DONT_CARE;
  framebufferAttachment.description.format = attachmentCreateInfo.format;
  framebufferAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  // 最終的なレイアウトはアタッチメントのタイプによって異なります。
  framebufferAttachment.description.finalLayout =
      framebufferAttachment.IsDepthStencil()
          ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
          : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  attachments.emplace_back(framebufferAttachment);
  return static_cast<uint32_t>(attachments.size() - 1);
}

VkResult Framebuffer::CreateSampler(const Device &device, VkFilter magFilter,
                                    VkFilter minFilter,
                                    VkSamplerAddressMode addressMode) {
  return ::CreateSampler(device, sampler, magFilter, minFilter, VK_FALSE,
                         VK_COMPARE_OP_LESS_OR_EQUAL, addressMode, addressMode,
                         addressMode, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f,
                         1.0f);
}

VkResult Framebuffer::CreateRenderPass(const Device &device) {
  std::vector<VkAttachmentDescription> attachmentDescriptions;
  for (const auto &attachment : attachments) {
    attachmentDescriptions.emplace_back(attachment.description);
  }

  std::vector<VkAttachmentReference> colorReferences;
  VkAttachmentReference depthReference{};
  bool hasDepth = false;
  bool hasColor = false;
  uint32_t attachmentIdx = 0;
  for (const auto &attachment : attachments) {
    if (attachment.IsDepthStencil()) {
      // 深度アタッチメントは一つのみ許されます。
      BOOST_ASSERT(!hasDepth);
      depthReference.attachment = attachmentIdx;
      depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      hasDepth = true;
    } else {
      colorReferences.emplace_back(VkAttachmentReference{
          attachmentIdx, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
      hasColor = true;
    }
    attachmentIdx++;
  }

  // デフォルトのレンダーパス設定では、1つのサブパスのみが使用されます。
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  if (hasColor) {
    subpass.pColorAttachments = colorReferences.data();
    subpass.colorAttachmentCount =
        static_cast<uint32_t>(colorReferences.size());
  }
  if (hasDepth) {
    subpass.pDepthStencilAttachment = &depthReference;
  }

  // アタッチメントのレイアウト遷移にサブパスの依存関係を使用します。
  std::array<VkSubpassDependency, 2> subpassDependencies{};

  subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependencies[0].dstSubpass = 0;
  subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  subpassDependencies[0].dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  subpassDependencies[1].srcSubpass = 0;
  subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependencies[1].srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  // レンダーパスを生成します。
  VkRenderPassCreateInfo renderPassCreateInfo =
      Initializer::RenderPassCreateInfo();
  renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
  renderPassCreateInfo.attachmentCount =
      static_cast<uint32_t>(attachmentDescriptions.size());
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpass;
  renderPassCreateInfo.dependencyCount =
      static_cast<uint32_t>(subpassDependencies.size());
  renderPassCreateInfo.pDependencies = subpassDependencies.data();
  VK_CHECK_RESULT(
      vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass));

  std::vector<VkImageView> attachmentViews;
  for (const auto &attachment : attachments) {
    attachmentViews.emplace_back(attachment.view);
  }

  uint32_t maxLayers = 0;
  for (const auto &attachment : attachments) {
    if (attachment.subresourceRange.layerCount > maxLayers) {
      maxLayers = attachment.subresourceRange.layerCount;
    }
  }

  VkFramebufferCreateInfo framebufferCreateInfo =
      Initializer::FramebufferCreateInfo();
  framebufferCreateInfo.renderPass = renderPass;
  framebufferCreateInfo.pAttachments = attachmentViews.data();
  framebufferCreateInfo.attachmentCount =
      static_cast<uint32_t>(attachmentViews.size());
  framebufferCreateInfo.width = width;
  framebufferCreateInfo.height = height;
  framebufferCreateInfo.layers = maxLayers;
  VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr,
                                      &framebuffer));

  return VK_SUCCESS;
}
