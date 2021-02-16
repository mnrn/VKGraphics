#include "ProtoSSAO.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <array>
#include <boost/assert.hpp>
#include <random>
#include <vector>

#include "VK/Common.h"
#include "VK/Initializer.h"
#include "VK/Utils.h"

#include "Math/UniformDistribution.h"

//*-----------------------------------------------------------------------------
// Overrides functions
//*-----------------------------------------------------------------------------

void ProtoSSAO::OnPostInit() {
  VkBase::OnPostInit();

  LoadAssets();
  PrepareOffscreenFramebuffer();
  PrepareUniformBuffers();

  SetupDescriptorPool();
  SetupDescriptorSet();
  SetupPipelines();

  BuildCommandBuffers();
}

void ProtoSSAO::OnPreDestroy() {
  vkDestroyPipeline(device, pipelines.lighting, nullptr);
  vkDestroyPipeline(device, pipelines.blur, nullptr);
  vkDestroyPipeline(device, pipelines.ssao, nullptr);
  vkDestroyPipeline(device, pipelines.gBuffer, nullptr);

  vkDestroyPipelineLayout(device, pipelineLayouts.lighting, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayouts.blur, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayouts.ssao, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayouts.gBuffer, nullptr);

  vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.lighting, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.blur, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.ssao, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.gBuffer, nullptr);

  frameBuffers.blur.Destroy(device);
  frameBuffers.ssao.Destroy(device);
  frameBuffers.gBuffer.Destroy(device);

  uniformBuffers.lighting.Destroy(device);
  uniformBuffers.ssao.Destroy(device);
  uniformBuffers.gBuffer.Destroy(device);

  textures.noise.Destroy(device);
  textures.wall.Destroy(device);
  textures.floor.Destroy(device);

  models.floor.Destroy(device);
  models.teapot.Destroy(device);
}

void ProtoSSAO::ViewChanged() { UpdateUniformBuffers(); }

//*-----------------------------------------------------------------------------
// Assets
//*-----------------------------------------------------------------------------

void ProtoSSAO::LoadAssets() {
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

  // Floor
  {
    const auto &floor = config["Floor"];
    modelCreateInfo.uvscale = glm::vec3(4.0f, 4.0f, 4.0f);
    models.floor.LoadFromFile(device,
                              config["Floor"]["Model"].get<std::string>(),
                              queue, vertexLayout, modelCreateInfo);
    textures.floor.Load(device, floor["Texture"].get<std::string>(), queue);
  }

  // Wall
  {
    const auto &wall = config["Wall"];
    modelCreateInfo.uvscale = glm::vec3(16.0f, 16.0f, 16.0f);
    textures.wall.Load(device, wall["Texture"].get<std::string>(), queue);
  }
}

//*-----------------------------------------------------------------------------
// Setup
//*-----------------------------------------------------------------------------

void ProtoSSAO::SetupDescriptorPool() {
  // APIに記述子の最大数を通知する必要があります。
  std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {
      Initializer::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16),
      Initializer::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      16),
  };

  // グローバル記述子プールを生成します。
  VkDescriptorPoolCreateInfo descriptorPoolInfo =
      Initializer::DescriptorPoolCreateInfo(descriptorPoolSizes,
                                            descriptorSets.maxSets);

  VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr,
                                         &descriptorPool));
}

/**
 * @brief 使用される記述子のレイアウトを設定します。<br>
 * 基本的に、様々なシェーダーステージを記述子に接続して、UniformBuffersやImageSamplerなどをバインドします。<br>
 * したがって、すべてのシェーダーバインディングは、1つの記述子セットレイアウトバインディングにマップする必要があります。
 */
