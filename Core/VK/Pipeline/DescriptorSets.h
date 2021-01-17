#pragma once

#include <vulkan/vulkan.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

struct Instance;

struct DescriptorSets {
public:
  void Destroy(const Instance &instance);
  const VkDescriptorSet &operator[](size_t) const;

  VkDescriptorSetLayout layout = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> handles;
};
