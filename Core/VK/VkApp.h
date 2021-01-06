/**
 * @brief Vulkan Application
 */

#pragma once

#include <boost/noncopyable.hpp>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VK/Debug.h"
#include "VK/Instance.h"

class VkApp : boost::noncopyable {
public:
  void OnCreate(const char *appName);
  void OnDestroy();

private:
  void CreateInstance(const char *appName);

  Instance instance_{};
  DebugMessenger debug_{};

  std::vector<const char *> validationLayers_ = {
#ifdef __APPLE__
      "VK_LAYER_KHRONOS_validation",
      "VK_LAYER_LUNARG_api_dump",
#else
#endif
  };
#ifdef NDEBUG
  const bool isEnableValidationLayers_ = false;
#else
  const bool isEnableValidationLayers_ = true;
#endif
};
