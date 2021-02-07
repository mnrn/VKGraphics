#include "Deferred.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <array>
#include <boost/assert.hpp>
#include <vector>

#include "VK/Common.h"
#include "VK/Initializer.h"
#include "VK/Utils.h"

//*-----------------------------------------------------------------------------
// Overrides functions
//*-----------------------------------------------------------------------------

void Deferred::OnPostInit() {
  VkBase::OnPostInit();

  LoadAssets();
  PrepareOffscreenFramebuffer();
  PrepareUniformBuffers();

  SetupDescriptorSetLayout();
  SetupPipelines();
  SetupDescriptorPool();
  SetupDescriptorSet();

  // UpdateUIOverlay();
  BuildCommandBuffers();
  BuildDeferredCommandBuffer();
}

void Deferred::OnPreDestroy() {
  vkDestroySemaphore(device, offscreenSemaphore, nullptr);

  vkDestroyPipeline(device, pipelines.composition, nullptr);
  vkDestroyPipeline(device, pipelines.offscreen, nullptr);

  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  offscreenFramebuffer.Destroy(device);

  uniformBuffers.composition.Destroy(device);
  uniformBuffers.offscreen.Destroy(device);

  models.floor.Destroy(device);
  models.torus.Destroy(device);
  models.teapot.Destroy(device);
}

void Deferred::OnUpdate(float t) {
  const float deltaT = prevTime == 0.0f ? 0.0f : t - prevTime;
  prevTime = t;

  camAngle = glm::mod(
      camAngle + config["Camera"]["RotationSpeed"].get<float>() * deltaT,
      glm::two_pi<float>());
  UpdateUniformBuffers();
}

void Deferred::OnRender() {
  VkBase::PrepareFrame();

  // シーンレンダリングコマンドバッファはオフスクリーンのレンダリングが終了まで待機する必要があります
  // これを確実にするために、オフスクリーンレンダリングが終了したときに通知される専用のオフスクリーン同期セマフォを使用します。
  // これは実装が両方のコマンドバッファを同時に開始する可能性があるため必要になります。
  // コマンドバッファがアプリによって送信された順序で実行される保証はありません。

  // オフスクリーンレンダリング

  // スワップチェインが終了するまで待機します。
  submitInfo.pWaitSemaphores = &semaphores.presentComplete;
  // オフスクリーンセマフォでシグナル準備します。
  submitInfo.pSignalSemaphores = &offscreenSemaphore;

  // Submit work
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &offscreenCmdBuffer;
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  // シーンレンダリング

  // オフスクリーンセマフォを待機します。
  submitInfo.pWaitSemaphores = &offscreenSemaphore;
  // 描画完了セマフォでシグナル準備完了です。
  submitInfo.pSignalSemaphores = &semaphores.renderComplete;

  // Submit work
  submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  VkBase::SubmitFrame();
}

void Deferred::ViewChanged() { UpdateUniformBuffers(); }

//*-----------------------------------------------------------------------------
// Assets
//*-----------------------------------------------------------------------------

void Deferred::LoadAssets() {
  ModelCreateInfo modelCreateInfo{};
  // Teapot
  {
    const auto &teapot = config["Teapot"];
    modelCreateInfo.color = glm::vec3(teapot["Color"][0].get<float>(),
                                      teapot["Color"][1].get<float>(),
                                      teapot["Color"][2].get<float>());
    models.teapot.LoadFromFile(device,
                               config["Teapot"]["Model"].get<std::string>(),
                               queue, vertexLayout, modelCreateInfo);
  }
  // Torus
  {
    const auto &torus = config["Torus"];
    modelCreateInfo.color = glm::vec3(torus["Color"][0].get<float>(),
                                      torus["Color"][1].get<float>(),
                                      torus["Color"][2].get<float>());
    models.torus.LoadFromFile(device,
                              config["Torus"]["Model"].get<std::string>(),
                              queue, vertexLayout, modelCreateInfo);
  }
  // Floor
  {
    const auto &floor = config["Floor"];
    modelCreateInfo.color = glm::vec3(floor["Color"][0].get<float>(),
                                      floor["Color"][1].get<float>(),
                                      floor["Color"][2].get<float>());
    models.floor.LoadFromFile(device,
                              config["Floor"]["Model"].get<std::string>(),
                              queue, vertexLayout, modelCreateInfo);
  }
}

