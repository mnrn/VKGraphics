#include "VK/Buffer/Command.h"

#include "VK/Instance.h"

namespace Command {
VkCommandBuffer BeginSingleTime(const Instance &instance) {
  VkCommandBufferAllocateInfo alloc{};
  alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc.commandPool = instance.pool;
  alloc.commandBufferCount = 1;

  VkCommandBuffer command;
  vkAllocateCommandBuffers(instance.device, &alloc, &command);

  VkCommandBufferBeginInfo begin{};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command, &begin);
  return command;
}

void EndSingleTime(const Instance &instance, VkCommandBuffer command) {
  vkEndCommandBuffer(command);

  VkSubmitInfo submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &command;

  vkQueueSubmit(instance.queues.graphics, 1, &submit, VK_NULL_HANDLE);
  vkQueueWaitIdle(instance.queues.graphics);

  vkFreeCommandBuffers(instance.device, instance.pool, 1, &command);
}
} // namespace Command
