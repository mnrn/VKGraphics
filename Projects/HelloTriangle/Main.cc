/**
 * @brief Hello Triangle
 */

#include <memory>

#include "App.h"
#include "JSON.h"
#include "VK/VkApp.h"


class HelloTriangle : public VkApp {};


int main() {
  const auto config = JSON::Parse("./Projects/HelloTriangle/Config.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");

  App app(config.value());
  std::unique_ptr<VkApp> hello = std::make_unique<HelloTriangle>();

  return app.Run(std::move(hello));
}