//*-----------------------------------------------------------------------------
// Setup
//*-----------------------------------------------------------------------------

/**
 * @brief 使用される記述子のレイアウトを設定します。<br>
 * 基本的に、様々なシェーダーステージを記述子に接続して、UniformBuffersやImageSamplerなどをバインドします。<br>
 * したがって、すべてのシェーダーバインディングは、1つの記述子セットレイアウトバインディングにマップする必要があります。
 */
void Deferred::SetupDescriptorSetLayout() {
  std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
      Initializer::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              VK_SHADER_STAGE_VERTEX_BIT, 0),
      Initializer::DescriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          VK_SHADER_STAGE_FRAGMENT_BIT, 1),
      Initializer::DescriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          VK_SHADER_STAGE_FRAGMENT_BIT, 2),
      Initializer::DescriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          VK_SHADER_STAGE_FRAGMENT_BIT, 3),
      Initializer::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              VK_SHADER_STAGE_FRAGMENT_BIT, 4),
  };

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
      Initializer::DescriptorSetLayoutCreateInfo(descriptorSetLayoutBindings);
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
      device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));

  // すべてのパイプラインで使用されるようにパイプラインレイアウトを共有します。
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
      Initializer::PipelineLayoutCreateInfo(&descriptorSetLayout);
  std::vector<VkPushConstantRange> pushConstantRanges = {
      Initializer::PushConstantRange(VK_SHADER_STAGE_VERTEX_BIT,
                                     sizeof(glm::mat4), 0),
  };
  pipelineLayoutCreateInfo.pushConstantRangeCount =
      static_cast<uint32_t>(pushConstantRanges.size());
  pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
  VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                         nullptr, &pipelineLayout));
}

void Deferred::SetupDescriptorPool() {
  // APIに記述子の最大数を通知する必要があります。
  std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {
      Initializer::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8),
      Initializer::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      9),
  };

  // グローバル記述子プールを生成します。
  VkDescriptorPoolCreateInfo descriptorPoolInfo =
      Initializer::DescriptorPoolCreateInfo(descriptorPoolSizes, 2);

  VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr,
                                         &descriptorPool));
}

void Deferred::SetupDescriptorSet() {
  // グローバル記述子プールから新しい記述子セットを割り当てます。
  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
      Initializer::DescriptorSetAllocateInfo(descriptorPool,
                                             &descriptorSetLayout, 1);

  // オフスクリーンカラーアタッチメントのイメージ記述子を設定します。　
  VkDescriptorImageInfo texPosDesc = Initializer::DescriptorImageInfo(
      offscreenFramebuffer.sampler, offscreenFramebuffer.attachments[0].view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  VkDescriptorImageInfo texNormDesc = Initializer::DescriptorImageInfo(
      offscreenFramebuffer.sampler, offscreenFramebuffer.attachments[1].view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  VkDescriptorImageInfo texAlbedoDesc = Initializer::DescriptorImageInfo(
      offscreenFramebuffer.sampler, offscreenFramebuffer.attachments[2].view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // Deferred Composition
  VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                                           &descriptorSets.composition));
  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      Initializer::WriteDescriptorSet(descriptorSets.composition,
                                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      1, &texPosDesc),
      Initializer::WriteDescriptorSet(descriptorSets.composition,
                                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      2, &texNormDesc),
      Initializer::WriteDescriptorSet(descriptorSets.composition,
                                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      3, &texAlbedoDesc),
      Initializer::WriteDescriptorSet(descriptorSets.composition,
                                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4,
                                      &uniformBuffers.composition.descriptor),
  };
  vkUpdateDescriptorSets(device,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, nullptr);

  // Offscreen Rendering
  VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                                           &descriptorSets.offscreen));
  writeDescriptorSets = {
      Initializer::WriteDescriptorSet(descriptorSets.offscreen,
                                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                      &uniformBuffers.offscreen.descriptor),
  };
  vkUpdateDescriptorSets(device,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, nullptr);
}

