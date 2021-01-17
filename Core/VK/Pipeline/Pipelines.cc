/**
 * @brief Pipeline
 */

#include "VK/Pipeline/Pipelines.h"

#include <boost/assert.hpp>

#include "VK/Instance.h"

const VkPipeline &Pipelines::operator[](size_t index) const {
  return handles[index];
}

void Pipelines::Destroy(const Instance &instance) {
  for (const auto &handle : handles) {
    vkDestroyPipeline(instance.device, handle, nullptr);
  }
  handles.clear();
  vkDestroyPipelineLayout(instance.device, layout, nullptr);
}
