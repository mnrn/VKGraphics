/**
 * @brief Vulkan Application
 */

#pragma once

#include <array>
#include <boost/noncopyable.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VK/Buffer/VertexBuffer.h"
#include "VK/Debug.h"
#include "VK/Instance.h"
#include "VK/Pipeline/Pipelines.h"
#include "VK/Swapchain.h"
#include "VK/Sync/SyncObjects.h"

class VkApp : boost::noncopyable {
public:
  void OnCreate(const nlohmann::json &config, GLFWwindow *window);
  void OnDestroy();
  void OnRender();

  void WaitIdle() const;

  static void OnResized(GLFWwindow *window, int width, int height);

protected:
  void CreateInstance(const char *appName);
  void CreateSurface();
  void SelectPhysicalDevice();
  void CreateLogicalDevice();
  float CalcDeviceScore(VkPhysicalDevice physicalDevice) const;

  void CreateRenderPass();
  void CreateCommandPool();
  void CreateFramebuffers();
  void CreateVertexBuffer();
  void CreateDrawCommandBuffers();
  void RecordDrawCommands();

  void CleanupSwapchain();
  void RecreateSwapchain();

  static constexpr size_t kMaxFramesInFlight = 2;

  Instance instance_{};
  Swapchain swapchain_{};
  Pipelines pipelines_{};
  VkRenderPass renderPass_ = VK_NULL_HANDLE;
  struct CommandBuffers {
    std::vector<VkCommandBuffer> draw;
    std::array<VkCommandBuffer, 3> push;
    size_t currentPush = 0;
  } commandBuffers_{};
  std::vector<VkFramebuffer> framebuffers_{};
  VertexBuffer vertexBuffer_;
  SyncObjects syncs_{};

  GLFWwindow *window_ = nullptr;
  nlohmann::json config_{};
  bool isFramebufferResized_ = false;
#if !defined(NDEBUG)
  DebugMessenger debug_{};
#endif
  std::vector<const char *> validationLayers_ = {
#ifdef __APPLE__
      "VK_LAYER_KHRONOS_validation",
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