/**
 * @note
 * Vulkanは、レンダリングパイプラインの概念を用いてFixedStatusをカプセル化し、OpenGLの複雑なステートマシンを置き換えます。<br>
 * パイプラインはGPUに保存およびハッシュされ、パイプラインの変更が非常に高速になります。
 */
void Deferred::SetupPipelines() {
  // 入力アセンブリステートはプリミティブがどのようにアセンブルされるかを記述します。
  // このパイプラインでは、頂点データを三角形リストとしてアセンブルします。
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
      Initializer::PipelineInputAssemblyStateCreateInfo(
          VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

  // ラスタライズステート
  VkPipelineRasterizationStateCreateInfo rasterizationState =
      Initializer::PipelineRasterizationStateCreateInfo(
          VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
          VK_FRONT_FACE_COUNTER_CLOCKWISE);

  // カラーブレンドステートは、ブレンド係数の計算方法を示します。
  // カラーアタッチメントごとに１つのブレンドアタッチメント状態が必要です。
  VkPipelineColorBlendAttachmentState blendAttachmentState =
      Initializer::PipelineColorBlendAttachmentState(0xf, VK_FALSE);
  VkPipelineColorBlendStateCreateInfo colorBlendState =
      Initializer::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

  // ビューポートステートは、このパイプラインで使用されるビューポートとシザーの数を設定します。
  // NOTE: これは実際には動的にオーバーライドされます。
  VkPipelineViewportStateCreateInfo viewportState =
      Initializer::PipelineViewportStateCreateInfo(1, 1);

  // ダイナミックステートを有効にします。
  // ほとんどのステートはパイプラインに組み込まれていますが、コマンドバッファ内で変更できる動的なステートがまだいくつかあります。
  // これらを変更できるようにするには、ダイナミックステートを指定する必要があります。それらの実際の状態は、後でコマンドバッファに設定されます。
  std::vector<VkDynamicState> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT,
                                            VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState =
      Initializer::PipelineDynamicStateCreateInfo(dynamicStates);

  // 深度とステンシルの比較およびテスト操作を含むデプスステンシルステート
  // 深度テストのみを使用し、深度テストと書き込みを有効にします。
  VkPipelineDepthStencilStateCreateInfo depthStencilState =
      Initializer::PipelineDepthStencilStateCreateInfo(
          VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

  // マルチサンプリングステート
  // この例では、マルチサンプリングを使用していません。
  VkPipelineMultisampleStateCreateInfo multisampleState =
      Initializer::PipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

  // パイプラインに使用されるレイアウトとレンダーパスを指定します。
  VkGraphicsPipelineCreateInfo pipelineCreateInfo =
      Initializer::GraphicsPipelineCreateInfo(pipelineLayout, renderPass);
  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pDynamicState = &dynamicState;

  // パイプラインシェーダーステージ情報を設定します。
  const auto &pipelinesConfig = config["Pipelines"];
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{
      CreateShader(
          device,
          pipelinesConfig["Composition"]["VertexShader"].get<std::string>(),
          VK_SHADER_STAGE_VERTEX_BIT),
      CreateShader(
          device,
          pipelinesConfig["Composition"]["FragmentShader"].get<std::string>(),
          VK_SHADER_STAGE_FRAGMENT_BIT),
  };
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();

  // 空の頂点入力ステートです。頂点は頂点シェーダーによって生成されます。
  VkPipelineVertexInputStateCreateInfo emptyVertexInputState =
      Initializer::PipelineVertexInputStateCreateInfo();
  pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;
  VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                            &pipelineCreateInfo, nullptr,
                                            &pipelines.composition));
  vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
  vkDestroyShaderModule(device, shaderStages[1].module, nullptr);

  // オフスクリーン用のパイプラインを生成します。

  std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
      Initializer::VertexInputBindingDescription(0, vertexLayout.Stride(),
                                                 VK_VERTEX_INPUT_RATE_VERTEX),
  };
  std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
      // location = 0 : position
      Initializer::VertexInputAttributeDescription(
          0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
      // location = 1 : color
      Initializer::VertexInputAttributeDescription(
          0, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)),
      // location = 2 : normal
      Initializer::VertexInputAttributeDescription(
          0, 2, VK_FORMAT_R32G32B32_SFLOAT, 6 * sizeof(float)),
  };
  VkPipelineVertexInputStateCreateInfo vertexInputState =
      Initializer::PipelineVertexInputStateCreateInfo(vertexInputBindings,
                                                      vertexInputAttributes);
  pipelineCreateInfo.pVertexInputState = &vertexInputState;
  shaderStages[0] = CreateShader(
      device, pipelinesConfig["Offscreen"]["VertexShader"].get<std::string>(),
      VK_SHADER_STAGE_VERTEX_BIT);
  shaderStages[1] = CreateShader(
      device, pipelinesConfig["Offscreen"]["FragmentShader"].get<std::string>(),
      VK_SHADER_STAGE_FRAGMENT_BIT);

  // レンダーパスは別にします。
  pipelineCreateInfo.renderPass = offscreenFramebuffer.renderPass;

  // カラーアタッチメントに何も描画しないようにします。
  std::array<VkPipelineColorBlendAttachmentState, 3>
      colorBlendAttachmentStates = {
          Initializer::PipelineColorBlendAttachmentState(0xf, VK_FALSE),
          Initializer::PipelineColorBlendAttachmentState(0xf, VK_FALSE),
          Initializer::PipelineColorBlendAttachmentState(0xf, VK_FALSE),
      };
  colorBlendState.attachmentCount =
      static_cast<uint32_t>(colorBlendAttachmentStates.size());
  colorBlendState.pAttachments = colorBlendAttachmentStates.data();

  VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                            &pipelineCreateInfo, nullptr,
                                            &pipelines.offscreen));
  vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
  vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

