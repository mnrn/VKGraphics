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
  void OnCreate(const char *appName, GLFWwindow *window);
  void OnDestroy();

private:
  void CreateInstance(const char *appName);
  void CreateSurface(GLFWwindow *window);
  void SelectPhysicalDevice();
  void CreateLogicalDevice();

  float CalcDeviceScore(VkPhysicalDevice device) const;

  Instance instance_{};
  DebugMessenger debug_{};

  std::vector<const char *> validationLayers_ = {
#ifdef __APPLE__
      "VK_LAYER_KHRONOS_validation",
      "VK_LAYER_LUNARG_api_dump",
#else
#endif
  };
  std::vector<const char *> deviceExtensions_ = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };
#if defined(NDEBUG)
  const bool isEnableValidationLayers_ = false;
#else
  const bool isEnableValidationLayers_ = true;
#endif
};