void ProtoSSAO::SetupDescriptorSet() {
  std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings{};
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
      Initializer::PipelineLayoutCreateInfo();
  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
      Initializer::DescriptorSetAllocateInfo(descriptorPool, nullptr, 1);
  std::vector<VkWriteDescriptorSet> writeDescriptorSets{};
  std::vector<VkDescriptorImageInfo> imageDescriptors{};

  // G-Buffer creation
  {
    descriptorSetLayoutBindings = {
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 3),
    };
    descriptorSetLayoutCreateInfo =
        Initializer::DescriptorSetLayoutCreateInfo(descriptorSetLayoutBindings);
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo,
                                    nullptr, &descriptorSetLayouts.gBuffer));

    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.gBuffer;
    std::vector<VkPushConstantRange> pushConstantRanges = {
        Initializer::PushConstantRange(VK_SHADER_STAGE_VERTEX_BIT |
                                           VK_SHADER_STAGE_FRAGMENT_BIT,
                                       sizeof(pushConsts), 0),
    };
    pipelineLayoutCreateInfo.pushConstantRangeCount =
        static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                           nullptr, &pipelineLayouts.gBuffer));

    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayouts.gBuffer;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                                             &descriptorSets.gBuffer));
    writeDescriptorSets = {
        Initializer::WriteDescriptorSet(descriptorSets.gBuffer,
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                        &uniformBuffers.gBuffer.descriptor),
        Initializer::WriteDescriptorSet(
            descriptorSets.gBuffer, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1, &textures.floor.descriptor),
        Initializer::WriteDescriptorSet(
            descriptorSets.gBuffer, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            2, &textures.wall.descriptor),
    };
    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, nullptr);

    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
  }

  // SSAO
  {
    descriptorSetLayoutBindings = {
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
    };
    descriptorSetLayoutCreateInfo =
        Initializer::DescriptorSetLayoutCreateInfo(descriptorSetLayoutBindings);
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo,
                                    nullptr, &descriptorSetLayouts.ssao));

    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.ssao;
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                           nullptr, &pipelineLayouts.ssao));

    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayouts.ssao;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                                             &descriptorSets.ssao));

    imageDescriptors = {
        Initializer::DescriptorImageInfo(
            frameBuffers.gBuffer.sampler,
            frameBuffers.gBuffer.attachments[0].view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
        Initializer::DescriptorImageInfo(
            frameBuffers.gBuffer.sampler,
            frameBuffers.gBuffer.attachments[1].view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
    };
    writeDescriptorSets = {
        Initializer::WriteDescriptorSet(
            descriptorSets.ssao, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
            &imageDescriptors[0]),
        Initializer::WriteDescriptorSet(
            descriptorSets.ssao, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
            &imageDescriptors[1]),
        Initializer::WriteDescriptorSet(
            descriptorSets.ssao, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
            &textures.noise.descriptor),
        Initializer::WriteDescriptorSet(descriptorSets.ssao,
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3,
                                        &uniformBuffers.ssao.descriptor),
    };
    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, nullptr);
  }

  // Blur
  {
    descriptorSetLayoutBindings = {
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 0),
    };
    descriptorSetLayoutCreateInfo =
        Initializer::DescriptorSetLayoutCreateInfo(descriptorSetLayoutBindings);
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo,
                                    nullptr, &descriptorSetLayouts.blur));

    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.blur;
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                           nullptr, &pipelineLayouts.blur));

    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayouts.blur;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                                             &descriptorSets.blur));

    imageDescriptors = {
        Initializer::DescriptorImageInfo(
            frameBuffers.gBuffer.sampler, frameBuffers.ssao.attachments[0].view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
    };
    writeDescriptorSets = {
        Initializer::WriteDescriptorSet(
            descriptorSets.blur, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
            &imageDescriptors[0]),
    };
    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, nullptr);
  }

  // Lighting
  {
    descriptorSetLayoutBindings = {
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 3),
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 4),
        Initializer::DescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 5),
    };
    descriptorSetLayoutCreateInfo =
        Initializer::DescriptorSetLayoutCreateInfo(descriptorSetLayoutBindings);
    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo,
                                    nullptr, &descriptorSetLayouts.lighting));

    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.lighting;
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                           nullptr, &pipelineLayouts.lighting));

    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayouts.lighting;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                                             &descriptorSets.lighting));

    imageDescriptors = {
        Initializer::DescriptorImageInfo(
            frameBuffers.gBuffer.sampler,
            frameBuffers.gBuffer.attachments[0].view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
        Initializer::DescriptorImageInfo(
            frameBuffers.gBuffer.sampler,
            frameBuffers.gBuffer.attachments[1].view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
        Initializer::DescriptorImageInfo(
            frameBuffers.gBuffer.sampler,
            frameBuffers.gBuffer.attachments[2].view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
        Initializer::DescriptorImageInfo(
            frameBuffers.gBuffer.sampler, frameBuffers.ssao.attachments[0].view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
        Initializer::DescriptorImageInfo(
            frameBuffers.gBuffer.sampler, frameBuffers.blur.attachments[0].view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
    };
    writeDescriptorSets = {
        Initializer::WriteDescriptorSet(descriptorSets.lighting,
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                        &uniformBuffers.lighting.descriptor),
        Initializer::WriteDescriptorSet(
            descriptorSets.lighting, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1, &imageDescriptors[0]),
        Initializer::WriteDescriptorSet(
            descriptorSets.lighting, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            2, &imageDescriptors[1]),
        Initializer::WriteDescriptorSet(
            descriptorSets.lighting, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            3, &imageDescriptors[2]),
        Initializer::WriteDescriptorSet(
            descriptorSets.lighting, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            4, &imageDescriptors[3]),
        Initializer::WriteDescriptorSet(
            descriptorSets.lighting, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            5, &imageDescriptors[4]),
    };
    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, nullptr);
  }
}

/**
 * @note
 * Vulkanは、レンダリングパイプラインの概念を用いてFixedStatusをカプセル化し、OpenGLの複雑なステートマシンを置き換えます。<br>
 * パイプラインはGPUに保存およびハッシュされ、パイプラインの変更が非常に高速になります。
 */
void ProtoSSAO::SetupPipelines() {
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
      Initializer::GraphicsPipelineCreateInfo(pipelineLayouts.lighting,
                                              renderPass);
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
          pipelinesConfig["Lighting"]["VertexShader"].get<std::string>(),
          VK_SHADER_STAGE_VERTEX_BIT),
      CreateShader(
          device,
          pipelinesConfig["Lighting"]["FragmentShader"].get<std::string>(),
          VK_SHADER_STAGE_FRAGMENT_BIT),
  };
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();

  // 空の頂点入力ステートです。頂点は頂点シェーダーによって生成されます。
  VkPipelineVertexInputStateCreateInfo emptyVertexInputState =
      Initializer::PipelineVertexInputStateCreateInfo();
  pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;
  // Lighting pipeline
  {
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                              &pipelineCreateInfo, nullptr,
                                              &pipelines.lighting));
    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
  }

  // SSAO pipeline
  {
    pipelineCreateInfo.renderPass = frameBuffers.ssao.renderPass;
    pipelineCreateInfo.layout = pipelineLayouts.ssao;
    struct SpecializationData {
      uint32_t kernelSize = KERNEL_SIZE;
    } specializationData;
    std::vector<VkSpecializationMapEntry> specializationMapEntries{
        Initializer::SpecializationMapEntry(
            0, offsetof(SpecializationData, kernelSize),
            sizeof(SpecializationData::kernelSize)),
    };
    VkSpecializationInfo specializationInfo = Initializer::SpecializationInfo(
        specializationMapEntries, sizeof(specializationData),
        &specializationData);
    shaderStages = {
        CreateShader(device,
                     pipelinesConfig["SSAO"]["VertexShader"].get<std::string>(),
                     VK_SHADER_STAGE_VERTEX_BIT),
        CreateShader(
            device,
            pipelinesConfig["SSAO"]["FragmentShader"].get<std::string>(),
            VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo),
    };

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                              &pipelineCreateInfo, nullptr,
                                              &pipelines.ssao));
    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
  }

  // Blur pipeline
  {
    pipelineCreateInfo.renderPass = frameBuffers.blur.renderPass;
    pipelineCreateInfo.layout = pipelineLayouts.blur;
    shaderStages = {
        CreateShader(device,
                     pipelinesConfig["Blur"]["VertexShader"].get<std::string>(),
                     VK_SHADER_STAGE_VERTEX_BIT),
        CreateShader(
            device,
            pipelinesConfig["Blur"]["FragmentShader"].get<std::string>(),
            VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                              &pipelineCreateInfo, nullptr,
                                              &pipelines.blur));
    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
  }

  // G-Buffer pipeline
  {
    std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
        Initializer::VertexInputBindingDescription(0, vertexLayout.Stride(),
                                                   VK_VERTEX_INPUT_RATE_VERTEX),
    };
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
        // location = 0 : position
        Initializer::VertexInputAttributeDescription(
            0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
        // location = 1 : normal
        Initializer::VertexInputAttributeDescription(
            0, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)),
        // location = 2 : color
        Initializer::VertexInputAttributeDescription(
            0, 2, VK_FORMAT_R32G32B32_SFLOAT, 6 * sizeof(float)),
        // location = 3 : uv
        Initializer::VertexInputAttributeDescription(
            0, 3, VK_FORMAT_R32G32_SFLOAT, 9 * sizeof(float)),
    };
    VkPipelineVertexInputStateCreateInfo vertexInputState =
        Initializer::PipelineVertexInputStateCreateInfo(vertexInputBindings,
                                                        vertexInputAttributes);
    pipelineCreateInfo.pVertexInputState = &vertexInputState;
    shaderStages[0] = CreateShader(
        device, pipelinesConfig["G-Buffer"]["VertexShader"].get<std::string>(),
        VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = CreateShader(
        device,
        pipelinesConfig["G-Buffer"]["FragmentShader"].get<std::string>(),
        VK_SHADER_STAGE_FRAGMENT_BIT);

    // レンダーパスは別にします。
    pipelineCreateInfo.renderPass = frameBuffers.gBuffer.renderPass;
    pipelineCreateInfo.layout = pipelineLayouts.gBuffer;

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
                                              &pipelines.gBuffer));
    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
  }
}