//*-----------------------------------------------------------------------------
// Prepare
//*-----------------------------------------------------------------------------

/**
 * @brief オフスクリーンレンダリング用に新しいフレームバッファを用意します。
 */
void Deferred::PrepareOffscreenFramebuffer() {
  offscreenFramebuffer.width = swapchain.extent.width;
  offscreenFramebuffer.height = swapchain.extent.height;

  AttachmentCreateInfo attachmentCreateInfo{};
  attachmentCreateInfo.width = offscreenFramebuffer.width;
  attachmentCreateInfo.height = offscreenFramebuffer.height;
  attachmentCreateInfo.layerCount = 1;
  attachmentCreateInfo.usage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  attachmentCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;

  // POSITION (World Space)
  offscreenFramebuffer.AddAttachment(device, attachmentCreateInfo);

  // NORMAL (World Space)
  offscreenFramebuffer.AddAttachment(device, attachmentCreateInfo);

  // ALBEDO (Color)
  attachmentCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  offscreenFramebuffer.AddAttachment(device, attachmentCreateInfo);

  // Depth attachment
  attachmentCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  attachmentCreateInfo.format = device.FindSupportedDepthFormat();
  offscreenFramebuffer.AddAttachment(device, attachmentCreateInfo);

  // カラーアタッチメントからサンプラーを生成します。
  VK_CHECK_RESULT(offscreenFramebuffer.CreateSampler(
      device, VK_FILTER_NEAREST, VK_FILTER_NEAREST,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));

  // フレームバッファ用のデフォルトのレンダーパスを生成します。
  VK_CHECK_RESULT(offscreenFramebuffer.CreateRenderPass(device));
}

