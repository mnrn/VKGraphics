/**
 * @brief Descriptor Sets
 */

#include "VK/Pipeline/DescriptorSets.h"

#include <boost/assert.hpp>

#include "VK/Instance.h"

const VkDescriptor &DescriptorSets::operator[](size_t index) const {
  return handles[index];
}

void DescriptorSets::Destroy(const Instance &instance) {
  vkDestroyDescriptorSetLayout(instance.device, layout, nullptr);
}
