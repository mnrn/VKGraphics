#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

struct Instance;

namespace Shader{
VkPipelineShaderStageCreateInfo Create(const Instance &instance, const std::string &filepath,
            VkShaderStageFlagBits stage, VkSpecializationInfo *specialization = nullptr);
}
