#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>

namespace JSON {
std::optional<nlohmann::json> Parse(const std::string& filepath) {
  std::ifstream ifs(filepath);
  if (!ifs) {
    return std::nullopt;
  }
  nlohmann::json config;
  ifs >> config;
  return std::make_optional(config);
}
}
