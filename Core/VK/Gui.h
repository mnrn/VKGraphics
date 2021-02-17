/**
 * #brief ImGuiを使ってVulkan用のGUIクラスを構築します。
 */

#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <string>

#include "VK/Buffer.h"

struct Device;
struct GLFWwindow;

struct Gui {
public:
  void OnInit(GLFWwindow *window, const Device &device, VkQueue queue,
              VkPipelineCache pipelineCache, VkRenderPass renderPass);
  void OnDestroy(const Device &device) const;
  bool Update(const Device &device);
  void Draw(VkCommandBuffer commandBuffer);
  static void OnResize(uint32_t width, uint32_t height);

  bool Header(const char *label) const;
  bool Checkbox(const char *label, bool *v);
  bool Combo(const char *label, int32_t *v,
                const std::vector<std::string> &items);
  bool SliderFloat(const char *label, float *v, float vmin, float vmax);
  bool ColorEdit3(const char *label, glm::vec3 *color);

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

  float scale = 1.0f;
  bool updated = false;

private:
  void InitImGui(GLFWwindow *window) const;
  void SetupResources(const Device &device, VkQueue queue);
  void SetupPipeline(const Device &device, VkPipelineCache pipelineCache,
                     VkRenderPass renderPass);
};
