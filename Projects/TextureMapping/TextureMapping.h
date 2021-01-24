#pragma once

#include "VK/VkBase.h"

#include <vector>

#include "VK/Buffer.h"
#include "VK/Texture.h"
#include "View/Camera.h"

class TextureMapping : public VkBase {
public:
  void OnPostInit() override;
  void OnPreDestroy() override;

  void LoadAssets();
  void PrepareCamera();
  void PrepareVertices();
  void PrepareUniformBuffers();
  void UpdateUniformBuffers();

  void SetupDescriptorSetLayout();
  void SetupPipelines();
  void SetupDescriptorPool();
  void SetupDescriptorSet();

  void BuildCommandBuffers() override;
  void ViewChanged() override;

private:
  Texture2D texture;

  struct UniformBufferObject {
    alignas(16) glm::mat4 mvp;
    alignas(4) float lodBias;
  } ubo;

  struct {
    VkPipelineVertexInputStateCreateInfo inputState{};
    std::vector<VkVertexInputBindingDescription> bindings{};
    std::vector<VkVertexInputAttributeDescription> attributes{};
  } vertex;

  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;

  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  Buffer vertexBuffer{};
  Buffer indexBuffer{};
  uint32_t indexCount = 0;
  Buffer uniformBuffer{};

  Camera camera{};
};
