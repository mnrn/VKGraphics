/**
 * @brief Depth Testing
 */

#include <memory>

#include "App.h"
#include "DepthTesting.h"
#include "JSON.h"

int main() {
  const auto config = JSON::Parse("./Projects/DepthTesting/Config.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");

  App app(config.value());
  return app.Run(std::make_unique<DepthTesting>());
}
