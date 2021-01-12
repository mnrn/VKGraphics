#pragma once

#include <glm/glm.hpp>

struct UniformBufferGlobal {
};

struct UniformBufferLocal {
  glm::mat4 modelView;
  glm::mat3 normal;
  glm::mat4 mvp;
};
