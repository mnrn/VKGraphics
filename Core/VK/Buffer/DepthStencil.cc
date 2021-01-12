#include "VK/Buffer/DepthStencil.h"

#include <boost/assert.hpp>

#include "VK/Instance.h"
#include "VK/Swapchain.h"

void DepthStencil::Create(const Instance &instance,
                          const Swapchain &swapchain) {
  const VkFormat format = FindDepthFormat(instance.physicalDevice);
}