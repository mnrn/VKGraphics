#include "VK/Buffer/Command.h"

#include <boost/assert.hpp>

#include "VK/Common.h"
#include "VK/Initializer.h"
#include "VK/Instance.h"

namespace Command {
VkCommandBuffer Get(const Instance &instance) {
  VkCommandBufferAllocateInfo alloc = Initializer::CommandBufferAllocateInfo(
      instance.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

  VkCommandBuffer command;
  VK_CHECK_RESULT(vkAllocateCommandBuffers(instance.device, &alloc, &command));

  VkCommandBufferBeginInfo begin = Initializer::CommandBufferBeginInfo();
  VK_CHECK_RESULT(vkBeginCommandBuffer(command, &begin));

  return command;
}

void Flush(const Instance &instance, VkCommandBuffer command) {
  BOOST_ASSERT(command != VK_NULL_HANDLE);

  VK_CHECK_RESULT(vkEndCommandBuffer(command));

  VkSubmitInfo submit = Initializer::SubmitInfo();
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &command;

  // フェンスを生成して、コマンドバッファの実行が終了したことを確認します。
  VkFenceCreateInfo fenceCreateInfo = Initializer::FenceCreateInfo();
  VkFence fence;
  VK_CHECK_RESULT(vkCreateFence(instance.device, &fenceCreateInfo, nullptr, &fence));

  // キューに送信します。
  VK_CHECK_RESULT(
      vkQueueSubmit(instance.queues.graphics, 1, &submit, fence));
  // コマンドバッファの実行が終了したことをフェンスが通知するのを待ちます。
  VK_CHECK_RESULT(vkWaitForFences(instance.device, 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max()));

  vkDestroyFence(instance.device, fence, nullptr);
  vkFreeCommandBuffers(instance.device, instance.pool, 1, &command);
}
} // namespace Command
