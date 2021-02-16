/**
 * @brief Physically based rendering basics
 */

#include <memory>

#include "App.h"
#include "ProtoSSAO.h"
#include "Json.h"

int main() {
  const auto config = Json::Parse("./Configs/SceneProtoSSAO.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");

  App app(config.value());
  return app.Run(std::make_unique<ProtoSSAO>());
}
