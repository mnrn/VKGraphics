#include "PBRBasic.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <array>
#include <boost/assert.hpp>
#include <imgui.h>
#include <vector>

#include "VK/Common.h"
#include "VK/Initializer.h"
#include "VK/Utils.h"

//*-----------------------------------------------------------------------------
// Overrides functions
//*-----------------------------------------------------------------------------

void PBRBasic::OnPostInit() {
  VkBase::OnPostInit();

  PrepareCamera();
  LoadAssets();
  PrepareUniformBuffers();

  SetupDescriptorSetLayout();
  SetupPipelines();
  SetupDescriptorPool();
  SetupDescriptorSet();

  UpdateUIOverlay();
  BuildCommandBuffers();
}

void PBRBasic::OnPreDestroy() {
  model.Destroy(device);

  uniformBuffers.params.Destroy(device);
  uniformBuffers.object.Destroy(device);

  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  vkDestroyPipeline(device, pipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

/**
 * @brief フレームバッファイメージごとに個別のコマンドバッファを構築します。
 * @note
 * OpenGLとは異なり、すべてのレンダリングコマンドはコマンドバッファに一度記録され、その後キューに再送信されます。<br>
 * これにより、Vulkanの最大の利点の１つである、複数のスレッドから事前に作業を生成できます。
 */
void PBRBasic::BuildCommandBuffers() {
  VkCommandBufferBeginInfo commandBufferBeginInfo =
      Initializer::CommandBufferBeginInfo();

  // LoadOpをclearに設定して　すべてのフレームバッファにclear値を設定します。
  // サブパスの開始時にクリアされる2つのアタッチメント(カラーとデプス)を使用するため、両方にクリア値を設定する必要があります。
  std::array<VkClearValue, 2> clear{};
  clear[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
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
                            pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline);

    // 三角形の頂点バッファとインデックスバッファをバインドします。
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &model.vertices.buffer,
                           offsets);
    vkCmdBindIndexBuffer(drawCmdBuffers[i], model.indices.buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    // Spot左側
    {
      const glm::vec3 pos = glm::vec3(-1.0f, 0.0f, 0.0);
      vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout,
                         VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec3),
                         &pos);
      Material mat{};
      mat.rough = settings.metalRough;
      mat.metal = 1.0f;
      mat.reflect = settings.dielectricReflectance;
      mat.r = settings.metalSpecular.r;
      mat.g = settings.metalSpecular.g;
      mat.b = settings.metalSpecular.b;
      vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout,
                         VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::vec3),
                         sizeof(mat), &mat);
      vkCmdDrawIndexed(drawCmdBuffers[i], model.indexCount, 1, 0, 0, 0);
    }
    // Spot右側
    {
      const glm::vec3 pos = glm::vec3(1.0f, 0.0f, 0.0);
      vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout,
                         VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec3),
                         &pos);
      Material mat{};
      mat.rough = settings.dielectricRough;
      mat.metal = 0.0f;
      mat.reflect = settings.dielectricReflectance;
      mat.r = settings.dielectricBaseColor.r;
      mat.g = settings.dielectricBaseColor.g;
      mat.b = settings.dielectricBaseColor.b;
      vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout,
                         VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::vec3),
                         sizeof(mat), &mat);
      vkCmdDrawIndexed(drawCmdBuffers[i], model.indexCount, 1, 0, 0, 0);
    }

    DrawUI(drawCmdBuffers[i]);

    vkCmdEndRenderPass(drawCmdBuffers[i]);

    // レンダーパスを終了すると、フレームバッファのカラーアタッチメントに移行する暗黙のバリアが追加されます。
    VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
  }
}

void PBRBasic::OnUpdate(float t) {
  const float deltaT = prevTime == 0.0f ? 0.0f : t - prevTime;
  prevTime = t;

  lightAngle = glm::mod(lightAngle + deltaT, glm::two_pi<float>());
  UpdateLights();
}

void PBRBasic::ViewChanged() { UpdateUniformBuffers(); }

//*-----------------------------------------------------------------------------
// Assets
//*-----------------------------------------------------------------------------