/**
 * @brief
 * シェーダーユニフォームを含むユニフォームバッファブロックを準備して初期化します。
 * @note
 * OpenGLのような単一のユニフォームはVulkanに存在しなくなりました。すべてのシェーダーユニフォームはユニフォームバッファブロックを介して渡されます。
 */
void Deferred::PrepareUniformBuffers() {
  VK_CHECK_RESULT(uniformBuffers.offscreen.Create(
      device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &uboOffscreenVS, sizeof(uboOffscreenVS)));
  VK_CHECK_RESULT(uniformBuffers.offscreen.Map(device));

  VK_CHECK_RESULT(uniformBuffers.composition.Create(
      device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &uboComposition, sizeof(uboComposition)));
  VK_CHECK_RESULT(uniformBuffers.composition.Map(device));

  UpdateUniformBuffers();
}

//*-----------------------------------------------------------------------------
// Render
//*-----------------------------------------------------------------------------

/**
 * @brief フレームバッファイメージごとに個別のコマンドバッファを構築します。
 * @note
 * OpenGLとは異なり、すべてのレンダリングコマンドはコマンドバッファに一度記録され、その後キューに再送信されます。<br>
 * これにより、Vulkanの最大の利点の１つである、複数のスレッドから事前に作業を生成できます。
 */
void Deferred::BuildCommandBuffers() {
  VkCommandBufferBeginInfo commandBufferBeginInfo =
      Initializer::CommandBufferBeginInfo();

  // LoadOpをclearに設定して　すべてのフレームバッファにclear値を設定します。
  // サブパスの開始時にクリアされる2つのアタッチメント(カラーとデプス)を使用するため、両方にクリア値を設定する必要があります。
  std::array<VkClearValue, 2> clear{};
  clear[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
  clear[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo =
      Initializer::RenderPassBeginInfo();
  renderPassBeginInfo.renderPass = renderPass;
  renderPassBeginInfo.renderArea.offset.x = 0;
  renderPassBeginInfo.renderArea.offset.y = 0;
  renderPassBeginInfo.renderArea.extent.width = swapchain.extent.width;
  renderPassBeginInfo.renderArea.extent.height = swapchain.extent.height;
  renderPassBeginInfo.clearValueCount = 2;
  renderPassBeginInfo.pClearValues = clear.data();

  for (size_t i = 0; i < drawCmdBuffers.size(); i++) {
    // ターゲットフレームバッファを設定します。
    renderPassBeginInfo.framebuffer = framebuffers[i];
    VK_CHECK_RESULT(
        vkBeginCommandBuffer(drawCmdBuffers[i], &commandBufferBeginInfo));

    // デフォルトのレンダーパス設定で指定された最初のサブパスを開始します。
    // これにより、色と奥行きのアタッチメントがクリアされます。
    vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    // ビューポートとシザーの更新
    VkViewport viewport = Initializer::Viewport(
        static_cast<float>(swapchain.extent.width),
        static_cast<float>(swapchain.extent.height), 0.0f, 1.0f);
    vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
    VkRect2D scissor = Initializer::Rect2D(swapchain.extent.width,
                                           swapchain.extent.height, 0, 0);
    vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

    // 記述子セットとパイプラインのバインド
    vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSets.composition,
                            0, nullptr);
    vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipelines.composition);

    vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

    DrawUI(drawCmdBuffers[i]);

    vkCmdEndRenderPass(drawCmdBuffers[i]);

    // レンダーパスを終了すると、フレームバッファのカラーアタッチメントに移行する暗黙のバリアが追加されます。
    VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
  }
}

