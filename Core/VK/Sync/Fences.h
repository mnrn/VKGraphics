#pragma once

#include <vulkan/vulkan.h>

#include <vector>

struct Fences {
  std::vector<VkFence> inFlight{};
  std::vector<VkFence> imagesInFlight{};
};