//*-----------------------------------------------------------------------------
// Prepare
//*-----------------------------------------------------------------------------

/**
 * @brief オフスクリーンレンダリング用に新しいフレームバッファを用意します。
 */
void ProtoSSAO::PrepareOffscreenFramebuffer() {

  AttachmentCreateInfo attachmentCreateInfo{};
  attachmentCreateInfo.width = swapchain.extent.width;
  attachmentCreateInfo.height = swapchain.extent.height;
  attachmentCreateInfo.layerCount = 1;

  // G-Buffer
  {
    frameBuffers.gBuffer.width = swapchain.extent.width;
    frameBuffers.gBuffer.height = swapchain.extent.height;

    attachmentCreateInfo.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    attachmentCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    // POSITION (World Space)
    frameBuffers.gBuffer.AddAttachment(device, attachmentCreateInfo);

    // NORMAL (World Space)
    frameBuffers.gBuffer.AddAttachment(device, attachmentCreateInfo);

    // ALBEDO (Color)
    attachmentCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    frameBuffers.gBuffer.AddAttachment(device, attachmentCreateInfo);

    // Depth attachment
    attachmentCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    attachmentCreateInfo.format = device.FindSupportedDepthFormat();
    frameBuffers.gBuffer.AddAttachment(device, attachmentCreateInfo);

    // カラーアタッチメントからサンプラーを生成します。
    VK_CHECK_RESULT(frameBuffers.gBuffer.CreateSampler(
        device, VK_FILTER_NEAREST, VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));

    // フレームバッファ用のデフォルトのレンダーパスを生成します。
    VK_CHECK_RESULT(frameBuffers.gBuffer.CreateRenderPass(device));
  }

  // SSAO
  {
    frameBuffers.ssao.width = swapchain.extent.width;
    frameBuffers.ssao.height = swapchain.extent.height;

    attachmentCreateInfo.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    attachmentCreateInfo.format = VK_FORMAT_R8_UNORM;
    frameBuffers.ssao.AddAttachment(device, attachmentCreateInfo);

    VK_CHECK_RESULT(frameBuffers.ssao.CreateRenderPass(device));
  }

  // SSAO Blur
  {
    frameBuffers.blur.width = swapchain.extent.width;
    frameBuffers.blur.height = swapchain.extent.height;

    frameBuffers.blur.AddAttachment(device, attachmentCreateInfo);

    VK_CHECK_RESULT(frameBuffers.blur.CreateRenderPass(device));
  }
}

