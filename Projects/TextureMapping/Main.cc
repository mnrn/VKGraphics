/**
 * @brief Texture Mapping
 */

#include <memory>

#include "App.h"
#include "TextureMapping.h"
#include "JSON.h"

int main() {
  const auto config = JSON::Parse("./Projects/TextureMapping/Config.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");

  App app(config.value());
  std::unique_ptr<VkApp> tex = std::make_unique<TextureMapping>();

  return app.Run(std::move(tex));
}
