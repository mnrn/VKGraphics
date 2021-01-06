/**
 * @brief  Wrapper for Scene and App class
 */

#ifndef SCENE_LOOP_H
#define SCENE_LOOP_H

// ********************************************************************************
// Including files
// ********************************************************************************

#include <memory>

#include "App.h"
#include "Scene.h"

// ********************************************************************************
// Functions
// ********************************************************************************

namespace SceneLoop {

/**
 * @brief 一つのシーンだけでループしたい場合に使用してください。
 * @note デストラクタの関係上、シーンの所有権はこちらに渡してもらいます。
 * @param app ループ対象となるアプリケーション
 * @param scene シーンへのポインタ
 */
static inline int Run(App &app, std::unique_ptr<Scene> &&scene) {
  return app.Run(
      [&scene](int w, int h) {
        scene->SetDimensions(w, h);
        scene->OnInit();
        scene->OnResize(w, h);
      },
      [&scene](float t) { scene->OnUpdate(t); },
      [&scene]() { scene->OnRender(); }, [&scene]() { scene->OnDestroy(); });
}
} // namespace SceneLoop

#endif
