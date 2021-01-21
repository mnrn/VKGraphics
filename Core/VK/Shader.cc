#include "VK/Shader.h"

#include <boost/assert.hpp>
#include <fstream>
#include <iostream>
#include <variant>

#include "VK/Common.h"

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
  return std::move(buffer);
}

VkPipelineShaderStageCreateInfo Create(const std::string &filepath,
                                       VkDevice device,
                                       VkShaderStageFlagBits stage,
                                       VkSpecializationInfo *specialization) {
  const auto v = ReadFile(filepath);
  if (std::holds_alternative<std::string>(v)) {
    std::cerr << std::get<std::string>(v) << std::endl;
    BOOST_ASSERT_MSG(!std::holds_alternative<std::string>(v), "Failed to create shader!");
  }
  const auto code = std::get<std::vector<char>>(v);

  VkShaderModuleCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create.codeSize = code.size();
  create.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule module;
  VK_CHECK_RESULT(vkCreateShaderModule(device, &create, nullptr, &module));

  VkPipelineShaderStageCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.stage = stage;
  info.module = module;
  info.pName = SHADER_ENTRY_POINT;
  info.pSpecializationInfo = specialization;

  return info;
}
} // namespace Shader
