#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace Initializer {

[[maybe_unused]] inline VkDeviceQueueCreateInfo DeviceQueueCreateInfo() {
  VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
  deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  return deviceQueueCreateInfo;
}

[[maybe_unused]] inline VkDeviceQueueCreateInfo
DeviceQueueCreateInfo(uint32_t queueFamilyIndex, uint32_t queueCount,
                      const float *pQueuePriorities,
                      VkDeviceQueueCreateFlags flags = 0) {
  VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
  deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
  deviceQueueCreateInfo.queueCount = queueCount;
  deviceQueueCreateInfo.pQueuePriorities = pQueuePriorities;
  deviceQueueCreateInfo.flags = flags;
  return deviceQueueCreateInfo;
}

[[maybe_unused]] inline VkMemoryAllocateInfo MemoryAllocateInfo() {
  VkMemoryAllocateInfo memoryAllocateInfo{};
  memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  return memoryAllocateInfo;
}

[[maybe_unused]] inline VkMappedMemoryRange MappedMemoryRange() {
  VkMappedMemoryRange mappedMemoryRange{};
  mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  return mappedMemoryRange;
}

[[maybe_unused]] inline VkCommandBufferAllocateInfo
CommandBufferAllocateInfo(VkCommandPool cmdPool, VkCommandBufferLevel level,
                          uint32_t bufferCount) {
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
  commandBufferAllocateInfo.sType =
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.commandPool = cmdPool;
  commandBufferAllocateInfo.level = level;
  commandBufferAllocateInfo.commandBufferCount = bufferCount;
  return commandBufferAllocateInfo;
}

[[maybe_unused]] inline VkCommandPoolCreateInfo CommandPoolCreateInfo() {
  VkCommandPoolCreateInfo commandPoolCreateInfo{};
  commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  return commandPoolCreateInfo;
}

[[maybe_unused]] inline VkCommandBufferBeginInfo CommandBufferBeginInfo() {
  VkCommandBufferBeginInfo commandBufferBeginInfo{};
  commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  return commandBufferBeginInfo;
}

[[maybe_unused]] inline VkRenderPassBeginInfo RenderPassBeginInfo() {
  VkRenderPassBeginInfo renderPassBeginInfo{};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  return renderPassBeginInfo;
}

[[maybe_unused]] inline VkRenderPassCreateInfo RenderPassCreateInfo() {
  VkRenderPassCreateInfo renderPassCreateInfo{};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  return renderPassCreateInfo;
}

[[maybe_unused]] inline VkImageMemoryBarrier ImageMemoryBarrier() {
  VkImageMemoryBarrier imageMemoryBarrier{};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  return imageMemoryBarrier;
}

[[maybe_unused]] inline VkBufferMemoryBarrier BufferMemoryBarrier() {
  VkBufferMemoryBarrier bufferMemoryBarrier{};
  bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  return bufferMemoryBarrier;
}

[[maybe_unused]] inline VkMemoryBarrier MemoryBarrier() {
  VkMemoryBarrier memoryBarrier{};
  memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  return memoryBarrier;
}

[[maybe_unused]] inline VkImageCreateInfo ImageCreateInfo() {
  VkImageCreateInfo imageCreateInfo{};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  return imageCreateInfo;
}

[[maybe_unused]] inline VkSamplerCreateInfo SamplerCreateInfo() {
  VkSamplerCreateInfo samplerCreateInfo{};
  samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCreateInfo.maxAnisotropy = 1.0f;
  return samplerCreateInfo;
}

[[maybe_unused]] inline VkImageViewCreateInfo ImageViewCreateInfo() {
  VkImageViewCreateInfo imageViewCreateInfo{};
  imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  return imageViewCreateInfo;
}

[[maybe_unused]] inline VkFramebufferCreateInfo FramebufferCreateInfo() {
  VkFramebufferCreateInfo framebufferCreateInfo{};
  framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  return framebufferCreateInfo;
}

[[maybe_unused]] inline VkSemaphoreCreateInfo SemaphoreCreateInfo() {
  VkSemaphoreCreateInfo semaphore{};
  semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  return semaphore;
}

[[maybe_unused]] inline VkFenceCreateInfo
FenceCreateInfo(VkFenceCreateFlags flags = 0) {
  VkFenceCreateInfo fence{};
  fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence.flags = flags;
  return fence;
}

[[maybe_unused]] inline VkEventCreateInfo EventCreateInfo() {
  VkEventCreateInfo eventCreateInfo{};
  eventCreateInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  return eventCreateInfo;
}

