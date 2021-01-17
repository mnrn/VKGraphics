/**
 * @brief Descriptors
 */

#include "VK/Pipeline/Descriptors.h"

#include <boost/assert.hpp>

#include "VK/Instance.h"

const VkDescriptorSet &Descriptors::operator[](size_t index) const {
  return handles[index];
}

void Descriptors::Destroy(const Instance &instance) {
  vkDestroyDescriptorSetLayout(instance.device, layout, nullptr);
}
