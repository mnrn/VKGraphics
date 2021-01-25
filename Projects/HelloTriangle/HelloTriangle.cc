#include "HelloTriangle.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <boost/assert.hpp>
#include <vector>

#include "VK/Common.h"
#include "VK/Initializer.h"
#include "VK/Shader.h"

//*-----------------------------------------------------------------------------
// Constant expressions
//*-----------------------------------------------------------------------------

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

//*-----------------------------------------------------------------------------
// Overrides functions
//*-----------------------------------------------------------------------------

void HelloTriangle::OnPostInit() {
  VkBase::OnPostInit();

  PrepareVertices();
  PrepareUniformBuffers();

  SetupDescriptorSetLayout();
  SetupPipelines();
  SetupDescriptorPool();
  SetupDescriptorSet();

  BuildCommandBuffers();
}

void HelloTriangle::OnPreDestroy() {
  vkDestroyBuffer(device, uniform.buffer, nullptr);
  vkFreeMemory(device, uniform.memory, nullptr);

  vkDestroyBuffer(device, indices.buffer, nullptr);
  vkFreeMemory(device, indices.memory, nullptr);

  vkDestroyBuffer(device, vertices.buffer, nullptr);
  vkFreeMemory(device, vertices.memory, nullptr);

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
void HelloTriangle::BuildCommandBuffers() {
  VkCommandBufferBeginInfo commandBufferBeginInfo =
      Initializer::CommandBufferBeginInfo();

  // LoadOpをclearに設定して　すべてのフレームバッファにclear値を設定します。
  // サブパスの開始時にクリアされる2つのアタッチメント(カラーとデプス)を使用するため、両方にクリア値を設定する必要があります。
  std::array<VkClearValue, 2> clear{};
  clear[0].color = {{0.2f, 0.2f, 0.2f, 1.0f}};
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

    // ビューポートの更新
    VkViewport viewport = Initializer::Viewport(
        static_cast<float>(swapchain.extent.width),
        static_cast<float>(swapchain.extent.height), 0.0f, 1.0f);
    vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

    // シザー(矩形)の更新
    VkRect2D scissor = Initializer::Rect2D(swapchain.extent.width,
                                           swapchain.extent.height, 0, 0);
    vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

    // シェーダーバインディングポイントを記述するvkCmdBindDescriptorSets
    vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    // レンダリングパイプラインをバインドします。
    // パイプライン(PSO)にはレンダリングパイプラインのすべての状態が含まれ、
    // バインドするとパイプラインの作成時に指定されたすべての状態が設定されます。
    vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline);

    // 三角形の頂点バッファをバインドします。
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &vertices.buffer, offsets);

    // 三角形のインデックスバッファをバインドします。
    vkCmdBindIndexBuffer(drawCmdBuffers[i], indices.buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    // インデックス付きの三角形を描画します。
    vkCmdDrawIndexed(drawCmdBuffers[i], indices.count, 1, 0, 0, 1);

    vkCmdEndRenderPass(drawCmdBuffers[i]);

    // レンダーパスを終了すると、フレームバッファのカラーアタッチメントに移行する暗黙のバリアが追加されます。
    VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
  }
}

void HelloTriangle::ViewChanged() { UpdateUniformBuffers(); }

//*-----------------------------------------------------------------------------
// Setup
//*-----------------------------------------------------------------------------

/**
 * @brief この例で使用される記述子のレイアウトを設定します。<br>
 * 基本的に、様々なシェーダーステージを記述子に接続して、UniformBuffersやImageSamplerなどをバインドします。<br>
 * したがって、すべてのシェーダーバインディングは、1つの記述子セットレイアウトバインディングにマップする必要があります。
 */
void HelloTriangle::SetupDescriptorSetLayout() {
  // Binding 0: Uniform buffer (Vertex shader)
  VkDescriptorSetLayoutBinding layoutBinding =
      Initializer::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              VK_SHADER_STAGE_VERTEX_BIT, 0);

  VkDescriptorSetLayoutCreateInfo descriptorLayout =
      Initializer::DescriptorSetLayoutCreateInfo(&layoutBinding, 1);
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout,
                                              nullptr, &descriptorSetLayout));

  // この記述子セットレイアウトに基づくレンダリングパイプラインを生成するために使用されるパイプラインレイアウトを作成します。
  // より複雑なシナリオでは、再利用できる記述子セットのレイアウトごとに異なるパイプラインレイアウトがあります。
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
      Initializer::PipelineLayoutCreateInfo(&descriptorSetLayout);

  VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                         nullptr, &pipelineLayout));
}

