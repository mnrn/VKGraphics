#pragma once

#include <vulkan/vulkan.h>

/**
 * @brief Pipeline
 */

#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

struct Instance;
struct Swapchain;
struct Texture;

struct Pipelines {
public:
  void Create(const Instance &instance, const Swapchain &swapchain,
              const VkRenderPass &renderPass, nlohmann::json &config);
  void CreateDescriptors(const Instance &instance,
                         std::unordered_map<std::string, Texture> &textures,
                         size_t objects);
  void Clear(const Instance &);
  void Clean(const Instance &);
  const VkPipeline &operator[](size_t) const;

  VkPipelineLayout layout = VK_NULL_HANDLE;
  std::vector<VkPipeline> handles;
  VkSampler sampler = VK_NULL_HANDLE;
  struct {
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE;
  } descriptor;
  struct {
    VkBuffer global;
    VkDeviceMemory globalMemory;
    VkBuffer local;
    VkDeviceMemory localMemory;
    uint32_t localAlignment;
  } uniform;

private:
  void CreateDescriptorSetLayout(const Instance &);
  void CreateTextureSampler(const Instance &);
  void CreateUniforms(const Instance &inst, size_t objectsNum);
};
