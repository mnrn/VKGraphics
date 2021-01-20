#include "VK/Buffer/Memory.h"

namespace Memory {
/**
 * @brief
 * この関数は、要求するすべてのプロパティフラグをサポートするデバイスメモリタイプを要求するために使用されます。(e.g.
 * device local, host visible)<br>
 * 成功すると、要求されたメモリプロパティに適合するメモリタイプのインデックスが返されます。<br>
 * 実装が異なるメモリプロパティを持つ任意の数のメモリタイプを提供するために必要になります。
 * @ref http://vulkan.gpuinfo.org/
 */
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
