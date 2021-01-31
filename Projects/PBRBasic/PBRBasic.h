#pragma once

#include "VK/VkBase.h"

#include <string>
#include <vector>

#include "VK/Buffer.h"
#include "VK/Model.h"
#include "VK/Texture.h"
#include "View/Camera.h"

class PBRBasic : public VkBase {
public:
  void OnPostInit() override;
  void OnPreDestroy() override;
  void OnUpdateUIOverlay() override;

  void PrepareCamera();
  void LoadAssets();
  void PrepareUniformBuffers();
  void UpdateUniformBuffers();
  void UpdateLights();

  void SetupDescriptorSetLayout();
  void SetupPipelines();
  void SetupDescriptorPool();
  void SetupDescriptorSet();

  void BuildCommandBuffers() override;

  void ViewChanged() override;

private:
  struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
  } ubo;

  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;

  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  Camera camera{};

  VertexLayout vertexLayout{{VertexLayoutComponent::Position}};
  Model model;
  Buffer uniformBuffer{};
};
