#include "VK/Sync/SyncObjects.h"

#include <boost/assert.hpp>

#include "VK/Instance.h"
#include "VK/Swapchain.h"

void SyncObjects::Create(const Instance &instance, const Swapchain &swapchain,
                         size_t frames) {
  semaphores.imageAvailable.resize(frames);
  semaphores.renderFinished.resize(frames);
  fences.inFlight.resize(frames);
  fences.imagesInFlight.resize(swapchain.views.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore{};
  semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence{};
  fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < frames; i++) {
    if (vkCreateSemaphore(instance.device, &semaphore, nullptr,
                          &semaphores.imageAvailable[i]) ||
        vkCreateSemaphore(instance.device, &semaphore, nullptr,
                          &semaphores.renderFinished[i]) ||
        vkCreateFence(instance.device, &fence, nullptr, &fences.inFlight[i])) {
      BOOST_ASSERT_MSG(false, "Failed to create synchronization objects for a frame!");
    }
  }
}

void SyncObjects::Destroy(const Instance &instance, size_t frames) const {
  for (size_t i = 0; i < frames; i++) {
    vkDestroyFence(instance.device, fences.inFlight[i], nullptr);
    vkDestroySemaphore(instance.device, semaphores.renderFinished[i],
                         nullptr);
    vkDestroySemaphore(instance.device, semaphores.imageAvailable[i],
                         nullptr);
  }
}