void Deferred::BuildDeferredCommandBuffer() {
  if (offscreenCmdBuffer == VK_NULL_HANDLE) {
    offscreenCmdBuffer =
        device.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
  }
  // オフスクリーンレンダリングと同期を行うために使用するセマフォを生成します。
  VkSemaphoreCreateInfo semaphoreCreateInfo =
      Initializer::SemaphoreCreateInfo();
  VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
                                    &offscreenSemaphore));

  VkCommandBufferBeginInfo commandBufferBeginInfo =
      Initializer::CommandBufferBeginInfo();

  // フラグメントシェーダーで使用するすべてのアタッチメントをこの値でクリアします。
  std::array<VkClearValue, 4> clearValues{};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[2].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[3].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo =
      Initializer::RenderPassBeginInfo();
  renderPassBeginInfo.renderPass = offscreenFramebuffer.renderPass;
  renderPassBeginInfo.framebuffer = offscreenFramebuffer.framebuffer;
  renderPassBeginInfo.renderArea.extent.width = offscreenFramebuffer.width;
  renderPassBeginInfo.renderArea.extent.height = offscreenFramebuffer.height;
  renderPassBeginInfo.clearValueCount =
      static_cast<uint32_t>(clearValues.size());
  renderPassBeginInfo.pClearValues = clearValues.data();

  VK_CHECK_RESULT(
      vkBeginCommandBuffer(offscreenCmdBuffer, &commandBufferBeginInfo));
  vkCmdBeginRenderPass(offscreenCmdBuffer, &renderPassBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = Initializer::Viewport(
      static_cast<float>(offscreenFramebuffer.width),
      static_cast<float>(offscreenFramebuffer.height), 0.0f, 1.0f);
  vkCmdSetViewport(offscreenCmdBuffer, 0, 1, &viewport);
  VkRect2D scissor = Initializer::Rect2D(offscreenFramebuffer.width,
                                         offscreenFramebuffer.height, 0, 0);
  vkCmdSetScissor(offscreenCmdBuffer, 0, 1, &scissor);

  vkCmdBindPipeline(offscreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.offscreen);
  VkDeviceSize offsets[] = {0};

  // Teapot
  {
    vkCmdBindDescriptorSets(offscreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSets.offscreen, 0,
                            nullptr);
    vkCmdBindVertexBuffers(offscreenCmdBuffer, 0, 1,
                           &models.teapot.vertices.buffer, offsets);
    vkCmdBindIndexBuffer(offscreenCmdBuffer, models.teapot.indices.buffer, 0,
                         VK_INDEX_TYPE_UINT32);
    const auto &teapot = config["Teapot"];
    const auto scale = glm::vec3(teapot["Scale"].get<float>());
    const auto model = glm::scale(glm::mat4(1.0f), scale);
    vkCmdPushConstants(offscreenCmdBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(model), &model);
    vkCmdDrawIndexed(offscreenCmdBuffer, models.teapot.indexCount, 1, 0, 0, 0);
  }
  // Torus
  {
    vkCmdBindDescriptorSets(offscreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSets.offscreen, 0,
                            nullptr);
    vkCmdBindVertexBuffers(offscreenCmdBuffer, 0, 1,
                           &models.torus.vertices.buffer, offsets);
    vkCmdBindIndexBuffer(offscreenCmdBuffer, models.torus.indices.buffer, 0,
                         VK_INDEX_TYPE_UINT32);
    const auto &torus = config["Torus"];
    const auto scale = glm::vec3(torus["Scale"].get<float>());
    const auto rotAxis = glm::vec3(torus["Rotate"]["Axis"][0].get<float>(),
                                   torus["Rotate"]["Axis"][1].get<float>(),
                                   torus["Rotate"]["Axis"][2].get<float>());
    const auto angle = glm::radians(torus["Rotate"]["Degrees"].get<float>());
    const auto trans = glm::vec3(torus["Position"][0].get<float>(),
                                 torus["Position"][1].get<float>(),
                                 torus["Position"][2].get<float>());
    auto model = glm::translate(glm::mat4(1.0f), trans);
    model = glm::rotate(model, angle, rotAxis);
    model = glm::scale(model, scale);
    vkCmdPushConstants(offscreenCmdBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(model), &model);
    vkCmdDrawIndexed(offscreenCmdBuffer, models.torus.indexCount, 1, 0, 0, 0);
  }

  // Floor
  {
    vkCmdBindDescriptorSets(offscreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSets.offscreen, 0,
                            nullptr);
    vkCmdBindVertexBuffers(offscreenCmdBuffer, 0, 1,
                           &models.floor.vertices.buffer, offsets);
    vkCmdBindIndexBuffer(offscreenCmdBuffer, models.floor.indices.buffer, 0,
                         VK_INDEX_TYPE_UINT32);
    const auto &floor = config["Floor"];
    const auto scale = glm::vec3(floor["Scale"].get<float>());
    const auto trans = glm::vec3(floor["Position"][0].get<float>(),
                                 floor["Position"][1].get<float>(),
                                 floor["Position"][2].get<float>());
    auto model = glm::translate(glm::mat4(1.0f), trans);
    model = glm::scale(model, scale);
    vkCmdPushConstants(offscreenCmdBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(model), &model);
    vkCmdDrawIndexed(offscreenCmdBuffer, models.floor.indexCount, 1, 0, 0, 0);
  }
  vkCmdEndRenderPass(offscreenCmdBuffer);
  VK_CHECK_RESULT(vkEndCommandBuffer(offscreenCmdBuffer));
}

