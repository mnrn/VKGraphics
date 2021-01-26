/**
 * @brief Hello Triangle
 */

#include <memory>

#include "App.h"
#include "HelloTriangle.h"
#include "Json.h"

int main() {
  const auto config = Json::Parse("./Projects/HelloTriangle/Config.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");

  App app(config.value());
  return app.Run(std::make_unique<HelloTriangle>());
}
