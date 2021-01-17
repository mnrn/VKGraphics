#pragma once

#include "VK/VkApp.h"

class HelloTriangle : public VkApp {
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
