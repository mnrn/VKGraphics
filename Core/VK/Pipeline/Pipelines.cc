/**
 * @brief Pipeline
 */

#include "VK/Pipeline/Pipelines.h"

#include <boost/assert.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <variant>

#include "VK/Instance.h"
#include "VK/Swapchain.h"

#define SHADER_ENTRY_POINT "main"

namespace Shader {

static std::variant<std::string, std::vector<char>>
ReadFile(const std::string &filename) {
  std::ifstream fin(filename, std::ios::ate | std::ios::binary);
  if (!fin.is_open()) {
    return "Failed to open file: " + filename;
  }
  const auto size = static_cast<size_t>(fin.tellg());
  std::vector<char> buffer(size);
  fin.seekg(0);
  fin.read(buffer.data(), size);

  fin.close();
  return buffer;
}

void Create(const Instance &instance, const std::string &filepath,
            VkShaderStageFlagBits stage, VkSpecializationInfo *specialization,
            std::vector<VkShaderModule> &shaders,
            std::vector<VkPipelineShaderStageCreateInfo> &infos) {
  const auto v = ReadFile(filepath);
  if (std::holds_alternative<std::string>(v)) {
    std::cerr << std::get<std::string>(v) << std::endl;
    BOOST_ASSERT_MSG(false, "Failed to create shader!");
  }
  const auto code = std::get<std::vector<char>>(v);

  VkShaderModuleCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create.codeSize = code.size();
  create.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shader;
  if (vkCreateShaderModule(instance.device, &create, nullptr, &shader)) {
    std::cerr << "Failed to create shader module: " + filepath;
    BOOST_ASSERT(false);
  }

  VkPipelineShaderStageCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.stage = stage;
  info.module = shader;
  info.pName = SHADER_ENTRY_POINT;
  info.pSpecializationInfo = specialization;

  shaders.emplace_back(shader);
  infos.emplace_back(info);
}
} // namespace Shader

void Pipelines::Create(const Instance &instance, const Swapchain &swapchain,
                       const VkRenderPass &renderpass, nlohmann::json &config) {
  CreateDescriptorSetLayout(instance);
  CreateTextureSampler(instance);

  VkPipelineLayoutCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create.setLayoutCount = 1;
  create.pSetLayouts = &descriptor.layout;
  /*
  if (push) {
    std::vector<VkPushConstantRange> ranges(2);
    ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ranges[0].offset = push.VertexOffset();
    ranges[0].size = push.VertexSize();
    ranges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    ranges[1].offset = push.FragmentOffset();
    ranges[1].size = push.FragmentSize();
    create.pushConstantRangeCount = static_cast<uint32_t>(ranges.size());
    create.pPushConstantRanges = ranges.data();
  }
  */
  if (vkCreatePipelineLayout(instance.device, &create, nullptr, &layout)) {
    BOOST_ASSERT_MSG(false, "Failed to create pipeline layout");
  }
}

const VkPipeline& Pipelines::operator[] (size_t index) const {
  return handles[index];
}

void Pipelines::Clear(const Instance& instance) {
  for (const auto& handle : handles) {
    vkDestroyPipeline(instance.device, handle, nullptr);
  }
  handles.clear();
  vkDestroyPipelineLayout(instance.device, layout, nullptr);
}

void Pipeline::CreateDescriptorSetLayout(const Instance& instance) {

}
