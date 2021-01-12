#include "VK/Buffer/Memory.h"

namespace Memory {
std::optional<uint32_t> FindType(uint32_t filter, VkMemoryPropertyFlags flags,
                                 VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceMemoryProperties props{};
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &props);

  for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
    if (filter & (1 << i) &&
        (props.memoryTypes[i].propertyFlags & flags) == flags) {
      return i;
    }
  }
  return std::nullopt;
}
} // namespace Memory
