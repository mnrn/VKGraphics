#include "UniformBuffers.h"

#include <boost/assert.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "VK/Pipeline/Shader.h"

//*-----------------------------------------------------------------------------
// Constant expressions
//*-----------------------------------------------------------------------------

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription Bindings() {
    VkVertexInputBindingDescription bind;
    bind.binding = 0;
    bind.stride = sizeof(Vertex);
    bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bind;
  }

  static std::array<VkVertexInputAttributeDescription, 2> Attributes() {
    std::array<VkVertexInputAttributeDescription, 2> attr{};
    attr[0].binding = 0;
    attr[0].location = 0;
    attr[0].format = VK_FORMAT_R32G32_SFLOAT;
    attr[0].offset = offsetof(Vertex, pos);
    attr[1].binding = 0;
    attr[1].location = 1;
    attr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attr[1].offset = offsetof(Vertex, color);

    return attr;
  }
};

static const std::vector<Vertex> vertices{{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                          {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                          {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
static const std::vector<uint16_t> indices{0, 1, 2};

//*-----------------------------------------------------------------------------
// Overrides functions
//*-----------------------------------------------------------------------------

void UniformBuffers::CreateRenderPass() {
  VkAttachmentDescription color{};
  color.format = swapchain_.format;
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

  if (vkCreateRenderPass(instance_.device, &create, nullptr, &renderPass_)) {
    BOOST_ASSERT_MSG(false, "Failed to create render pass!");
  }
}

void UniformBuffers::CreatePipelines() {
  {
    VkPipelineLayoutCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create.setLayoutCount = 0;
    if (vkCreatePipelineLayout(instance_.device, &create, nullptr,
                               &pipelines_.layout)) {
      BOOST_ASSERT_MSG(false, "Failed to create pipeline layout");
    }
  }

  for (const auto &pipeline : config_["Pipelines"]) {
    std::vector<VkShaderModule> modules;
    std::vector<VkPipelineShaderStageCreateInfo> stages;

    {
      const std::string &vs = pipeline["VertexShader"].get<std::string>();
      Shader::Create(instance_, vs, VK_SHADER_STAGE_VERTEX_BIT, nullptr,
                     modules, stages);
      const std::string &fs = pipeline["FragmentShader"].get<std::string>();
      Shader::Create(instance_, fs, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr,
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
    vp.width = static_cast<float>(swapchain_.extent.width);
    vp.height = static_cast<float>(swapchain_.extent.height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    VkRect2D sc{};
    sc.offset = {0, 0};
    sc.extent = swapchain_.extent;

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
    create.renderPass = renderPass_;
    create.subpass = 0;
    create.basePipelineHandle = VK_NULL_HANDLE;
    create.basePipelineIndex = -1;

    VkPipeline handle;
    if (vkCreateGraphicsPipelines(instance_.device, VK_NULL_HANDLE, 1, &create,
                                  nullptr, &handle)) {
      BOOST_ASSERT_MSG(false, "Failed to create graphics pipeline");
    }
    pipelines_.handles.emplace_back(handle);
    for (const auto &module : modules) {
      vkDestroyShaderModule(instance_.device, module, nullptr);
    }
  }
}

void UniformBuffers::CreateFramebuffers() {
  framebuffers_.resize(swapchain_.views.size());
  for (size_t i = 0; i < framebuffers_.size(); i++) {
    std::vector<VkImageView> attachments = {swapchain_.views[i],
                                            /* depthView */};
    VkFramebufferCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create.renderPass = renderPass_;
    create.attachmentCount = static_cast<uint32_t>(attachments.size());
    create.pAttachments = attachments.data();
    create.width = swapchain_.extent.width;
    create.height = swapchain_.extent.height;
    create.layers = 1;
    if (vkCreateFramebuffer(instance_.device, &create, nullptr,
                            &framebuffers_[i])) {
      BOOST_ASSERT_MSG(false, "Failed to create framebuffer!");
    }
  }
}

void UniformBuffers::CreateVertexBuffer() {
  vertex_.Create(instance_, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void UniformBuffers::CreateIndexBuffer() {
  index_.Create(instance_, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void UniformBuffers::CreateDrawCommandBuffers() {
  commandBuffers_.draw.resize(framebuffers_.size());

  VkCommandBufferAllocateInfo alloc{};
  alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc.commandPool = instance_.pool;
  alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc.commandBufferCount = static_cast<uint32_t>(commandBuffers_.draw.size());
  if (vkAllocateCommandBuffers(instance_.device, &alloc,
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
    render.renderPass = renderPass_;
    render.framebuffer = framebuffers_[i];
    render.renderArea.offset = {0, 0};
    render.renderArea.extent = swapchain_.extent;

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

      vkCmdDrawIndexed(commandBuffers_.draw[i],
                       static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(commandBuffers_.draw[i]);

    if (vkEndCommandBuffer(commandBuffers_.draw[i])) {
      BOOST_ASSERT_MSG(false, "Failed to record draw command buffer!");
    }
  }
}
