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
  void Destroy(const Instance &instance);
  const VkPipeline &operator[](size_t) const;

  VkPipelineLayout layout = VK_NULL_HANDLE;
  std::vector<VkPipeline> handles;
};
