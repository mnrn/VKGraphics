/**
 * @brief 視錐台クラス
 */

#include "View/Frustum.h"

#include <boost/assert.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

void Frustum::SetupPerspective(float fovy, float aspectRatio, float near,
                               float far) {
  fovy_ = fovy;
  ar_ = aspectRatio;
  near_ = near;
  far_ = far;

  type_ = ProjectionType::Perspective;
}

void Frustum::SetupOrtho(float left, float right, float bottom, float top,
                         float near, float far) {
  left_ = left;
  right_ = right;
  bottom_ = bottom;
  top_ = top;
  near_ = near;
  far_ = far;

  type_ = ProjectionType::Ortho;
}

void Frustum::SetupCorners(const glm::vec3 &eyePt, const glm::vec3 &lookatPt,
                           const glm::vec3 &upVec) {
  corners_[0] = glm::vec3(-1.0, 1.0, 1.0);
  corners_[1] = glm::vec3(1.0, 1.0, 1.0);
  corners_[2] = glm::vec3(1.0, -1.0, 1.0);
  corners_[3] = glm::vec3(-1.0, -1.0, 1.0);

  corners_[4] = glm::vec3(-1.0, 1.0, -1.0);
  corners_[5] = glm::vec3(1.0, 1.0, -1.0);
  corners_[6] = glm::vec3(1.0, -1.0, -1.0);
  corners_[7] = glm::vec3(-1.0, -1.0, -1.0);

  const auto kView = glm::lookAt(eyePt, lookatPt, upVec);
  const auto kProj = GetProjectionMatrix();
  const auto kInvVP = glm::inverse(kProj * kView);
  for (int i = 0; i < 8; i++) {
    const auto corner = kInvVP * glm::vec4(corners_[i], 1.0f);
    corners_[i] = glm::vec3(corner) / corner.w;
  }
}

BSphere Frustum::ComputeBSphere() const {
  glm::vec3 center = glm::vec3(0.0f);
  for (int i = 0; i < 8; i++) {
    center += corners_[i];
  }
  center /= 8.0f;

  float radius = 0.0f;
  for (int i = 0; i < 8; i++) {
    float len = glm::length(corners_[i] - center);
    radius = glm::max(radius, len);
  }
  radius = std::ceil(radius * 16.0f) / 16.0f;

  return {center, radius};
}

glm::mat4 Frustum::GetProjectionMatrix() const {
  if (type_ == ProjectionType::Perspective) {
    return glm::perspective(fovy_, ar_, near_, far_);
  } else {
    return glm::ortho(left_, right_, bottom_, top_, near_, far_);
  }
}
