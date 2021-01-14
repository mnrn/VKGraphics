/**
 * @brief Pipeline
 */

#include "VK/Pipeline/Pipelines.h"

#include <boost/assert.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

#include "VK/Image/Texture.h"
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
  return std::move(buffer);
}

void Create(const Instance &instance, const std::string &filepath,
            VkShaderStageFlagBits stage, VkSpecializationInfo *specialization,
            std::vector<VkShaderModule> &modules,
            std::vector<VkPipelineShaderStageCreateInfo> &stages) {
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

  VkShaderModule module;
  if (vkCreateShaderModule(instance.device, &create, nullptr, &module)) {
    std::cerr << "Failed to create shader module: " + filepath;
    BOOST_ASSERT(false);
  }

  VkPipelineShaderStageCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.stage = stage;
  info.module = module;
  info.pName = SHADER_ENTRY_POINT;
  info.pSpecializationInfo = specialization;

  modules.emplace_back(module);
  stages.emplace_back(info);
}
} // namespace Shader

void Pipelines::Create(const Instance &instance, const Swapchain &swapchain,
                       const VkRenderPass &renderPass,
                       const nlohmann::json &config) {
  //CreateDescriptorSetLayout(instance);
  //CreateTextureSampler(instance);

  {
    VkPipelineLayoutCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create.setLayoutCount = 0;
    //create.setLayoutCount = 1;
    //create.pSetLayouts = &descriptor.layout;
    if (vkCreatePipelineLayout(instance.device, &create, nullptr, &layout)) {
      BOOST_ASSERT_MSG(false, "Failed to create pipeline layout");
    }
  }

  for (const auto &pipeline : config) {
    std::vector<VkShaderModule> modules;
    std::vector<VkPipelineShaderStageCreateInfo> stages;

    {
      const std::string &vs = pipeline["VertexShader"].get<std::string>();
      Shader::Create(instance, vs, VK_SHADER_STAGE_VERTEX_BIT, nullptr, modules,
                     stages);
      const std::string &fs = pipeline["FragmentShader"].get<std::string>();
      Shader::Create(instance, fs, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr,
                     modules, stages);
    }

    const bool useTess = pipeline.contains("TessellationControlShader");
    if (useTess) {
      const std::string &tsc =
          pipeline["TessellationControlShader"].get<std::string>();
      Shader::Create(instance, tsc, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
                     nullptr, modules, stages);
      const std::string &tse =
          pipeline["TessellationEvaluationShader"].get<std::string>();
      Shader::Create(instance, tse, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                     nullptr, modules, stages);
    }

    // TODO: VertexInput対応
    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount = 0;
    vi.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo as{};
    as.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    as.topology = useTess ? VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
                          : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    as.primitiveRestartEnable = VK_FALSE;

    VkViewport vp{};
    vp.x = 0.0f;
    vp.y = 0.0f;
    vp.width = static_cast<float>(swapchain.extent.width);
    vp.height = static_cast<float>(swapchain.extent.height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    VkRect2D sc{};
    sc.offset = {0, 0};
    sc.extent = swapchain.extent;

    VkPipelineViewportStateCreateInfo vs{};
    vs.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vs.viewportCount = 1;
    vs.pViewports = &vp;
    vs.scissorCount = 1;
    vs.pScissors = &sc;

    VkPipelineRasterizationStateCreateInfo rz{};
    rz.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rz.depthClampEnable = VK_FALSE;
    rz.rasterizerDiscardEnable = VK_FALSE;
    rz.polygonMode = pipeline.contains("Wireframe") ? VK_POLYGON_MODE_LINE
                                                    : VK_POLYGON_MODE_FILL;
    rz.lineWidth = 1.0f;
    rz.cullMode = VK_CULL_MODE_BACK_BIT;
    rz.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rz.depthBiasEnable = VK_FALSE;
    rz.depthBiasConstantFactor = 0.0f;
    rz.depthBiasClamp = 0.0f;
    rz.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.sampleShadingEnable = VK_FALSE;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    ms.minSampleShading = 1.0f;
    ms.pSampleMask = nullptr;
    ms.alphaToCoverageEnable = VK_FALSE;
    ms.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState bl{};
    bl.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    bl.blendEnable = VK_FALSE;
    bl.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    bl.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    bl.colorBlendOp = VK_BLEND_OP_ADD;
    bl.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    bl.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    bl.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo bs{};
    bs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    bs.logicOpEnable = VK_FALSE;
    bs.logicOp = VK_LOGIC_OP_COPY;
    bs.attachmentCount = 1;
    bs.pAttachments = &bl;
    bs.blendConstants[0] = 0.0f;
    bs.blendConstants[1] = 0.0f;
    bs.blendConstants[2] = 0.0f;
    bs.blendConstants[3] = 0.0f;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.minDepthBounds = 0.0f;
    ds.maxDepthBounds = 1.0f;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = {};
    ds.back = {};

    VkGraphicsPipelineCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create.stageCount = static_cast<uint32_t>(stages.size());
    create.pStages = stages.data();
    create.pVertexInputState = &vi;
    create.pInputAssemblyState = &as;
    create.pViewportState = &vs;
    create.pRasterizationState = &rz;
    create.pMultisampleState = &ms;
    create.pDepthStencilState = &ds;
    create.pColorBlendState = &bs;
    create.pDynamicState = nullptr;
    create.layout = layout;
    create.renderPass = renderPass;
    create.subpass = 0;
    create.basePipelineHandle = VK_NULL_HANDLE;
    create.basePipelineIndex = -1;

    VkPipelineTessellationStateCreateInfo ts{};
    if (useTess) {
      ts.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
      ts.patchControlPoints = pipeline["PatchControlPoints"].get<int>();
      create.pTessellationState = &ts;
    }

    VkPipeline handle;
    if (vkCreateGraphicsPipelines(instance.device, VK_NULL_HANDLE, 1, &create,
                                  nullptr, &handle)) {
      BOOST_ASSERT_MSG(false, "Failed to create graphics pipeline");
    }
    handles.emplace_back(handle);
    for (const auto &module : modules) {
      vkDestroyShaderModule(instance.device, module, nullptr);
    }
  }
}

const VkPipeline &Pipelines::operator[](size_t index) const {
  return handles[index];
}

void Pipelines::Cleanup(const Instance &instance) {
  for (const auto &handle : handles) {
    vkDestroyPipeline(instance.device, handle, nullptr);
  }
  handles.clear();
  vkDestroyPipelineLayout(instance.device, layout, nullptr);
}

void Pipelines::Destroy(const Instance &instance) {
  vkDestroySampler(instance.device, sampler, nullptr);
  if (descriptor.pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(instance.device, descriptor.pool, nullptr);
  }
  vkDestroyDescriptorSetLayout(instance.device, descriptor.layout, nullptr);
  if (uniform.global != VK_NULL_HANDLE) {
    vkDestroyBuffer(instance.device, uniform.global, nullptr);
  }
  if (uniform.local != VK_NULL_HANDLE) {
    vkDestroyBuffer(instance.device, uniform.local, nullptr);
  }
  if (uniform.globalMemory != VK_NULL_HANDLE) {
    vkFreeMemory(instance.device, uniform.globalMemory, nullptr);
  }
  if (uniform.localMemory != VK_NULL_HANDLE) {
    vkFreeMemory(instance.device, uniform.localMemory, nullptr);
  }
}

void Pipelines::CreateDescriptorSetLayout(const Instance &instance) {
  if (descriptor.layout != VK_NULL_HANDLE) {
    return;
  }

  VkDescriptorSetLayoutBinding ubo{};
  ubo.binding = 0;
  ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubo.descriptorCount = 1;
  ubo.stageFlags = VK_SHADER_STAGE_ALL;
  ubo.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding smp;
  smp.binding = 1;
  smp.descriptorCount = 1;
  smp.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  smp.stageFlags = VK_SHADER_STAGE_ALL;
  smp.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding dyn{};
  dyn.binding = 2;
  dyn.descriptorCount = 1;
  dyn.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  dyn.stageFlags = VK_SHADER_STAGE_ALL;
  dyn.pImmutableSamplers = nullptr;

  std::vector<VkDescriptorSetLayoutBinding> bindings = {ubo, smp, dyn};
  VkDescriptorSetLayoutCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  create.bindingCount = static_cast<uint32_t>(bindings.size());
  create.pBindings = bindings.data();
  if (vkCreateDescriptorSetLayout(instance.device, &create, nullptr,
                                  &descriptor.layout)) {
    BOOST_ASSERT_MSG(false, "Failed to create descriptor set layout!");
  }
}

void Pipelines::CreateTextureSampler(const Instance &instance) {
  if (sampler != VK_NULL_HANDLE) {
    return;
  }

  VkSamplerCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  create.magFilter = VK_FILTER_LINEAR;
  create.minFilter = VK_FILTER_LINEAR;
  create.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  create.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  create.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  create.anisotropyEnable = VK_TRUE;
  create.maxAnisotropy = 16;
  create.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
  create.unnormalizedCoordinates = VK_FALSE;
  create.compareEnable = VK_FALSE;
  create.compareOp = VK_COMPARE_OP_ALWAYS;
  create.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  create.mipLodBias = 0.0f;
  create.minLod = 0.0f;
  create.maxLod = 0.0f;
  if (vkCreateSampler(instance.device, &create, nullptr, &sampler)) {
    BOOST_ASSERT_MSG(false, "Failed to create texture sampler!");
  }
}

#if false

void Pipelines::CreateDescriptors(
    const Instance &instance,
    std::unordered_map<std::string, Texture> &textures, size_t objects) {
  if (descriptor.pool != VK_NULL_HANDLE) {
    return;
  }
  BOOST_ASSERT_MSG(!textures.empty(), "Cannot handle 0 textures at the moment!");

  std::array<VkDescriptorPoolSize, 3> sizes{};
  sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sizes[0].descriptorCount = static_cast<uint32_t>(textures.size());
  sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sizes[1].descriptorCount = static_cast<uint32_t>(textures.size());
  sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  sizes[2].descriptorCount = static_cast<uint32_t>(textures.size());

  VkDescriptorPoolCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  create.poolSizeCount = static_cast<uint32_t>(sizes.size());
  create.pPoolSizes = sizes.data();
  create.maxSets = static_cast<uint32_t>(textures.size());

  if (vkCreateDescriptorPool(instance.device, &create, nullptr, &descriptor.pool)) {
    BOOST_ASSERT_MSG(false, "Failed to create descriptor pool!");
  }

  CreateUniforms(instance, objects);

  for (auto & texture : textures) {
    textures.second.CreateDescriptorSet(instance, *this);
  }
}

void Pipelines::CreateUniforms(const Instance &instance, size_t objects) {
  VkDeviceSize size = sizeof(UniformBufferGlobal);
  Buffer::Create(instance, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniform.global, uniform.globalMemomy);

  size_t alignment = instance.properties.limits.minUniformBufferOffsetAlignment;
  size_t structSize = sizeof(UniformBufferLocal);
  uint32_t dynamicAlignment =
      static_cast<uint32_t>(structSize / alignment * alignment +
                            ((structSize % alignment) > 0 ? alignment : 0));
  size = objects * dynamicAlignment;
  uniform.localAlignment = dyncamicAlignment;
  Buffer::Create(instance, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, uniform.local,
                 uniform.localMemory);
}

#endif
