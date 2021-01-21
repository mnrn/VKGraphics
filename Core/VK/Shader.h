#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace Shader {
VkPipelineShaderStageCreateInfo
Create(const std::string &filepath, VkDevice device,
       VkShaderStageFlagBits stage,
       VkSpecializationInfo *specialization = nullptr);
}