//*-----------------------------------------------------------------------------
// Update
//*-----------------------------------------------------------------------------

void Deferred::UpdateUniformBuffers() {
  const auto CAMERA_RADIUS = config["Camera"]["Radius"].get<float>();
  camera.SetupOrient(glm::vec3(CAMERA_RADIUS * std::sin(camAngle), 1.0f,
                               CAMERA_RADIUS * std::cos(camAngle)),
                     glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  camera.SetupPerspective(glm::radians(60.0f),
                          static_cast<float>(swapchain.extent.width) /
                              static_cast<float>(swapchain.extent.height),
                          0.3f, 100.0f);
  UpdateOffscreenUniformBuffers();
  UpdateCompositionUniformBuffers();
}

void Deferred::UpdateOffscreenUniformBuffers() {
  // 行列をシェーダーに渡します。
  const auto view = camera.GetViewMatrix();
  const auto proj = camera.GetProjectionMatrix();
  uboOffscreenVS.viewProj = proj * view;

  // ユニフォームバッファへコピーします。
  uniformBuffers.offscreen.Copy(&uboOffscreenVS, sizeof(uboOffscreenVS));
}

void Deferred::UpdateCompositionUniformBuffers() {
  uboComposition.viewPos = glm::vec4(camera.GetPosition(), 0.0f);

  int i = 0;
  for (const auto &light : config["Lights"]) {
    for (int j = 0; j < 4; j++) {
      uboComposition.lights[i].pos[j] = light["Position"][j].get<float>();
    }
    for (int j = 0; j < 3; j++) {
      uboComposition.lights[i].color[j] = light["Color"][j].get<float>();
    }
    uboComposition.lights[i].radius = light["Radius"].get<float>();
    i++;
  }

  uboComposition.lightsNum = static_cast<int>(config["Lights"].size());
  uboComposition.dispTarget = settings.dispRenderTarget;

  uniformBuffers.composition.Copy(&uboComposition, sizeof(uboComposition));
}

void Deferred::OnUpdateUIOverlay() {
  if (uiOverlay.Combo("Display Render Target", &settings.dispRenderTarget,
                      {"Final Result", "Position", "Normal", "Albedo"})) {
    UpdateCompositionUniformBuffers();
  }
}
