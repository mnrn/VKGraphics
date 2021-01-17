/**
 * @brief Uniform Buffers Example
 */

#include <memory>

#include "App.h"
#include "JSON.h"
#include "UniformBuffers.h"

int main() {
  const auto config = JSON::Parse("./Projects/HelloTriangle/Config.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");

  App app(config.value());
  std::unique_ptr<VkApp> ub = std::make_unique<UniformBuffers>();

  return app.Run(std::move(ub));
}
