/**
 * @brief Vulkan Application
 */

#pragma once

#include <boost/noncopyable.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VK/Debug.h"
#include "VK/Instance.h"
#include "VK/Pipeline/Pipelines.h"
#include "VK/Swapchain.h"

class VkApp : boost::noncopyable {
public:
  void OnCreate(const nlohmann::json &config, GLFWwindow *window);
  void OnDestroy();

private:
  void CreateInstance(const char *appName);
  void CreateSurface(GLFWwindow *window);
  void SelectPhysicalDevice();
  void CreateLogicalDevice();
  void CreateRenderPass();

  void CleanupSwapchain();

  float CalcDeviceScore(VkPhysicalDevice physicalDevice) const;
  std::optional<VkFormat>
  FindSuitableDepthFormat(VkPhysicalDevice physicalDevice) const;
  std::optional<VkFormat>
  FindSupportedFormat(const std::vector<VkFormat> &candicates,
                      VkImageTiling tiling, VkFormatFeatureFlags features,
                      VkPhysicalDevice physicalDevice) const;

  Instance instance_{};
  Swapchain swapchain_{};
  Pipelines pipelines_{};
  VkRenderPass renderPass_ = VK_NULL_HANDLE;
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