void PBRBasic::LoadAssets() {
  const auto &modelPath = config["Model"].get<std::string>();
  const auto result =
      model.LoadFromFile(device, modelPath, queue, vertexLayout);
  BOOST_ASSERT_MSG(result, "Failed to load model!");
}

//*-----------------------------------------------------------------------------
// Setup
//*-----------------------------------------------------------------------------

/**
 * @brief 使用される記述子のレイアウトを設定します。<br>
 * 基本的に、様々なシェーダーステージを記述子に接続して、UniformBuffersやImageSamplerなどをバインドします。<br>
 * したがって、すべてのシェーダーバインディングは、1つの記述子セットレイアウトバインディングにマップする必要があります。
 */
void PBRBasic::SetupDescriptorSetLayout() {
  std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
      Initializer::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              VK_SHADER_STAGE_VERTEX_BIT, 0),
      Initializer::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              VK_SHADER_STAGE_FRAGMENT_BIT, 1),
  };

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
      Initializer::DescriptorSetLayoutCreateInfo(descriptorSetLayoutBindings);
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
      device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));

  // この記述子セットレイアウトに基づくレンダリングパイプラインを生成するために使用されるパイプラインレイアウトを作成します。
  // より複雑なシナリオでは、再利用できる記述子セットのレイアウトごとに異なるパイプラインレイアウトがあります。
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
      Initializer::PipelineLayoutCreateInfo(&descriptorSetLayout);

  std::vector<VkPushConstantRange> pushConstantRanges = {
      Initializer::PushConstantRange(VK_SHADER_STAGE_VERTEX_BIT,
                                     sizeof(glm::vec3), 0),
      Initializer::PushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT,
                                     sizeof(Material), sizeof(glm::vec3)),
  };
  pipelineLayoutCreateInfo.pushConstantRangeCount =
      static_cast<uint32_t>(pushConstantRanges.size());
  pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
  VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                         nullptr, &pipelineLayout));
}

void PBRBasic::SetupDescriptorPool() {
  // APIに記述子の最大数を通知する必要があります。
  std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {
      Initializer::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16),
  };

  // グローバル記述子プールを生成します。
  VkDescriptorPoolCreateInfo descriptorPoolInfo =
      Initializer::DescriptorPoolCreateInfo(descriptorPoolSizes, 2);

  VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr,
                                         &descriptorPool));
}

void PBRBasic::SetupDescriptorSet() {
  // グローバル記述子プールから新しい記述子セットを割り当てます。
  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
      Initializer::DescriptorSetAllocateInfo(descriptorPool,
                                             &descriptorSetLayout, 1);
  VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                                           &descriptorSet));

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      Initializer::WriteDescriptorSet(descriptorSet,
                                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                      &uniformBuffers.object.descriptor),
      Initializer::WriteDescriptorSet(descriptorSet,
                                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                      &uniformBuffers.params.descriptor),
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
void PBRBasic::SetupPipelines() {
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
  std::vector<VkDynamicState> dynamicStates;
  dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
  dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);
  VkPipelineDynamicStateCreateInfo dynamicState =
      Initializer::PipelineDynamicStateCreateInfo(dynamicStates);

  // 深度とステンシルの比較およびテスト操作を含むデプスステンシルステート
  // 深度テストのみを使用し、深度テストと書き込みを有効にします。
  VkPipelineDepthStencilStateCreateInfo depthStencilState =
      Initializer::PipelineDepthStencilStateCreateInfo(
          VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
  depthStencilState.depthBoundsTestEnable = VK_FALSE;
  depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
  depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
  depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
  depthStencilState.stencilTestEnable = VK_FALSE;
  depthStencilState.front = depthStencilState.back;

  // マルチサンプリングステート
  // この例では、マルチサンプリングを使用していません。
  VkPipelineMultisampleStateCreateInfo multisampleState =
      Initializer::PipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
  multisampleState.pSampleMask = nullptr;

  // 頂点入力バインディング
  // この例では、バインディングポイント0で単一の頂点入力バインディングを使用しています。
  std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
      Initializer::VertexInputBindingDescription(0, vertexLayout.Stride(),
                                                 VK_VERTEX_INPUT_RATE_VERTEX),
  };

  // 入力属性バインディングはシェーダー属性の場所とメモリレイアウトを記述します。
  // これらはシェーダーレイアウトに一致します。
  std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
      Initializer::VertexInputAttributeDescription(
          0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
      Initializer::VertexInputAttributeDescription(
          0, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)),
  };

  // パイプラインの作成に使用される頂点入力ステート
  VkPipelineVertexInputStateCreateInfo vertexInputState =
      Initializer::PipelineVertexInputStateCreateInfo(vertexInputBindings,
                                                      vertexInputAttributes);

  // パイプラインに使用されるレイアウトとレンダーパスを指定します。
  VkGraphicsPipelineCreateInfo pipelineCreateInfo =
      Initializer::GraphicsPipelineCreateInfo(pipelineLayout, renderPass);

  // パイプラインシェーダーステージ情報を設定します。
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
  shaderStages[0] =
      CreateShader(device, config["VertexShader"].get<std::string>(),
                   VK_SHADER_STAGE_VERTEX_BIT);
  shaderStages[1] =
      CreateShader(device, config["FragmentShader"].get<std::string>(),
                   VK_SHADER_STAGE_FRAGMENT_BIT);
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();

  // パイプラインステートをpipelineCreateInfoに代入してきます。
  pipelineCreateInfo.pVertexInputState = &vertexInputState;
  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pDynamicState = &dynamicState;

  // 指定されたステートを使用してレンダリングパイプラインを作成します。
  VK_CHECK_RESULT(vkCreateGraphicsPipelines(
      device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));

  // グラフィックスパイプラインを作成した後は、シェーダーモジュールは不要になります。
  vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
  vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

