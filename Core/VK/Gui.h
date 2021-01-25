/**
 * #brief ImGuiを使ってVulkan用のGUIクラスを構築します。
 */

#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "VK/Buffer.h"

struct Device;

struct Gui {
public:
  Gui(const Device &device, VkQueue queue, VkPipelineCache pipelineCache,
      VkRenderPass renderPass);
  /*
  void OnInit(const Device &device, VkQueue queue,
              VkPipelineCache pipelineCache, VkRenderPass renderPass);
              */
  void OnDestroy(const Device &device) const;
  bool OnUpdate(const Device &device);
  void OnRender(VkCommandBuffer commandBuffer);
  static void OnResize(uint32_t width, uint32_t height);

  uint32_t subpass = 0;

  Buffer vertexBuffer{};
  uint32_t vertexCount = 0;
  Buffer indexBuffer{};
  uint32_t indexCount = 0;

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;

  struct {
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
  } font;
  VkSampler sampler = VK_NULL_HANDLE;

  struct PushConst {
    alignas(8) glm::vec2 scale;
    alignas(8) glm::vec2 translate;
  } pushConst;

  bool visible = true;
  bool updated = false;

private:
  void SetupResources(const Device &device, VkQueue queue);
  void SetupPipeline(const Device &device, VkPipelineCache pipelineCache,
                     VkRenderPass renderPass);
};