/**
 * @note
 * Vulkanは、レンダリングパイプラインの概念を用いてFixedStatusをカプセル化し、OpenGLの複雑なステートマシンを置き換えます。<br>
 * パイプラインはGPUに保存およびハッシュされ、パイプラインの変更が非常に高速になります。
 */
void HelloTriangle::SetupPipelines() {
  // パイプラインに使用されるレイアウトとレンダーパスを指定します。
  VkGraphicsPipelineCreateInfo pipelineCreateInfo =
      Initializer::GraphicsPipelineCreateInfo(pipelineLayout, renderPass);

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
  VkVertexInputBindingDescription vertexInputBindingDescription =
      Initializer::VertexInputBindingDescription(0, sizeof(Vertex),
                                                 VK_VERTEX_INPUT_RATE_VERTEX);

  // 入力属性バインディングはシェーダー属性の場所とメモリレイアウトを記述します。
  // これらはシェーダーレイアウトに一致します。
  std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributes{};
  vertexInputAttributes[0] = Initializer::VertexInputAttributeDescription(
      0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos));
  vertexInputAttributes[1] = Initializer::VertexInputAttributeDescription(
      0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));

  // パイプラインの作成に使用される頂点入力ステート
  VkPipelineVertexInputStateCreateInfo vertexInputState =
      Initializer::PipelineVertexInputStateCreateInfo();
  vertexInputState.vertexBindingDescriptionCount = 1;
  vertexInputState.pVertexBindingDescriptions = &vertexInputBindingDescription;
  vertexInputState.vertexAttributeDescriptionCount = 2;
  vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
  shaderStages[0] =
      Shader::Create(device, config["VertexShader"].get<std::string>(),
                     VK_SHADER_STAGE_VERTEX_BIT);
  shaderStages[1] =
      Shader::Create(device, config["FragmentShader"].get<std::string>(),
                     VK_SHADER_STAGE_FRAGMENT_BIT);

  // パイプラインシェーダーステージ情報を設定します。
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

void HelloTriangle::SetupDescriptorPool() {
  // APIに記述子の最大数を通知する必要があります。
  VkDescriptorPoolSize typeCount =
      Initializer::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);

  // グローバル記述子プールを生成します。
  VkDescriptorPoolCreateInfo descriptorPoolInfo =
      Initializer::DescriptorPoolCreateInfo(1, &typeCount, 1);

  VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr,
                                         &descriptorPool));
}

void HelloTriangle::SetupDescriptorSet() {
  // グローバル記述子プールから新しい記述子セットを割り当てます。
  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
      Initializer::DescriptorSetAllocateInfo(descriptorPool,
                                             &descriptorSetLayout, 1);

  VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                                           &descriptorSet));

  // シェーダーバインディングポイントを決定する記述子セットを更新します。
  // シェーダーで使用されるすべてのバインディングポイントにはそのバインディングポイントに一致する記述子セットが必要です。
  VkWriteDescriptorSet writeDescriptorSet = Initializer::WriteDescriptorSet(
      descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniform.descriptor);

  vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}

//*-----------------------------------------------------------------------------
// Prepare
//*-----------------------------------------------------------------------------

/**
 * @brief
 * インデックス付き三角形の頂点バッファとインデックスバッファを準備します。
 * @note
 * ステージングを使用して、それらをデバイスのローカルメモリにアップロードし、頂点シェーダーに一致するように頂点入力と属性バインディングを初期化します。
 */
