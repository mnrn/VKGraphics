/**
 * @brief Hello Triangle
 */

#include "App.h"

#include "JSON.h"

namespace AppDelegate {
  static void OnInit(){}
  static void OnUpdate(float) {}
  static void OnDestroy(){}
}

int main() {
  const auto config = JSON::Parse("./Projects/HelloTriangle/Config.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");
  App app(config.value());
  return app.Run(AppDelegate::OnInit, AppDelegate::OnUpdate, AppDelegate::OnDestroy);
}
