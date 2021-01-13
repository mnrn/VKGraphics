#pragma once

#include <vulkan/vulkan.h>

#include "VK/Sync/Fences.h"
#include "VK/Sync/Semaphores.h"

struct Instance;
struct Swapchain;

struct SyncObjects {
  void Create(const Instance &instance, const Swapchain &swapchain,
              size_t frames);
  void Destroy(const Instance &instance, size_t frames) const;
  Semaphores semaphores{};
  Fences fences{};
  size_t currentFrame = 0;
};