[[maybe_unused]] inline VkSubmitInfo SubmitInfo() {
  VkSubmitInfo submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  return submit;
}

[[maybe_unused]] inline VkViewport Viewport(float width, float height,
                                            float minDepth, float maxDepth) {
  VkViewport viewport{};
  viewport.width = width;
  viewport.height = height;
  viewport.minDepth = minDepth;
  viewport.maxDepth = maxDepth;
  return viewport;
}

[[maybe_unused]] inline VkRect2D Rect2D(uint32_t width, uint32_t height,
                                        int32_t offsetX, int32_t offsetY) {
  VkRect2D rect2D{};
  rect2D.extent.width = width;
  rect2D.extent.height = height;
  rect2D.offset.x = offsetX;
  rect2D.offset.y = offsetY;
  return rect2D;
}

[[maybe_unused]] inline VkBufferCreateInfo BufferCreateInfo() {
  VkBufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  return bufferCreateInfo;
}

[[maybe_unused]] inline VkBufferCreateInfo
BufferCreateInfo(VkBufferUsageFlags usage, VkDeviceSize size) {
  VkBufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.usage = usage;
  bufferCreateInfo.size = size;
  return bufferCreateInfo;
}

[[maybe_unused]] inline VkDescriptorPoolCreateInfo
DescriptorPoolCreateInfo(uint32_t poolSizeCount,
                         VkDescriptorPoolSize *pPoolSize, uint32_t maxSets) {
  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
  descriptorPoolCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.poolSizeCount = poolSizeCount;
  descriptorPoolCreateInfo.pPoolSizes = pPoolSize;
  descriptorPoolCreateInfo.maxSets = maxSets;
  return descriptorPoolCreateInfo;
}

[[maybe_unused]] inline VkDescriptorPoolCreateInfo
DescriptorPoolCreateInfo(const std::vector<VkDescriptorPoolSize> &poolSize,
                         uint32_t maxSets) {
  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
  descriptorPoolCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.poolSizeCount =
      static_cast<uint32_t>(poolSize.size());
  descriptorPoolCreateInfo.pPoolSizes = poolSize.data();
  descriptorPoolCreateInfo.maxSets = maxSets;
  return descriptorPoolCreateInfo;
}

[[maybe_unused]] inline VkDescriptorPoolSize
DescriptorPoolSize(VkDescriptorType type, uint32_t descriptorCount) {
  VkDescriptorPoolSize descriptorPoolSize{};
  descriptorPoolSize.type = type;
  descriptorPoolSize.descriptorCount = descriptorCount;
  return descriptorPoolSize;
}

[[maybe_unused]] inline VkDescriptorSetLayoutBinding
DescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags,
                           uint32_t binding, uint32_t descriptorCount = 1) {
  VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
  descriptorSetLayoutBinding.descriptorType = type;
  descriptorSetLayoutBinding.stageFlags = stageFlags;
  descriptorSetLayoutBinding.binding = binding;
  descriptorSetLayoutBinding.descriptorCount = descriptorCount;
  return descriptorSetLayoutBinding;
}

[[maybe_unused]] inline VkDescriptorSetLayoutCreateInfo
DescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutBinding *pBindings,
                              uint32_t bindingCount) {
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
  descriptorSetLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.pBindings = pBindings;
  descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
  return descriptorSetLayoutCreateInfo;
}

[[maybe_unused]] inline VkDescriptorSetLayoutCreateInfo
DescriptorSetLayoutCreateInfo(
    const std::vector<VkDescriptorSetLayoutBinding> &bindings) {
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
  descriptorSetLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.pBindings = bindings.data();
  descriptorSetLayoutCreateInfo.bindingCount =
      static_cast<uint32_t>(bindings.size());
  return descriptorSetLayoutCreateInfo;
}

[[maybe_unused]] inline VkPipelineLayoutCreateInfo
PipelineLayoutCreateInfo(const VkDescriptorSetLayout *pSetLayouts,
                         uint32_t setLayoutCount = 1) {
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
  pipelineLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
  pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
  return pipelineLayoutCreateInfo;
}

[[maybe_unused]] inline VkPipelineLayoutCreateInfo
PipelineLayoutCreateInfo(uint32_t setLayoutCount = 1) {
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
  pipelineLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
  return pipelineLayoutCreateInfo;
}

[[maybe_unused]] inline VkDescriptorSetAllocateInfo
DescriptorSetAllocateInfo(VkDescriptorPool descriptorPool,
                          const VkDescriptorSetLayout *pSetLayouts,
                          uint32_t descriptorSetCount) {
  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
  descriptorSetAllocateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocateInfo.descriptorPool = descriptorPool;
  descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
  descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
  return descriptorSetAllocateInfo;
}

