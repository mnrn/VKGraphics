#pragma once

#include <vulkan/vulkan.h>

#include <set>
#include <vector>

struct Device;

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
