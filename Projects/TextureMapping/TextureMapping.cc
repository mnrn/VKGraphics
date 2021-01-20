#include "TextureMapping.h"

#include <boost/assert.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "VK/Pipeline/Shader.h"

//*-----------------------------------------------------------------------------
// Constant expressions
//*-----------------------------------------------------------------------------

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription Bindings() {
    VkVertexInputBindingDescription bind;
    bind.binding = 0;
    bind.stride = sizeof(Vertex);
    bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bind;
  }

  static std::array<VkVertexInputAttributeDescription, 3> Attributes() {
    std::array<VkVertexInputAttributeDescription, 3> attr{};
    attr[0].binding = 0;
    attr[0].location = 0;
    attr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attr[0].offset = offsetof(Vertex, pos);

    attr[1].binding = 0;
    attr[1].location = 1;
    attr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attr[1].offset = offsetof(Vertex, color);

    attr[2].binding = 0;
    attr[2].location = 2;
    attr[2].format = VK_FORMAT_R32G32_SFLOAT;
    attr[2].offset = offsetof(Vertex, texCoord);

    return attr;
  }
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

static const std::vector<Vertex> vertices{
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}};
static const std::vector<uint16_t> indices{0, 1, 2, 2, 3, 0};

static constexpr float kRotSpeed = glm::quarter_pi<float>();

//*-----------------------------------------------------------------------------
// Overrides functions
//*-----------------------------------------------------------------------------

void TextureMapping::OnUpdate(float t) {
  const float deltaT = tPrev_ == 0.0f ? 0.0f : t - tPrev_;
  tPrev_ = t;

  angle_ += kRotSpeed * deltaT;
  if (angle_ > glm::two_pi<float>()) {
    angle_ -= glm::two_pi<float>();
  }
}

void TextureMapping::CreateRenderPass() {
  VkAttachmentDescription color{};
  color.format = swapchain.format;
  color.samples = VK_SAMPLE_COUNT_1_BIT;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorRef{};
  colorRef.attachment = 0;
  colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = nullptr;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  std::vector<VkAttachmentDescription> attachments{color};
  VkRenderPassCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  create.attachmentCount = static_cast<uint32_t>(attachments.size());
  create.pAttachments = attachments.data();
  create.subpassCount = 1;
  create.pSubpasses = &subpass;
  create.dependencyCount = 1;
  create.pDependencies = &dependency;

  if (vkCreateRenderPass(instance.device, &create, nullptr, &renderPass)) {
    BOOST_ASSERT_MSG(false, "Failed to create render pass!");
  }
}

void TextureMapping::CreateDescriptorSetLayouts() {
  if (descriptors_.layout != VK_NULL_HANDLE) {
    return;
  }

  VkDescriptorSetLayoutBinding ubo{};
  ubo.binding = 0;
  ubo.descriptorCount = 1;
  ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubo.pImmutableSamplers = nullptr;
  ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding sampler{};
  sampler.binding = 1;
  sampler.descriptorCount = 1;
  sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler.pImmutableSamplers = nullptr;
  sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::vector<VkDescriptorSetLayoutBinding> binds{ubo, sampler};
  VkDescriptorSetLayoutCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  create.bindingCount = static_cast<uint32_t>(binds.size());
  create.pBindings = binds.data();

  if (vkCreateDescriptorSetLayout(instance.device, &create, nullptr,
                                  &descriptors_.layout)) {
    BOOST_ASSERT_MSG(false, "Failed to create descriptor set layout!");
  }
}

void TextureMapping::DestroyDescriptorSetLayouts() {
  vkDestroyDescriptorSetLayout(instance.device, descriptors_.layout, nullptr);
}

