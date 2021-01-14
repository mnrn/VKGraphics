#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <type_traits>
#include <vector>

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 texCoord;
  // glm::vec3 tangent;

  static std::vector<VkVertexInputBindingDescription> Bindings();
  static std::vector<VkVertexInputAttributeDescription> Attributes();
};

static_assert(std::is_pod_v<Vertex>);
