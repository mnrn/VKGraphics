#pragma once

#include "VK/VkApp.h"

class UniformBuffer : public VkApp {
private:
  void CreateRenderPass() override;
  void CreatePipelines() override;
  void CreateFramebuffers() override;
  void CreateVertexBuffer() override;
  void CreateIndexBuffer() override;
  void CreateDrawCommandBuffers() override;
};
