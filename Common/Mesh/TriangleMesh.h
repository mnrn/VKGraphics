#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <vector>

#include "VK/Buffer/Buffer.h"
#include "VK/Instance.h"

class TriangleMesh {
public:
  virtual ~TriangleMesh() = default;
  template <typename Vertex>
  void Init(const Instance &instance, const std::vector<Vertex> &vertices,
            const std::vector<uint32_t> &indices) {
    vertexBuffer.Create(instance, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer.Create(instance, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    indexCount = static_cast<uint32_t>(indices.size());
  }
  virtual void Destroy(const Instance &instance);
  virtual void Draw(VkCommandBuffer command, VkPipelineLayout pipelineLayout);

protected:
  Buffer vertexBuffer{};
  Buffer indexBuffer{};
  Buffer uniformBuffer{};
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  uint32_t indexCount = 0;
};