/**
 * @brief
 * シェーダーユニフォームを含むユニフォームバッファブロックを準備して初期化します。
 * @note
 * OpenGLのような単一のユニフォームはVulkanに存在しなくなりました。すべてのシェーダーユニフォームはユニフォームバッファブロックを介して渡されます。
 */
void ProtoSSAO::PrepareUniformBuffers() {
  VK_CHECK_RESULT(
      uniformBuffers.gBuffer.Create(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    sizeof(uboGBuffer), &uboGBuffer));
  VK_CHECK_RESULT(uniformBuffers.gBuffer.Map(device));

  VK_CHECK_RESULT(
      uniformBuffers.ssao.Create(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 sizeof(uboSSAO), &uboSSAO));
  VK_CHECK_RESULT(uniformBuffers.ssao.Map(device));

  VK_CHECK_RESULT(
      uniformBuffers.lighting.Create(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                     sizeof(uboLighting), &uboLighting));
  VK_CHECK_RESULT(uniformBuffers.lighting.Map(device));

  std::random_device rd;
  std::mt19937 engine(rd());
  UniformDistribution dist;

  // SSAO
  {
    uboSSAO.radius = 0.55f;
    uboSSAO.bias = 0.1f;
    for (size_t i = 0; i < KERNEL_SIZE; i++) {
      glm::vec3 randDir = dist.OnHemisphere(engine);
      const float scale = static_cast<float>(i * i) /
                          static_cast<float>(KERNEL_SIZE * KERNEL_SIZE);
      randDir *= glm::mix(0.1f, 1.0f, scale);

      uboSSAO.kernel[i].x = randDir.x;
      uboSSAO.kernel[i].y = randDir.y;
      uboSSAO.kernel[i].z = randDir.z;
      uboSSAO.kernel[i].w = 0.0f;
    }

    std::vector<glm::vec4> randDir(ROT_TEX_SIZE * ROT_TEX_SIZE);
    for (size_t i = 0; i < ROT_TEX_SIZE * ROT_TEX_SIZE; i++) {
      glm::vec2 v = dist.OnCircle(engine);
      randDir[i].x = v.x;
      randDir[i].y = v.y;
      randDir[i].z = 0.0f;
      randDir[i].w = 0.0f;
    }
    textures.noise.FromBuffer(device, randDir.data(),
                              randDir.size() * sizeof(glm::vec4),
                              VK_FORMAT_R32G32B32A32_SFLOAT, ROT_TEX_SIZE,
                              ROT_TEX_SIZE, queue, VK_FILTER_NEAREST);
  }

  // Lighting
  {
    uboLighting.displayRenderTarget = 0;
    uboLighting.useBlur = true;
    uboLighting.ao = 8.0f;
  }

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
void ProtoSSAO::BuildCommandBuffers() {
  VkCommandBufferBeginInfo commandBufferBeginInfo =
      Initializer::CommandBufferBeginInfo();

  for (size_t i = 0; i < drawCmdBuffers.size(); i++) {

    VK_CHECK_RESULT(
        vkBeginCommandBuffer(drawCmdBuffers[i], &commandBufferBeginInfo));

    VkRenderPassBeginInfo renderPassBeginInfo =
        Initializer::RenderPassBeginInfo();

    // Fill G-Buffer
    {
      // フラグメントシェーダーで使用するすべてのアタッチメントをこの値でクリアします。
      std::array<VkClearValue, 4> clearValues{};
      clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
      clearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
      clearValues[2].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
      clearValues[3].depthStencil = {1.0f, 0};

      renderPassBeginInfo.renderPass = frameBuffers.gBuffer.renderPass;
      renderPassBeginInfo.framebuffer = frameBuffers.gBuffer.framebuffer;
      renderPassBeginInfo.renderArea.extent.width = frameBuffers.gBuffer.width;
      renderPassBeginInfo.renderArea.extent.height =
          frameBuffers.gBuffer.height;
      renderPassBeginInfo.clearValueCount =
          static_cast<uint32_t>(clearValues.size());
      renderPassBeginInfo.pClearValues = clearValues.data();

      vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo,
                           VK_SUBPASS_CONTENTS_INLINE);

      VkViewport viewport = Initializer::Viewport(
          static_cast<float>(frameBuffers.gBuffer.width),
          static_cast<float>(frameBuffers.gBuffer.height), 0.0f, 1.0f);
      vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
      VkRect2D scissor = Initializer::Rect2D(frameBuffers.gBuffer.width,
                                             frameBuffers.gBuffer.height, 0, 0);
      vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

      vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelines.gBuffer);
      vkCmdBindDescriptorSets(
          drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipelineLayouts.gBuffer, 0, 1, &descriptorSets.gBuffer, 0, nullptr);
      VkDeviceSize offsets[] = {0};

      // Teapot
      {
        vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1,
                               &models.teapot.vertices.buffer, offsets);
        vkCmdBindIndexBuffer(drawCmdBuffers[i], models.teapot.indices.buffer, 0,
                             VK_INDEX_TYPE_UINT32);
        const auto &teapot = config["Teapot"];
        const auto scale = glm::vec3(teapot["Scale"].get<float>());
        const auto trans = glm::vec3(teapot["Position"][0].get<float>(),
                                     teapot["Position"][1].get<float>(),
                                     teapot["Position"][2].get<float>());
        auto model = glm::translate(glm::mat4(1.0f), trans);
        model = glm::rotate(model, glm::radians(30.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, scale);
        pushConsts.model = model;
        pushConsts.tex = 0;
        vkCmdPushConstants(drawCmdBuffers[i], pipelineLayouts.gBuffer,
                           VK_SHADER_STAGE_VERTEX_BIT |
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pushConsts), &pushConsts);
        vkCmdDrawIndexed(drawCmdBuffers[i], models.teapot.indexCount, 1, 0, 0,
                         0);
      }

      // Floor
      {
        vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1,
                               &models.floor.vertices.buffer, offsets);
        vkCmdBindIndexBuffer(drawCmdBuffers[i], models.floor.indices.buffer, 0,
                             VK_INDEX_TYPE_UINT32);
        const auto scale = glm::vec3(4.0f);
        const auto trans = glm::vec3(0.0f, 0.0f, 0.0f);
        auto model = glm::translate(glm::mat4(1.0f), trans);
        model = glm::scale(model, scale);
        pushConsts.model = model;
        pushConsts.tex = 1;
        vkCmdPushConstants(drawCmdBuffers[i], pipelineLayouts.gBuffer,
                           VK_SHADER_STAGE_VERTEX_BIT |
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pushConsts), &pushConsts);
        vkCmdDrawIndexed(drawCmdBuffers[i], models.floor.indexCount, 1, 0, 0,
                         0);
      }

      // Wall1
      {
        vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1,
                               &models.floor.vertices.buffer, offsets);
        vkCmdBindIndexBuffer(drawCmdBuffers[i], models.floor.indices.buffer, 0,
                             VK_INDEX_TYPE_UINT32);
        const auto scale = glm::vec3(4.0f);
        const auto trans = glm::vec3(0.0f, 0.0f, -2.0f);
        auto model = glm::translate(glm::mat4(1.0f), trans);
        model = glm::rotate(model, glm::radians(90.0f),
                            glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, scale);
        pushConsts.model = model;
        pushConsts.tex = 2;
        vkCmdPushConstants(drawCmdBuffers[i], pipelineLayouts.gBuffer,
                           VK_SHADER_STAGE_VERTEX_BIT |
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pushConsts), &pushConsts);
        vkCmdDrawIndexed(drawCmdBuffers[i], models.floor.indexCount, 1, 0, 0,
                         0);
      }

      // Wall2
      {
        vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1,
                               &models.floor.vertices.buffer, offsets);
        vkCmdBindIndexBuffer(drawCmdBuffers[i], models.floor.indices.buffer, 0,
                             VK_INDEX_TYPE_UINT32);
        const auto scale = glm::vec3(4.0f);
        const auto trans = glm::vec3(-2.0f, 0.0f, 0.0f);
        auto model = glm::translate(glm::mat4(1.0f), trans);
        model = glm::rotate(model, glm::radians(90.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));
        model =
            glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0, 0.0f));
        model = glm::scale(model, scale);
        pushConsts.model = model;
        pushConsts.tex = 2;
        vkCmdPushConstants(drawCmdBuffers[i], pipelineLayouts.gBuffer,
                           VK_SHADER_STAGE_VERTEX_BIT |
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pushConsts), &pushConsts);
        vkCmdDrawIndexed(drawCmdBuffers[i], models.floor.indexCount, 1, 0, 0,
                         0);
      }

      vkCmdEndRenderPass(drawCmdBuffers[i]);
    }

    // SSAO
    {
      std::vector<VkClearValue> clearValues(2);
      clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
      clearValues[1].depthStencil = {1.0f, 0};

      renderPassBeginInfo.renderPass = frameBuffers.ssao.renderPass;
      renderPassBeginInfo.framebuffer = frameBuffers.ssao.framebuffer;
      renderPassBeginInfo.renderArea.extent.width = frameBuffers.ssao.width;
      renderPassBeginInfo.renderArea.extent.height = frameBuffers.ssao.height;
      renderPassBeginInfo.clearValueCount =
          static_cast<uint32_t>(clearValues.size());
      renderPassBeginInfo.pClearValues = clearValues.data();

      vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo,
                           VK_SUBPASS_CONTENTS_INLINE);

      VkViewport viewport = Initializer::Viewport(
          static_cast<float>(frameBuffers.ssao.width),
          static_cast<float>(frameBuffers.ssao.height), 0.0f, 1.0f);
      vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
      VkRect2D scissor = Initializer::Rect2D(frameBuffers.ssao.width,
                                             frameBuffers.ssao.height, 0, 0);
      vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

      vkCmdBindDescriptorSets(
          drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipelineLayouts.ssao, 0, 1, &descriptorSets.ssao, 0, nullptr);
      vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelines.ssao);
      vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

      vkCmdEndRenderPass(drawCmdBuffers[i]);
    }

    // Blur
    {
      std::vector<VkClearValue> clearValues(2);
      clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
      clearValues[1].depthStencil = {1.0f, 0};

      renderPassBeginInfo.renderPass = frameBuffers.blur.renderPass;
      renderPassBeginInfo.framebuffer = frameBuffers.blur.framebuffer;
      renderPassBeginInfo.renderArea.extent.width = frameBuffers.blur.width;
      renderPassBeginInfo.renderArea.extent.height = frameBuffers.blur.height;
      renderPassBeginInfo.clearValueCount =
          static_cast<uint32_t>(clearValues.size());
      renderPassBeginInfo.pClearValues = clearValues.data();

      vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo,
                           VK_SUBPASS_CONTENTS_INLINE);

      VkViewport viewport = Initializer::Viewport(
          static_cast<float>(frameBuffers.blur.width),
          static_cast<float>(frameBuffers.blur.height), 0.0f, 1.0f);
      vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
      VkRect2D scissor = Initializer::Rect2D(frameBuffers.blur.width,
                                             frameBuffers.blur.height, 0, 0);
      vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

      vkCmdBindDescriptorSets(
          drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipelineLayouts.blur, 0, 1, &descriptorSets.blur, 0, nullptr);
      vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelines.blur);
      vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

      vkCmdEndRenderPass(drawCmdBuffers[i]);
    }

    // Lighting
    {
      std::array<VkClearValue, 2> clear{};
      clear[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
      clear[1].depthStencil = {1.0f, 0};

      renderPassBeginInfo.framebuffer = framebuffers[i];
      renderPassBeginInfo.renderPass = renderPass;
      renderPassBeginInfo.renderArea.extent.width = swapchain.extent.width;
      renderPassBeginInfo.renderArea.extent.height = swapchain.extent.height;

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
      vkCmdBindDescriptorSets(
          drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipelineLayouts.lighting, 0, 1, &descriptorSets.lighting, 0, nullptr);
      vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelines.lighting);

      vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

      DrawUI(drawCmdBuffers[i]);

      vkCmdEndRenderPass(drawCmdBuffers[i]);
    }

    // レンダーパスを終了すると、フレームバッファのカラーアタッチメントに移行する暗黙のバリアが追加されます。
    VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
  }
}