void HelloTriangle::PrepareVertices() {
  // 頂点を設定します。
  std::vector<Vertex> vertexBuffer = {
      {{-1.0f, -0.866f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      {{1.0f, -0.866f, 0.0f}, {0.0f, 1.0f, 0.0f}},
      {{0.0f, 0.866f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

  uint32_t vertexBufferSize =
      static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

  // インデックスを設定します。
  std::vector<uint32_t> indexBuffer = {0, 1, 2};
  indices.count = static_cast<uint32_t>(indexBuffer.size());
  uint32_t indexBufferSize = indices.count * sizeof(uint32_t);

  // 頂点やインデックスバッファなどの静的データはGPUによる最適な(そして最速の)アクセスのためにデバイスメモリに保存する必要があります。
  // これを実現するために、いわゆる「ステージングバッファ」を使用します。
  // - ホストに表示される(マップ可能な)バッファを生成します。
  // - データをこのバッファにコピーします。
  // - 同じサイズのデバイス(VRAM)でローカルな別のバッファを生成します。
  // - コマンドバッファを使用して、ホストからデバイスにデータをコピーします。
  // - ホストの可視(ステージング)バッファを削除します。
  // - レンダリングにデバイスのローカルバッファを使用します。
  struct StagingBuffer {
    VkDeviceMemory memory;
    VkBuffer buffer;
  };
  struct {
    StagingBuffer vertices;
    StagingBuffer indices;
  } stagingBuffers{};

  // 頂点バッファ
  VkBufferCreateInfo vertexBufferCreateInfo = Initializer::BufferCreateInfo(
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vertexBufferSize);
  //頂点データをコピーするホストに表示されるバッファ(ステージングバッファ)を生成します。
  VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferCreateInfo, nullptr,
                                 &(stagingBuffers.vertices.buffer)));

  // データのコピーに使用できるホストの可視メモリタイプを要求します。
  // また、バッファのマッピングを解除した直後に書き込みがGPUに表示されるようにコヒーレント(干渉可能)であることを要求します。
  VkMemoryRequirements memoryRequirements{};
  vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer,
                                &memoryRequirements);
  VkMemoryAllocateInfo memoryAllocateInfo = Initializer::MemoryAllocateInfo();
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex =
      device.FindMemoryType(memoryRequirements.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocateInfo, nullptr,
                                   &stagingBuffers.vertices.memory));

  // マップしてコピーします。
  void *data;
  VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.vertices.memory, 0,
                              memoryAllocateInfo.allocationSize, 0, &data));
  std::memcpy(data, vertexBuffer.data(), vertexBufferSize);
  vkUnmapMemory(device, stagingBuffers.vertices.memory);
  VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.vertices.buffer,
                                     stagingBuffers.vertices.memory, 0));

  // (ホストローカル)頂点データがコピーされ、レンダリングに使用されるデバイスローカルバッファを生成します。
  vertexBufferCreateInfo.usage =
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferCreateInfo, nullptr,
                                 &vertices.buffer));
  vkGetBufferMemoryRequirements(device, vertices.buffer, &memoryRequirements);
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex = device.FindMemoryType(
      memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(
      vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &vertices.memory));
  VK_CHECK_RESULT(
      vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

  // インデックスバッファ
  VkBufferCreateInfo indexBufferCreateInfo = Initializer::BufferCreateInfo(
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT, indexBufferSize);

  // インデックスデータをホストに表示されるバッファ(ステージングバッファ)にコピーします。
  VK_CHECK_RESULT(vkCreateBuffer(device, &indexBufferCreateInfo, nullptr,
                                 &stagingBuffers.indices.buffer));
  vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer,
                                &memoryRequirements);
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex =
      device.FindMemoryType(memoryRequirements.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocateInfo, nullptr,
                                   &stagingBuffers.indices.memory));
  VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.indices.memory, 0,
                              indexBufferSize, 0, &data));
  std::memcpy(data, indexBuffer.data(), indexBufferSize);
  vkUnmapMemory(device, stagingBuffers.indices.memory);
  VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.indices.buffer,
                                     stagingBuffers.indices.memory, 0));

  // デバイス側のみ可視としバッファを生成します。
  indexBufferCreateInfo.usage =
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  VK_CHECK_RESULT(
      vkCreateBuffer(device, &indexBufferCreateInfo, nullptr, &indices.buffer));
  vkGetBufferMemoryRequirements(device, indices.buffer, &memoryRequirements);
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex = device.FindMemoryType(
      memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(
      vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &indices.memory));
  VK_CHECK_RESULT(
      vkBindBufferMemory(device, indices.buffer, indices.memory, 0));

  // バッファコピーはキューに送信する必要があるため、それらのコマンドバッファが必要です。
  // NOTE:一部のデバイスは、大量のコピーを実行する場合に高速になる可能性がある専用の転送キュー(転送ビットのみが設定されている)を提供します。
  VkCommandBuffer copyCmd =
      device.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  // バッファ領域のコピーをコマンドバッファに代入します。
  VkBufferCopy copyRegion{};

  // 頂点バッファ
  copyRegion.size = vertexBufferSize;
  vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vertices.buffer, 1,
                  &copyRegion);

  // インデックスバッファ
  copyRegion.size = indexBufferSize;
  vkCmdCopyBuffer(copyCmd, stagingBuffers.indices.buffer, indices.buffer, 1,
                  &copyRegion);

  // コマンドバッファをフラッシュすると、それもキューに送信され、フェンスを使用して、戻る前にすべてのコマンドが実行されたことを確認します。
  device.FlushCommandBuffer(copyCmd, queue);

  // ステージングバッファを破棄します。
  // NOTE:コピーを送信して実行する前に、ステージングバッファを削除しないでください。
  vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
  vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
  vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
  vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);
}

