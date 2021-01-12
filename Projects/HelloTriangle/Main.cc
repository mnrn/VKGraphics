/**
 * @brief Hello Triangle
 */

#include "App.h"

#include "JSON.h"

int main() {
  const auto config = JSON::Parse("./Projects/HelloTriangle/Config.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");
  App app(config.value());
  return 0;
}
