#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

struct Device;

namespace Shader {
VkPipelineShaderStageCreateInfo
Create(const Device& device, const std::string &filepath,
       VkShaderStageFlagBits stage,
       VkSpecializationInfo *specialization = nullptr);
}
