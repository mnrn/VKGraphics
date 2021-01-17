#include "VK/Buffer/BufferObject.h"

//*-----------------------------------------------------------------------------
// Create & Destroy
//*-----------------------------------------------------------------------------

void BufferObject::Destroy(const Instance &instance) const {
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(instance.device, buffer, nullptr);
  }
  if (memory != VK_NULL_HANDLE) {
    vkFreeMemory(instance.device, memory, nullptr);
  }
}
