#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vector>

#include "Geometry/BSphere.h"

enum struct ProjectionType {
  Perspective,
  Ortho,
  Num,
};

class Frustum {
public:
  void SetupPerspective(float fovy, float aspectRatio, float near, float far);
  void SetupOrtho(float left, float right, float bottom, float top, float near,
                  float far);

  void SetupCorners(const glm::vec3 &eyePt, const glm::vec3 &lookatPt,
                    const glm::vec3 &upVec);

  void SetNear(float n) { near_ = n; }
  float GetNear() const { return near_; }
  void SetFar(float f) { far_ = f; }
  float GetFar() const { return far_; }
  float GetFOVY() const { return fovy_; }
  float GetAspectRatio() const { return ar_; }

  glm::mat4 GetProjectionMatrix() const;
  glm::vec3 GetCorner(std::size_t idx) const { return corners_.at(idx); }
  BSphere ComputeBSphere() const;

private:
  ProjectionType type_;

  float fovy_;
  float ar_;
  float near_;
  float far_;

  float left_;
  float right_;
  float bottom_;
  float top_;

  /**
   * @brief frustum from 8 corner coordinates.
   * @param corners the corners of the frustum
   *
   * The corners should be specified in this order:
   * 0. far top left
   * 1. far top right
   * 2. far bottom right
   * 3. far bottom left
   * 4. near top left
   * 5. near top right
   * 6. near bottom right
   * 7. near bottom left
   *
   *     0----1
   *    /|   /|
   *   4----5 |
   *   | 3--|-2      far
   *   |/   |/       /
   *   7----6      near
   *
   */
  std::array<glm::vec3, 8> corners_;
};
