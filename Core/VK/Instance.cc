/**
 * @brief Vulkan Instance
 */

#include "Instance.h"

#include <boost/assert.hpp>
#include <vector>

//*-----------------------------------------------------------------------------
// Instance
//*-----------------------------------------------------------------------------

const VkInstance &Instance::Get() const { return instance; }

VkInstance *Instance::Set() {
  BOOST_ASSERT_MSG(instance == VK_NULL_HANDLE,
                   "Failed to set instance. It is already set.");
  return &instance;
}

void Instance::Destroy() const {
  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
}

//*-----------------------------------------------------------------------------
// Queue Families
//*-----------------------------------------------------------------------------

bool QueueFamilies::IsComplete() const {
  return graphics >= 0 && presentation >= 0;
}

std::set<int> QueueFamilies::UniqueFamilies() const {
  return {graphics, presentation};
}

QueueFamilies QueueFamilies::Find(const Instance &instance) {
  return QueueFamilies::Find(instance.physicalDevice, instance.surface);
}

QueueFamilies QueueFamilies::Find(VkPhysicalDevice physicalDevice,
                                  VkSurfaceKHR surface) {
  uint32_t familyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> families(familyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount,
                                           families.data());

  QueueFamilies choice;
  uint32_t index = 0;
  for (const auto &family : families) {
    VkBool32 canPresent = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface,
                                         &canPresent);
    if (family.queueCount > 0 && family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      choice.graphics = index;
    }
    if (family.queueCount > 0 && canPresent) {
      choice.presentation = index;
    }
    if (choice.IsComplete()) {
      break;
    }
    ++index;
  }
  return choice;
}