/**
 * @brief
 * シェーダーユニフォームを含むユニフォームバッファブロックを準備して初期化します。
 * @note
 * OpenGLのような単一のユニフォームはVulkanに存在しなくなりました。すべてのシェーダーユニフォームはユニフォームバッファブロックを介して渡されます。
 */
void HelloTriangle::PrepareUniformBuffers() {
  // このバッファをユニフォームバッファとして使用されます。
  VkBufferCreateInfo bufferCreateInfo = Initializer::BufferCreateInfo(
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(UniformBufferObject));
  // 新しいバッファを作成します。
  VK_CHECK_RESULT(
      vkCreateBuffer(device, &bufferCreateInfo, nullptr, &uniform.buffer));

  // サイズ、配置、メモリタイプなどのメモリ要件を取得します。
  VkMemoryRequirements memReqs;
  vkGetBufferMemoryRequirements(device, uniform.buffer, &memReqs);

  VkMemoryAllocateInfo memoryAllocateInfo = Initializer::MemoryAllocateInfo();
  memoryAllocateInfo.allocationSize = memReqs.size;
  memoryAllocateInfo.memoryTypeIndex = device.FindMemoryType(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // ユニフォームバッファをメモリに割り当てます。
  VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocateInfo, nullptr,
                                   &(uniform.memory)));
  // メモリをバッファにバインドします。
  VK_CHECK_RESULT(
      vkBindBufferMemory(device, uniform.buffer, uniform.memory, 0));

  // 記述子セットで使用されるユニフォームの記述子に情報を格納します。
  uniform.descriptor.buffer = uniform.buffer;
  uniform.descriptor.offset = 0;
  uniform.descriptor.range = sizeof(UniformBufferObject);

  UpdateUniformBuffers();
}

//*-----------------------------------------------------------------------------
// Update
//*-----------------------------------------------------------------------------

void HelloTriangle::UpdateUniformBuffers() {
  // 行列をシェーダーに渡します。
  UniformBufferObject ubo{};
  ubo.model = glm::mat4(1.0f);
  ubo.view =
      glm::lookAt(glm::vec3(0.0f, 0.0f, -2.5f), glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 1.0f, 0.0f));
  ubo.proj = glm::perspective(glm::radians(60.0f),
                              static_cast<float>(swapchain.extent.width) /
                                  static_cast<float>(swapchain.extent.height),
                              1.0f, 100.0f);
  ubo.proj[1][1] *= -1.0f;

  // ユニフォームバッファをマップして更新します。
  std::byte *pData;
  VK_CHECK_RESULT(vkMapMemory(device, uniform.memory, 0, sizeof(ubo), 0,
                              reinterpret_cast<void **>(&pData)));
  std::memcpy(pData, &ubo, sizeof(ubo));
  // データがコピーされたあとにマップを解除します。
  // NOTE:ユニフォームバッファ用にホストコヒーレントメモリタイプをリクエストしたため、書き込みはGPUに即座に表示されます。
  vkUnmapMemory(device, uniform.memory);
}
