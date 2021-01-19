/**
 * @brief Uniform Buffers Example
 */

#include <memory>

#include "App.h"
#include "JSON.h"
#include "UniformBuffers.h"

int main() {
  const auto config = JSON::Parse("./Projects/UniformBuffers/Config.json");
  BOOST_ASSERT_MSG(config, "Failed to open Config.json!");

  App app(config.value());
  std::unique_ptr<VkBase> ub = std::make_unique<UniformBuffers>();

  return app.Run(std::move(ub));
}
