#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

struct Instance;

namespace Shader{
void Create(const Instance &instance, const std::string &filepath,
            VkShaderStageFlagBits stage, VkSpecializationInfo *specialization,
            std::vector<VkShaderModule> &modules,
            std::vector<VkPipelineShaderStageCreateInfo> &stages);
}
