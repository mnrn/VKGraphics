#pragma once

#include "VK/VkBase.h"

#include "VK/Pipeline/Pipelines.h"

class HelloTriangle : public VkBase {
private:
  void CreateRenderPass() override;
  void CreateDescriptorSetLayouts() override {}
  void DestroyDescriptorSetLayouts() override {}
  void CreatePipelines() override;
  void DestroyPipelines() override;
  void CreateFramebuffers() override;
  void CreateVertexBuffer() override;
  void CreateIndexBuffer() override;
  void CreateUniformBuffers() override {}
  void DestroyUniformBuffers() override {}
  void CreateDescriptorPool() override {}
  void CreateDescriptorSets() override {}
  void CreateDrawCommandBuffers() override;

  Pipelines pipelines_{};
};