//*-----------------------------------------------------------------------------
// Update
//*-----------------------------------------------------------------------------

void ProtoSSAO::UpdateUniformBuffers() {
  const auto cameraConfig = config["Camera"];
  camera.SetupOrient(glm::vec3(cameraConfig["Position"][0].get<float>(),
                               cameraConfig["Position"][1].get<float>(),
                               cameraConfig["Position"][2].get<float>()),
                     glm::vec3(cameraConfig["Target"][0].get<float>(),
                               cameraConfig["Target"][1].get<float>(),
                               cameraConfig["Target"][2].get<float>()),
                     glm::vec3(0.0f, 1.0f, 0.0f));
  camera.SetupPerspective(glm::radians(60.0f),
                          static_cast<float>(swapchain.extent.width) /
                              static_cast<float>(swapchain.extent.height),
                          0.3f, 100.0f);

  UpdateGBufferUniformBuffer();
  UpdateSSAOUniformBuffer();
  UpdateLightingUniformBuffer();
}

void ProtoSSAO::UpdateGBufferUniformBuffer() {
  // 行列をシェーダーに渡します。
  uboGBuffer.view = camera.GetViewMatrix();
  uboGBuffer.proj = camera.GetProjectionMatrix();

  // ユニフォームバッファへコピーします。
  uniformBuffers.gBuffer.Copy(&uboGBuffer, sizeof(uboGBuffer));
}