[[maybe_unused]] inline VkDescriptorImageInfo
DescriptorImageInfo(VkSampler sampler, VkImageView imageView,
                    VkImageLayout imageLayout) {
  VkDescriptorImageInfo descriptorImageInfo{};
  descriptorImageInfo.sampler = sampler;
  descriptorImageInfo.imageView = imageView;
  descriptorImageInfo.imageLayout = imageLayout;
  return descriptorImageInfo;
}

[[maybe_unused]] inline VkWriteDescriptorSet
WriteDescriptorSet(VkDescriptorSet dstSet, VkDescriptorType descriptorType,
                   uint32_t dstBinding, VkDescriptorBufferInfo *pBufferInfo,
                   uint32_t descriptorCount = 1) {
  VkWriteDescriptorSet writeDescriptorSet{};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.dstSet = dstSet;
  writeDescriptorSet.descriptorType = descriptorType;
  writeDescriptorSet.dstBinding = dstBinding;
  writeDescriptorSet.pBufferInfo = pBufferInfo;
  writeDescriptorSet.descriptorCount = descriptorCount;
  return writeDescriptorSet;
}

[[maybe_unused]] inline VkWriteDescriptorSet
WriteDescriptorSet(VkDescriptorSet dstSet, VkDescriptorType descriptorType,
                   uint32_t dstBinding, VkDescriptorImageInfo *pImageInfo,
                   uint32_t descriptorCount = 1) {
  VkWriteDescriptorSet writeDescriptorSet{};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.dstSet = dstSet;
  writeDescriptorSet.descriptorType = descriptorType;
  writeDescriptorSet.dstBinding = dstBinding;
  writeDescriptorSet.pImageInfo = pImageInfo;
  writeDescriptorSet.descriptorCount = descriptorCount;
  return writeDescriptorSet;
}

[[maybe_unused]] inline VkVertexInputBindingDescription
VertexInputBindingDescription(uint32_t binding, uint32_t stride,
                              VkVertexInputRate inputRate) {
  VkVertexInputBindingDescription vertexInputBindingDescription{};
  vertexInputBindingDescription.binding = binding;
  vertexInputBindingDescription.stride = stride;
  vertexInputBindingDescription.inputRate = inputRate;
  return vertexInputBindingDescription;
}

[[maybe_unused]] inline VkVertexInputAttributeDescription
VertexInputAttributeDescription(uint32_t binding, uint32_t location,
                                VkFormat format, uint32_t offset) {
  VkVertexInputAttributeDescription vertexInputAttributeDescription{};
  vertexInputAttributeDescription.binding = binding;
  vertexInputAttributeDescription.location = location;
  vertexInputAttributeDescription.format = format;
  vertexInputAttributeDescription.offset = offset;
  return vertexInputAttributeDescription;
}

[[maybe_unused]] inline VkPipelineVertexInputStateCreateInfo
PipelineVertexInputStateCreateInfo() {
  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
  pipelineVertexInputStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  return pipelineVertexInputStateCreateInfo;
}

[[maybe_unused]] inline VkPipelineVertexInputStateCreateInfo
PipelineVertexInputStateCreateInfo(
    const std::vector<VkVertexInputBindingDescription>
        &vertexBindingDescriptions,
    const std::vector<VkVertexInputAttributeDescription>
        &vertexAttributeDescriptions) {
  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
  pipelineVertexInputStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount =
      static_cast<uint32_t>(vertexBindingDescriptions.size());
  pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions =
      vertexBindingDescriptions.data();
  pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(vertexAttributeDescriptions.size());
  pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions =
      vertexAttributeDescriptions.data();
  return pipelineVertexInputStateCreateInfo;
}

[[maybe_unused]] inline VkPipelineInputAssemblyStateCreateInfo
PipelineInputAssemblyStateCreateInfo(
    VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flags,
    VkBool32 primitiveRestartEnable) {
  VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
  pipelineInputAssemblyStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  pipelineInputAssemblyStateCreateInfo.topology = topology;
  pipelineInputAssemblyStateCreateInfo.flags = flags;
  pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable =
      primitiveRestartEnable;
  return pipelineInputAssemblyStateCreateInfo;
}

[[maybe_unused]] inline VkPipelineRasterizationStateCreateInfo
PipelineRasterizationStateCreateInfo(
    VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace,
    VkPipelineRasterizationStateCreateFlags flags = 0) {
  VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
  pipelineRasterizationStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
  pipelineRasterizationStateCreateInfo.cullMode = cullMode;
  pipelineRasterizationStateCreateInfo.frontFace = frontFace;
  pipelineRasterizationStateCreateInfo.flags = flags;
  pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
  pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
  return pipelineRasterizationStateCreateInfo;
}

