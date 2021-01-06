/**
 * @brief カメラクラス
 */

#include "View/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

void Camera::SetupOrient(const glm::vec3 &eyePt, const glm::vec3 &lookatPt,
                         const glm::vec3 &upVec) {
  eyePt_ = eyePt;
  lookatPt_ = lookatPt;
  upVec_ = upVec;
}

void Camera::SetupPerspective(float fovy, float aspectRatio, float near,
                              float far) {
  frustum_.SetupPerspective(fovy, aspectRatio, near, far);
}

void Camera::SetupOrtho(float left, float right, float bottom, float top, float near, float far) {
  frustum_.SetupOrtho(left, right, bottom, top, near, far);
}

glm::mat4 Camera::GetViewMatrix() const {
  return glm::lookAt(eyePt_, lookatPt_, upVec_);
}

glm::mat4 Camera::GetInverseViewMatrix() const {
  const glm::vec3 n = glm::normalize(eyePt_ - lookatPt_);
  const glm::vec3 u = glm::normalize(glm::cross(upVec_, n));
  const glm::vec3 v = glm::normalize(glm::cross(n, u));

  const glm::mat4 rot(u.x, u.y, u.z, 0.0f, v.x, v.y, v.z, 0.0f, n.x, n.y, n.z,
                      0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
  const glm::mat4 trans =
      glm::translate(glm::mat4(1.0f), glm::vec3(eyePt_.x, eyePt_.y, eyePt_.z));
  return trans * rot;
}

glm::mat4 Camera::GetProjectionMatrix() const {
  return frustum_.GetProjectionMatrix();
}
