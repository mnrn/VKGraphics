/**
 * @brief Screen Space Ambient Occlusion
 */

#include <memory>

#include "App.h"
#include "Json.h"
#include "SSAO.h"

int main() {
  const auto config = Json::Parse("./Configs/SceneSSAO.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");

  App app(config.value());
  return app.Run(std::make_unique<SSAO>());
}