void ProtoSSAO::UpdateSSAOUniformBuffer() {
  uboSSAO.proj = camera.GetProjectionMatrix();

  uniformBuffers.ssao.Copy(&uboSSAO, sizeof(uboSSAO));
}

void ProtoSSAO::UpdateLightingUniformBuffer() {
  int i = 0;
  for (const auto &light : config["Lights"]) {
    for (int j = 0; j < 4; j++) {
      uboLighting.lights[i].pos[j] = light["Position"][j].get<float>();
    }
    uboLighting.lights[i].pos = uboGBuffer.view * uboLighting.lights[i].pos;
    for (int j = 0; j < 3; j++) {
      uboLighting.lights[i].La[j] = light["La"][j].get<float>();
      uboLighting.lights[i].Ld[j] = light["Ld"][j].get<float>();
    }
    i++;
  }

  uboLighting.lightsNum = static_cast<int>(config["Lights"].size());

  uniformBuffers.lighting.Copy(&uboLighting, sizeof(uboLighting));
}

void ProtoSSAO::OnUpdateUIOverlay() {
  if (uiOverlay.Combo("Display Render Target", &uboLighting.displayRenderTarget,
                      {"Final Result", "Only SSAO", "No SSAO", "Position",
                       "Normal", "Albedo"})) {
    UpdateLightingUniformBuffer();
  }
  if (uiOverlay.Checkbox("Use Blur", &uboLighting.useBlur)) {
    UpdateLightingUniformBuffer();
  }
  if (uiOverlay.SliderFloat("Sampling Radius", &uboSSAO.radius, 0.1f, 1.0f)) {
    UpdateSSAOUniformBuffer();
  }
  if (uiOverlay.SliderFloat("Sampling Bias", &uboSSAO.bias, 0.0f, 1.0f)) {
    UpdateSSAOUniformBuffer();
  }
  if (uiOverlay.SliderFloat("AO Parameterization", &uboLighting.ao, 1.0f, 10.0f)) {
    UpdateLightingUniformBuffer();
  }
}