void TextureMapping::CreatePipelines() {
  {
    VkPipelineLayoutCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create.setLayoutCount = 1;
    create.pSetLayouts = &descriptors_.layout;
    if (vkCreatePipelineLayout(instance.device, &create, nullptr,
                               &pipelines_.layout)) {
      BOOST_ASSERT_MSG(false, "Failed to create pipeline layout");
    }
  }

  for (const auto &pipeline : config_["Pipelines"]) {
    std::vector<VkShaderModule> modules;
    std::vector<VkPipelineShaderStageCreateInfo> stages;

    {
      const std::string &vs = pipeline["VertexShader"].get<std::string>();
      Shader::Create(instance, vs, VK_SHADER_STAGE_VERTEX_BIT, nullptr,
                     modules, stages);
      const std::string &fs = pipeline["FragmentShader"].get<std::string>();
      Shader::Create(instance, fs, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr,
                     modules, stages);
    }

    auto binds = Vertex::Bindings();
    auto attrs = Vertex::Attributes();
    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &binds;
    vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vi.pVertexAttributeDescriptions = attrs.data();

    VkPipelineInputAssemblyStateCreateInfo as{};
    as.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    as.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
    rz.polygonMode = VK_POLYGON_MODE_FILL;
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
    create.layout = pipelines_.layout;
    create.renderPass = renderPass;
    create.subpass = 0;
    create.basePipelineHandle = VK_NULL_HANDLE;
    create.basePipelineIndex = -1;

    VkPipeline handle;
    if (vkCreateGraphicsPipelines(instance.device, VK_NULL_HANDLE, 1, &create,
                                  nullptr, &handle)) {
      BOOST_ASSERT_MSG(false, "Failed to create graphics pipeline");
    }
    pipelines_.handles.emplace_back(handle);
    for (const auto &module : modules) {
      vkDestroyShaderModule(instance.device, module, nullptr);
    }
  }
}

void TextureMapping::DestroyPipelines() { pipelines_.Destroy(instance); }

void TextureMapping::CreateFramebuffers() {
  framebuffers.resize(swapchain.views.size());
  for (size_t i = 0; i < framebuffers.size(); i++) {
    std::vector<VkImageView> attachments = {swapchain.views[i],
                                            /* depthView */};
    VkFramebufferCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create.renderPass = renderPass;
    create.attachmentCount = static_cast<uint32_t>(attachments.size());
    create.pAttachments = attachments.data();
    create.width = swapchain.extent.width;
    create.height = swapchain.extent.height;
    create.layers = 1;
    if (vkCreateFramebuffer(instance.device, &create, nullptr,
                            &framebuffers[i])) {
      BOOST_ASSERT_MSG(false, "Failed to create framebuffer!");
    }
  }
}

void TextureMapping::SetupAssets() {
  texture_.Create(instance, "./Assets/Textures/SelfMade/star.png");

  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(instance.physicalDevice, &props);

  VkSamplerCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  create.magFilter = VK_FILTER_LINEAR;
  create.minFilter = VK_FILTER_LINEAR;
  create.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  create.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  create.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  create.anisotropyEnable = VK_TRUE;
  create.maxAnisotropy = props.limits.maxSamplerAnisotropy;
  create.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  create.unnormalizedCoordinates = VK_FALSE;
  create.compareEnable = VK_FALSE;
  create.compareOp = VK_COMPARE_OP_ALWAYS;
  create.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if (vkCreateSampler(instance.device, &create, nullptr, &texture_.sampler)) {
    BOOST_ASSERT_MSG(false, "Failed to create texture sampler!");
  }
}

void TextureMapping::CleanupAssets() { texture_.Destroy(instance); }

