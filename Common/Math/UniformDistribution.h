#pragma once

#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

class UniformDistribution {
public:
  /** 円周上の一様乱数 */
  template <typename RNG> glm::vec2 OnCircle(RNG &gen) {
    const float u = dist01_(gen);
    const float theta = glm::two_pi<float>() * u;
    const float x = glm::cos(theta);
    const float y = glm::sin(theta);
    return glm::vec2(x, y);
  }

  /** 円周内の一様乱数 */
  template <typename RNG> glm::vec2 InCircle(RNG &gen) {
    const float v = dist01_(gen);
    const float r = glm::sqrt(v);
    return r * OnCircle(gen);
  }

  /** 半球面上の一様乱数 */
  template <typename RNG> glm::vec3 OnHemisphere(RNG &gen) {
    const float v = dist01_(gen);
    const float r = glm::sqrt(1.0f - v * v);

    const float u = dist01_(gen);
    const float theta = glm::two_pi<float>() * u;
    const float x = glm::cos(theta);
    const float y = glm::sin(theta);
    return glm::vec3(r * x, r * y, u);
  }

private:
  std::uniform_real_distribution<float> dist01_{0.0f, 1.0f};
};
