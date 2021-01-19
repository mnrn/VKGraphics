#pragma once

#include "VK/VkApp.h"

#include "VK/Buffer/DepthStencil.h"
#include "VK/Image/Texture.h"
#include "VK/Pipeline/Descriptors.h"
#include "VK/Pipeline/Pipelines.h"

class DepthTesting : public VkApp {
public:
  void OnUpdate(float t) override;

private:
  void CreateRenderPass() override;
  void CreateDescriptorSetLayouts() override;
  void DestroyDescriptorSetLayouts() override;
  void CreatePipelines() override;
  void DestroyPipelines() override;
  void CreateDepthStencil() override;
  void DestroyDepthStencil() override;
  void CreateFramebuffers() override;
  void SetupAssets() override;
  void CleanupAssets() override;
  void CreateVertexBuffer() override;
  void CreateIndexBuffer() override;
  void CreateUniformBuffers() override;
  void DestroyUniformBuffers() override;
  void UpdateUniformBuffers(uint32_t imageIndex) override;
  void CreateDescriptorPool() override;
  void CreateDescriptorSets() override;
  void CreateDrawCommandBuffers() override;

  Pipelines pipelines_{};
  Descriptors descriptors_{};
  std::vector<Buffer> uniforms_{};
  Texture texture_{};
  DepthStencil depth_{};

  float angle_ = 0.0f;
  float tPrev_ = 0.0f;
};
