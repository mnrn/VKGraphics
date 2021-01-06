/**
 * @brief  Scene Base Class
 * @date   2017/03/19
 */

#ifndef SCENE_H
#define SCENE_H

// ********************************************************************************
// Including files
// ********************************************************************************

#include <glm/glm.hpp>

// ********************************************************************************
// Class
// ********************************************************************************

/**
 * @brief Scene Base Class
 */
class Scene {
public:
  virtual ~Scene() = default;

  virtual void OnInit() = 0;
  virtual void OnDestroy() {}
  virtual void OnUpdate(float t) = 0;
  virtual void OnRender() = 0;
  virtual void OnResize(int w, int h) = 0;

  void SetDimensions(int w, int h) {
    width_ = w;
    height_ = h;
  }

protected:
  int width_;
  int height_;

  glm::mat4 model_; // モデル行列
  glm::mat4 view_;  // ビュー行列
  glm::mat4 proj_;  // 射影行列
};

#endif