[[maybe_unused]] inline VkPipelineColorBlendAttachmentState
PipelineColorBlendAttachmentState(VkColorComponentFlags colorWriteMask,
                                  VkBool32 blendEnable) {
  VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
  pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
  pipelineColorBlendAttachmentState.blendEnable = blendEnable;
  return pipelineColorBlendAttachmentState;
}

[[maybe_unused]] inline VkPipelineColorBlendStateCreateInfo
PipelineColorBlendStateCreateInfo(
    uint32_t attachmentCount,
    const VkPipelineColorBlendAttachmentState *pAttachments) {
  VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
  pipelineColorBlendStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
  pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
  return pipelineColorBlendStateCreateInfo;
}

[[maybe_unused]] inline VkPipelineDepthStencilStateCreateInfo
PipelineDepthStencilStateCreateInfo(VkBool32 depthTestEnable,
                                    VkBool32 depthWriteEnable,
                                    VkCompareOp depthCompareOp) {
  VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
  pipelineDepthStencilStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
  pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
  pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
  pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
  return pipelineDepthStencilStateCreateInfo;
}

[[maybe_unused]] inline VkPipelineViewportStateCreateInfo
PipelineViewportStateCreateInfo(uint32_t viewportCount, uint32_t scissorCount,
                                VkPipelineViewportStateCreateFlags flags = 0) {
  VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
  pipelineViewportStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  pipelineViewportStateCreateInfo.viewportCount = viewportCount;
  pipelineViewportStateCreateInfo.scissorCount = scissorCount;
  pipelineViewportStateCreateInfo.flags = flags;
  return pipelineViewportStateCreateInfo;
}

[[maybe_unused]] inline VkPipelineMultisampleStateCreateInfo
PipelineMultisampleStateCreateInfo(
    VkSampleCountFlagBits rasterizationSamples,
    VkPipelineMultisampleStateCreateFlags flags = 0) {
  VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
  pipelineMultisampleStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  pipelineMultisampleStateCreateInfo.rasterizationSamples =
      rasterizationSamples;
  pipelineMultisampleStateCreateInfo.flags = flags;
  return pipelineMultisampleStateCreateInfo;
}

[[maybe_unused]] inline VkPipelineDynamicStateCreateInfo
PipelineDynamicStateCreateInfo(const VkDynamicState *pDynamicStates,
                               uint32_t dynamicStateCount,
                               VkPipelineDynamicStateCreateFlags flags = 0) {
  VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
  pipelineDynamicStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
  pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
  pipelineDynamicStateCreateInfo.flags = flags;
  return pipelineDynamicStateCreateInfo;
}

[[maybe_unused]] inline VkPipelineDynamicStateCreateInfo
PipelineDynamicStateCreateInfo(const std::vector<VkDynamicState> &dynamicStates,
                               VkPipelineDynamicStateCreateFlags flags = 0) {
  VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
  pipelineDynamicStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
  pipelineDynamicStateCreateInfo.dynamicStateCount =
      static_cast<uint32_t>(dynamicStates.size());
  pipelineDynamicStateCreateInfo.flags = flags;
  return pipelineDynamicStateCreateInfo;
}

[[maybe_unused]] inline VkPipelineTessellationStateCreateInfo
PipelineTessellationStateCreateInfo(uint32_t patchControlPoints) {
  VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo{};
  pipelineTessellationStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
  pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
  return pipelineTessellationStateCreateInfo;
}

[[maybe_unused]] inline VkGraphicsPipelineCreateInfo
PipelineCreateInfo(VkPipelineLayout layout, VkRenderPass renderPass,
                   VkPipelineCreateFlags flags = 0) {
  VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.layout = layout;
  pipelineCreateInfo.renderPass = renderPass;
  pipelineCreateInfo.flags = flags;
  pipelineCreateInfo.basePipelineIndex = -1;
  pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  return pipelineCreateInfo;
}

[[maybe_unused]] inline VkGraphicsPipelineCreateInfo
PipelineCreateInfo(VkPipelineLayout layout, VkPipelineCreateFlags flags = 0) {
  VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.layout = layout;
  pipelineCreateInfo.flags = flags;
  return pipelineCreateInfo;
}

[[maybe_unused]] inline VkGraphicsPipelineCreateInfo PipelineCreateInfo() {
  VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.basePipelineIndex = -1;
  pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  return pipelineCreateInfo;
}

} // namespace Initializer
