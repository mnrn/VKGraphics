/**
 * @brief Pipeline
 */

#include "VK/Pipeline/Pipelines.h"

#include <boost/assert.hpp>

#include "VK/Instance.h"

const VkPipeline &Pipelines::operator[](size_t index) const {
  return handles[index];
}

void Pipelines::Cleanup(const Instance &instance) {
  for (const auto &handle : handles) {
    vkDestroyPipeline(instance.device, handle, nullptr);
  }
  handles.clear();
  vkDestroyPipelineLayout(instance.device, layout, nullptr);
}

void Pipelines::Destroy(const Instance &instance) const {
  vkDestroySampler(instance.device, sampler, nullptr);
  if (descriptor.pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(instance.device, descriptor.pool, nullptr);
  }
  vkDestroyDescriptorSetLayout(instance.device, descriptor.layout, nullptr);
  if (uniform.global != VK_NULL_HANDLE) {
    vkDestroyBuffer(instance.device, uniform.global, nullptr);
  }
  if (uniform.local != VK_NULL_HANDLE) {
    vkDestroyBuffer(instance.device, uniform.local, nullptr);
  }
  if (uniform.globalMemory != VK_NULL_HANDLE) {
    vkFreeMemory(instance.device, uniform.globalMemory, nullptr);
  }
  if (uniform.localMemory != VK_NULL_HANDLE) {
    vkFreeMemory(instance.device, uniform.localMemory, nullptr);
  }
}
