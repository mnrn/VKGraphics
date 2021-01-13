#pragma once

#include <vulkan/vulkan.h>

#include <vector>

struct Semaphores {
  std::vector<VkSemaphore> imageAvailable{};
  std::vector<VkSemaphore> renderFinished{};
};
