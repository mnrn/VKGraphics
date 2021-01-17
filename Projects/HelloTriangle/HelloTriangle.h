#pragma once

#include "VK/VkApp.h"

class HelloTriangle : public VkApp {
private:
  void CreateRenderPass() override;
  void CreatePipelines() override;
  void CreateFramebuffers() override;
  void CreateVertexBuffer() override;
  void RecordDrawCommands() override;
};
