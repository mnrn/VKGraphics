/**
 * #brief ImGuiを使ってVulkan用のGUIクラスを構築します。
 */

#include "VK/Gui.h"

#include <imgui.h>

#include <vulkan/vulkan.h>

#include "VK/Common.h"
#include "VK/Device.h"
#include "VK/Initializer.h"
#include "VK/Utils.h"

#define UI_OVERLAY_VERTEX_SHADER_PATH                                          \
  "./Assets/Shaders/GLSL/SPIR-V/UI/UIOverlay.vs.spv"
#define UI_OVERLAY_FRAGMENT_SHADER_PATH                                        \
  "./Assets/Shaders/GLSL/SPIR-V/UI/UIOverlay.fs.spv"

Gui::Gui(const Device &device, VkQueue queue, VkPipelineCache pipelineCache,
         VkRenderPass renderPass) {
  ImGui::CreateContext();
  ImGui::StyleColorsClassic();

  SetupResources(device, queue);
  SetupPipeline(device, pipelineCache, renderPass);
}

void Gui::OnDestroy(const Device &device) const {
  vkDestroyPipeline(device, pipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroySampler(device, sampler, nullptr);
  vkDestroyImageView(device, font.view, nullptr);
  vkFreeMemory(device, font.memory, nullptr);
  vkDestroyImage(device, font.image, nullptr);
  indexBuffer.Destroy(device);
  vertexBuffer.Destroy(device);
  ImGui::DestroyContext();
}

void Gui::OnResize(uint32_t width, uint32_t height) {
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize =
      ImVec2(static_cast<float>(width), static_cast<float>(height));
}

/**
 * @brief 必要に応じてImGuiの頂点とインデックスバッファを更新します。
 */
bool Gui::Update(const Device &device) {
  ImDrawData *imDrawData = ImGui::GetDrawData();
  if (imDrawData == nullptr) {
    return false;
  }

  VkDeviceSize vertexBufferSize =
      imDrawData->TotalVtxCount * sizeof(ImDrawVert);
  VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

  if (vertexBufferSize == 0 || indexBufferSize == 0) {
    return false;
  }

  bool updateCmdBuffers = false;
  // 頂点バッファ
  if ((vertexBuffer.buffer == VK_NULL_HANDLE) ||
      (vertexCount != static_cast<uint32_t>(imDrawData->TotalVtxCount))) {
    vertexBuffer.Unmap(device);
    vertexBuffer.Destroy(device);

    VK_CHECK_RESULT(vertexBuffer.Create(
        device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBufferSize));

    vertexCount = imDrawData->TotalVtxCount;
    VK_CHECK_RESULT(vertexBuffer.Map(device));
    updateCmdBuffers = true;
  }

  // インデックスバッファ
  if ((indexBuffer.buffer == VK_NULL_HANDLE) ||
      (indexCount < static_cast<uint32_t>(imDrawData->TotalIdxCount))) {
    indexBuffer.Unmap(device);
    indexBuffer.Destroy(device);
    VK_CHECK_RESULT(indexBuffer.Create(device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                       indexBufferSize));
    indexCount = imDrawData->TotalIdxCount;
    VK_CHECK_RESULT(indexBuffer.Map(device));
    updateCmdBuffers = true;
  }

  // データをアップロードします。
  auto *vtxDst = static_cast<ImDrawVert *>(vertexBuffer.mapped);
  auto *idxDst = static_cast<ImDrawIdx *>(indexBuffer.mapped);
  for (int i = 0; i < imDrawData->CmdListsCount; i++) {
    const ImDrawList *cmdList = imDrawData->CmdLists[i];
    std::memcpy(vtxDst, cmdList->VtxBuffer.Data,
                cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
    std::memcpy(idxDst, cmdList->IdxBuffer.Data,
                cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
    vtxDst += cmdList->VtxBuffer.Size;
    idxDst += cmdList->IdxBuffer.Size;
  }

  // 書き込みをGPUに表示するためにフラッシュします。
  VK_CHECK_RESULT(vertexBuffer.Flush(device));
  VK_CHECK_RESULT(indexBuffer.Flush(device));

  return updateCmdBuffers;
}

void Gui::Draw(VkCommandBuffer commandBuffer) {
  ImDrawData *imDrawData = ImGui::GetDrawData();
  if ((imDrawData == nullptr) || (imDrawData->CmdListsCount == 0)) {
    return;
  }

  ImGuiIO &io = ImGui::GetIO();
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

  pushConst.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
  pushConst.translate = glm::vec2(-1.0f);
  vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                     0, sizeof(PushConst), &pushConst);

  VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0,
                       VK_INDEX_TYPE_UINT16);

  int32_t vertexOffset = 0;
  int32_t indexOffset = 0;
  for (int i = 0; i < imDrawData->CmdListsCount; i++) {
    const ImDrawList *cmdList = imDrawData->CmdLists[i];
    for (int j = 0; j < cmdList->CmdBuffer.size(); j++) {
      const ImDrawCmd *pCmd = &cmdList->CmdBuffer[j];
      VkRect2D scissor = Initializer::Rect2D(
          static_cast<uint32_t>(pCmd->ClipRect.z - pCmd->ClipRect.x),
          static_cast<uint32_t>(pCmd->ClipRect.w - pCmd->ClipRect.y),
          std::max(static_cast<int32_t>(pCmd->ClipRect.x), 0),
          std::max(static_cast<int32_t>(pCmd->ClipRect.y), 0));
      vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
      vkCmdDrawIndexed(commandBuffer, pCmd->ElemCount, 1, indexOffset,
                       vertexOffset, 0);
      indexOffset += pCmd->ElemCount;
    }
    vertexOffset += cmdList->VtxBuffer.Size;
  }
}

//*-----------------------------------------------------------------------------
// Setup
//*-----------------------------------------------------------------------------

/**
 * @brief GUI構築に必要なVulkanリソースを設定します。
 * @param device Vulkanデバイス
 */
void Gui::SetupResources(const Device &device, VkQueue queue) {
  ImGuiIO &io = ImGui::GetIO();

  unsigned char *fontData;
  int texW, texH;
  io.Fonts->GetTexDataAsRGBA32(&fontData, &texW, &texH);
  VkDeviceSize uploadSize = texW * texH * 4 * sizeof(char);

  // コピー用のターゲットイメージを生成します。
  VkImageCreateInfo imageCreateInfo = Initializer::ImageCreateInfo();
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageCreateInfo.extent = {
      static_cast<uint32_t>(texW),
      static_cast<uint32_t>(texH),
      1,
  };
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.usage =
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VK_CHECK_RESULT(
      vkCreateImage(device, &imageCreateInfo, nullptr, &font.image));
  VkMemoryRequirements memoryRequirements{};
  vkGetImageMemoryRequirements(device, font.image, &memoryRequirements);
  VkMemoryAllocateInfo memoryAllocateInfo = Initializer::MemoryAllocateInfo();
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex = device.FindMemoryType(
      memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(
      vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &font.memory));
  VK_CHECK_RESULT(vkBindImageMemory(device, font.image, font.memory, 0));

  // イメージビュー
  VkImageViewCreateInfo imageViewCreateInfo =
      Initializer::ImageViewCreateInfo();
  imageViewCreateInfo.image = font.image;
  imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageViewCreateInfo.subresourceRange.levelCount = 1;
  imageViewCreateInfo.subresourceRange.layerCount = 1;
  imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  VK_CHECK_RESULT(
      vkCreateImageView(device, &imageViewCreateInfo, nullptr, &font.view));

  // フォントデータをアップロードするためのステージングバッファです。
  Buffer stagingBuffer;
  VK_CHECK_RESULT(stagingBuffer.Create(device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       uploadSize));
  VK_CHECK_RESULT(stagingBuffer.Map(device));
  stagingBuffer.CopyTo(fontData, uploadSize);
  stagingBuffer.Unmap(device);

  // バッファデータをフォントイメージにコピーします。
  VkCommandBuffer copyCmd = device.CreateCommandBuffer();
  TransitionImageLayout(
      copyCmd, font.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT);
  VkBufferImageCopy bufferCopyRegion{};
  bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferCopyRegion.imageSubresource.layerCount = 1;
  bufferCopyRegion.imageExtent = {
      static_cast<uint32_t>(texW),
      static_cast<uint32_t>(texH),
      1,
  };
  vkCmdCopyBufferToImage(copyCmd, stagingBuffer.buffer, font.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &bufferCopyRegion);

  // シェーダー読み取りの準備
  TransitionImageLayout(copyCmd, font.image, VK_IMAGE_ASPECT_COLOR_BIT,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  device.FlushCommandBuffer(copyCmd, queue);
  stagingBuffer.Destroy(device);

  // テクスチャーサンプラー
  VkSamplerCreateInfo samplerCreateInfo = Initializer::SamplerCreateInfo();
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK_RESULT(
      vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler));

  // 記述子プール
  std::vector<VkDescriptorPoolSize> poolSizes = {
      Initializer::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      1),
  };
  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo =
      Initializer::DescriptorPoolCreateInfo(poolSizes, 2);
  VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo,
                                         nullptr, &descriptorPool));

  // 記述子セットレイアウト
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      Initializer::DescriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          VK_SHADER_STAGE_FRAGMENT_BIT, 0),
  };
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
      Initializer::DescriptorSetLayoutCreateInfo(setLayoutBindings);
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
      device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));

  // 記述子セット
  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
      Initializer::DescriptorSetAllocateInfo(descriptorPool,
                                             &descriptorSetLayout, 1);
  VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                                           &descriptorSet));
  VkDescriptorImageInfo fontDescriptor = Initializer::DescriptorImageInfo(
      sampler, font.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      Initializer::WriteDescriptorSet(descriptorSet,
                                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      0, &fontDescriptor),
  };
  vkUpdateDescriptorSets(device,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, nullptr);
}

