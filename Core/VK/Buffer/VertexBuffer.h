#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "VK/Primitive/Vertex.h"

struct Instance;

struct VertexBuffer {
  void Create(const Instance &instance);
  void Destroy(const Instance &instance);

  std::vector<Vertex> vertices{};
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
};
