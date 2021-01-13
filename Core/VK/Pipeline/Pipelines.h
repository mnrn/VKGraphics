#pragma once

#include <vulkan/vulkan.h>

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
              const VkRenderPass &renderPass, const nlohmann::json &config);
#if false
  void CreateDescriptors(const Instance &instance,
                         std::unordered_map<std::string, Texture> &textures,
                         size_t objects);
#endif
  void Clear(const Instance &);
  void Cleanup(const Instance &);
  const VkPipeline &operator[](size_t) const;

  VkPipelineLayout layout = VK_NULL_HANDLE;
  std::vector<VkPipeline> handles;
  VkSampler sampler = VK_NULL_HANDLE;
  struct {
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE;
  } descriptor;
  struct {
    VkBuffer global = VK_NULL_HANDLE;
    VkDeviceMemory globalMemory = VK_NULL_HANDLE;
    VkBuffer local = VK_NULL_HANDLE;
    VkDeviceMemory localMemory = VK_NULL_HANDLE;
    uint32_t localAlignment = 0;
  } uniform;

private:
  void CreateDescriptorSetLayout(const Instance &);
  void CreateTextureSampler(const Instance &);
#if false
  void CreateUniforms(const Instance &, size_t objects);
#endif
};
