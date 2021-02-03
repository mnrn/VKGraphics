/**
 * @brief Physically based rendering basics
 */

#include <memory>

#include "App.h"
#include "Json.h"
#include "PBRBasic.h"

int main() {
  const auto config = Json::Parse("./Configs/ScenePBRBasic.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");

  App app(config.value());
  return app.Run(std::make_unique<PBRBasic>());
}
