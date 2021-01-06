#pragma once

#include <glm/glm.hpp>
#include <optional>

#include "View/Frustum.h"

class Camera {
public:
  void SetupOrient(const glm::vec3 &eyePt, const glm::vec3 &lookatPt,
                   const glm::vec3 &upVec);
  void SetupPerspective(float fovy, float aspectRatio, float near, float far);
  void SetupOrtho(float left, float right, float bottom, float top, float near,
                  float far);

  void SetPosition(const glm::vec3 &eyePt) { eyePt_ = eyePt; }

  glm::vec3 GetPosition() const { return eyePt_; }
  glm::vec3 GetTarget() const { return lookatPt_; }
  glm::vec3 GetUpVec() const { return upVec_; }
  glm::mat4 GetViewMatrix() const;
  glm::mat4 GetInverseViewMatrix() const;
  glm::mat4 GetProjectionMatrix() const;

  float GetFOVY() const { return frustum_.GetFOVY(); }
  float GetAspectRatio() const { return frustum_.GetAspectRatio(); }
  float GetNear() const { return frustum_.GetNear(); }
  float GetFar() const { return frustum_.GetFar(); }

  // void OnPostUpdate() { frustum_.SetupCorners(eyePt_, lookatPt_, upVec_); }

private:
  glm::vec3 eyePt_;
  glm::vec3 lookatPt_;
  glm::vec3 upVec_;

  Frustum frustum_;
};
