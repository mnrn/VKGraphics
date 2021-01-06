#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <limits>
#include <utility>

struct AABB {
  AABB() { Reset(); }
  void Reset() {
    mini = glm::vec3(std::numeric_limits<float>::max());
    maxi = glm::vec3(std::numeric_limits<float>::lowest());
  }

  void Merge(const glm::vec3 &pt) { Merge(pt.x, pt.y, pt.z); }

  void Merge(float x, float y, float z) {
    mini.x = std::fmin(mini.x, x);
    maxi.x = std::fmax(maxi.x, x);

    mini.y = std::fmin(mini.y, y);
    maxi.y = std::fmax(maxi.y, y);

    mini.z = std::fmin(mini.z, z);
    maxi.z = std::fmax(maxi.z, z);
  }

  glm::vec3 mini;
  glm::vec3 maxi;
};
