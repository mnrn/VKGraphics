/**
 * @brief Texture Mapping
 */

#include <memory>

#include "App.h"
#include "Json.h"
#include "TextureMapping.h"

int main() {
  const auto config = Json::Parse("./Configs/SceneTextureMapping.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");

  App app(config.value());
  return app.Run(std::make_unique<TextureMapping>());
}
