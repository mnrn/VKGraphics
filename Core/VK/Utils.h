#pragma once

#include <vulkan/vulkan.h>

#include <set>
#include <vector>

struct Device;

VkPipelineShaderStageCreateInfo
CreateShader(const Device& device, const std::string &filepath,
       VkShaderStageFlagBits stage,
       VkSpecializationInfo *specialization = nullptr);

VkResult
CreateImageView(const Device &device, VkImageView &view, VkImage image,
                VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D,
                VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
                VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                uint32_t baseMipLevel = 0, uint32_t mipLevels = 1,
                uint32_t baseArraySlice = 0, uint32_t arraySize = 1,
                VkComponentSwizzle r = VK_COMPONENT_SWIZZLE_R,
                VkComponentSwizzle g = VK_COMPONENT_SWIZZLE_G,
                VkComponentSwizzle b = VK_COMPONENT_SWIZZLE_B,
                VkComponentSwizzle a = VK_COMPONENT_SWIZZLE_A);

VkResult CreateSampler(
    const Device &device, VkSampler &sampler,
    VkFilter magFilter = VK_FILTER_LINEAR,
    VkFilter minFilter = VK_FILTER_LINEAR, VkBool32 compareEnable = VK_FALSE,
    VkCompareOp compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    float minLod = 0.0f, float maxLod = 0.0f, float mipLodBias = 0.0f,
    VkBool32 anisotropyEnable = VK_FALSE, float maxAnisotropy = 1.0f,
    VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE);

void TransitionImageLayout(
    VkCommandBuffer commandBuffer, VkImage image,
    VkImageSubresourceRange imageSubresourceRange, VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

void TransitionImageLayout(
    VkCommandBuffer commandBuffer, VkImage image,
    VkImageAspectFlags imageAspectFlags, VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

float CalcDeviceScore(VkPhysicalDevice physicalDevice,
                      const std::vector<const char *> &deviceExtensions);