void TextureMapping::CreateVertexBuffer() {
  vertex_.Create(instance, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void TextureMapping::CreateIndexBuffer() {
  index_.Create(instance, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void TextureMapping::CreateDrawCommandBuffers() {
  commandBuffers_.draw.resize(framebuffers.size());

  VkCommandBufferAllocateInfo alloc{};
  alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc.commandPool = instance.pool;
  alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc.commandBufferCount = static_cast<uint32_t>(commandBuffers_.draw.size());
  if (vkAllocateCommandBuffers(instance.device, &alloc,
                               commandBuffers_.draw.data())) {
    BOOST_ASSERT_MSG(false, "Failed to alloc draw command buffers!");
  }

  for (size_t i = 0; i < commandBuffers_.draw.size(); i++) {
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffers_.draw[i], &begin)) {
      BOOST_ASSERT_MSG(false, "Failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo render{};
    render.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render.renderPass = renderPass;
    render.framebuffer = framebuffers[i];
    render.renderArea.offset = {0, 0};
    render.renderArea.extent = swapchain.extent;

    std::array<VkClearValue, 1> clear{};
    clear[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    render.clearValueCount = static_cast<uint32_t>(clear.size());
    render.pClearValues = clear.data();

    vkCmdBeginRenderPass(commandBuffers_.draw[i], &render,
                         VK_SUBPASS_CONTENTS_INLINE);
    {
      vkCmdBindPipeline(commandBuffers_.draw[i],
                        VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines_[0]);
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffers_.draw[i], 0, 1, &vertex_.buffer,
                             offsets);
      vkCmdBindIndexBuffer(commandBuffers_.draw[i], index_.buffer, 0,
                           VK_INDEX_TYPE_UINT16);
      vkCmdBindDescriptorSets(
          commandBuffers_.draw[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipelines_.layout, 0, 1, &descriptors_[i], 0, nullptr);

      vkCmdDrawIndexed(commandBuffers_.draw[i],
                       static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(commandBuffers_.draw[i]);

    if (vkEndCommandBuffer(commandBuffers_.draw[i])) {
      BOOST_ASSERT_MSG(false, "Failed to record draw command buffer!");
    }
  }
}

void TextureMapping::CreateUniformBuffers() {
  VkDeviceSize size = sizeof(UniformBufferObject);
  uniforms_.resize(swapchain.images.size());

  for (size_t i = 0; i < swapchain.images.size(); i++) {
    Buffer::Create(instance, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   uniforms_[i].buffer, uniforms_[i].memory);
  }
}

void TextureMapping::DestroyUniformBuffers() {
  for (size_t i = 0; i < swapchain.images.size(); i++) {
    uniforms_[i].Destroy(instance);
  }
}

void TextureMapping::UpdateUniformBuffers(uint32_t imageIndex) {
  UniformBufferObject ubo;
  ubo.model = glm::rotate(glm::mat4(1.0f), angle_, glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view =
      glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f),
                              static_cast<float>(swapchain.extent.width) /
                                  static_cast<float>(swapchain.extent.height),
                              0.1f, 10.0f);
  ubo.proj[1][1] *= -1;

  void *data;
  vkMapMemory(instance.device, uniforms_[imageIndex].memory, 0, sizeof(ubo), 0,
              &data);
  std::memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(instance.device, uniforms_[imageIndex].memory);
}

void TextureMapping::CreateDescriptorPool() {
  std::array<VkDescriptorPoolSize, 2> sizes{};
  sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sizes[0].descriptorCount = static_cast<uint32_t>(swapchain.images.size());
  sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sizes[1].descriptorCount = static_cast<uint32_t>(swapchain.images.size());

  VkDescriptorPoolCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  create.poolSizeCount = static_cast<uint32_t>(sizes.size());
  create.pPoolSizes = sizes.data();
  create.maxSets = static_cast<uint32_t>(swapchain.images.size());

  if (vkCreateDescriptorPool(instance.device, &create, nullptr,
                             &descriptorPool)) {
    BOOST_ASSERT_MSG(false, "Failed to create descriptor pool!");
  }
}

void TextureMapping::CreateDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(swapchain.images.size(),
                                             descriptors_.layout);
  VkDescriptorSetAllocateInfo alloc{};
  alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc.descriptorPool = descriptorPool;
  alloc.descriptorSetCount = static_cast<uint32_t>(swapchain.images.size());
  alloc.pSetLayouts = layouts.data();

  descriptors_.handles.resize(swapchain.images.size());
  if (vkAllocateDescriptorSets(instance.device, &alloc,
                               descriptors_.handles.data())) {
    BOOST_ASSERT_MSG(false, "Failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < swapchain.images.size(); i++) {
    VkDescriptorBufferInfo buffer{};
    buffer.buffer = uniforms_[i].buffer;
    buffer.offset = 0;
    buffer.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo image{};
    image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image.imageView = texture_.view;
    image.sampler = texture_.sampler;

    std::array<VkWriteDescriptorSet, 2> writes{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptors_[i];
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo = &buffer;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = descriptors_[i];
    writes[1].dstBinding = 1;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &image;

    vkUpdateDescriptorSets(instance.device,
                           static_cast<uint32_t>(writes.size()), writes.data(),
                           0, nullptr);
  }
}