/**
 * @brief メインとは別のUI用のパイプラインを設定します。
 * @param pipelineCache
 * @param renderPass
 */
void Gui::SetupPipeline(const Device &device, VkPipelineCache pipelineCache,
                        VkRenderPass renderPass) {
  // パイプラインレイアウトにUIレンダリングパラメータのプッシュ定数を設定します。
  VkPushConstantRange pushConstantRange = Initializer::PushConstantRange(
      VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConst), 0);
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
      Initializer::PipelineLayoutCreateInfo(&descriptorSetLayout, 1);
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
  VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                         nullptr, &pipelineLayout));

  // UIレンダリング用のグラフィックパイプラインを設定します。
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
      Initializer::PipelineInputAssemblyStateCreateInfo(
          VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

  VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo =
      Initializer::PipelineRasterizationStateCreateInfo(
          VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
          VK_FRONT_FACE_COUNTER_CLOCKWISE);

  VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
  colorBlendAttachmentState.blendEnable = VK_TRUE;
  colorBlendAttachmentState.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachmentState.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachmentState.srcAlphaBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
  VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo =
      Initializer::PipelineColorBlendStateCreateInfo(
          1, &colorBlendAttachmentState);

  VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo =
      Initializer::PipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE,
                                                       VK_COMPARE_OP_ALWAYS);

  VkPipelineViewportStateCreateInfo viewportStateCreateInfo =
      Initializer::PipelineViewportStateCreateInfo(1, 1, 0);

  VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo =
      Initializer::PipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

  std::vector<VkDynamicState> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo =
      Initializer::PipelineDynamicStateCreateInfo(dynamicStates);

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
      CreateShader(device, UI_OVERLAY_VERTEX_SHADER_PATH,
                   VK_SHADER_STAGE_VERTEX_BIT),
      CreateShader(device, UI_OVERLAY_FRAGMENT_SHADER_PATH,
                   VK_SHADER_STAGE_FRAGMENT_BIT),
  };

  std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions =
      {
          Initializer::VertexInputBindingDescription(
              0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
      };
  std::vector<VkVertexInputAttributeDescription>
      vertexInputAttributeDescriptions = {
          Initializer::VertexInputAttributeDescription(
              0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),
          Initializer::VertexInputAttributeDescription(
              0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),
          Initializer::VertexInputAttributeDescription(
              0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),
      };
  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo =
      Initializer::PipelineVertexInputStateCreateInfo(
          vertexInputBindingDescriptions, vertexInputAttributeDescriptions);

  VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
      Initializer::GraphicsPipelineCreateInfo(pipelineLayout, renderPass);
  graphicsPipelineCreateInfo.pInputAssemblyState =
      &inputAssemblyStateCreateInfo;
  graphicsPipelineCreateInfo.pRasterizationState =
      &rasterizationStateCreateInfo;
  graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
  graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
  graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
  graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
  graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
  graphicsPipelineCreateInfo.stageCount =
      static_cast<uint32_t>(shaderStages.size());
  graphicsPipelineCreateInfo.pStages = shaderStages.data();
  graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
  graphicsPipelineCreateInfo.subpass = subpass;

  VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                            &graphicsPipelineCreateInfo,
                                            nullptr, &pipeline));

  // グラフィックスパイプラインを作成した後は、シェーダーモジュールは不要になります。
  vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
  vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}