//*-----------------------------------------------------------------------------
// Prepare
//*-----------------------------------------------------------------------------

void PBRBasic::PrepareCamera() {
  camera.SetupOrient(glm::vec3(0.0f, 1.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f));
  camera.SetupPerspective(glm::radians(60.0f),
                          static_cast<float>(swapchain.extent.width) /
                              static_cast<float>(swapchain.extent.height),
                          1.0f, 100.0f);
}

/**
 * @brief
 * シェーダーユニフォームを含むユニフォームバッファブロックを準備して初期化します。
 * @note
 * OpenGLのような単一のユニフォームはVulkanに存在しなくなりました。すべてのシェーダーユニフォームはユニフォームバッファブロックを介して渡されます。
 */
void PBRBasic::PrepareUniformBuffers() {
  VK_CHECK_RESULT(
      uniformBuffers.object.Create(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   &ubo, sizeof(ubo)));
  VK_CHECK_RESULT(uniformBuffers.object.Map(device));

  VK_CHECK_RESULT(
      uniformBuffers.params.Create(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   &uboParams, sizeof(uboParams)));
  VK_CHECK_RESULT(uniformBuffers.params.Map(device));

  UpdateUniformBuffers();
  UpdateLights();
}

//*-----------------------------------------------------------------------------
// Update
//*-----------------------------------------------------------------------------

void PBRBasic::UpdateUniformBuffers() {
  // 行列をシェーダーに渡します。
  ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f),
                          glm::vec3(0.0f, 1.0f, 0.0f));
  ubo.view = camera.GetViewMatrix();
  ubo.proj = camera.GetProjectionMatrix();

  // ユニフォームバッファへコピーします。
  uniformBuffers.object.Copy(&ubo, sizeof(ubo));
}

void PBRBasic::UpdateLights() {
  uboParams.eye = -1.0f * camera.GetPosition();
  uboParams.lights[0].pos = glm::vec4(3.0f * std::cos(lightAngle), 0.0f,
                                      3.0f * std::sin(lightAngle), 1.0f);
  uboParams.lights[0].intensity = 25.0f;
  uboParams.lights[1].pos = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
  uboParams.lights[1].intensity = 5.0f;
  uboParams.lights[2].pos = glm::vec4(0.0f, 0.0f, -3.0f, 1.0f);
  uboParams.lights[2].intensity = 25.0f;

  uboParams.lightsNum = 3;

  uniformBuffers.params.Copy(&uboParams, sizeof(uboParams));
}

void PBRBasic::OnUpdateUIOverlay() {
  if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
  }
}
